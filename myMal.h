#ifndef __MYMALH__
#define __MYMALH__

#include <stddef.h>

#define PAGE_SIZE 4096
#define ALIGNMENT 16
// biggest request that still fits in one page
#define MAX_ALLOC (PAGE_SIZE - sizeof(struct mem_page) - sizeof(struct heap_header))

struct heap_header{
	size_t block_size;
	unsigned char is_free;
	struct heap_header *next;
	struct heap_header *prev;
};

struct mem_page {
	size_t avail;
	struct mem_page *next;
	struct mem_page *prev;
	struct heap_header *start;
};

// headers must keep the user pointer aligned
_Static_assert(sizeof(struct heap_header) % ALIGNMENT == 0, "heap_header breaks alignment");
_Static_assert(sizeof(struct mem_page) % ALIGNMENT == 0, "mem_page breaks alignment");

extern struct mem_page *start_page;

struct mem_page *request_page();
struct heap_header *searchFreeHeap(size_t size, struct mem_page *page);
void *jumpNBytes(void *addr_to_jump_from, size_t howMuch);
struct mem_page *searchFreePage(size_t howMuchNeed, struct mem_page *start);
void *my_malloc(size_t size);
void heapConcat(struct heap_header *a, struct heap_header *b);
void sanitizePage(struct heap_header *iterrator);
void goToStart(struct heap_header **ptr);
void my_free(void *ptr);
void cleanup();

__attribute__((destructor))
void auto_cleanup();

#endif
