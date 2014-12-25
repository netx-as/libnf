
#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <netinet/in.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "libnf_internal.h"
#include "libnf.h"
#include "fields.h"

/* define aggregation functions */
/* gets *a and *b and set result to *a */
#define GEN_AGGR_SUM(name, type) static void name (char *a, char *b) { \
	*(( type *)a) += *(( type *)b);	\
}								
GEN_AGGR_SUM(lnf_mem_aggr_SUM_UINT8, uint8_t);
GEN_AGGR_SUM(lnf_mem_aggr_SUM_UINT16, uint16_t);
GEN_AGGR_SUM(lnf_mem_aggr_SUM_UINT32, uint32_t);
GEN_AGGR_SUM(lnf_mem_aggr_SUM_UINT64, uint64_t);


#define GEN_AGGR_OR(name, type) static void name (char *a, char *b) { \
	*(( type *)a) |= *(( type *)b);	\
}
GEN_AGGR_OR(lnf_mem_aggr_OR_UINT8, uint8_t);
GEN_AGGR_OR(lnf_mem_aggr_OR_UINT16, uint16_t);
GEN_AGGR_OR(lnf_mem_aggr_OR_UINT32, uint32_t);
GEN_AGGR_OR(lnf_mem_aggr_OR_UINT64, uint64_t);
								
#define GEN_AGGR_MIN(name, type) static void name (char *a, char *b) {	\
	if ( *((type *)a) > *((type *)b) ) *((type *)a) = *((type *)b); \
}
GEN_AGGR_MIN(lnf_mem_aggr_MIN_UINT8, uint8_t);
GEN_AGGR_MIN(lnf_mem_aggr_MIN_UINT16, uint16_t);
GEN_AGGR_MIN(lnf_mem_aggr_MIN_UINT32, uint32_t);
GEN_AGGR_MIN(lnf_mem_aggr_MIN_UINT64, uint64_t);

#define GEN_AGGR_MAX(name, type) static void name (char *a, char *b) {	\
	if ( *((type *)a) < *((type *)b) ) *((type *)a) = *((type *)b); \
}
GEN_AGGR_MAX(lnf_mem_aggr_MAX_UINT8, uint8_t);
GEN_AGGR_MAX(lnf_mem_aggr_MAX_UINT16, uint16_t);
GEN_AGGR_MAX(lnf_mem_aggr_MAX_UINT32, uint32_t);
GEN_AGGR_MAX(lnf_mem_aggr_MAX_UINT64, uint64_t);

static void lnf_mem_aggr_EMPTY (char *a, char *b) { }

/* initialize memory heap structure */
int lnf_mem_init(lnf_mem_t **lnf_memp) {
	lnf_mem_t *lnf_mem;

	lnf_mem = malloc(sizeof(lnf_mem_t));

	if (lnf_mem == NULL) {
        return LNF_ERR_NOMEM;
    }

	lnf_mem->key_list = NULL;
	lnf_mem->key_len = 0;
	lnf_mem->val_list = NULL;
	lnf_mem->val_len = 0;
//	lnf_mem->sort_list = NULL;
//	lnf_mem->sort_len = 0;
	lnf_mem->sort_offset = 0;
	lnf_mem->sort_flags = LNF_SORT_FLD_NONE;

	lnf_mem->fastaggr_mode = LNF_FAST_AGGR_NONE;

	//lnf_mem->hash_ptr = NULL;
	lnf_mem->sorted = 0;
	lnf_mem->numthreads = 0;
	lnf_mem->read_index = 0;
#ifdef LNF_THREADS
	if (pthread_mutex_init(&lnf_mem->thread_mutex, NULL) != 0) {
		free(lnf_mem);
        return LNF_ERR_OTHER;
	}
	
	if (pthread_key_create(&lnf_mem->thread_id_key, NULL) != 0) {
		free(lnf_mem);
        return LNF_ERR_OTHER;
	}
#else 
	lnf_mem->thread_id_key = NULL;
#endif

	*lnf_memp =  lnf_mem;

	return LNF_OK;
}

