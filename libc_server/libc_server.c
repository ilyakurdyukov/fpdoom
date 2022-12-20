/*
// libc server to serve files from the working directory to the feature phone.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
*/

#define _GNU_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifndef LIBUSB_DETACH
/* detach the device from crappy kernel drivers */
#define LIBUSB_DETACH 1
#endif

#if USE_LIBUSB
#include <libusb-1.0/libusb.h>
#else
#include <termios.h>
#include <fcntl.h>
#include <poll.h>
#endif
#include <unistd.h>

#include "cmd_def.h"

static void print_mem(FILE *f, const uint8_t *buf, size_t len) {
	size_t i; int a, j, n;
	for (i = 0; i < len; i += 16) {
		n = len - i;
		if (n > 16) n = 16;
		for (j = 0; j < n; j++) fprintf(f, "%02x ", buf[i + j]);
		for (; j < 16; j++) fprintf(f, "   ");
		fprintf(f, " |");
		for (j = 0; j < n; j++) {
			a = buf[i + j];
			fprintf(f, "%c", a > 0x20 && a < 0x7f ? a : '.');
		}
		fprintf(f, "|\n");
	}
}

static void print_string(FILE *f, uint8_t *buf, size_t n) {
	size_t i; int a, b = 0;
	fprintf(f, "\"");
	for (i = 0; i < n; i++) {
		a = buf[i]; b = 0;
		switch (a) {
		case '"': case '\\': b = a; break;
		case 0: b = '0'; break;
		case '\b': b = 'b'; break;
		case '\t': b = 't'; break;
		case '\n': b = 'n'; break;
		case '\f': b = 'f'; break;
		case '\r': b = 'r'; break;
		}
		if (b) fprintf(f, "\\%c", b);
		else if (a >= 32 && a < 127) fprintf(f, "%c", a);
		else fprintf(f, "\\x%02x", a);
	}
	fprintf(f, "\"\n");
}

#define ERR_EXIT(...) \
	do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)

#define DBG_LOG(...) fprintf(stderr, __VA_ARGS__)

#define RECV_BUF_LEN 1024
#define TEMP_BUF_LEN 0x4000

typedef struct {
	uint8_t *recv_buf, *buf;
#if USE_LIBUSB
	libusb_device_handle *dev_handle;
	int endp_in, endp_out;
#else
	int serial;
#endif
	int flags, recv_len, recv_pos, nread;
	int verbose, timeout;
} usbio_t;

#if USE_LIBUSB
static void find_endpoints(libusb_device_handle *dev_handle, int result[2]) {
	int endp_in = -1, endp_out = -1;
	int i, k, err;
	//struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	libusb_device *device = libusb_get_device(dev_handle);
	if (!device)
		ERR_EXIT("libusb_get_device failed\n");
	//if (libusb_get_device_descriptor(device, &desc) < 0)
	//	ERR_EXIT("libusb_get_device_descriptor failed");
	err = libusb_get_config_descriptor(device, 0, &config);
	if (err < 0)
		ERR_EXIT("libusb_get_config_descriptor failed : %s\n", libusb_error_name(err));

	for (k = 0; k < config->bNumInterfaces; k++) {
		const struct libusb_interface *interface;
		const struct libusb_interface_descriptor *interface_desc;
		int claim = 0;
		interface = config->interface + k;
		if (interface->num_altsetting < 1) continue;
		interface_desc = interface->altsetting + 0;
		for (i = 0; i < interface_desc->bNumEndpoints; i++) {
			const struct libusb_endpoint_descriptor *endpoint;
			endpoint = interface_desc->endpoint + i;
			if (endpoint->bmAttributes == 2) {
				int addr = endpoint->bEndpointAddress;
				err = 0;
				if (addr & 0x80) {
					if (endp_in >= 0) ERR_EXIT("more than one endp_in\n");
					endp_in = addr;
					claim = 1;
				} else {
					if (endp_out >= 0) ERR_EXIT("more than one endp_out\n");
					endp_out = addr;
					claim = 1;
				}
			}
		}
		if (claim) {
			i = interface_desc->bInterfaceNumber;
#if LIBUSB_DETACH
			err = libusb_kernel_driver_active(dev_handle, i);
			if (err > 0) {
				DBG_LOG("kernel driver is active, trying to detach\n");
				err = libusb_detach_kernel_driver(dev_handle, i);
				if (err < 0)
					ERR_EXIT("libusb_detach_kernel_driver failed : %s\n", libusb_error_name(err));
			}
#endif
			err = libusb_claim_interface(dev_handle, i);
			if (err < 0)
				ERR_EXIT("libusb_claim_interface failed : %s\n", libusb_error_name(err));
			break;
		}
	}
	if (endp_in < 0) ERR_EXIT("endp_in not found\n");
	if (endp_out < 0) ERR_EXIT("endp_out not found\n");
	libusb_free_config_descriptor(config);

	//DBG_LOG("USB endp_in=%02x, endp_out=%02x\n", endp_in, endp_out);

	result[0] = endp_in;
	result[1] = endp_out;
}
#else
static void init_serial(int serial) {
	struct termios tty = { 0 };

	// B921600
	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);

	tty.c_cflag = CS8 | CLOCAL | CREAD;
	tty.c_iflag = IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;

	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	tcflush(serial, TCIFLUSH);
	tcsetattr(serial, TCSANOW, &tty);
}
#endif

