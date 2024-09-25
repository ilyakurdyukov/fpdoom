#include <string.h>
#include "sfc.h"
#include "common.h"
#include "syscode.h"

void sfc_init(void) {
	MEM4((uint32_t)SFC_BASE + 0x204) |= 3;
}

void sfc_cmdclr(sfc_base_t *sfc) {
	unsigned i;
	for (i = 0; i < 12; i++) sfc->cmd[i] = 0;
}

uint32_t sfc_cmd_read(int cs, unsigned cmd, unsigned n) {
	sfc_base_t *sfc0 = SFC_BASE, *sfc;
	sfc = sfc0 + cs;

	sfc->tbuf_clr = 1;
	sfc->cmd[7] = 0;
	sfc0->cmd_set &= ~1; // write
	SFC_CMDSET(sfc, 1, 7);
	sfc->cmd[0] = cmd;
	sfc->type_info[0 / 4] = SFC_TYPEINFO(1, 1, WRITE, 0);
	sfc->type_info[7 / 4] = SFC_TYPEINFO(1, n, READ, 0) << 24;
	sfc0->int_clr = 1 << cs;
	sfc0->soft_req |= 1;
	while (!(sfc->status & 1));

	return sfc->cmd[7];
}

void sfc_write_status(int cs, unsigned val) {
	sfc_base_t *sfc0 = SFC_BASE, *sfc;
	sfc = sfc0 + cs;

	sfc_write_enable(cs);

	sfc->tbuf_clr = 1;
	sfc0->cmd_set &= ~1; // write
	SFC_CMDSET(sfc, 1, 7);
	sfc->cmd[0] = 0x01; // Write Status Register
	sfc->cmd[1] = val;
	sfc->type_info[0] =
			SFC_TYPEINFO(1, 1, WRITE, 0) |
			SFC_TYPEINFO(1, 1, WRITE, 0) << 8;
	sfc0->int_clr = 1 << cs;
	sfc0->soft_req |= 1;
	while (!(sfc->status & 1));

	SFC_WRITE_WAIT(cs);
}

void sfc_write_enable(int cs) {
	sfc_base_t *sfc0 = SFC_BASE, *sfc;
	sfc = sfc0 + cs;

	sfc->tbuf_clr = 1;
	sfc0->cmd_set &= ~1; // write
	SFC_CMDSET(sfc, 1, 7);
	sfc->cmd[0] = 0x06; // Write Enable
	sfc->type_info[0 / 4] = SFC_TYPEINFO(1, 1, WRITE, 0);
	sfc0->int_clr = 1 << cs;
	sfc0->soft_req |= 1;
	while (!(sfc->status & 1));

	while (!(sfc_read_status(cs) & 2));
}

void sfc_erase(int cs, int addr, int cmd, int addr_len) {
	uint32_t tmp;
	sfc_base_t *sfc0 = SFC_BASE, *sfc;
	sfc = sfc0 + cs;
	sfc_write_enable(cs);

	sfc->tbuf_clr = 1;
	sfc0->cmd_set &= ~1; // write
	SFC_CMDSET(sfc, 1, 7);
	sfc->cmd[0] = cmd;
	sfc->cmd[1] = addr;
	tmp = SFC_TYPEINFO(1, 1, WRITE, 0);
	if (addr_len)
		tmp |= SFC_TYPEINFO(1, addr_len, WRITE, 1) << 8;
	sfc->type_info[0] = tmp;
	sfc0->int_clr = 1 << cs;
	sfc0->soft_req |= 1;
	while (!(sfc->status & 1));

	SFC_WRITE_WAIT(cs);
}

void sfc_write(int cs, int addr, const void *buf, unsigned size) {
	const uint8_t *src = (const uint8_t*)buf, *end = src + size;
	sfc_base_t *sfc0 = SFC_BASE, *sfc;
	sfc = sfc0 + cs;

	while (src < end) {
		uint32_t tmp;
		sfc_write_enable(cs);

		sfc->tbuf_clr = 1;
		sfc0->cmd_set &= ~1; // write
		SFC_CMDSET(sfc, 1, 7);
		sfc->cmd[0] = addr >> 24 ? 0x12 : 0x02; // Page Program
		sfc->cmd[1] = addr;
		tmp = SFC_TYPEINFO(1, 1, WRITE, 0) |
				SFC_TYPEINFO(1, 3, WRITE, 1) << 8;
		if (addr >> 24) tmp += 1 << (3 + 8);
		sfc->type_info[0] = tmp;
		{
			unsigned k, n = end - src;
			// could be 40, but FDL use 32
			if (n > 32) n = 32;
			// don't allow to cross page boundary
			k = 256 - (addr & 255);
			if (n > k) n = k;
			addr += n;
			for (k = 2; n >= 4; k++, n -= 4, src += 4) {
				sfc->cmd[k] = src[0] | src[1] << 8 | src[2] << 16 | src[3] << 24;
				tmp = SFC_TYPEINFO(1, 4, WRITE, 0);
				sfc->type_info[k >> 2] |= tmp << (k & 3) * 8;
			}
			if (n) {
				uint32_t a = src[0];
				if (n >= 2) a |= src[1] << 8;
				if (n > 2) a |= src[2] << 16;
				sfc->cmd[k] = a; src += n;
				tmp = SFC_TYPEINFO(1, n, WRITE, 0);
				sfc->type_info[k >> 2] |= tmp << (k & 3) * 8;
			}
		}
		sfc0->int_clr = 1 << cs;
		sfc0->soft_req |= 1;
		while (!(sfc->status & 1));

		SFC_WRITE_WAIT(cs);
	}
}

