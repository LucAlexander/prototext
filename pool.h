#ifndef POOL_H
#define POOL_H

#include <inttypes.h>

typedef enum POOL_TAG {
	POOL_STATIC=0,
	POOL_DYNAMIC=1,
	NO_POOL
} POOL_TAG;

typedef struct pool {
	POOL_TAG tag;
	void* buffer;
	void* ptr;
	size_t left;
	struct pool* next;
	void* ptr_save;
	size_t left_save;
} pool;

pool pool_alloc(size_t cap, POOL_TAG t);
void pool_empty(pool* const p);
void pool_dealloc(pool* const p);
void* pool_request(pool* const p, size_t bytes);
void* pool_byte(pool* const p);
void pool_save(pool* const p);
void pool_load(pool* const p);

#endif
