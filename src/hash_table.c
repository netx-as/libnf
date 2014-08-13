

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "hash_table.h"
#include "xxhash.h"

/* allocane a new bucket in hash table */
void * hash_table_allocate_new_bucket(hash_table_t *t, int buckets) {
		
	void *p;
	int i;

	for (i = 0; i < buckets; i++) {

		//p = malloc(t->rowlen * HASH_TABLE_BUCKET_SIZE * buckets);
		p = malloc(t->rowlen * HASH_TABLE_BUCKET_SIZE);

		/* cannot allocate memroty */
		if (p == NULL) {
			return NULL;
		}

		/* clear allocated memory */
		//memset(p, 0x0, t->rowlen * HASH_TABLE_BUCKET_SIZE * buckets);
		memset(p, 0x0, t->rowlen * HASH_TABLE_BUCKET_SIZE);

		//t->bucket[t->numbuckets] = p + (i * t->rowlen * HASH_TABLE_BUCKET_SIZE);
		t->bucket[t->numbuckets] = p;
		t->numbuckets++;
	}

	return p;	
}

/* initialise hash table */ 
/* argumets - aggregation key size, sort key size, values size */
hash_table_t * hash_table_init(hash_table_t *t, int keylen, int vallen, 
			hash_table_aggr_callback_t acb, hash_table_sort_callback_t scb, void *callback_data) {

	t->keylen = keylen;
	t->vallen = vallen;
	t->rowlen = sizeof(hash_table_row_flags_t) + vallen + keylen;
	t->aggr_callback = acb;
	t->sort_callback = scb;
	t->callback_data = callback_data;
	
	t->numbuckets = 0;
	t->rows_used = 0;
	t->rows_inserted = 0;

	/* allocate first bucket */
	if (hash_table_allocate_new_bucket(t, 1) == NULL) {
		return NULL;
	}

	return t;

}

/* compute hash function */
unsigned long hash_table_hash(char * key, int len) {

	int i;	
	unsigned long h = 0;
	char *hp = (char *)&h;

	for (i = 0; i < len; i++) {
		printf("HASH: %d %d %x \n", i % sizeof(h), i, key[i] & 0xFF);
		//hp[i % sizeof(h)] += key[i] & 0xFF;
		h += key[i] & 0xFF;
	}

	return h;
}

