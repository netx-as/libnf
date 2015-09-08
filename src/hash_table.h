/* 

 Copyright (c) 2013-2015, Tomas Podermanski
    
 This file is part of libnf.net project.

 Libnf is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Libnf is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libnf.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdint.h>

#define HASH_TABLE_INIT_SIZE  500000


/* hash table header */
/* items are organizes into linked hash table via hnext */
/* and linked list via snext */
/* during insertion hnext and snext are identical */
/* after sorting or calling lnf_mem_first snext is rearranged into simple linked list */
typedef struct hash_table_row_hdr_s {
	unsigned long hash;
	char *hnext;		/* next item for hash table */
	char *snext;		/* linked list for sorted items */
} hash_table_row_hdr_t;


/* callback called when same key is found */
typedef void (*hash_table_aggr_callback_t)(char *key, char *hval, char *uval, void *p);
typedef int (*hash_table_sort_callback_t)(char *key1, char *val1, char *key2, char *val2, void *p);

/* hash table handler */
typedef struct hash_table_s {
//	int	alocating_buckets;		/* in process of allocationg new buckets */
	int keylen;					/* size of aggregation key */
	int vallen;					/* size of vallues key */
//	int rowlen;					/* total size of tow (row_flags_t + akey, skey, val) */
//	unsigned long rows_used;		/* number of filled rows */
//	unsigned long collisions;		/* number of collisions */
//	unsigned long rows_inserted;	/* number of rows  */
	hash_table_aggr_callback_t aggr_callback;
	hash_table_sort_callback_t sort_callback;
	void * callback_data;		/* data tructure handled when called callback */
	int numbuckets;				/* number of allocated buckets */
	char ** buckets;
	char * sfirst;			/* entry point into linked list of elements */
	char * slast;			/* last element in linked list */
	unsigned long numentries; 
//	unsigned long read_index; 
	
	char ** sort_array;
//	unsigned long sort_items;
} hash_table_t;


/*
hash_table_t * hash_table_init(hash_table_t *t, int keylen, int vallen,
            hash_table_aggr_callback_t acb, hash_table_sort_callback_t scb, void *callback_data);
void * hash_table_insert(hash_table_t *t, char *key, char *val, int allow_newbck, int *firstentry);
unsigned long hash_table_fetch(hash_table_t *t, unsigned long index, char **key, char **val);
void hash_table_free(hash_table_t *t);
*/

hash_table_t * hash_table_init(hash_table_t *t, int numbuckets,
            hash_table_aggr_callback_t acb,
            hash_table_sort_callback_t scb,
            void *callback_data);

char * hash_table_first(hash_table_t *t);
char * hash_table_next(hash_table_t *t, char *prow);
void hash_table_fetch(hash_table_t *t, char *prow, char **pkey, char **pval);
void hash_table_entry_len(hash_table_t *t, int keylen, int vallen);
char * hash_table_insert_hash(hash_table_t *t, char *key, char *val);
char * hash_table_insert_list(hash_table_t *t, char *key, char *val);
char * hash_table_lookup(hash_table_t *t, char *key, char **val, unsigned long *hash);
int hash_table_sort_callback(char *prow1, char *prow2, void *p);
void hash_table_link(hash_table_t *t);
int hash_table_sort(hash_table_t *t);
hash_table_t * hash_table_join(hash_table_t *td, hash_table_t *ts);
hash_table_t * hash_table_merge(hash_table_t *td, hash_table_t *ts);
void hash_table_free(hash_table_t *t);


