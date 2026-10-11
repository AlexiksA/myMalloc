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
	new_page->avail = MAX_ALLOC;
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
	for(i = page->start; i != NULL; i = i->next) {
		if(i->is_free && i->block_size >= size) {
			return i;
		}
	}
	return NULL;
}

void *jumpNBytes(void *addr_to_jump_from, size_t howMuch) {
	char *i = addr_to_jump_from;
	i += howMuch;
	return i;
}

struct mem_page *searchFreePage(size_t howMuchNeed, struct mem_page *start) {
	struct mem_page *i;
	for(i = start; i != NULL; i = i->next) {
		// avail is the total of the free blocks, so also check one of them is big enough
		if(i->avail >= howMuchNeed && searchFreeHeap(howMuchNeed, i)) {
			return i;
		}
		if(!i->next) {
			struct mem_page *new_page = request_page();
			if(!new_page) {
				return NULL;
			}
			i->next = new_page;
			new_page->prev = i;
			return new_page;
		}
	}
	return NULL;
}

void *my_malloc(size_t size) {

	if(size > MAX_ALLOC) {
		return NULL;
	}
	size = (size + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1);

	if(!start_page) {
		start_page = request_page();
		if(!start_page) {
			fprintf(stderr, "Mmap failed\n");
			return NULL;
		}
	}
	size_t howMuchNeed = size + sizeof(struct heap_header);

	struct mem_page *page = searchFreePage(size, start_page);
	if(!page) {
		fprintf(stderr, "Mmap failed\n");
		return NULL;
	}

	struct heap_header *new_heap = searchFreeHeap(size, page);
	if(!new_heap) {
		return NULL;
	}
	struct heap_header *frag_heap = NULL;
	// split only if what is left can hold a header and some data
	if(new_heap->block_size >= howMuchNeed + ALIGNMENT) {
		frag_heap = (struct heap_header *)jumpNBytes((struct heap_header *)new_heap, howMuchNeed);
		frag_heap->block_size = new_heap->block_size - howMuchNeed;
		frag_heap->next = new_heap->next;
		if(frag_heap->next) frag_heap->next->prev = frag_heap;
		frag_heap->prev = new_heap;
		frag_heap->is_free = 1;

		new_heap->next = frag_heap;
		new_heap->block_size = size;
		page->avail -= sizeof(struct heap_header);
	}
	new_heap->is_free = 0;
	page->avail -= new_heap->block_size;
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

void my_free(void *ptr) {
	if(!ptr) return;

	struct heap_header *toFree = (struct heap_header *)ptr - 1;
	if(toFree->is_free) return; // double free

	toFree->is_free = 1;

	goToStart(&toFree);

	struct mem_page *page = (struct mem_page *)toFree - 1;

	sanitizePage(toFree);

	// merged blocks give their headers back, so recount
	page->avail = 0;
	for(struct heap_header *i = page->start; i != NULL; i = i->next) {
		if(i->is_free) page->avail += i->block_size;
	}
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
