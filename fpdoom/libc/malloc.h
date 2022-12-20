struct _mchunk { struct _mchunk *prev; intptr_t size; };
struct _mfree { struct _mfree *next, *prev; };
struct _mchunk *_heap;
struct _mfree *_mfree0;

#define MCHUNK_NEXT(c) (c->size < 0 ? c->size == -(int)sizeof(void*) ? NULL : \
	(struct _mchunk*)((char*)c - c->size) : \
	(struct _mchunk*)((char*)c + c->size) )
#define MFREE_CHUNK(f) (((struct _mchunk*)f)-1)
#define MFREE_SIZE(f) MFREE_CHUNK(f)->size

#define REMOVENODE(_prev, _free) do { \
	struct _mfree *_next = _free->next; \
	_prev->next = _next; \
	if (_next) _next->prev = _prev; \
} while (0)

#define MALLOC_MIN (int)(sizeof(struct _mchunk) + sizeof(struct _mfree))

#ifndef MEMCHECK
#define MEMCHECK 1
#endif

void _malloc_init(void *addr, size_t size) {
	struct _mchunk *chunk, *end;
	struct _mfree *free;
	chunk = (struct _mchunk*)addr;
	_heap = chunk;

	// dummy chunk
	chunk->prev = NULL;
	chunk->size = -(int)sizeof(struct _mchunk);
	// empty space
	chunk[1].prev = chunk;
	chunk[1].size = size - sizeof(struct _mchunk) * 2;
	// last chunk
	end = (struct _mchunk*)((char*)addr + size);
	end[-1].prev = chunk + 1;
	end[-1].size = -(int)sizeof(void*);

	free = (struct _mfree*)(chunk + 2);
	_mfree0 = free;
	free->next = NULL;
	free->prev = (struct _mfree*)&_mfree0;
}

void* PREFIX(malloc)(size_t size) {
	struct _mfree *free, *prev, *two, *next;
	intptr_t free_size, rem_size;

	// fprintf(stderr, "!!! malloc(%d)\n", size);

	if ((size - 1) >= 0x40000000) return NULL;
	if (size < sizeof(struct _mfree))
		size = sizeof(struct _mfree);
	size += sizeof(struct _mchunk) + sizeof(void*) - 1;
	size &= -sizeof(void*);

	free = (struct _mfree*)&_mfree0;
	do {
		free = free->next;
		if (!free) {
			fprintf(stderr, "!!! malloc(%d) failed\n", size);
			exit(-1);
			return NULL;
		}
		free_size = MFREE_SIZE(free);
	} while (free_size < (intptr_t)size);

	prev = free->prev;
	rem_size = free_size - size;
	if (rem_size < MALLOC_MIN) {
		MFREE_SIZE(free) = -free_size;
		REMOVENODE(prev, free);
		return free;
	}
	MFREE_SIZE(free) = -size;
	two = (struct _mfree*)((char*)free + size);
	MFREE_SIZE(two) = rem_size;
	next = (struct _mfree*)((char*)two + rem_size);
	MFREE_CHUNK(next)->prev = MFREE_CHUNK(two);
	MFREE_CHUNK(two)->prev = MFREE_CHUNK(free);
	next = free->next;
	prev->next = two;
	two->next = next;
	two->prev = prev;
	if (next) next->prev = two;
//printf("!!! %p\n", free); fflush(stdout);
	return free;
}

#if MEMCHECK
__attribute__((noreturn))
static void malloc_err(int dbl) {
	if (!dbl) {
		fprintf(stderr, "!!! bad free\n");
		exit(254);
	} else {
		fprintf(stderr, "!!! double free\n");
		exit(253);
	}
}
#endif

