
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

	lnf_mem->hash_ptr = NULL;
	lnf_mem->sorted = 0;
	lnf_mem->numthreads = 0;
	if (pthread_mutex_init(&lnf_mem->thread_mutex, NULL) != 0) {
		free(lnf_mem);
        return LNF_ERR_OTHER;
	}
	
	if (pthread_key_create(&lnf_mem->thread_id_key, NULL) != 0) {
		free(lnf_mem);
        return LNF_ERR_OTHER;
	}

	*lnf_memp =  lnf_mem;

	return LNF_OK;
}

/* initialise thread specific data structures */
int lnf_mem_thread_init(lnf_mem_t *lnf_mem) {

	int *id; 

	if (lnf_mem->numthreads > LNF_MAX_THREADS) {
		return LNF_ERR_OTHER;	
	}

	id = malloc(sizeof(int));

	if (id == NULL) {
		return LNF_ERR_NOMEM;
	}

	/* determine ID for the current thread and store */
	pthread_mutex_lock(&lnf_mem->thread_mutex);

	*id = lnf_mem->numthreads;
	lnf_mem->numthreads++;
	pthread_setspecific(lnf_mem->thread_id_key, (void *)id);
		
	pthread_mutex_unlock(&lnf_mem->thread_mutex);

	if (hash_table_init(&lnf_mem->hash_table[*id], HASH_TABLE_INIT_SIZE,
			&lnf_mem_aggr_callback, &lnf_mem_sort_callback, lnf_mem) == NULL) {

		return LNF_ERR_NOMEM;

	}
	
	hash_table_entry_len(&lnf_mem->hash_table[*id], lnf_mem->key_len, lnf_mem->val_len);

	return LNF_OK;
}

