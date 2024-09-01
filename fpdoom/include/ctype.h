#ifndef __CTYPE
#define __CTYPE

static inline int isdigit(int c) {
	return (unsigned)c - '0' < 10;
}

static inline int isalpha(int c) {
  return (unsigned)(c | 32) - 'a' < 26;
}

static inline int isalnum(int c) {
	return isalpha(c) || isdigit(c);
}

static inline int isspace(int c) {
	return c == ' ' || (unsigned)c - 9 < 5;
}

static inline int isupper(int c) {
	return (unsigned)c - 'A' < 26;
}

static inline int islower(int c) {
	return (unsigned)c - 'a' < 26;
}

static inline int ispunct(int c) {
	return (unsigned)c - 0x21 < 0x5e && !isalnum(c);
}

static inline int isprint(int c) {
	return (unsigned)c - 0x20 < 0x5f;
}

static inline int toupper(int c) {
	return (unsigned)c - 'a' < 26 ? c - 32 : c;
}

static inline int tolower(int c) {
	return (unsigned)c - 'A' < 26 ? c + 32 : c;
}

#endif
