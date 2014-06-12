

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "bit_array.h"


bit_array_t * bit_array_init(bit_array_t *a, int size) {

	int bytes = size / BA_DATA_LEN + 1;

	a->size = size;
	//a->data = malloc(bytes * 1000);
	a->data = malloc(bytes);

	if (a->data != NULL) {
		memset(a->data, 0x0, bytes);
		return a;
	} else 
		return NULL;	
}

int bit_array_clear(bit_array_t *a) {

	int bytes = a->size / BA_DATA_LEN + 1;

	if (a->data != NULL) {
		memset(a->data, 0x0, bytes);
		return 1;
	} else 
		return -1;	
}

int bit_array_get(bit_array_t *a, int pos) {
	if (pos >= a->size) 
		return -1;
    return a->data[pos / BA_DATA_LEN] & (1 << (pos % BA_DATA_LEN));
}

int bit_array_set(bit_array_t *a, int pos, int val) {
	if (pos > a->size) 
		return -1;

	if (val) 
		a->data[ pos / BA_DATA_LEN ] |= 1 << (pos % BA_DATA_LEN );
	 else 
		a->data[ pos / BA_DATA_LEN ] &= ~ (1 << (pos % BA_DATA_LEN ));

	return 0;
}


int bit_array_cmp(bit_array_t *a, bit_array_t *b) {

	int bytes = a->size / BA_DATA_LEN + 1;

	/* can can compare only arrays with same sizes */
	if (a->size != b->size) 
		return -1;

	return memcmp(a->data, b->data, bytes);
}

int bit_array_copy(bit_array_t *d, bit_array_t *s) {
	
	int bytes = s->size / BA_DATA_LEN + 1;

	/* we can copy only arrays with same sizes */
	if (s->size != d->size) 
		return -1;

	return (memcpy(d->data, s->data, bytes) != NULL);

}


void bit_array_release(bit_array_t *a) {

	a->size = 0;
	free(a->data); 

}

