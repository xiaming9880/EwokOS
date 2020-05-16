#ifndef TRKMALLOC_H
#define TRKMALLOC_H

#include <stdint.h>

typedef struct mem_block {
	struct mem_block* next;
	struct mem_block* prev;
	
	uint32_t size: 31;
	uint32_t used: 1;
	char* mem;
} mem_block_t;

typedef struct {
	void* arg;

	int32_t (*expand)(void* arg, int32_t pages);
	void (*shrink)(void* arg, int32_t pages);
	void* (*get_mem_tail)(void*);

	mem_block_t* head;
	mem_block_t* tail;
} malloc_t;

char* trunk_malloc(malloc_t* m, uint32_t size);
char* trunk_realloc(malloc_t* m, char* p, uint32_t size);
void trunk_free(malloc_t* m, char* p);

#endif