static void sfc_read_req(int cs, int cmd, int addr_len,
		int addr, uint8_t *buf, unsigned n) {
	uint32_t tmp, k, n2;
	sfc_base_t *sfc0 = SFC_BASE, *sfc = sfc0 + cs;

	while (sfc_read_status(cs) & 1);

	sfc->tbuf_clr = 1;
	sfc0->cmd_set &= 1; // write
	SFC_CMDSET(sfc, 1, 7);
	sfc->cmd[0] = cmd;
	sfc->cmd[1] = addr;
	sfc->type_info[0] =
			SFC_TYPEINFO(1, 1, WRITE, 0) |
			SFC_TYPEINFO(1, addr_len, WRITE, 1) << 8;
	k = 7; n2 = n; do {
		tmp = 4;
		if (n2 < 4) tmp = n2;
		n2 -= tmp;
		tmp = SFC_TYPEINFO(1, tmp, READ, 0);
		sfc->type_info[k >> 2] |= tmp << (k & 3) * 8;
		k++;
	} while (n2);
	sfc0->int_clr = 1 << cs;
	sfc0->soft_req |= 1;
	while (!(sfc->status & 1));

	for (k = 7; n >= 4; k++, n -= 4, buf += 4) {
		tmp = sfc->cmd[k];
		buf[0] = tmp >> 24;
		buf[1] = tmp >> 16;
		buf[2] = tmp >> 8;
		buf[3] = tmp;
	}
	if (n) {
		tmp = sfc->cmd[k];
		buf[0] = tmp >> 24;
		if (n >= 2) buf[1] = tmp >> 16;
		if (n > 2) buf[2] = tmp >> 8;
	}
}

static unsigned sfc_read_chunk(int cs, int addr, void *buf, unsigned n) {
	uint32_t j, k = 20;
	if ((j = 0xffffff - addr) < 19) k = j + 1;
	if (n > k) n = k;
	sfc_read_req(cs, addr >> 24 ? 0x13 : 0x03, addr >> 24 ? 4 : 3, addr, buf, n);
	return n;
}

void sfc_read(int cs, int addr, void *buf, unsigned size) {
	uint8_t *dst = (uint8_t*)buf, *end = dst + size;
	unsigned n;

	while ((n = end - dst)) {
		n = sfc_read_chunk(cs, addr, dst, n);
		addr += n; dst += n;
	}
}

/* Serial Flash Discoverable Parameter */
void sfc_read_sfdp(int cs, int addr, void *buf, unsigned size) {
	uint8_t *dst = (uint8_t*)buf, *end = dst + size;
	unsigned n;

	while ((n = end - dst)) {
		if (n > 20) n = 20;
		sfc_read_req(cs, 0x5a, 4, addr << 8, dst, n);
		addr += n; dst += n;
	}
}

unsigned sfc_compare(int cs, int addr, const void *src0, unsigned size, uint32_t mode) {
	const uint8_t *src = (const uint8_t*)src0, *end = src + size;
	uint32_t diff = 0, tmp2; unsigned n;

	while ((n = end - src)) {
		uint32_t tmp, k; uint8_t buf[20];
		n = sfc_read_chunk(cs, addr, buf, n);
		addr += n;
		k = 0; do {
			tmp = ~0;
			if (mode != SFC_CMP_ERASE) tmp = *src;
			tmp2 = buf[k] ^ tmp;
			if (tmp2 & (tmp | mode)) goto end;
			diff |= tmp2; src++;
		} while (++k < n);
	}
	if (diff) return ~0;
end:
	return end - src;
}

/* return to reading mode */
void sfc_spiread(int cs) {
	sfc_base_t *sfc0 = SFC_BASE, *sfc;
	sfc = sfc0 + cs;

	sfc->tbuf_clr = 1;
	sfc_cmdclr(sfc);
	sfc0->cmd_set |= 1; // read
	SFC_CMDSET(sfc, 1, 7);
	sfc->cmd[0] = 0x03;
	sfc->type_info[0] =
			SFC_TYPEINFO(1, 1, WRITE, 0) |
			SFC_TYPEINFO(1, 3, WRITE, 1) << 8;
	sfc0->int_clr = 1 << cs;
	//sfc->clk = 2;
}

static const char *sfc_vendor_list =
#define X(id, name) id name "\0"
X("\x01", "Spansion")
X("\x20", "XMC")
X("\x25", "Silicon Kaiser")
X("\x2c", "Micron")
X("\x5e", "Zbit")
X("\x68", "BoyaMicro")
X("\x8c", "ESMT")
X("\xbf", "SST")
X("\xc2", "Macronix")
X("\xc8", "GigaDevice")
X("\xe0", "Winbond?")
X("\xef", "Winbond")
X("\xf8", "Fidelix")
#undef X
;