/* initialise thread specific data structures */
int lnf_mem_thread_init(lnf_mem_t *lnf_mem) {

	int *id; 
	void *aggr_callback;

	if (lnf_mem->numthreads > LNF_MAX_THREADS) {
		return LNF_ERR_OTHER;	
	}

	id = malloc(sizeof(int));

	if (id == NULL) {
		return LNF_ERR_NOMEM;
	}

#ifdef LNF_THREADS
	pthread_mutex_lock(&lnf_mem->thread_mutex);
#endif

	/* determine ID for the current thread and store */
	*id = lnf_mem->numthreads;
	lnf_mem->numthreads++;

#ifdef LNF_THREADS
	pthread_setspecific(lnf_mem->thread_id_key, (void *)id);
	pthread_mutex_unlock(&lnf_mem->thread_mutex);
#else
	lnf_mem->thread_id_key = id;
#endif

	if (lnf_mem->fastaggr_mode == LNF_FAST_AGGR_BASIC) {
		aggr_callback = &lnf_mem_fastaggr_callback;
	} else {
		aggr_callback = &lnf_mem_aggr_callback;
	}

	if (hash_table_init(&lnf_mem->hash_table[*id], HASH_TABLE_INIT_SIZE,
			aggr_callback, &lnf_mem_sort_callback, lnf_mem) == NULL) {

		return LNF_ERR_NOMEM;

	}
	
	hash_table_entry_len(&lnf_mem->hash_table[*id], lnf_mem->key_len, lnf_mem->val_len);

	return LNF_OK;
}

/*! 
* add item to linked list 
* if the node with same ID exists in list then only update 
* aggr_flag, sort_flag numbits and numbits6 
* @sizep sum size of all fields 
* @offset of updated of added item 
*/
int lnf_filedlist_add_or_upd(lnf_fieldlist_t **list, lnf_fieldlist_t *snode, int *sizep, int maxsize, int *roffset) {

	lnf_fieldlist_t *node, *tmp_node;
	int offset = 0;

	/* find item in list and update */
	tmp_node = *list;
	while (tmp_node != NULL) {
		if (tmp_node->field == snode->field) {
			tmp_node->aggr_flag = snode->aggr_flag;
			tmp_node->sort_flag = snode->sort_flag;
			tmp_node->numbits = snode->numbits;
			tmp_node->numbits6 = snode->numbits6;
			tmp_node->aggr_func = snode->aggr_func;

			*roffset = tmp_node->offset;
			/* go via remaining items in list to find the size of the list */
			while (tmp_node != NULL) {
				*sizep = tmp_node->offset + tmp_node->size;
				tmp_node = tmp_node->next;
			}

			return LNF_OK;	
		}
		tmp_node = tmp_node->next;
	}


	/* field was not found - add into list */
	node = malloc(sizeof(lnf_fieldlist_t));

	if (node == NULL) {
		return LNF_ERR_NOMEM;
	}

	memcpy(node, snode, sizeof(lnf_fieldlist_t));

	node->offset = 0;

	if (*list == NULL) {
		*list = node;	
	} else {
		tmp_node = *list;
		offset = tmp_node->size;
		while (tmp_node->next != NULL) {
			tmp_node = tmp_node->next;
			offset = tmp_node->offset + tmp_node->size;
		}
		/* not enough space in buffer */
		if (maxsize <= node->offset + node->size) {
			free(node);
			return LNF_ERR_NOMEM;
		}
		tmp_node->next = node;
	}

	node->offset = offset;
	node->next = NULL;
	*sizep = node->offset + node->size;

	*roffset = offset;

	return LNF_OK;
}

void lnf_filedlist_free(lnf_fieldlist_t *list) {

	lnf_fieldlist_t *node, *tmp_node;

	node = list;

	while (node == NULL) {
		tmp_node = node;
		node = node->next;
		free(tmp_node);
	}
}

