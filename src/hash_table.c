

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "hash_table.h"


hash_table_t * hash_table_init(hash_table_t *t, int keysize, int valsize) {

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

