

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "hash_table.h"
#include "heap_sort.h"
#include "xxhash.h"

/* initialise hash table */ 
/* argumets - aggregation key size, sort key size, values size */
hash_table_t * hash_table_init(hash_table_t *t, int numbuckets,
			hash_table_aggr_callback_t acb, 
			hash_table_sort_callback_t scb, 
			void *callback_data) {

	t->buckets = calloc(sizeof(void *), numbuckets);
	if (t->buckets == NULL) {
		return NULL;
	}

	t->numbuckets = numbuckets;
	t->aggr_callback = acb;
	t->sort_callback = scb;
	t->callback_data = callback_data;
	t->sort_array = NULL;
	t->entrypoint = NULL;	/* entry point */
	t->numentries = 0;
	
//	t->rows_used = 0;

	return t;

}

void hash_table_entry_len(hash_table_t *t, int keylen, int vallen) {

	t->keylen = keylen;	
	t->vallen = vallen;	

}

/* lookup key in hash table */
char * hash_table_lookup(hash_table_t *t, char *key, char **val, unsigned long *hash) {

	unsigned long index;
	char *prow;
	hash_table_row_hdr_t *phdr;
	char *pkey;
	char *pval;

	*hash = XXH64(key, t->keylen, 0);	

	index = *hash % t->numbuckets;

	prow = t->buckets[index];

	while (prow != NULL) {
		/* collison */
		phdr = (hash_table_row_hdr_t *)prow;
		pkey = prow + sizeof(hash_table_row_hdr_t);
		pval = prow + sizeof(hash_table_row_hdr_t) + t->keylen;

		if (memcmp(pkey, key, t->keylen) == 0) {
			/* found key  */
			*val = pval;
			return prow;
		} else {
			/* keys do not match - try next item in list */
			prow = phdr->hnext;
		}
	}

	return NULL;

}

/* insert element into hash table */
char * hash_table_insert(hash_table_t *t, char *key, char *val) {

	unsigned long hash, index;
	char *prow, *tmp;
	hash_table_row_hdr_t *phdr;
	char *pkey;
	char *pval;

	/* element is already in hash table */
	if ((prow = hash_table_lookup(t, key, &pval, &hash)) != NULL) {
		t->aggr_callback(key, pval, val, t->callback_data);
		return prow;
	}

	/* new entry */
	prow = malloc(sizeof(hash_table_row_hdr_t) + t->keylen + t->vallen);
	t->numentries++;	

	if (prow == NULL) {
		return NULL;
	}

	phdr = (hash_table_row_hdr_t *)prow;
	pkey = prow + sizeof(hash_table_row_hdr_t);
	pval = prow + sizeof(hash_table_row_hdr_t) + t->keylen;

	memcpy(pkey, key, t->keylen);
	memcpy(pval, val, t->vallen);

	index = hash % t->numbuckets;

	phdr->hash = hash;
	phdr->hnext = t->buckets[index];
	t->buckets[index] = prow;

	/* add entry at the begginging of linked list */
	tmp = t->entrypoint;
	t->entrypoint = prow;
	phdr->snext = tmp;

	return prow;
}

int hash_table_sort_callback(char *prow1, char *prow2, void *p) {

	hash_table_t *t = p;
	char *pkey1, *pval1;
	char *pkey2, *pval2;

	pkey1 = (prow1 + sizeof(hash_table_row_hdr_t));
	pval1 = (prow1 + sizeof(hash_table_row_hdr_t) + t->keylen);
	
	pkey2 = (prow2 + sizeof(hash_table_row_hdr_t));
	pval2 = (prow2 + sizeof(hash_table_row_hdr_t) + t->keylen);

	return t->sort_callback(pkey1, pval1, pkey2, pval2, t->callback_data);

}

/* convert hash table into simple array */
int hash_table_sort(hash_table_t *t) {

	unsigned long index;
	char *prow_tmp;
	hash_table_row_hdr_t *phdr;
	unsigned long index_array = 0;

	t->sort_array = malloc(t->numentries * sizeof(void *));
			
	if (t->sort_array == NULL) {
		return 0;
	}

	/* should be redesigned to some linked list sort algorithm */
	prow_tmp = t->entrypoint;

	while (prow_tmp != NULL) {
		t->sort_array[index_array++] = prow_tmp;
		phdr = (hash_table_row_hdr_t *)prow_tmp;
		prow_tmp = phdr->snext;
	}

	heap_sort(t->sort_array, t->numentries, &hash_table_sort_callback, t);

	/* after sorting make linked list of elements */
	t->entrypoint = t->sort_array[0];

	for (index = 1; index < t->numentries; index++) {
		
		phdr = (hash_table_row_hdr_t *)t->sort_array[index-1];
		phdr->snext = t->sort_array[index];
	}

	if (index > 1) {
		phdr = (hash_table_row_hdr_t *)t->sort_array[index - 1];
		phdr->snext = NULL;
	}
	return 1;
	
}

/* return pointer to first record in list */
char * hash_table_first(hash_table_t *t) {

	return t->entrypoint;

}

/* return pointer to next record in list or NULL if there is no mere entries */
char * hash_table_next(hash_table_t *t, char *prow) {

	if (prow == NULL) {

		return NULL;

	}

	hash_table_row_hdr_t *phdr = (hash_table_row_hdr_t *)prow;

	return phdr->snext;

}

/* fetch data on prow pointer; no extra chexks for ranges! */
void hash_table_fetch(hash_table_t *t, char *prow, char **pkey, char **pval) {

	hash_table_row_hdr_t *phdr;

	phdr = (hash_table_row_hdr_t *)prow;	
	*pkey = (prow + sizeof(hash_table_row_hdr_t));
	*pval = (prow + sizeof(hash_table_row_hdr_t) + t->keylen);

}


/* merge ts table into td */
hash_table_t * hash_table_merge(hash_table_t *td, hash_table_t *ts) {
	
	char *prow, *pkey, *pval, *tmp;
	hash_table_row_hdr_t *phdr;
	int index;

	for (index = 0; index < ts->numbuckets; index++) {

		if (ts->buckets[index] != NULL) {
			
			prow = ts->buckets[index];

			while (prow != NULL) {
				phdr = (hash_table_row_hdr_t *)prow;	
				pkey = (prow + sizeof(hash_table_row_hdr_t));
				pval = (prow + sizeof(hash_table_row_hdr_t) + ts->keylen);
				if (hash_table_insert(td, pkey, pval) == NULL) {
					return NULL;
				}
				/* row inserted into new table - we can remove it */
				tmp = phdr->hnext;
				free(prow);
				prow = tmp;
			}
		} 
	}

	/* all items are in a new table */
	free(ts->buckets);
	ts->buckets = NULL;

	return td;
}

void hash_table_free(hash_table_t *t) {

	unsigned long index;


	/* if has was sorted remove all items */
	if (t->sort_array != NULL) {
		for (index = 0 ; index < t->numentries; index++) {
			if (t->sort_array[index] != NULL) {
				free(t->sort_array[index]);
			}
		}
		free(t->sort_array);
	}

	/* remove buckets structure */
	if (t->buckets != NULL) {
		free(t->buckets);
	}
}
