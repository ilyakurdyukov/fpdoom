enum {
	HOST_CONNECT = 0x00,

	CMD_MESSAGE = 0x80,
	CMD_FOPEN = 0x81,
	CMD_FREAD = 0x82,
	CMD_FWRITE = 0x83,
	CMD_FCLOSE = 0x84,
	CMD_FSEEK = 0x85,
	CMD_FTELL = 0x86,
	CMD_GETARGS = 0x87,
	CMD_RESETIO = 0x88,
	CMD_USBBENCH = 0x89,
};

#define CHECKSUM_INIT 0x5a5a

#define MAX_FILES 255
#if UMS9117
// FIXME: should not request more than the USB buffer size
#define MAX_IOSIZE 0x3000
#else
#define MAX_IOSIZE 0x4000
#endif

#define _IO_LINEBUF 1
#define _IO_WRITE 2
#define _IO_PIPE 4
#define _IO_APPEND 8

#define _ARGV_GET_ARGC 1
#define _ARGV_GET_SIZE 2
#define _ARGV_GET_ARGV 4