#if USE_LIBUSB
static usbio_t* usbio_init(libusb_device_handle *dev_handle, int flags) {
#else
static usbio_t* usbio_init(int serial, int flags) {
#endif
	uint8_t *p; usbio_t *io;

#if USE_LIBUSB
	int endpoints[2];
	find_endpoints(dev_handle, endpoints);
#else
	init_serial(serial);
	// fcntl(serial, F_SETFL, FNDELAY);
	tcflush(serial, TCIOFLUSH);
#endif

	p = (uint8_t*)malloc(sizeof(usbio_t) + RECV_BUF_LEN + TEMP_BUF_LEN);
	io = (usbio_t*)p; p += sizeof(usbio_t);
	if (!p) ERR_EXIT("malloc failed\n");
	io->flags = flags;
#if USE_LIBUSB
	io->dev_handle = dev_handle;
	io->endp_in = endpoints[0];
	io->endp_out = endpoints[1];
#else
	io->serial = serial;
#endif
	io->recv_len = 0;
	io->recv_pos = 0;
	io->recv_buf = p; p += RECV_BUF_LEN;
	io->buf = p;
	io->verbose = 0;
	io->timeout = 1000;
	return io;
}

static void usbio_free(usbio_t* io) {
	if (!io) return;
#if USE_LIBUSB
	libusb_close(io->dev_handle);
#else
	close(io->serial);
#endif
	free(io);
}

#define WRITE16_LE(p, a) do { \
	((uint8_t*)(p))[0] = (uint8_t)(a); \
	((uint8_t*)(p))[1] = (a) >> 8; \
} while (0)

#define WRITE32_LE(p, a) do { \
	((uint8_t*)(p))[0] = (uint8_t)(a); \
	((uint8_t*)(p))[1] = (a) >> 8; \
	((uint8_t*)(p))[2] = (a) >> 16; \
	((uint8_t*)(p))[3] = (a) >> 24; \
} while (0)

#define READ16_LE(p) ( \
	((uint8_t*)(p))[1] << 8 | \
	((uint8_t*)(p))[0])

#define READ32_LE(p) ( \
	((uint8_t*)(p))[3] << 24 | \
	((uint8_t*)(p))[2] << 16 | \
	((uint8_t*)(p))[1] << 8 | \
	((uint8_t*)(p))[0])