/* set fast aggregation mode */
int lnf_mem_fastaggr(lnf_mem_t *lnf_mem, int mode) {

	if (mode != LNF_FAST_AGGR_BASIC && mode != LNF_FAST_AGGR_ALL) {
		return LNF_ERR_OTHER;
	}

	lnf_mem->fastaggr_mode = LNF_FAST_AGGR_BASIC;

	lnf_mem_fadd(lnf_mem, LNF_FLD_FIRST, LNF_AGGR_MIN, 0, 0);
	lnf_mem_fadd(lnf_mem, LNF_FLD_LAST, LNF_AGGR_MAX, 0, 0);
	lnf_mem_fadd(lnf_mem, LNF_FLD_DOCTETS, LNF_AGGR_SUM, 0, 0);
	lnf_mem_fadd(lnf_mem, LNF_FLD_DPKTS, LNF_AGGR_SUM, 0, 0);
	lnf_mem_fadd(lnf_mem, LNF_FLD_AGGR_FLOWS, LNF_AGGR_SUM, 0, 0);

//	lnf_mem->fastaggr_mode = LNF_FAST_AGGR_ALL;

	return LNF_OK;
}

int lnf_mem_fadd(lnf_mem_t *lnf_mem, int field, int flags, int numbits, int numbits6) {

	lnf_fieldlist_t fld;
	int offset;
	
	fld.field = field;
	switch (lnf_fld_type(field)) { 
		case LNF_UINT8: fld.size = sizeof(uint8_t); break;
		case LNF_UINT16: fld.size = sizeof(uint16_t); break;
		case LNF_UINT32: fld.size = sizeof(uint32_t); break;
		case LNF_UINT64: fld.size = sizeof(uint64_t); break;
		case LNF_ADDR: fld.size = sizeof(lnf_ip_t); break;
		case LNF_MAC: fld.size = sizeof(lnf_mac_t); break;
		default : 
			return LNF_ERR_UNKFLD;
	}

	fld.type = lnf_fld_type(field);
	fld.numbits = numbits;
	fld.numbits6 = numbits6;
	if (!flags) {
		lnf_fld_info(field, LNF_FLD_INFO_AGGR, &fld.aggr_flag);
	} else {
		fld.aggr_flag = flags & LNF_AGGR_FLAGS;
		fld.sort_flag = flags & LNF_SORT_FLAGS;
	}

	/* select aggregation func for item */
	fld.aggr_func = lnf_mem_aggr_EMPTY;
	switch (lnf_fld_type(fld.field)) {
		case LNF_UINT8: 
			switch (fld.aggr_flag) {
			case LNF_AGGR_SUM: fld.aggr_func = lnf_mem_aggr_SUM_UINT8; break;
			case LNF_AGGR_MIN: fld.aggr_func = lnf_mem_aggr_MIN_UINT8; break;
			case LNF_AGGR_MAX: fld.aggr_func = lnf_mem_aggr_MAX_UINT8; break;
			case LNF_AGGR_OR:  fld.aggr_func = lnf_mem_aggr_OR_UINT8; break;
			} break;
		case LNF_UINT16: 
			switch (fld.aggr_flag) {
			case LNF_AGGR_SUM: fld.aggr_func = lnf_mem_aggr_SUM_UINT16; break;
			case LNF_AGGR_MIN: fld.aggr_func = lnf_mem_aggr_MIN_UINT16; break;
			case LNF_AGGR_MAX: fld.aggr_func = lnf_mem_aggr_MAX_UINT16; break;
			case LNF_AGGR_OR:  fld.aggr_func = lnf_mem_aggr_OR_UINT16; break;
			} break;
		case LNF_UINT32: 
			switch (fld.aggr_flag) {
			case LNF_AGGR_SUM: fld.aggr_func = lnf_mem_aggr_SUM_UINT32; break;
			case LNF_AGGR_MIN: fld.aggr_func = lnf_mem_aggr_MIN_UINT32; break;
			case LNF_AGGR_MAX: fld.aggr_func = lnf_mem_aggr_MAX_UINT32; break;
			case LNF_AGGR_OR:  fld.aggr_func = lnf_mem_aggr_OR_UINT32; break;
			} break;
		case LNF_UINT64: 
			switch (fld.aggr_flag) {
			case LNF_AGGR_SUM: fld.aggr_func = lnf_mem_aggr_SUM_UINT64; break;
			case LNF_AGGR_MIN: fld.aggr_func = lnf_mem_aggr_MIN_UINT64; break;
			case LNF_AGGR_MAX: fld.aggr_func = lnf_mem_aggr_MAX_UINT64; break;
			case LNF_AGGR_OR:  fld.aggr_func = lnf_mem_aggr_OR_UINT64; break;
			} break;
		/* other data types are ignored so far */
	}
	

	/* add to key list */
	if ((flags & LNF_AGGR_FLAGS) == LNF_AGGR_KEY) {
		if ( lnf_filedlist_add_or_upd(&lnf_mem->key_list, &fld, &lnf_mem->key_len, LNF_MAX_KEY_LEN, &offset) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
		if ((flags & LNF_SORT_FLAGS) != LNF_SORT_NONE) {
			lnf_mem->sort_field = field;
			lnf_mem->sort_offset = offset;
			lnf_mem->sort_flags = LNF_SORT_FLD_IN_KEY;
		}
	} else { /* add to value list */
		if ( lnf_filedlist_add_or_upd(&lnf_mem->val_list, &fld, &lnf_mem->val_len, LNF_MAX_VAL_LEN, &offset) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
		if ((flags & LNF_SORT_FLAGS) != LNF_SORT_NONE) {
			lnf_mem->sort_field = field;
			lnf_mem->sort_offset = offset;
			lnf_mem->sort_flags = LNF_SORT_FLD_IN_VAL;
		}
	}

	return LNF_OK;
}

/* set bits "from" to zero value (for IPv4) */
static void inline lnf_clear_bits_v4(uint32_t *buf, int from) {
	uint32_t tmp;

	if ( from < sizeof(uint32_t) * 8 ) {
		tmp = ntohl(*buf);
		tmp &= ~ ( 0xFFFFFFFF >> from );
		*buf = htonl(tmp); 
	}

}

/* set bits "from" to zero value (for IPv6) */
static void inline lnf_clear_bits_v6(uint64_t *buf, int from) {
	uint64_t tmp;

	if ( from == sizeof(uint64_t) * 8 ) {		/* 64 bits */
		buf[1] = 0x0;						/* clear top 64 bits */
	} else if ( from < sizeof(uint64_t) * 8 ) {	/* 0..63 bits */
		tmp = ntohll(buf[0]);
		tmp &= ~ ( 0xFFFFFFFFFFFFFFFF >> from );
		buf[0] = htonll(tmp); 
		buf[1] = 0x0;						/* clear top 64 bits */
	} else if ( from > sizeof(uint64_t) * 8 && from < 2 * sizeof(uint64_t) * 8 ) {
		tmp = ntohll(buf[1]);				/* 65 .. 127 bits */
		tmp &= ~ ( 0xFFFFFFFFFFFFFFFF >> (from - sizeof(uint64_t) * 8 ));
		buf[1] = htonll(tmp); 
	} /* else 128 bits */

}

/* fill buffer according to the field list */
int lnf_mem_fill_buf(lnf_fieldlist_t *fld, lnf_rec_t *rec, char *buf) {

	int keysize = 0;

	while (fld != NULL) {
		char *ckb = (char *)buf + fld->offset;

		/* put contenf of the field to the buf + offset */
		__lnf_rec_fget(rec, fld->field, ckb);

		/* clear numbits for IP address field */
		/* ! address is always stored in network order */
		if (fld->type == LNF_ADDR) {
			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)ckb)) {
				lnf_clear_bits_v4((uint32_t *)&(((lnf_ip_t *)ckb)->data[3]), fld->numbits);
				//lnf_clear_bits((char *)&(((lnf_ip_t *)ckb)->data[3]), sizeof(uint32_t), fld->numbits);
			} else {
				lnf_clear_bits_v6((uint64_t *)ckb, fld->numbits6);
				//lnf_clear_bits(ckb, sizeof(lnf_ip_t), fld->numbits6);
			}
		}
		keysize += fld->size;
		fld = fld->next;
	}

	return keysize;
}

