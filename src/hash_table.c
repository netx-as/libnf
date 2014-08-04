

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "hash_table.h"

/* allocane a new bucket in hash table */
void * hash_table_allocate_new_bucket(hash_table_t *t) {
		
	void *p;

	p = malloc(t->rowlen * HASH_TABLE_BUCKET_SIZE);

	/* cannot allocate memroty */
	if (p == NULL) {
		return NULL;
	}

	/* clear allocated memory */
	memset(p, 0x0, t->rowlen * HASH_TABLE_BUCKET_SIZE);

	t->bucket[t->numbuckets] = p;
	t->numbuckets++;

	return p;	
}

/* initialise hash table */ 
/* argumets - aggregation key size, sort key size, values size */
hash_table_t * hash_table_init(hash_table_t *t, int keylen, int vallen, hash_table_callback_t cb) {

	t->keylen = keylen;
	t->vallen = vallen;
	t->rowlen = sizeof(hash_table_row_flags_t) + vallen + keylen;
	t->callback = cb;
	
	t->numbuckets = 0;

	/* allocate first bucket */
	if (hash_table_allocate_new_bucket(t) == NULL) {
		return NULL;
	}

	return t;

}

/* compute hash function */
unsigned long hash_table_hash_index(hash_table_t *t, char * key, int len) {

	int i;	
	unsigned long h = 0;
	char *hp = (char *)&h;

	for (i = 0; i < len; i++) {
		hp[i % sizeof(h)] ^= key[i];
	}

	return h % (t->numbuckets * HASH_TABLE_BUCKET_SIZE);
}

/* get row ptw according index */
void * hash_table_row_ptr(hash_table_t *t, int index) {

	unsigned long bucket, row;

	bucket = index % t->numbuckets;
	row = index % HASH_TABLE_BUCKET_SIZE;	

	return t->bucket[bucket] + (row * t->rowlen); 
}

unsigned long hash_table_row_next(hash_table_t *t, int index) {
	
	index++;

	return index < (t->numbuckets * HASH_TABLE_BUCKET_SIZE) ? index : 0;
}

/* insert element into hash table */
void * hash_table_insert(hash_table_t *t, char *key, char *val, void *p) {

	unsigned long hash, index, bucket, row, i;
	void *prow;
	hash_table_row_flags_t *pflags;
	char *pkey;
	char *pval;
	int collisions = 0;

	index = hash_table_hash_index(t, key, t->keylen);	


lookup:
	/* determine bucket and row in bucket */
	prow =  hash_table_row_ptr(t, index);

	pflags = (hash_table_row_flags_t *)prow;
	pkey = prow + sizeof(hash_table_row_flags_t);
	pval = prow + sizeof(hash_table_row_flags_t) + t->keylen;

	/* critical section !! */
	if (!pflags->occupied) {

		pflags->locked = 1;
		memcpy(pkey, key, t->keylen);
		memcpy(pval, val, t->vallen);
		pflags->occupied = 1;
		pflags->locked = 0;

		return prow;

	}
	/* critical section !! */

	/* same key */
	if (memcmp(pkey, key,  t->keylen) == 0) {

		/* add values */
		t->callback(pkey, pval, val, p);
		return prow;

	/* collision */
	} else {
		index = hash_table_row_next(t, index);
		collisions++;
		if (collisions > HASH_TABLE_COLLISIONS) {
			printf("XXX hash table new bucket \n");
			if (hash_table_allocate_new_bucket(t) == NULL) {
				return NULL;
			}
			collisions = 0;
		}
		goto lookup;
	}

	return NULL;
}

/* return next non occupied field */
unsigned long hash_table_fetch(hash_table_t *t, unsigned long index, char **pkey, char **pval) {

	hash_table_row_flags_t *pflags;
	char *prow;

lookup:
	prow =  hash_table_row_ptr(t, index);
	
	index = hash_table_row_next(t, index);

	pflags = (hash_table_row_flags_t *)prow;

	/* critical section !! */
	if (!pflags->occupied) {
		goto lookup;
	}

	*pkey = (prow + sizeof(hash_table_row_flags_t));
	*pval = (prow + sizeof(hash_table_row_flags_t) + t->keylen);

	return index;
}