const char* sfc_getvendor(uint32_t id) {
	const uint8_t *p = (uint8_t*)sfc_vendor_list;
	unsigned id1 = id >> 16 & 0xff, a;
	if ((id - 0xf84300) >> 8 == 0)
		return "Dosilicon";
	while ((a = *p++)) {
		if (a == id1) return (char*)p;
		p += strlen((char*)p) + 1;
	}
	return NULL;
}

static const char *sfc_flash_list =
#define X(id, name) id name "\0"

// Most devices contain Winbond or GigaDevice chips,
// references to the others are found in the firmware.

/* Spansion */
X("\x01\x02\x13", "S25FL008A")
X("\x01\x02\x14", "S25FL016A")
X("\x01\x02\x15", "S25FL032A")
X("\x01\x02\x16", "S25FL064A")
X("\x01\x20\x18", "S25FL128P") /* has an extended id */

/* XMC */
X("\x20\x38\x17", "XM25QU64A")
X("\x20\x50\x16", "XM25QH32B")
X("\x20\x50\x18", "XM25QH128B")

/* Silicon Kaiser */
X("\x25\x70\x15", "SK25LE016")
X("\x25\x70\x16", "SK25LE032")
X("\x25\x70\x17", "SK25LE064")
X("\x25\x70\x18", "SK25LE0128")
X("\x25\x70\x19", "SK25LE0256")

/* Micron */
X("\x2c\xcb\x16", "N25W032")
X("\x2c\xcb\x17", "N25W064")
X("\x2c\xcb\x18", "N25W128")

/* Zbit */
X("\x5e\x40\x16", "ZB25VQ32")
X("\x5e\x40\x17", "ZB25VQ64")
X("\x5e\x50\x16", "ZB25LQ32")

/* BoyaMicro */
X("\x68\x40\x16", "BY25Q32B")
X("\x68\x40\x17", "BY25Q64B")

/* ESMT */
X("\x8c\x25\x37", "F25D64QA")

/* SST */
X("\xbf\x25\x01", "SST25WF512")
X("\xbf\x25\x02", "SST25WF010")
X("\xbf\x25\x03", "SST25WF020")
X("\xbf\x25\x04", "SST25WF040")
X("\xbf\x25\x8d", "SST25VF040B")
X("\xbf\x25\x8e", "SST25VF080B")
X("\xbf\x25\x41", "SST25VF016B")
X("\xbf\x25\x4a", "SST25VF032B")

/* Macronix */
X("\xc2\x20\x15", "MX25L1605D")
X("\xc2\x20\x16", "MX25L3205D")
X("\xc2\x20\x17", "MX25L6405D")
X("\xc2\x20\x18", "MX25L12805D")
X("\xc2\x25\x36", "MX25U3235E")
X("\xc2\x25\x37", "MX25U6435E")
X("\xc2\x25\x38", "MX25U12835E")
X("\xc2\x26\x18", "MX25L12855E")

/* GigaDevice */
X("\xc8\x40\x16", "GD25Q32B")
X("\xc8\x40\x17", "GD25Q64B")
X("\xc8\x60\x15", "GD25LQ16")
X("\xc8\x60\x16", "GD25LQ32B")
X("\xc8\x60\x17", "GD25LQ64B")
X("\xc8\x60\x18", "GD25LQ128")
X("\xc8\x60\x19", "GD25LQ256")

/* Winbond */
X("\xef\x30\x15", "W25X16")
X("\xef\x30\x16", "W25X32")
X("\xef\x30\x17", "W25X64")
X("\xef\x40\x15", "W25Q16")
X("\xef\x40\x16", "W25Q32FV")
X("\xef\x40\x17", "W25Q64FV")
X("\xef\x40\x18", "W25Q128FV")
X("\xef\x40\x19", "W25Q256FV")
X("\xef\x60\x16", "W25Q32DW")
X("\xef\x60\x17", "W25Q64FW")
X("\xef\x60\x18", "W25Q128FW")

/* Fidelix */
X("\xf8\x32\x16", "FM25Q32")
X("\xf8\x42\x15", "FM25M16A")
X("\xf8\x42\x16", "FM25M32A")
X("\xf8\x42\x17", "FM25M64A")
X("\xf8\x42\x18", "FM25M128A")
/* Dosilicon */
X("\xf8\x43\x17", "FM25M64C")
X("\xf8\x43\x18", "FM25M4AA")

#undef X
;

const char* sfc_getname(uint32_t id) {
	const uint8_t *p = (uint8_t*)sfc_flash_list;
	unsigned id1 = id >> 16 & 0xff, a;
	unsigned id2 = id >> 8 & 0xff, id3 = id & 0xff;
	if (id1 == 0xe0) id1 = 0xef;
	while ((a = *p)) {
		if (a == id1 && p[1] == id2 && p[2] == id3)
			return (char*)p + 3;
		p += strlen((char*)p + 3) + 4;
	}
	return NULL;
}