/* fill record the list from buffer */
void lnf_mem_fill_rec(lnf_fieldlist_t *fld, char *buf, lnf_rec_t *rec) {

	while (fld != NULL) {
		char *ckb = (char *)buf + fld->offset;

		/*get content of the field from the buf + offset */
		__lnf_rec_fset(rec, fld->field, ckb);

		fld = fld->next;
	}

	return;
}

/* callback for updating items in hash table */
void lnf_mem_aggr_callback(char *key, char *hval, char *uval, void *lnf_mem) {

	lnf_fieldlist_t *fld = ((lnf_mem_t *)lnf_mem)->val_list;

	while (fld != NULL) {
		char *hckb = (char *)hval + fld->offset;
		char *uckb = (char *)uval + fld->offset;

		fld->aggr_func(hckb, uckb); 

		fld = fld->next;
	}
}

/* callback for updating items in hash table - fast aggregation version */
void lnf_mem_fastaggr_callback(char *key, char *hval, char *uval, void *lnf_mem) {

	lnf_fastaggr_t *h = (lnf_fastaggr_t *)hval;
	lnf_fastaggr_t *u = (lnf_fastaggr_t *)hval;

	if (u->first < h->first) h->first = u->first;
	if (u->last > h->last) h->last = u->last;
	h->doctets += u->doctets;
	h->dpkts += u->dpkts;
	h->aggr_flows += u->aggr_flows;

}

