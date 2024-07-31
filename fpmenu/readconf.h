
typedef struct {
	int argc, skip; char *end;
} extract_args_t;

static char* get_first_arg(char *s) {
	int a, n; char *ret;
	n = *(uint8_t*)(s + 1) << 8 | *(uint8_t*)s;
	//printf("argc = %d\n", n);
	s += 2;
	for (; n; n--) {
		if (*s != 1) return s;
		a = *(uint8_t*)&s[1];
		a |= *(uint8_t*)&s[2] << 8;
		a |= *(uint8_t*)&s[3] << 16;
		//printf("var: 0x%x, %p\n", a, s - a);
		ret = get_first_arg(s - a);
		if (ret) return ret;
		s += 4;
	}
	return NULL;
}

static char* extract_args(int depth, extract_args_t *x, char *s, char *d) { 
	int a, n;
	n = *(uint8_t*)(s + 1) << 8 | *(uint8_t*)s;
	//printf("argc = %d\n", n);
	s += 2;
	for (; n; n--)
		if (*s != 1) {
			int skip = x->skip;
			if (skip) {
				x->skip = skip - 1;
				while (*s++);
			} else {
				char *e = x->end;
				//printf("str: \"%s\"\n", s);
				do {
					a = *s++;
					if (d == e) return NULL;
					*d++ = a;
				} while (a);
				x->argc++;
			}
		} else {
			a = *(uint8_t*)&s[1];
			a |= *(uint8_t*)&s[2] << 8;
			a |= *(uint8_t*)&s[3] << 16;
			//printf("var: 0x%x, %p\n", a, s - a);
			if (depth >= 10) return NULL;
			d = extract_args(depth + 1, x, s - a, d);
			if (!d) return d;
			s += 4;
		}
	return d;
}

static char* find_var(const char *s, char *vars) {
	unsigned a, i;
	do {
		char *p = vars + 4;
		//printf("enum: %s\n", p);
		i = 0; do {
			if ((a = p[i]) != (unsigned)s[i]) goto skip;
			i++;
		} while (a);
		//printf("found: %p\n", p + i);
		return p + i;
skip:
		vars -= a = *(uint32_t*)vars;
	} while (a);
	return NULL;
}

static char* parse_var(char *d, char *e, char *vars, char *old) {
	if (e - d < 4) return NULL;
	//printf("var: %s\n", d);
	vars = find_var(d + 1, vars);
	if (vars) {
		uint32_t a = d - vars;
		*d = 1; d[1] = a;
		d[2] = a >> 8;
		d[3] = a >> 16;
		return d + 4;
	}
	return old;
}

static int read_args(FILE *fi, char **rd, char *e, char *vars) {
	int argc = 0, j, q = 0;
	char *d = *rd, *var = NULL;
	if (e - d < 2) return -1;
	d += 2;
loop:
	if (var && !(d = parse_var(var, e, vars, d))) return -1;
	j = 1; var = NULL;
	for (;;) {
		int a = fgetc(fi);
		if (a == '#') {
			do {
				a = fgetc(fi);
				if (a == EOF) goto end;
			} while (a != '\n');
			if (argc || vars) break;
			continue;
		}
		if (a <= 0x20) {
			if (a == '\r') continue;
			if (a != ' ' && a != '\t' && a != '\n') return -1;
			if (!q) {
				if (a == '\n' && (argc || vars)) break;
				if (j) continue;
				a = 0;
			}
		} else if (a == '"' || a == '\'') {
			if (!q) {	q = a; argc += j; j = 0; continue; }
			if (q == a) { q = 0; continue; }
		} else if (a == '\\' && q != '\'') {
			do a = fgetc(fi); while (a == '\r');
			if (a == '\n') continue;
			if (a == EOF || a < 0x20) return -1;
		} else if (a == '$' && j && vars) var = d;
		argc += j; j = 0;
		if (d == e) return -1;
		*d++ = a; if (!a) goto loop;
	}
end:
	if (q) return -1;
	if (!j) *d++ = 0;
	if (var && !(d = parse_var(var, e, vars, d))) return -1;
	e = *rd; *rd = d;
	e[0] = argc;
	e[1] = argc >> 8;
	if (argc >> 16) return -1;
	//print_args(e);
	return argc;
}

static char* parse_config(FILE *fi, unsigned size) {
	char *buf, *end, *p;
	char *item, *vars;

	buf = malloc(size);
	if (!buf) return NULL;
	end = buf + size;
	item = buf; p = buf + 8;

	*(uint32_t*)p = 0;
	vars = p; p += 4;
	*p++ = '@'; *p++ = 0;
	if (read_args(fi, &p, end, NULL) < 0) goto err;

	for (;;) {
		int a = fgetc(fi), k;
		if (a == EOF) break;
		if (a == '#') {
			do {
				a = fgetc(fi);
				if (a == EOF) goto end;
			} while (a != '\n');
			continue;
		}
		if (a == ' ' || a == '\t' || a == '\n' || a == '\r') continue;
		if (a == '$') { // variable
			p = (char*)(((intptr_t)p + 3) & ~3);
			if (end - p < 4) goto err;
			*(uint32_t*)p = p - vars;
			vars = p; p += 4; k = 0;
			for (;;) {
				if (p == end) goto err;
				a = fgetc(fi);
				if (a == '=') break;
				if (a == ' ' || a == '\t' || a == '\n' || a == '\r') {
					k = 1; continue;
				}
				if (a == EOF || a <= 0x20 || k) goto err;
				*p++ = a;
			}
			*p++ = 0;
			if (read_args(fi, &p, end, vars - *(uint32_t*)vars) < 0) goto err;
			continue;
		}
		if (a == '|') { // menu item
			p = (char*)(((intptr_t)p + 3) & ~3);
			if (end - p < 4) goto err;
			*(uint32_t*)item = p - item;
			item = p; p += 4;
			// name
			for (;;) {
				if (p == end) goto err;
				a = fgetc(fi);
				if (a == '|') break;
				if (a == EOF || a < 0x20) goto err;
				*p++ = a;
			}
			*p++ = 0;
			if (read_args(fi, &p, end, vars) < 0) goto err;
		}
	}
end:
	*(uint32_t*)item = 0; // end of chain
	*(uint32_t*)(buf + 4) = vars - buf;
	return buf;
err:
	fprintf(stderr, "parse_config: error at 0x%x\n", (int)ftell(fi));
	free(buf);
	return NULL;
}
