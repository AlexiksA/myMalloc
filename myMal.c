#define _GNU_SOURCE
#include "myMal.h"
#include <sys/mman.h>
#include <stdio.h>

struct mem_page *start_page = NULL;

struct mem_page *request_page() {
	struct mem_page *new_page = mmap(NULL, //Kernel chooses
									 PAGE_SIZE, // 4096
									 PROT_READ | PROT_WRITE, // r+w
									 MAP_PRIVATE | MAP_ANONYMOUS, // private to me and not a file (plus 0x0-ed)
									 -1, 0);
	if(new_page == MAP_FAILED) {
		return NULL;
	}
	new_page->avail = PAGE_SIZE - (sizeof(size_t) 
							  + 2 * sizeof(struct mem_page *)
							  + sizeof(struct heap_header));
	new_page->next = NULL;
	new_page->prev = NULL;
	new_page->start = (struct heap_header *) (new_page + 1);
	new_page->start->block_size = new_page->avail;
	new_page->start->is_free = 1;
	new_page->start->next = NULL;
	new_page->start->prev = NULL;
	return new_page;
}

struct heap_header *searchFreeHeap(size_t size, struct mem_page *page) {
	struct heap_header *i;
	for(i = page->start; i->block_size < size || !i->is_free; i = i->next) {
		if(!i->next) {
			return NULL;
		}
	}
	return i;
}

void *jumpNBytes(void *addr_to_jump_from, size_t howMuch) {
	char *i = addr_to_jump_from;
	i += howMuch;
	return i;
}

struct mem_page *searchFreePage(size_t howMuchNeed, struct mem_page *start) {
	struct mem_page *i;
	for(i = start; i != NULL && i->avail < howMuchNeed; i = i->next) {
		if(!i->next && i->avail < howMuchNeed) {
			struct mem_page *new_page = request_page();
			i->next = new_page;
			i->next->prev = i;
			i = i->next;
			break;
		}
	}
	return i;
}

void *my_malloc(size_t size) {

	if(!start_page) {
		start_page = request_page();
		if(!start_page) {
			fprintf(stderr, "Mmap failed\n");
			return NULL;
		}
	}
	size_t howMuchNeed = size + sizeof(struct heap_header);

	struct mem_page *page = searchFreePage(howMuchNeed, start_page);

	struct heap_header *new_heap = searchFreeHeap(howMuchNeed, page);
	if(!new_heap) {
		return NULL;
	}
	struct heap_header *frag_heap = NULL;
	if(howMuchNeed < page->avail) {
		frag_heap = (struct heap_header *)jumpNBytes((struct heap_header *)new_heap, howMuchNeed);
		frag_heap->block_size = new_heap->block_size - howMuchNeed;
		frag_heap->next = new_heap->next;
		if(frag_heap->next) frag_heap->next->prev = frag_heap;
		frag_heap->prev = new_heap;
		frag_heap->is_free = 1;

		new_heap->next = frag_heap;
		new_heap->block_size = size;
		new_heap->is_free = 0;

	} else {
		new_heap->is_free = 0;
	}
	page->avail -= howMuchNeed;
	return new_heap + 1;
}

void heapConcat(struct heap_header *a, struct heap_header *b) {
	// a - first b - second
	a->block_size += b->block_size + sizeof(struct heap_header);
	a->next = b->next;
	if(a->next) {
		a->next->prev = a;
	}
}

void sanitizePage(struct heap_header *iterrator) {
	while(iterrator->next) {
		if(iterrator->is_free && iterrator->next->is_free) {
			heapConcat(iterrator, iterrator->next);
			continue;
		}
		iterrator = iterrator->next;
	}
}

void goToStart(struct heap_header **ptr) {
	while ((*ptr)->prev) (*ptr) = (*ptr)->prev;
}

void free(void *ptr) {
	struct heap_header *toFree = (struct heap_header *)ptr - 1;

	toFree->is_free = 1;
	size_t freeBlockSz = toFree->block_size
						 + sizeof(struct heap_header);

	goToStart(&toFree);

	struct mem_page *page = (struct mem_page *)toFree - 1;
	page->avail += freeBlockSz;

	sanitizePage(toFree);
}

void cleanup() {
	if(!start_page) return;

	struct mem_page *i = start_page;
	while(i->next) {
		struct mem_page *toMunmap = i;
		i = i->next;
		munmap(toMunmap, PAGE_SIZE);
	}

	if(i) {
		munmap(i, PAGE_SIZE);
	}

	start_page = NULL;
}

__attribute__((destructor))
void auto_cleanup() {
	cleanup();
}