static int usb_send(usbio_t *io, const void *data, int len) {
	const uint8_t *buf = (const uint8_t*)data;
	int ret;

	if (!buf) buf = io->buf;
	if (!len) ERR_EXIT("empty message\n");
	if (io->verbose >= 2) {
		DBG_LOG("send (%d):\n", len);
		print_mem(stderr, buf, len);
	}

#if USE_LIBUSB
	{
		int err = libusb_bulk_transfer(io->dev_handle,
				io->endp_out, (uint8_t*)buf, len, &ret, io->timeout);
		if (err < 0)
			ERR_EXIT("usb_send failed : %s\n", libusb_error_name(err));
	}
#else
	ret = write(io->serial, buf, len);
#endif
	if (ret != len)
		ERR_EXIT("usb_send failed (%d / %d)\n", ret, len);

#if !USE_LIBUSB
	tcdrain(io->serial);
	// usleep(1000);
#endif
	return ret;
}

static int usb_recv(usbio_t *io, int plen) {
	uint8_t *buf = io->buf;
	int a, pos, len, nread = 0;
	if (plen > TEMP_BUF_LEN)
		ERR_EXIT("target length too long\n");

	len = io->recv_len;
	pos = io->recv_pos;
	while (nread < plen) {
		if (pos >= len) {
#if USE_LIBUSB
			int err = libusb_bulk_transfer(io->dev_handle, io->endp_in, io->recv_buf, RECV_BUF_LEN, &len, io->timeout);
			if (err == LIBUSB_ERROR_NO_DEVICE)
				ERR_EXIT("connection closed\n");
			else if (err == LIBUSB_ERROR_TIMEOUT) break;
			else if (err < 0)
				ERR_EXIT("usb_recv failed : %s\n", libusb_error_name(err));
#else
			if (io->timeout >= 0) {
				struct pollfd fds = { 0 };
				fds.fd = io->serial;
				fds.events = POLLIN;
				a = poll(&fds, 1, io->timeout);
				if (a < 0) ERR_EXIT("poll failed, ret = %d\n", a);
				if (fds.revents & POLLHUP)
					ERR_EXIT("connection closed\n");
				if (!a) break;
			}
			len = read(io->serial, io->recv_buf, RECV_BUF_LEN);
#endif
			if (len < 0)
				ERR_EXIT("usb_recv failed, ret = %d\n", len);

			if (io->verbose >= 2) {
				DBG_LOG("recv (%d):\n", len);
				print_mem(stderr, io->recv_buf, len);
			}
			pos = 0;
			if (!len) break;
		}
		a = io->recv_buf[pos++];
		io->buf[nread++] = a;
	}
	io->recv_len = len;
	io->recv_pos = pos;
	io->nread = nread;
	return nread;
}

static uint8_t* loadfile(const char *fn, size_t *num) {
	size_t n, j = 0; uint8_t *buf = 0;
	FILE *fi = fopen(fn, "rb");
	if (fi) {
		fseek(fi, 0, SEEK_END);
		n = ftell(fi);
		if (n) {
			fseek(fi, 0, SEEK_SET);
			buf = (uint8_t*)malloc(n);
			if (buf) j = fread(buf, 1, n, fi);
		}
		fclose(fi);
	}
	if (num) *num = j;
	return buf;
}

typedef struct {
	FILE *file;
} file_t;

static file_t open_files[MAX_FILES];

static FILE* get_file(unsigned handle) {
	if (handle >= MAX_FILES) return NULL;
	return open_files[handle].file;
}

static int get_free_handle(void) {
	int i;
	for (i = 0; i < MAX_FILES; i++) {
		if (!open_files[i].file) return i;
	}
	return -1;
}

static unsigned fastchk16(unsigned crc, const void *src, int len) {
	uint8_t *s = (uint8_t*)src;

	while (len > 1) {
		crc += s[1] << 8 | s[0]; s += 2;
		len -= 2;
	}
	if (len) crc += *s;
	crc = (crc >> 16) + (crc & 0xffff);
	crc += crc >> 16;
	return crc & 0xffff;
}