/* callback for comparing two items */
int lnf_mem_sort_callback(char *key1, char *val1, char *key2, char *val2, void *p) {

	lnf_mem_t *lnf_mem = p;
	char *i1;
	char *i2;

	switch (lnf_mem->sort_flags) {

		case LNF_SORT_FLD_IN_KEY:
			i1 = key1 + lnf_mem->sort_offset;
			i2 = key2 + lnf_mem->sort_offset;
			break;

		case LNF_SORT_FLD_IN_VAL:
			i1 = val1 + lnf_mem->sort_offset;
			i2 = val2 + lnf_mem->sort_offset;
			break;

		default: 
			return 0;
	}

	switch (__lnf_fld_type(lnf_mem->sort_field)) {
		case LNF_UINT64: 
			return *(uint64_t *)i1 < *(uint64_t *)i2; 
			break;
		case LNF_UINT32: 
			return *(uint32_t *)i1 < *(uint32_t *)i2; 
			break;
		case LNF_UINT16: 
			return *(uint16_t *)i1 < *(uint16_t *)i2; 
			break;
		case LNF_UINT8: 
			return *(uint8_t *)i1 < *(uint8_t *)i2; 
			break;
		case LNF_ADDR:
			return (memcmp(i1, i2, sizeof(lnf_ip_t)) > 0); 
			break;
		case LNF_MAC: 
			return (memcmp(i1, i2, sizeof(lnf_mac_t)) > 0 ); 
			break;
		default: 
			return 0;
	}
	
}

/* store record in memory heap */
int lnf_mem_write(lnf_mem_t *lnf_mem, lnf_rec_t *rec) {

	int *id;
	char keybuf[LNF_MAX_KEY_LEN]; 
	char valbuf[LNF_MAX_VAL_LEN];
	lnf_fastaggr_t *fa = (lnf_fastaggr_t *)valbuf;


	/* build key */
	lnf_mem_fill_buf(lnf_mem->key_list, rec, keybuf);

	/* build values */
	if (lnf_mem->fastaggr_mode == LNF_FAST_AGGR_BASIC) {
	
		__lnf_rec_fget(rec, LNF_FLD_FIRST, (void *)&fa->first);	
		__lnf_rec_fget(rec, LNF_FLD_LAST, (void *)&fa->last);	
		__lnf_rec_fget(rec, LNF_FLD_DOCTETS, (void *)&fa->doctets);	
		__lnf_rec_fget(rec, LNF_FLD_DPKTS, (void *)&fa->dpkts);	
		__lnf_rec_fget(rec, LNF_FLD_AGGR_FLOWS, (void *)&fa->aggr_flows);	

	//	lnf_mem_fill_buf(lnf_mem->val_list, rec, valbuf + sizeof(lnf_fastaggr_t));

	} else {
		lnf_mem_fill_buf(lnf_mem->val_list, rec, valbuf);
	}

#ifdef LNF_THREADS
	id = pthread_getspecific(lnf_mem->thread_id_key);
#else 
	id = lnf_mem->thread_id_key;
#endif

	/* no thread specific data */
	if (id == NULL) {
		int ret = lnf_mem_thread_init(lnf_mem);
		if (ret != LNF_OK) {
			return ret;
		}

#ifdef LNF_THREADS
		id = pthread_getspecific(lnf_mem->thread_id_key);
#else
		id = lnf_mem->thread_id_key;
#endif
		if (id == NULL) {
			return LNF_ERR_OTHER;
		}
		lnf_mem->thread_status[*id] = LNF_TH_WRITE;
	}


	/* insert record */
	if (hash_table_insert(&lnf_mem->hash_table[*id], keybuf, valbuf) == NULL) {
		return LNF_ERR_NOMEM;
	}

	return LNF_OK;
}

