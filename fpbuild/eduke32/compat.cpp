#include "compat.h"
#include "palette.h"
#include "baselayer.h"

uint8_t basepaltable[BASEPALCOUNT][768];

void paletteSetColorTable(int32_t id, const uint8_t *table) {
	//Bmemcpy(basepaltable[id], table, 768);
	uint8_t *dpal = basepaltable[id];
	for (int i = 0; i < 768; i++)
		dpal[i] = table[i] >> 2;
}

const char g_keyAsciiTable[128] = {
	  0,   0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',   0,   0,
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',   0,   0, 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  39, '`',   0,  92, 'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', '/',   0, '*',   0,  32,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
	'2', '3', '0', '.'
};

const char g_keyAsciiTableShift[128] = {
	   0,   0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',   0,   0,
	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',   0,   0, 'A', 'S',
	 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0, '|', 'Z', 'X', 'C', 'V',
	 'B', 'N', 'M', '<', '>', '?',   0, '*',   0,  32,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
	 '2', '3', '0', '.'
};

int32_t G_CheckCmdSwitch(int32_t argc, char const * const * argv, const char *str) {
	int32_t i;
	for (i = 0; i < argc; i++) {
		if (str && !Bstrcasecmp(argv[i], str))
			return 1;
	}
	return 0;
}

void fnlist_clearnames(fnlist_t *fnl) {
	klistfree(fnl->finddirs);
	klistfree(fnl->findfiles);

	fnl->finddirs = fnl->findfiles = NULL;
	fnl->numfiles = fnl->numdirs = 0;
}

// dirflags, fileflags:
//  -1 means "don't get dirs/files",
//  otherwise ORed to flags for respective klistpath
int32_t fnlist_getnames(fnlist_t *fnl, const char *dirname, const char *pattern,
                        int32_t dirflags, int32_t fileflags) {
	BUILDVFS_FIND_REC *r;
	fnlist_clearnames(fnl);

	if (dirflags != -1)
		fnl->finddirs = klistpath(dirname, "*", BUILDVFS_FIND_DIR | dirflags);
	if (fileflags != -1)
		fnl->findfiles = klistpath(dirname, pattern, BUILDVFS_FIND_FILE | fileflags);

	for (r = fnl->finddirs; r; r = r->next) fnl->numdirs++;
	for (r = fnl->findfiles; r; r = r->next) fnl->numfiles++;
	return 0;
} 

int win_priorityclass;
int paletteloaded;
int Numsprites = 0;
char *g_defNamePtr = NULL;
int CONTROL_BindsEnabled;

uint32_t wrandomseed = 1;

// JFBuild
int nextvoxid = 0;