/* add item to linked list */
int lnf_filedlist_add(lnf_fieldlist_t **list, lnf_fieldlist_t *snode, int *sizep, int maxsize, int *roffset) {

	lnf_fieldlist_t *node, *tmp_node;
	int offset = 0;	


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

int lnf_mem_fadd(lnf_mem_t *lnf_mem, int field, int flags, int numbits, int numbits6) {

	lnf_fieldlist_t fld;
	int offset;
	
	fld.field = field;
	switch (LNF_GET_TYPE(field)) { 
		case LNF_UINT8: fld.size = sizeof(uint8_t); break;
		case LNF_UINT16: fld.size = sizeof(uint16_t); break;
		case LNF_UINT32: fld.size = sizeof(uint32_t); break;
		case LNF_UINT64: fld.size = sizeof(uint64_t); break;
		case LNF_ADDR: fld.size = sizeof(lnf_ip_t); break;
		case LNF_MAC: fld.size = sizeof(lnf_mac_t); break;
		default : 
			return LNF_ERR_UKNFLD;
	}
	fld.numbits = numbits;
	fld.numbits6 = numbits6;
	fld.aggr_flag = flags & LNF_AGGR_FLAGS;
	fld.sort_flag = flags & LNF_SORT_FLAGS;

	/* add to key list */
	if ((flags & LNF_AGGR_FLAGS) == LNF_AGGR_KEY) {
		if ( lnf_filedlist_add(&lnf_mem->key_list, &fld, &lnf_mem->key_len, LNF_MAX_KEY_LEN, &offset) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
		if ((flags & LNF_SORT_FLAGS) != LNF_SORT_NONE) {
			lnf_mem->sort_field = field;
			lnf_mem->sort_offset = offset;
			lnf_mem->sort_flags = LNF_SORT_FLD_IN_KEY;
		}
	} else { /* add to value list */
		if ( lnf_filedlist_add(&lnf_mem->val_list, &fld, &lnf_mem->val_len, LNF_MAX_VAL_LEN, &offset) != LNF_OK ) {
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

/* set bits "from" to the end of the buffer to zero value  */
void lnf_clear_bits(char *buf, int buflen, int from) {
	int i;
	int o;

	/* index and (bit) offset for/of the first cleared octet */
	i = from / 8;
	o = from % 8;

	while (i < buflen) {
		buf[i] = buf[i] & ~ ( 0xFF >> o );
		o = 0;
		i++;
	}	
}

/* fill buffer according to the field list */
int lnf_mem_fill_buf(lnf_fieldlist_t *fld, lnf_rec_t *rec, char *buf) {

	int keysize = 0;

	while (fld != NULL) {
		char *ckb = (char *)buf + fld->offset;

		/* put contenf of the field to the buf + offset */
		lnf_rec_fget(rec, fld->field, ckb);

		/* clear numbits for IP address field */
		/* ! address is always stored in network order */
		if (LNF_GET_TYPE(fld->field) == LNF_ADDR) {
			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)ckb)) {
				lnf_clear_bits((char *)&(((lnf_ip_t *)ckb)->data[4]), sizeof(uint32_t), fld->numbits);
				fprintf(stderr, "XXX clearbits4: %d\n", fld->numbits);
			} else {
				lnf_clear_bits(ckb, sizeof(lnf_ip_t), fld->numbits6);
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
		lnf_rec_fset(rec, fld->field, ckb);

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

		switch (LNF_GET_TYPE(fld->field)) {
			case LNF_UINT8: 
				switch (fld->aggr_flag) {
					case LNF_AGGR_SUM: *((uint8_t *)hckb) += *((uint8_t *)uckb); break;
					case LNF_AGGR_MIN: if ( *((uint8_t *)uckb) < *((uint8_t *)hckb) ) *((uint8_t *)hckb) = *((uint8_t *)uckb); break;
					case LNF_AGGR_MAX: if ( *((uint8_t *)uckb) > *((uint8_t *)hckb) ) *((uint8_t *)hckb) = *((uint8_t *)uckb); break;
					case LNF_AGGR_OR: *((uint8_t *)hckb) |= *((uint8_t *)uckb); break;
				}
				break;
			case LNF_UINT16: 
				switch (fld->aggr_flag) {
					case LNF_AGGR_SUM: *((uint16_t *)hckb) += *((uint16_t *)uckb); break;
					case LNF_AGGR_MIN: if ( *((uint16_t *)uckb) < *((uint16_t *)hckb) ) *((uint16_t *)hckb) = *((uint16_t *)uckb); break;
					case LNF_AGGR_MAX: if ( *((uint16_t *)uckb) > *((uint16_t *)hckb) ) *((uint16_t *)hckb) = *((uint16_t *)uckb); break;
					case LNF_AGGR_OR: *((uint16_t *)hckb) |= *((uint16_t *)uckb); break;
				}
				break;
			case LNF_UINT32: 
				switch (fld->aggr_flag) {
					case LNF_AGGR_SUM: *((uint32_t *)hckb) += *((uint32_t *)uckb); break;
					case LNF_AGGR_MIN: if ( *((uint32_t *)uckb) < *((uint32_t *)hckb) ) *((uint32_t *)hckb) = *((uint32_t *)uckb); break;
					case LNF_AGGR_MAX: if ( *((uint32_t *)uckb) > *((uint32_t *)hckb) ) *((uint32_t *)hckb) = *((uint32_t *)uckb); break;
					case LNF_AGGR_OR: *((uint32_t *)hckb) |= *((uint32_t *)uckb); break;
				}
				break;
			case LNF_UINT64: 
				switch (fld->aggr_flag) {
					case LNF_AGGR_SUM: *((uint64_t *)hckb) += *((uint64_t *)uckb); break;
					case LNF_AGGR_MIN: if ( *((uint64_t *)uckb) < *((uint64_t *)hckb) ) *((uint64_t *)hckb) = *((uint64_t *)uckb); break;
					case LNF_AGGR_MAX: if ( *((uint64_t *)uckb) > *((uint64_t *)hckb) ) *((uint64_t *)hckb) = *((uint64_t *)uckb); break;
					case LNF_AGGR_OR: *((uint64_t *)hckb) |= *((uint64_t *)uckb); break;
				}
				break;
			/* other data types are ignored so far */
		}

		fld = fld->next;
	}
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

	switch (LNF_GET_TYPE(lnf_mem->sort_field)) {
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

	int keylen, vallen;
	int *id;
	char keybuf[LNF_MAX_KEY_LEN]; 
	char valbuf[LNF_MAX_VAL_LEN];


	/* build key */
	keylen = lnf_mem_fill_buf(lnf_mem->key_list, rec, keybuf);

	/* build values */
	vallen = lnf_mem_fill_buf(lnf_mem->val_list, rec, valbuf);

	id = pthread_getspecific(lnf_mem->thread_id_key);

	/* no thread specific data */
	if (id == NULL) {
		int ret = lnf_mem_thread_init(lnf_mem);
		if (ret != LNF_OK) {
			return ret;
		}
		id = pthread_getspecific(lnf_mem->thread_id_key);
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
			hash_table_free(&lnf_mem->hash_table[id2]);

			pthread_mutex_lock(&lnf_mem->thread_mutex);
			lnf_mem->thread_status[*id] = LNF_TH_MERGE; 
			lnf_mem->thread_status[id2] = LNF_TH_CLEARED; 
			pthread_mutex_unlock(&lnf_mem->thread_mutex);
		}
	}
	
}

/* read netx record from memory heap */
int lnf_mem_read(lnf_mem_t *lnf_mem, lnf_rec_t *rec) {

	char *key; 
	char *val;

	if (!lnf_mem->sorted) {
		hash_table_sort(&lnf_mem->hash_table[0]);
		lnf_mem->sorted = 1;
		lnf_mem->hash_ptr = hash_table_first(&lnf_mem->hash_table[0]);
	} else {
		lnf_mem->hash_ptr = hash_table_next(&lnf_mem->hash_table[0], lnf_mem->hash_ptr);
	}

	if (lnf_mem->hash_ptr == NULL) {
		//lnf_mem->hash_index = 0;
		return LNF_EOF;
	}

	hash_table_fetch(&lnf_mem->hash_table[0], lnf_mem->hash_ptr, &key, &val);

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