static int read_msg(usbio_t *io) {
	if (usb_recv(io, 3) != 3)
		ERR_EXIT("response timeout\n");
	int len = io->buf[0];
	int chk = CHECKSUM_INIT, chk2;
	chk += CMD_MESSAGE + (len << 8);
	chk2 = READ16_LE(io->buf + 1);

	if (usb_recv(io, len) != len)
		ERR_EXIT("response timeout\n");
	chk = fastchk16(chk, io->buf, len);
	if (chk != chk2)
		ERR_EXIT("bad checksum\n");

	io->buf[len] = 0;
	printf("!!! %s\n", io->buf);
	return len;
}

static int check_path(char *path) {
	char *s = path, *d = s;
	int a, b = '/';
	if (s[0] == '.' && s[1] == b) s += 2;
	for (;;) {
		a = *s++;
		if (!a) break;
		// ambiguous character
		if (a == '\\') return -1;
		// printable ASCII characters only
		if ((unsigned)(a - 0x20) >= 0x5f) return -1;
		// don't allow names to end with a space
		if (b == ' ' && a == '/') return -1;
		if (b == '/') {
			if (a == b) continue;
			// don't allow names to start with a space
			if (a == ' ') return -1;
			if (a == '.') {
				// don't allow reading hidden files
				if (s[0] != '.' || s[1] != '/') return -1;
				// try to process "../"
				if (d == path) return -1;
				--d;
				do {
					if (d == path) return -1;
					a = *--d;
				} while (a != '/');
			}
		}
		*d++ = b = a;
	}
	// don't allow filenames to end with a space
	if (b == ' ') return -1;
	*d = a;
	return d - path;
}

static void send_args(usbio_t *io, const char *name,
		int flags, int argc, char **argv, int skip) {
	uint16_t buf[3];
	size_t size = 0; int i, chk;

	argc = argc < skip ? 0 : argc - skip;
	argv += skip;

	for (i = 0; i < argc; i++) {
		const char *str = i || !name ? argv[i] : name;
		size_t len = strlen(str) + 1;
		if (TEMP_BUF_LEN - size < len)
			ERR_EXIT("fread: requested length too big\n");
		memcpy(io->buf + size, str, len);
		size += len;
	}
	i = 0;
	if (flags & _ARGV_GET_ARGC) buf[i++] = argc;
	if (flags & _ARGV_GET_SIZE) buf[i++] = size;
	chk = fastchk16(CHECKSUM_INIT, buf, i * 2);
	if (flags & _ARGV_GET_ARGV)
		chk = fastchk16(chk, io->buf, size);
	buf[i++] = chk;
	usb_send(io, buf, i * 2);
	if (flags & _ARGV_GET_ARGV)
		usb_send(io, NULL, size);
}

#define REOPEN_FREQ 2

