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

static void sfc_read_req(int cs, int addr, unsigned n) {
	uint32_t tmp, k;
	sfc_base_t *sfc0 = SFC_BASE, *sfc;
	sfc = sfc0 + cs;

	while (sfc_read_status(cs) & 1);

	sfc->tbuf_clr = 1;
	sfc0->cmd_set &= 1; // write
	SFC_CMDSET(sfc, 1, 7);
	sfc->cmd[0] = addr >> 24 ? 0x13 : 0x03; // Read Array
	sfc->cmd[1] = addr;
	tmp = SFC_TYPEINFO(1, 1, WRITE, 0) |
			SFC_TYPEINFO(1, 3, WRITE, 1) << 8;
	if (addr >> 24) tmp += 1 << (3 + 8);
	sfc->type_info[0] = tmp;
	k = 7; do {
		tmp = 4;
		if (n < 4) tmp = n;
		n -= tmp;
		tmp = SFC_TYPEINFO(1, tmp, READ, 0);
		sfc->type_info[k >> 2] |= tmp << (k & 3) * 8;
		k++;
	} while (n);
	sfc0->int_clr = 1 << cs;
	sfc0->soft_req |= 1;
	while (!(sfc->status & 1));
}

void sfc_read(int cs, int addr, void *buf, unsigned size) {
	uint8_t *dst = (uint8_t*)buf, *end = dst + size;
	sfc_base_t *sfc0 = SFC_BASE, *sfc;
	sfc = sfc0 + cs;

	while (dst < end) {
		uint32_t tmp, k, n = end - dst;
		if (n > 20) n = 20;
		sfc_read_req(cs, addr, n);
		addr += n;
		for (k = 7; n >= 4; k++, n -= 4, dst += 4) {
			tmp = sfc->cmd[k];
			dst[0] = tmp >> 24;
			dst[1] = tmp >> 16;
			dst[2] = tmp >> 8;
			dst[3] = tmp;
		}
		if (n) {
			tmp = sfc->cmd[k];
			dst[0] = tmp >> 24;
			if (n >= 2) dst[1] = tmp >> 16;
			if (n > 2) dst[2] = tmp >> 8;
			dst += n;
		}
	}
}

unsigned sfc_compare(int cs, int addr, const void *buf, unsigned size, uint32_t mode) {
	const uint8_t *src = (const uint8_t*)buf, *end = src + size;
	sfc_base_t *sfc = SFC_BASE + cs;
	uint32_t diff = 0, tmp2;

	while (src < end) {
		uint32_t tmp, k, n = end - src;
		if (n > 20) n = 20;
		sfc_read_req(cs, addr, n);
		addr += n;
		for (k = 7; n >= 4; k++, n -= 4, src += 4) {
			tmp = ~0;
			if (mode != SFC_CMP_ERASE)
				tmp = src[0] << 24 | src[1] << 16 | src[2] << 8 | src[3];
			tmp2 = sfc->cmd[k] ^ tmp;
			if (tmp2 & (tmp | mode)) goto end;
			diff |= tmp2;
		}
		if (n) {
			tmp = ~0;
			if (mode != SFC_CMP_ERASE) {
				tmp = src[0] << 24;
				if (n >= 2) tmp |= src[1] << 16;
				if (n > 2) tmp |= src[2] << 8;
			}
			tmp2 = sfc->cmd[k] ^ tmp;
			tmp >>= k = (4 - n) * 8; tmp2 >>= k;
			if (tmp2 & (tmp | mode)) goto end;
			diff |= tmp2;
			src += n;
		}
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

const char* sfc_getvendor(uint32_t id) {
	const char *name = NULL;
	switch (id >> 16) {
	case 0x01: name = "Spansion"; break;
	case 0x20: name = "XMC"; break;
	case 0x2c: name = "Micron"; break;
	case 0x8c: name = "ESMT"; break;
	case 0xbf: name = "SST"; break;
	case 0xc2: name = "Macronix"; break;
	case 0xc8: name = "GigaDevice"; break;
	case 0xef: name = "Winbond"; break;
	case 0xf8: name = "Fidelix/Dosilicon"; break;
	}
	return name;
}

static const char *sfc_flash_list = 
#define X(id, name) id name "\0"

/* Spansion */
X("\x01\x02\x13", "S25FL008A")
X("\x01\x02\x14", "S25FL016A")
X("\x01\x02\x15", "S25FL032A")
X("\x01\x02\x16", "S25FL064A")
X("\x01\x20\x18", "S25FL128P") /* has an extended id */

/* XMC */
X("\x20\x38\x17", "XM25QU64A")

/* Micron */
X("\x2c\xcb\x17", "N25W064")
X("\x2c\xcb\x18", "N25W128")

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
X("\xc8\x40\x16", "25Q32B")
X("\xc8\x40\x17", "25Q64B")
X("\xc8\x60\x15", "25LQ16")
X("\xc8\x60\x16", "25LQ32B")
X("\xc8\x60\x17", "25LQ64B")
X("\xc8\x60\x18", "25LQ128")
X("\xc8\x60\x19", "25LQ256")

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
	while ((a = *p)) {
		if (a == id1 && p[1] == id2 && p[2] == id3)
			return (char*)p + 3;
		p += strlen((char*)p + 3) + 4;
	}
	return NULL;
}

