#include "a.h"
#include "compat.h"

#ifdef USE_ASM
extern
#endif
struct {
	int bpl, transmode;
	int logx, logy, bxinc, byinc, pinc;
	unsigned char *buf, *pal, *hlinepal, *trans;
} buildasm_data;
#define g buildasm_data

// Global variable functions
void setvlinebpl(int dabpl) { g.bpl = dabpl; }
void fixtransluscence(void *datransoff) { g.trans = datransoff; }
void settransnormal(void) { g.transmode = 0; }
void settransreverse(void) { g.transmode = 1; }

// Ceiling/floor horizontal line functions
void sethlinesizes(int logx, int logy, void *bufplc)
	{ g.logx = logx; g.logy = logy; g.buf = bufplc; }
void setpalookupaddress(void *paladdr) { g.hlinepal = paladdr; }
void setuphlineasm4(int bxinc, int byinc) { g.bxinc = bxinc; g.byinc = byinc; }

// Sloped ceiling/floor vertical line functions
void setupslopevlin(int logylogx, void *bufplc, int pinc) {
	g.logx = logylogx & 255; g.logy = logylogx >> 8;
	g.buf = bufplc; g.pinc = pinc;
}

// Wall,face sprite/wall sprite vertical line functions
void setupvlineasm(int neglogy) { g.logy = neglogy; }
void setupmvlineasm(int neglogy) { g.logy = neglogy; }
void setuptvlineasm(int neglogy) { g.logy = neglogy; }

// Floor sprite horizontal line functions
void msethlineshift(int logx, int logy) { g.logx = logx; g.logy = logy; }
void tsethlineshift(int logx, int logy) { g.logx = logx; g.logy = logy; }

// Rotatesprite vertical line functions
void setupspritevline(void *paloffs, int bxinc, int byinc, int ysiz) {
	g.pal = paloffs; g.bxinc = bxinc; g.byinc = byinc; g.logy = ysiz;
}

void msetupspritevline(void *paloffs, int bxinc, int byinc, int ysiz) {
	g.pal = paloffs; g.bxinc = bxinc; g.byinc = byinc; g.logy = ysiz;
}

void tsetupspritevline(void *paloffs, int bxinc, int byinc, int ysiz) {
	g.pal = paloffs; g.bxinc = bxinc; g.byinc = byinc; g.logy = ysiz;
}

void setupdrawslab (int dabpl, void *pal) {
	g.bpl = dabpl; g.pal = pal;
}