/* merge data in hash tables from all threads */
int lnf_mem_merge_threads(lnf_mem_t *lnf_mem) {
#ifdef LNF_THREADS

	int *id;
	int id2, i, finish, merge;

	id = pthread_getspecific(lnf_mem->thread_id_key);

	if (id == NULL) {
		return LNF_OK;
	}

//	printf("READY FOR MERGE %d\n", *id);

	/* set status of the current thread to "ready for merge" */
	pthread_mutex_lock(&lnf_mem->thread_mutex);
	lnf_mem->thread_status[*id] = LNF_TH_MERGE;
	pthread_mutex_unlock(&lnf_mem->thread_mutex);

	for (;;) {

		/* choose somebody who can merge with me */
		pthread_mutex_lock(&lnf_mem->thread_mutex);

		finish = 1;
		merge = 0;

		for (i = *id + 1; i < lnf_mem->numthreads; i++) {
			if (lnf_mem->thread_status[*id] == LNF_TH_MERGE && lnf_mem->thread_status[i] == LNF_TH_MERGE) {
				id2 = i; 
				lnf_mem->thread_status[i] = LNF_TH_MERGED;
				lnf_mem->thread_status[*id] = LNF_TH_MERGING;
				finish = 0;
				merge = 1;
			}
			if (lnf_mem->thread_status[i] != LNF_TH_CLEARED) {
				finish = 0;
			}
		}

		/* nothing to merge */
		if (finish) {
			pthread_mutex_unlock(&lnf_mem->thread_mutex);
//			printf("FINISH %d \n", *id );
			return LNF_OK;
		}


		pthread_mutex_unlock(&lnf_mem->thread_mutex);

		if (merge) {
//			printf("MERGE %d <- %d [%d] \n", *id, id2, lnf_mem->numthreads);

			if (hash_table_merge(&lnf_mem->hash_table[*id], &lnf_mem->hash_table[id2]) == NULL) {
//				printf("MERGE FAIL: %d\n", *id);
				return LNF_ERR_NOMEM;
			}
//			printf("MERGE %d <- %d [%d] DONE \n", *id, id2, lnf_mem->numthreads);
//			printf("FREE %d\n", id2);
			hash_table_free(&lnf_mem->hash_table[id2]);

			pthread_mutex_lock(&lnf_mem->thread_mutex);
			lnf_mem->thread_status[*id] = LNF_TH_MERGE; 
			lnf_mem->thread_status[id2] = LNF_TH_CLEARED; 
			pthread_mutex_unlock(&lnf_mem->thread_mutex);
		}
	}

#else 
	return LNF_OK;
#endif	/* ifdef LNF_THREADS */
}

/* read netx record from memory heap */
int lnf_mem_read(lnf_mem_t *lnf_mem, lnf_rec_t *rec) {

	char *key; 
	char *val;

	if (!lnf_mem->sorted) {
		hash_table_sort(&lnf_mem->hash_table[0]);
		lnf_mem->sorted = 1;
		lnf_mem->read_index = 0;
	}

	if (!hash_table_fetch(&lnf_mem->hash_table[0], lnf_mem->read_index, &key, &val)) {
		return LNF_EOF;
	}

	lnf_mem->read_index++;

	lnf_rec_clear(rec);

	/* fields from key */
	lnf_mem_fill_rec(lnf_mem->key_list, key, rec);

	/* fields from values */
	lnf_mem_fill_rec(lnf_mem->val_list, val, rec);

	return LNF_OK;
}

void lnf_mem_free(lnf_mem_t *lnf_mem) {

	hash_table_free(&lnf_mem->hash_table[0]);

	/* clean lists */
	lnf_filedlist_free(lnf_mem->key_list);
	lnf_filedlist_free(lnf_mem->val_list);

	free(lnf_mem);

}