int main(int argc, char **argv) {
#if USE_LIBUSB
	libusb_device_handle *device;
#else
	int serial;
#endif
	usbio_t *io; int ret, i;
	int wait = 0 * REOPEN_FREQ;
	const char *tty = "/dev/ttyUSB0";
	int verbose = 0, is_tty;

#if USE_LIBUSB
	ret = libusb_init(NULL);
	if (ret < 0)
		ERR_EXIT("libusb_init failed: %s\n", libusb_error_name(ret));
#endif

	while (argc > 1) {
		if (!strcmp(argv[1], "--tty")) {
			if (argc <= 2) ERR_EXIT("bad option\n");
			tty = argv[2];
			argc -= 2; argv += 2;
		} else if (!strcmp(argv[1], "--wait")) {
			if (argc <= 2) ERR_EXIT("bad option\n");
			wait = atoi(argv[2]) * REOPEN_FREQ;
			argc -= 2; argv += 2;
		} else if (!strcmp(argv[1], "--verbose")) {
			if (argc <= 2) ERR_EXIT("bad option\n");
			verbose = atoi(argv[2]);
			argc -= 2; argv += 2;
		} else if (!strcmp(argv[1], "--")) {
			argc -= 1; argv += 1;
			break;
		} else if (argv[1][0] == '-') {
			ERR_EXIT("unknown option\n");
		} else break;
	}

	for (i = 0; ; i++) {
#if USE_LIBUSB
		device = libusb_open_device_with_vid_pid(NULL, 0x1782, 0x4d00);
		if (device) break;
		if (i >= wait)
			ERR_EXIT("libusb_open_device failed\n");
#else
		serial = open(tty, O_RDWR | O_NOCTTY | O_SYNC);
		if (serial >= 0) break;
		if (i >= wait)
			ERR_EXIT("open(ttyUSB) failed\n");
#endif
		if (!i) DBG_LOG("Waiting for connection (%ds)\n", wait / REOPEN_FREQ);
		usleep(1000000 / REOPEN_FREQ);
	}

#if USE_LIBUSB
	io = usbio_init(device, 0);
#else
	io = usbio_init(serial, 0);
#endif
	io->verbose = verbose;

	// init
	memset(open_files, 0, sizeof(open_files));
	open_files[0].file = stdin;
	open_files[1].file = stdout;
	open_files[2].file = stderr;

	{
		int ret = 0, i, old_timeout;

		old_timeout = io->timeout;
		io->timeout = 100;
		for (i = 0; i < 10; i++) {
			io->buf[0] = HOST_CONNECT;
			usb_send(io, NULL, 1);
			ret = usb_recv(io, 1);
			if (ret) break;
		}
		io->timeout = old_timeout;
		if (!ret) ERR_EXIT("not respond\n");
	}

	if (io->buf[0] != CMD_MESSAGE)
		ERR_EXIT("unexpected response\n");
	read_msg(io);

  is_tty = isatty(fileno(stdin));

	for (;;) {
		int cmd;
		if (usb_recv(io, 1) != 1) continue;
		cmd = io->buf[0];
		switch (cmd) {
		case CMD_FOPEN:
			if (usb_recv(io, 5) != 5)
				ERR_EXIT("response timeout\n");
			{
				int flags = io->buf[0], len;
				int ret = get_free_handle();
				const char *mstr;
				int chk = CHECKSUM_INIT, chk2;

				len = READ16_LE(io->buf + 1);
				chk2 = READ16_LE(io->buf + 3);
				chk += cmd + (io->buf[0] << 8);
				chk += len;

				if (len >= TEMP_BUF_LEN)
					ERR_EXIT("fread: requested length too big\n");
				if (usb_recv(io, len) != len)
					ERR_EXIT("response timeout\n");
				chk = fastchk16(chk, io->buf, len);
				if (chk != chk2)
					ERR_EXIT("bad checksum\n");
				io->buf[len] = 0;

				mstr = "rb";
				if (flags & _IO_WRITE)
					mstr = flags & _IO_APPEND ? "ab" : "wb";

				if (check_path((char*)io->buf) < 1) ret = -1;
				if (ret >= 0) {
					FILE *f = fopen((char*)io->buf, mstr);
					if (f) open_files[ret].file = f;
					else ret = -1;
				}
				io->buf[0] = ret;
				usb_send(io, NULL, 1);
			}
			break;

		case CMD_FREAD:
			if (usb_recv(io, 5) != 5)
				ERR_EXIT("response timeout\n");
			{
				FILE *f = get_file(io->buf[0]);
				int chk = CHECKSUM_INIT;
				size_t ret, size = READ16_LE(io->buf + 1);
				if (size > TEMP_BUF_LEN)
					ERR_EXIT("fread: requested length too big\n");

				chk += cmd + (io->buf[0] << 8);
				chk = fastchk16(chk, io->buf + 1, 2);
				if (chk != READ16_LE(io->buf + 3))
					ERR_EXIT("bad checksum\n");

				if (is_tty && f == stdin) {
					for (ret = 0; ret < size; ret++) {
						int ch = fgetc(f);
						io->buf[ret] = ch;
						if (ch == '\n') break;
					}
				} else {
					ret = fread(io->buf, 1, size, f);
				}
				chk = CHECKSUM_INIT + (ret & 0xffff);
				chk = (chk + (chk >> 16)) & 0xffff;
				if (ret > 0)
					chk = fastchk16(chk, io->buf, ret);
				{
					uint8_t buf[4];
					WRITE16_LE(buf, ret);
					WRITE16_LE(buf + 2, chk);
					usb_send(io, buf, 4);
				}
				if (ret > 0) usb_send(io, NULL, ret);
			}
			break;

		case CMD_FWRITE:
			if (usb_recv(io, 5) != 5)
				ERR_EXIT("response timeout\n");
			{
				FILE *f = get_file(io->buf[0]);
				int n2, chk = CHECKSUM_INIT, chk2;
				size_t ret = 0, size = READ16_LE(io->buf + 1);
				if (size > TEMP_BUF_LEN)
					ERR_EXIT("fwrite: requested length too big\n");

				chk += cmd + (io->buf[0] << 8);
				chk += size;
				// chk = (chk + (chk >> 16)) & 0xffff;
				chk2 = READ16_LE(io->buf + 3);
				if (usb_recv(io, size) != (int)size)
					ERR_EXIT("response timeout\n");
				chk = fastchk16(chk, io->buf, size);
				if (chk != chk2)
					ERR_EXIT("bad checksum (0x%04x, expected 0x%04x)\n", chk2, chk);

				if (f) ret = fwrite(io->buf, 1, size, f);

				WRITE16_LE(io->buf, ret);
				usb_send(io, NULL, 2);
			}
			break;

		case CMD_FCLOSE:
			if (usb_recv(io, 3) != 3)
				ERR_EXIT("response timeout\n");
			{
				int handle = io->buf[0];
				FILE *f = get_file(handle);
				int ret = EOF;
				int chk = CHECKSUM_INIT;
				chk += cmd + (io->buf[0] << 8);
				chk = (chk + (chk >> 16)) & 0xffff;
				if (chk != READ16_LE(io->buf + 1))
					ERR_EXIT("bad checksum\n");

				if (f) {
					open_files[handle].file = NULL;
					ret = fclose(f);
				}
				io->buf[0] = ret ? EOF : 0;
				usb_send(io, NULL, 1);
			}
			break;

		case CMD_FSEEK:
			if (usb_recv(io, 9) != 9)
				ERR_EXIT("response timeout\n");
			{
				FILE *f = get_file(io->buf[0]);
				int origin = READ16_LE(io->buf + 1), ret = -1;
				int32_t offset = READ32_LE(io->buf + 3);

				int chk = CHECKSUM_INIT;
				chk += cmd + (io->buf[0] << 8);
				chk = fastchk16(chk, io->buf + 1, 6);
				if (chk != READ16_LE(io->buf + 7))
					ERR_EXIT("bad checksum\n");

				if (f) ret = fseek(f, offset, origin);

				io->buf[0] = ret;
				usb_send(io, NULL, 1);
			}
			break;

		case CMD_FTELL:
			if (usb_recv(io, 3) != 3)
				ERR_EXIT("response timeout\n");
			{
				FILE *f = get_file(io->buf[0]);
				int ret = EOF;
				int chk = CHECKSUM_INIT;
				chk += cmd + (io->buf[0] << 8);
				chk = (chk + (chk >> 16)) & 0xffff;
				if (chk != READ16_LE(io->buf + 1))
					ERR_EXIT("bad checksum\n");

				if (f) ret = ftell(f);
				WRITE32_LE(io->buf, ret);
				usb_send(io, NULL, 4);
			}
			break;

		case CMD_MESSAGE:
			read_msg(io);
			break;

		case CMD_GETARGS:
			if (usb_recv(io, 5) != 5)
				ERR_EXIT("response timeout\n");
			{
				int flags = io->buf[0];
				int skip = READ16_LE(io->buf + 1);

				int chk = CHECKSUM_INIT;
				chk += cmd + (io->buf[0] << 8);
				chk = fastchk16(chk, io->buf + 1, 2);
				if (chk != READ16_LE(io->buf + 3))
					ERR_EXIT("bad checksum\n");

				send_args(io, NULL, flags, argc, argv, skip + 1);
			}
			break;

		default:
			ERR_EXIT("unknown command\n");
		}
	}

	usbio_free(io);
#if USE_LIBUSB
	libusb_exit(NULL);
#endif
	return 0;
}