void PREFIX(free)(void *ptr) {
	struct _mfree *next, *temp, *free;
	struct _mchunk *prev;
	intptr_t size, prev_size, next_size;
	if (!ptr) return;
#if MEMCHECK
	if ((uintptr_t)ptr & (sizeof(void*) - 1)) goto bad;
#endif
	free = (struct _mfree*)ptr;
	size = MFREE_SIZE(free);
#if MEMCHECK
	if (size > -MALLOC_MIN) goto dbl;
	if (size & (sizeof(void*) - 1)) goto bad;
#endif
	// size is negative
	next = (struct _mfree*)((char*)free - size);
	prev = MFREE_CHUNK(free)->prev;
#if MEMCHECK
	if ((uintptr_t)prev >= (uintptr_t)MFREE_CHUNK(free)) goto bad;
	if (MFREE_CHUNK(next)->prev != MFREE_CHUNK(free)) goto bad;
#endif
	prev_size = prev->size;
	next_size = MFREE_SIZE(next);
	if (prev_size >= 0) {
		prev_size -= size;
		if (next_size >= 0) {
			prev_size += next_size;
			temp = next->prev;
			REMOVENODE(temp, next);
			next = (struct _mfree*)((char*)next + next_size);
		}
		prev->size = prev_size;
		MFREE_CHUNK(next)->prev = prev;
	} else if (next_size >= 0) {
		MFREE_SIZE(free) = next_size - size;
		temp = (struct _mfree*)((char*)next + next_size);
		MFREE_CHUNK(temp)->prev = MFREE_CHUNK(free);
		REMOVENODE(free, next);
		temp = next->prev;
		temp->next = free;
		free->prev = temp;
	} else {
		MFREE_SIZE(free) = -size;
		temp = _mfree0;
		if (temp) temp->prev = free;
		free->next = temp;
		_mfree0 = free;
		free->prev = (struct _mfree*)&_mfree0;
	}
	return;

#if MEMCHECK
bad: malloc_err(0);
dbl: malloc_err(1);
#endif
}

void* PREFIX(calloc)(size_t size, size_t count) {
	void *mem;
#if __GNUC__ >= 5
	size_t size2;
	if (__builtin_mul_overflow(size, count, &size2)) return NULL;
#else
	uint64_t size2;
	size2 = (uint64_t)size * count;
	if (size2 >> 32) return NULL;
#endif
	size = size2;
	mem = PREFIX(malloc)(size);
	if (mem) memset(mem, 0, size);
	return mem;
}

void* PREFIX(realloc)(void *ptr, size_t size) {
	struct _mfree *tofree, *next;
	void *new_ptr;
	intptr_t chunk_size;
	if (!ptr) return PREFIX(malloc)(size);
	if (!size) {
		free(ptr);
		return NULL;
	}

	if (size > 0x40000000) return NULL;
	if (size < sizeof(struct _mfree))
		size = sizeof(struct _mfree);
	size += sizeof(struct _mchunk) + sizeof(void*) - 1;
	size &= -sizeof(void*);

#if MEMCHECK
	if ((uintptr_t)ptr & (sizeof(void*) - 1)) goto bad;
#endif
	chunk_size = MFREE_SIZE(ptr);
#if MEMCHECK
	if (chunk_size > -MALLOC_MIN) goto dbl;
#endif
	chunk_size = -chunk_size;
	if ((size_t)chunk_size >= size) {
		intptr_t diff = chunk_size - size;
		if (diff < MALLOC_MIN) return ptr;
		MFREE_SIZE(ptr) = -size;
		tofree = (struct _mfree*)((char*)ptr + size);
		next = (struct _mfree*)((char*)ptr + chunk_size);
		MFREE_CHUNK(tofree)->prev = MFREE_CHUNK(ptr);
		MFREE_CHUNK(next)->prev = MFREE_CHUNK(tofree);
		MFREE_SIZE(tofree) = -diff;
		PREFIX(free)(tofree);
		return ptr;
	}
	new_ptr = PREFIX(malloc)(size - sizeof(struct _mchunk));
	if (!new_ptr) return NULL;
	memcpy(new_ptr, ptr, chunk_size - sizeof(struct _mchunk));
	PREFIX(free)(ptr);
	return new_ptr;

#if MEMCHECK
bad: malloc_err(0);
dbl: malloc_err(1);
#endif
}

