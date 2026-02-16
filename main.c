#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdio.h>
#include "myMal.h"

//TODO: functie care concateneaza toate block-urile libere
//TODO; free :)
int main(void) {
	int *v = (int *) my_malloc(sizeof(int) * 500);
	if(!v) {
		fprintf(stdout, "Not enough mem\n");
		return -1;
	}
	for(int i = 0; i < 500; i++) {
		v[i] = i;
		printf("%d", v[i]);
	}
	free(v);
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