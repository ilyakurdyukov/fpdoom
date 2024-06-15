typedef void (*ctor_t)(int, char**, char**);
extern ctor_t __init_array_start[1];
extern ctor_t __init_array_end[1];

static inline void cxx_init(int argc, char **argv) {
	ctor_t *p = __init_array_start;
	for (; p != __init_array_end; p++)
		(*p)(argc, argv, NULL);
}
