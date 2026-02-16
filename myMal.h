#ifndef __MYMALH__
#define __MYMALH__

#define PAGE_SIZE 4096
#define NULL ((void *)0)
typedef unsigned long size_t;

struct __attribute__((packed)) heap_header{
	size_t block_size;
	unsigned char is_free;
	struct heap_header *next;
	struct heap_header *prev;
};

struct __attribute__((packed)) mem_page {
	size_t avail;
	struct mem_page *next;
	struct mem_page *prev;
	struct heap_header *start;
};

extern struct mem_page *start_page;

struct mem_page *request_page();
struct heap_header *searchFreeHeap(size_t size, struct mem_page *page);
void *jumpNBytes(void *addr_to_jump_from, size_t howMuch);
struct mem_page *searchFreePage(size_t howMuchNeed, struct mem_page *start);
void *my_malloc(size_t size);
void heapConcat(struct heap_header *a, struct heap_header *b);
void sanitizePage(struct heap_header *iterrator);
void goToStart(struct heap_header **ptr);
void free(void *ptr);
void cleanup();

__attribute__((destructor))
void auto_cleanup();

#endif