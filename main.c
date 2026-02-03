#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdio.h>

#define PAGE_SIZE 4096

struct __attribute__((packed)) heap_header{
	size_t block_size;
	unsigned char is_free;
	struct mem_page *papa;
	// ^^^ inefficient but I want to workaround some bs when freeing memory
	struct heap_header *next;
	struct heap_header *prev;
};

struct __attribute__((packed)) mem_page {
	size_t avail;
	struct mem_page *next;
	struct mem_page *prev;
	struct heap_header *start;
};

struct mem_page *request_page() {
	struct mem_page *new_page = mmap(NULL, //Alege kernel-ul locatia unde mi da pagina
									 PAGE_SIZE, // 4096
									 PROT_READ | PROT_WRITE, // r+w
									 MAP_PRIVATE | MAP_ANONYMOUS, // private to me and not a file (plus 0x0-ed)
									 -1, 0);
	new_page->avail = 4096 - (sizeof(size_t) 
							  + 2 * sizeof(struct mem_page *)
							  + sizeof(struct heap_header));
	new_page->next = NULL;
	new_page->prev = NULL;
	new_page->start = (struct heap_header *) (new_page + 1);
	new_page->start->block_size = new_page->avail;
	new_page->start->is_free = 1;
	new_page->start->papa = new_page;
	new_page->start->next = NULL;
	new_page->start->prev = NULL;
	return new_page;
}

struct heap_header *searchFreeHeap(size_t size, struct mem_page *page) {
	struct heap_header *i;
	for(i = page->start; i->block_size < size && i->is_free; i = i->next);
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
			i = i->next;
			break;
		}
	}
	return i;
}

void *my_malloc(size_t size) {
	static struct mem_page *start_page = NULL;
	if(!start_page) {
		start_page = request_page();
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

		new_heap->next = frag_heap;
		new_heap->block_size = size;
		new_heap->is_free = 0;

	} else {
		new_heap->next = NULL;
	}
	page->avail -= howMuchNeed;
	return new_heap + 1;
}

void scratchList(struct heap_header *a, struct heap_header *b) {
	// a - first b - second
	a->block_size += b->block_size + sizeof(struct heap_header);
	a->next = b->next;
	a->next->prev = a;
}

void sanitizePage(struct mem_page *page) {
	struct heap_header *i = page->start;
	while(i->next) {
		if(i->is_free && i->next->is_free) {
			scratchList(i, i->next);
			continue;
		}
		i = i->next;
	}
}


//TODO: functie care concateneaza toate block-urile libere
//TODO; free :)
int main(void) {
	int *v = (int *) my_malloc(sizeof(int) * 1100);
	if(!v) {
		fprintf(stdout, "Not enough mem\n");
		return -1;
	}
	for(int i = 0; i < 500; i++) {
		v[i] = i;
		printf("%d", v[i]);
	}

	v = (int *) my_malloc(sizeof(int) * 500);
	if(!v) {
		fprintf(stdout, "Not enough mem\n");
		return -1;
	}
	for(int i = 0; i < 500; i++) {
		v[i] = i;
		printf("%d", v[i]);
	}

	return 0;
}