/* return index to hash table */
unsigned long hash_table_index(hash_table_t *t, unsigned long hash) {

	return hash % (t->numbuckets * HASH_TABLE_BUCKET_SIZE);

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
void * hash_table_insert(hash_table_t *t, char *key, char *val, int allow_newbckt, int *first_entry) {

	unsigned long hash, index, bucket, row, i;
	void *prow;
	hash_table_row_flags_t *pflags;
	char *pkey;
	char *pval;
	int collisions = 0;

	//hash = hash_table_hash(key, t->keylen);	
	hash = XXH64(key, t->keylen, 0);	

	index = hash_table_index(t, hash);	
	t->rows_inserted++;

	
lookup:

	/* determine bucket and row in bucket */
	prow =  hash_table_row_ptr(t, index);

	pflags = (hash_table_row_flags_t *)prow;
	pkey = prow + sizeof(hash_table_row_flags_t);
	pval = prow + sizeof(hash_table_row_flags_t) + t->keylen;

	/* critical section !! - replace with compare and swap */
	if (!pflags->locked && !pflags->occupied) {

		pflags->locked = 1;

		memcpy(pkey, key, t->keylen);
		memcpy(pval, val, t->vallen);
		pflags->occupied = 1;
		pflags->hash = hash;
		pflags->numbuckets = t->numbuckets;

		t->rows_used++;
		pflags->locked = 0;

		*first_entry = 1;
		return prow;

	}
	/* critical section !! */

	/* same key */
	if (!pflags->locked && memcmp(pkey, key,  t->keylen) == 0) {

		/* add values */
		pflags->locked = 1;

		t->aggr_callback(pkey, pval, val, t->callback_data);
		pflags->numbuckets = t->numbuckets;
		pflags->locked = 0;

		*first_entry = 0;
		return prow;

	/* collision */
	} else {
		index = hash_table_row_next(t, index);
		t->collisions++;
		if ( allow_newbckt && (100 * t->rows_used) / (t->numbuckets * HASH_TABLE_BUCKET_SIZE) > 30) {
//		if (collisions > HASH_TABLE_COLLISIONS * 100) {
			printf("XXX hash table new bucket total: %d, occupied: %d, buckets: %d ratio: %d collisions: %d rows: %d\n", 
					t->numbuckets * HASH_TABLE_BUCKET_SIZE, t->rows_used, t->numbuckets, 
					(100 * t->rows_used) / (t->numbuckets * HASH_TABLE_BUCKET_SIZE), 
					t->collisions, t->rows_inserted );
			if (hash_table_allocate_new_bucket(t, t->numbuckets * 2) == NULL) {
				return NULL;
			}
			t->collisions = 0;
			index = hash_table_index(t, hash);	
		}
		goto lookup;
	}

	return NULL;
}

int hash_table_sort_callback(char *prow1, char *prow2, void *p) {

	hash_table_t *t = p;
	char *pkey1, *pval1;
	char *pkey2, *pval2;

	pkey1 = (prow1 + sizeof(hash_table_row_flags_t));
	pval1 = (prow1 + sizeof(hash_table_row_flags_t) + t->keylen);
	
	pkey2 = (prow2 + sizeof(hash_table_row_flags_t));
	pval2 = (prow2 + sizeof(hash_table_row_flags_t) + t->keylen);

	return t->sort_callback(pkey1, pval1, pkey2, pval2, t->callback_data);

}

/* insert element into hash table */
void hash_table_arrange(hash_table_t *t, void *p) {

	unsigned long index;
	void *prow, *prow_new;
	hash_table_row_flags_t *pflags;
	char *pkey;
	char *pval;
	int firstentry;

	printf("XXX arrange \n");

	/* prepare sort array */
	t->sort_items = 0;
	t->sort_data = malloc(t->rows_used * sizeof(char *));

	for (index = 0 ; index < t->numbuckets * HASH_TABLE_BUCKET_SIZE; index++) {

		prow =  hash_table_row_ptr(t, index);
		pflags = (hash_table_row_flags_t *)prow;

		if (pflags->occupied) {
			/* item shoul be placed to the differend position */
			if ( t->numbuckets != pflags->numbuckets ) {
				pkey = prow + sizeof(hash_table_row_flags_t);
				pval = prow + sizeof(hash_table_row_flags_t) + t->keylen;
				
				prow_new = hash_table_insert(t, pkey, pval, 0, &firstentry); /* do not allow realloc */
				// if inserted to the a new position then remove from current */
				if (prow != prow_new) {
					pflags->locked = 1;
				} 
				/* we created first entry - add to list */
				if (firstentry) {
					t->sort_data[t->sort_items++] = prow;
				}
			} else {
				/* item is on the right position */
				t->sort_data[t->sort_items++] = prow;
			}
		}
	}

	printf("HEAP sort\n");
	heap_sort(t->sort_data, t->sort_items, &hash_table_sort_callback, t);
	
}

/* return next field */
unsigned long hash_table_fetch(hash_table_t *t, unsigned long index, char **pkey, char **pval) {

	hash_table_row_flags_t *pflags;
	char *prow;

	if (index >= t->sort_items) {
		return 0;
	}

	prow = t->sort_data[index++];

	*pkey = (prow + sizeof(hash_table_row_flags_t));
	*pval = (prow + sizeof(hash_table_row_flags_t) + t->keylen);

	return index;
}

void hash_table_free(hash_table_t *t) {

	int i;

	for (i = 0; i < t->numbuckets; i++) {
		free(t->bucket[i]);
	}
	free(t->sort_data);
}
