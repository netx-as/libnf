

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "bit_array.h"


bit_array_t * bit_array_init(bit_array_t *a, int size) {

	a->size = size;
	//a->data = malloc(bytes * 1000);
	a->data = malloc(size * sizeof(BA_TYPE));

	if (a->data != NULL) {
		memset(a->data, 0x0, a->size * sizeof(BA_TYPE));
		return a;
	} else 
		return NULL;	
}

int bit_array_clear(bit_array_t *a) {

	if (a->data != NULL) {
		memset(a->data, 0x0, a->size * sizeof(BA_TYPE));
		return 1;
	} else 
		return -1;	
}

int bit_array_get(bit_array_t *a, int pos) {

	if (pos >= a->size) 
		return -1;

    return __bit_array_get(a, pos);
}

int bit_array_set(bit_array_t *a, int pos, int val) {

	if (pos > a->size) 
		return -1;

	__bit_array_set(a, pos, val);

	return 0;
}


int bit_array_cmp(bit_array_t *a, bit_array_t *b) {

	/* can can compare only arrays with same sizes */
	if (a->size != b->size) 
		return -1;

	return memcmp(a->data, b->data, a->size * sizeof(BA_TYPE));
}

int bit_array_copy(bit_array_t *d, bit_array_t *s) {
	
	/* we can copy only arrays with same sizes */
	if (s->size != d->size) 
		return -1;

	return (memcpy(d->data, s->data, d->size * sizeof(BA_TYPE)) != NULL);

}


void bit_array_release(bit_array_t *a) {

	a->size = 0;
	free(a->data); 

}

