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
GEN_AGGR_SUM(lnf_mem_aggr_SUM_DOUBLE, double);


#define GEN_AGGR_OR(name, type) static void name (char *a, char *b) { \
	*(( type *)a) |= *(( type *)b);	\
}
GEN_AGGR_OR(lnf_mem_aggr_OR_UINT8, uint8_t);
GEN_AGGR_OR(lnf_mem_aggr_OR_UINT16, uint16_t);
GEN_AGGR_OR(lnf_mem_aggr_OR_UINT32, uint32_t);
GEN_AGGR_OR(lnf_mem_aggr_OR_UINT64, uint64_t);
//GEN_AGGR_OR(lnf_mem_aggr_OR_DOUBLE, double);
								
#define GEN_AGGR_MIN(name, type) static void name (char *a, char *b) {	\
	if ( *((type *)a) > *((type *)b) ) *((type *)a) = *((type *)b); \
}
GEN_AGGR_MIN(lnf_mem_aggr_MIN_UINT8, uint8_t);
GEN_AGGR_MIN(lnf_mem_aggr_MIN_UINT16, uint16_t);
GEN_AGGR_MIN(lnf_mem_aggr_MIN_UINT32, uint32_t);
GEN_AGGR_MIN(lnf_mem_aggr_MIN_UINT64, uint64_t);
GEN_AGGR_MIN(lnf_mem_aggr_MIN_DOUBLE, double);

#define GEN_AGGR_MAX(name, type) static void name (char *a, char *b) {	\
	if ( *((type *)a) < *((type *)b) ) *((type *)a) = *((type *)b); \
}
GEN_AGGR_MAX(lnf_mem_aggr_MAX_UINT8, uint8_t);
GEN_AGGR_MAX(lnf_mem_aggr_MAX_UINT16, uint16_t);
GEN_AGGR_MAX(lnf_mem_aggr_MAX_UINT32, uint32_t);
GEN_AGGR_MAX(lnf_mem_aggr_MAX_UINT64, uint64_t);
GEN_AGGR_MAX(lnf_mem_aggr_MAX_DOUBLE, double);

static void lnf_mem_aggr_EMPTY (char *a, char *b) { }

/* initialize memory heap structure */
int lnf_mem_init(lnf_mem_t **lnf_memp) {
	lnf_mem_t *lnf_mem;
	int i;

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
	lnf_mem->sort_field = LNF_FLD_ZERO_; 
	lnf_mem->sorted = 0;
	lnf_mem->statistics_mode = 0;
	lnf_mem->list_mode = 0;
	lnf_mem->nfdump_comp_statistics = 0;
	lnf_mem->numthreads = 0;
	lnf_mem->read_cursor = NULL;
	lnf_mem->hash_table_buckets = HASH_TABLE_INIT_SIZE;
	
	if (!lnf_rec_init(&lnf_mem->lnf_rec)) {
		free(lnf_mem);
		return LNF_ERR_NOMEM;
	}
	
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

	/* set all threads to LNF_TH_EMPTY */
	for (i = 0; i < LNF_MAX_THREADS; i++) {
		lnf_mem->thread_status[i] = LNF_TH_EMPTY;
	}

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

	if (hash_table_init(&lnf_mem->hash_table[*id], lnf_mem->hash_table_buckets, 
			aggr_callback, &lnf_mem_sort_callback, lnf_mem) == NULL) {

		return LNF_ERR_NOMEM;

	}
	
	hash_table_entry_len(&lnf_mem->hash_table[*id], lnf_mem->key_len, lnf_mem->val_len);

	return LNF_OK;
}

/* set options for lnf_mem_t */
int lnf_mem_setopt(lnf_mem_t *lnf_mem, int opt, void *data, size_t size) {

	switch (opt) {
		case LNF_OPT_HASHBUCKETS: 
			if (size != sizeof(int)) {
				return LNF_ERR_OTHER;
			}
			
			lnf_mem->hash_table_buckets = *((int *)data);
			break;	

		case LNF_OPT_LISTMODE:
			lnf_mem->list_mode = 1;
			break;

		case LNF_OPT_COMP_STATSCMP:
			lnf_mem->nfdump_comp_statistics = 1;
			break;
		default: 
			return LNF_ERR_OTHER;
			break;
	}

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

/*! 
* lookup field in linked list and retur pointer to node or NULL
*/
lnf_fieldlist_t* lnf_filedlist_lookup(lnf_fieldlist_t *list, int field) {

	lnf_fieldlist_t *tmp_node;

	/* find item in list and update */
	tmp_node = list;
	while (tmp_node != NULL) {
		if (tmp_node->field == field) {
			return tmp_node;
		} else {
			tmp_node = tmp_node->next;
		}
	}

	return NULL;
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
	int offset, calcnum, ret;
	
	fld.field = field;
	switch (lnf_fld_type(field)) { 
		case LNF_UINT8: fld.size = sizeof(uint8_t); break;
		case LNF_UINT16: fld.size = sizeof(uint16_t); break;
		case LNF_UINT32: fld.size = sizeof(uint32_t); break;
		case LNF_UINT64: fld.size = sizeof(uint64_t); break;
		case LNF_DOUBLE: fld.size = sizeof(LNF_DOUBLE_T); break;
		case LNF_ADDR: fld.size = sizeof(lnf_ip_t); break;
		case LNF_MAC: fld.size = sizeof(lnf_mac_t); break;
		default : 
			return LNF_ERR_UNKFLD;
	}

	fld.type = lnf_fld_type(field);
	fld.numbits = numbits;
	fld.numbits6 = numbits6;
	if ((flags & LNF_AGGR_FLAGS) == 0) {
		/* if type is UINT64 and numbits is set then field is key */
		if (fld.numbits > 0 && fld.type == LNF_UINT64) {
			fld.aggr_flag = LNF_AGGR_KEY;
		} else {
			lnf_fld_info(field, LNF_FLD_INFO_AGGR, &fld.aggr_flag, sizeof(fld.aggr_flag));
		}
	} else {
		fld.aggr_flag = flags & LNF_AGGR_FLAGS;
	}

	fld.sort_flag = flags & LNF_SORT_FLAGS;
		
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
		case LNF_DOUBLE: 
			switch (fld.aggr_flag) {
			case LNF_AGGR_SUM: fld.aggr_func = lnf_mem_aggr_SUM_DOUBLE; break;
			case LNF_AGGR_MIN: fld.aggr_func = lnf_mem_aggr_MIN_DOUBLE; break;
			case LNF_AGGR_MAX: fld.aggr_func = lnf_mem_aggr_MAX_DOUBLE; break;
			case LNF_AGGR_OR:  fld.aggr_func = lnf_mem_aggr_EMPTY; break;
			} break;
		/* other data types are ignored so far */
	}
	

	/* add to key list */
	if (fld.aggr_flag == LNF_AGGR_KEY) {
		if ( lnf_filedlist_add_or_upd(&lnf_mem->key_list, &fld, &lnf_mem->key_len, LNF_MAX_KEY_LEN, &offset) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
		if (fld.sort_flag != LNF_SORT_NONE) {
			lnf_mem->sort_field = field;
			lnf_mem->sort_offset = offset;
			lnf_mem->sort_flags = LNF_SORT_FLD_IN_KEY | fld.sort_flag;
		}
		/* ser statistics mode for lnf_mem if there is pair field */
		if (__lnf_fld_pair(field, 1) != LNF_FLD_ZERO_ && __lnf_fld_pair(field, 2) != LNF_FLD_ZERO_) {
			lnf_mem->statistics_mode = 1;
		}
	} else { /* add to value list */
		if ( lnf_filedlist_add_or_upd(&lnf_mem->val_list, &fld, &lnf_mem->val_len, LNF_MAX_VAL_LEN, &offset) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
		if (fld.sort_flag != LNF_SORT_NONE) {
			lnf_mem->sort_field = field;
			lnf_mem->sort_offset = offset;
			lnf_mem->sort_flags = LNF_SORT_FLD_IN_VAL | fld.sort_flag;
		}
	}

	/* if the filed is calculated field with dependencies add fields that the field is depenedend on */
	calcnum = 0;
	while ( __lnf_fld_calc_dep(field, calcnum) != LNF_FLD_ZERO_) {

		if ( (lnf_filedlist_lookup(lnf_mem->key_list, __lnf_fld_calc_dep(field, calcnum)) == NULL) &&
			 (lnf_filedlist_lookup(lnf_mem->val_list, __lnf_fld_calc_dep(field, calcnum)) == NULL) ) {

			if ((ret = lnf_mem_fadd(lnf_mem, __lnf_fld_calc_dep(field, calcnum), 0, 0,  0)) != LNF_OK) {
				return ret;
			}

		}

		calcnum++;

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


/* align time value */
static void lnf_align_uint64(uint64_t *buf, int align) {

	if (align > 0) {
		*buf = *buf - (*buf % (align * 1000));	
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
/* pairset 0 - do regular aggregation, 1 - use first pair id, 2 - use seconf pair id */
int lnf_mem_fill_buf(lnf_fieldlist_t *fld, lnf_rec_t *rec, char *buf, int pairset) {

	int keysize = 0;
	int field;

	while (fld != NULL) {
		char *ckb = (char *)buf + fld->offset;

		/* detect whether we process aggregation in pair mode - means have pair fields */
		/* but only for pair items */
		if (pairset != 0 && __lnf_fld_pair(fld->field, 1) != LNF_FLD_ZERO_ && __lnf_fld_pair(fld->field, 2) != LNF_FLD_ZERO_) {
			/* get propper field id depend on field set -  1 - first field, 2 - second field */
			field = __lnf_fld_pair(fld->field, pairset);
		} else {
			/* it is not pair field or we are not in pair mode -> choose as ordinary field */
			field = fld->field;	
		}

		/* put contenf of the field to the buf + offset */
		__lnf_rec_fget(rec, field, ckb);

		/* clear numbits for IP address field */
		/* ! address is always stored in network order */
		if (fld->type == LNF_ADDR) {
			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)ckb)) {
				lnf_clear_bits_v4((uint32_t *)&(((lnf_ip_t *)ckb)->data[3]), fld->numbits);
			} else {
				lnf_clear_bits_v6((uint64_t *)ckb, fld->numbits6);
			}
		} else if (fld->type == LNF_UINT64 && fld->numbits > 0) { 
			/* align time fields */
			lnf_align_uint64((uint64_t *)ckb, fld->numbits);
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
	lnf_fastaggr_t *u = (lnf_fastaggr_t *)uval;

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
	int ret = 0;

	switch (lnf_mem->sort_flags & LNF_SORT_FLD_IN_FLAGS) {

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
			ret = *(uint64_t *)i1 < *(uint64_t *)i2; 
			break;
		case LNF_UINT32: 
			ret = *(uint32_t *)i1 < *(uint32_t *)i2; 
			break;
		case LNF_UINT16: 
			ret = *(uint16_t *)i1 < *(uint16_t *)i2; 
			break;
		case LNF_UINT8: 
			ret = *(uint8_t *)i1 < *(uint8_t *)i2; 
			break;
		case LNF_DOUBLE: 
			ret = *(LNF_DOUBLE_T *)i1 < *(LNF_DOUBLE_T *)i2; 
			break;
		case LNF_ADDR:
			ret = (memcmp(i1, i2, sizeof(lnf_ip_t)) > 0); 
			break;
		case LNF_MAC: 
			ret = (memcmp(i1, i2, sizeof(lnf_mac_t)) > 0 ); 
			break;
		default: 
			return 0;
	}

	if ((lnf_mem->sort_flags & LNF_SORT_FLAGS) == LNF_SORT_DESC) {
		return !ret;
	} else {
		return ret;
	}
}

/* store in raw format in the memory heap */
int lnf_mem_write_raw(lnf_mem_t *lnf_mem, char *buff, int buffsize) {

	int *id;

	/* compare the size of buffer - have to macht size of key + val fields */
	if (buffsize != lnf_mem->key_len + lnf_mem->val_len) {
		return LNF_ERR_OTHER;
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

	if (lnf_mem->list_mode) {
		if (hash_table_insert_list(&lnf_mem->hash_table[*id], buff, buff + lnf_mem->key_len) == NULL) {
			return LNF_ERR_NOMEM;
		} else {
			return LNF_OK;
		}
	}

	/* insert record */
	if (hash_table_insert_hash(&lnf_mem->hash_table[*id], buff, buff + lnf_mem->key_len) == NULL) {
		return LNF_ERR_NOMEM;
	}

	return LNF_OK;
}

/* store record in memory heap */
int lnf_mem_write(lnf_mem_t *lnf_mem, lnf_rec_t *rec) {

	int *id;
	int pairset;
	char keybuf[LNF_MAX_KEY_LEN]; 
	char valbuf[LNF_MAX_VAL_LEN];
	lnf_fastaggr_t *fa = (lnf_fastaggr_t *)valbuf;

	/* test if we are in simpple aggregation or statistics mode (if we use pair item in key or not) */
	if (lnf_mem->statistics_mode) {
		pairset = 1;	/* set to first pair set */
	}  else {
		pairset = 0;	/* disable pair mode */
	}

	/* build key */
	lnf_mem_fill_buf(lnf_mem->key_list, rec, keybuf, pairset);

	/* build values */
	if (lnf_mem->fastaggr_mode == LNF_FAST_AGGR_BASIC) {
	
		__lnf_rec_fget(rec, LNF_FLD_FIRST, (void *)&fa->first);	
		__lnf_rec_fget(rec, LNF_FLD_LAST, (void *)&fa->last);	
		__lnf_rec_fget(rec, LNF_FLD_DOCTETS, (void *)&fa->doctets);	
		__lnf_rec_fget(rec, LNF_FLD_DPKTS, (void *)&fa->dpkts);	
		__lnf_rec_fget(rec, LNF_FLD_AGGR_FLOWS, (void *)&fa->aggr_flows);	

	//	lnf_mem_fill_buf(lnf_mem->val_list, rec, valbuf + sizeof(lnf_fastaggr_t));

	} else {
		lnf_mem_fill_buf(lnf_mem->val_list, rec, valbuf, 0);	/* pair sets have no sence for values */
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


	if (lnf_mem->list_mode) {
		if (hash_table_insert_list(&lnf_mem->hash_table[*id], keybuf, valbuf) == NULL) {
			return LNF_ERR_NOMEM;
		} else {
			return LNF_OK;
		}
	}

	/* insert record */
	if (hash_table_insert_hash(&lnf_mem->hash_table[*id], keybuf, valbuf) == NULL) {
		return LNF_ERR_NOMEM;
	}

	/* insert record for pair set 2 if the statistics mode is enabled */
	if (pairset != 0) {
		char keybuf2[LNF_MAX_KEY_LEN]; 

		pairset = 2;

		/* build key for pairset 2 */
		lnf_mem_fill_buf(lnf_mem->key_list, rec, keybuf2, pairset);

		if (lnf_mem->nfdump_comp_statistics) {
			if (memcmp(keybuf, keybuf2, lnf_mem->key_len) == 0) {
				goto END_PAIR;
			}
		}

		/* insert record */
		if (hash_table_insert_hash(&lnf_mem->hash_table[*id], keybuf2, valbuf) == NULL) {
			return LNF_ERR_NOMEM;
		} 
	}

END_PAIR:


	return LNF_OK;
}

/* merge data in hash tables from all threads */
int lnf_mem_merge_threads(lnf_mem_t *lnf_mem) {
#ifdef LNF_THREADS

	int *id;
	int id2 = 0;
	int i, merge;

	id = pthread_getspecific(lnf_mem->thread_id_key);

	if (id == NULL) {
		return LNF_OK;
	}

//	printf("\nREADY FOR MERGE * %d\n", *id);

	/* set status of the current thread to "ready for merge" */
	pthread_mutex_lock(&lnf_mem->thread_mutex);
	lnf_mem->thread_status[*id] = LNF_TH_MERGE;
	pthread_mutex_unlock(&lnf_mem->thread_mutex);

	/* thread id 0 have nothing to do */
	if (*id == 0) {
		return LNF_OK;
	}

	for (;;) {

		/* choose somebody who can I merge to */
//		printf("WAIT READY FOR MERGE * %d\n", *id);

		merge = 0;

		pthread_mutex_lock(&lnf_mem->thread_mutex);
		for (i = 0; i < *id; i++) {
			if (lnf_mem->thread_status[*id] == LNF_TH_MERGE && lnf_mem->thread_status[i] == LNF_TH_MERGE) {
				id2 = i; 
				lnf_mem->thread_status[*id] = LNF_TH_MERGED;
				lnf_mem->thread_status[i] = LNF_TH_MERGING;
				merge = 1;
				break;
			}
		}
		pthread_mutex_unlock(&lnf_mem->thread_mutex);

		if (merge) {
//			printf("MERGE %d <- %d [%d] \n", id2, *id, lnf_mem->numthreads);
		
			/* we either merge hash tables or join them */
			if (lnf_mem->list_mode) {
				if (hash_table_join(&lnf_mem->hash_table[id2], &lnf_mem->hash_table[*id]) == NULL) {
//					printf("JOIN FAIL: %d\n", *id);
					return LNF_ERR_NOMEM;
				}
			} else {
				if (hash_table_merge(&lnf_mem->hash_table[id2], &lnf_mem->hash_table[*id]) == NULL) {
//					printf("MERGE FAIL: %d\n", *id);
					return LNF_ERR_NOMEM;
				}
			}
//			printf("MERGE %d <- %d [%d] DONE \n", id2, *id, lnf_mem->numthreads);
			hash_table_free(&lnf_mem->hash_table[*id]);

			pthread_mutex_lock(&lnf_mem->thread_mutex);
			lnf_mem->thread_status[*id] = LNF_TH_CLEARED; 
			lnf_mem->thread_status[id2] = LNF_TH_MERGE; 
			pthread_mutex_unlock(&lnf_mem->thread_mutex);
			return LNF_OK;
		}

	}

#else 
	return LNF_OK;
#endif	/* ifdef LNF_THREADS */
}

/* walk via all record and recalculate the value of the calculated fields */
void lnf_mem_upd_calc_fields(lnf_mem_t *lnf_mem) {

	char *key;
	char *val;

	lnf_mem_cursor_t *cursor;
	
	cursor = (lnf_mem_cursor_t *)hash_table_first(&lnf_mem->hash_table[0]);

	while ( (cursor != NULL ) ) { 
		hash_table_fetch(&lnf_mem->hash_table[0], (void *)cursor, &key, &val);

    	lnf_mem_fill_rec(lnf_mem->val_list, val, lnf_mem->lnf_rec);
    	lnf_mem_fill_rec(lnf_mem->key_list, key, lnf_mem->lnf_rec);
		lnf_mem_fill_buf(lnf_mem->val_list, lnf_mem->lnf_rec, val, 0);

		cursor = (lnf_mem_cursor_t *)hash_table_next(&lnf_mem->hash_table[0], (void *)cursor);
	}
}

/* set cursot to first record from memory heap */
/* used later by lnf_mem_read and lnf_mem_read_raw */
int lnf_mem_first_c(lnf_mem_t *lnf_mem, lnf_mem_cursor_t **cursor) {

	if (lnf_mem->thread_status[0] == LNF_TH_EMPTY) {
		*cursor = NULL;
		return LNF_EOF;
	}

	if (!lnf_mem->sorted && lnf_mem->sort_field != LNF_FLD_ZERO_) {

		/* if the sorted item is calculated we have to update calculated item first */
		if (__lnf_fld_calc_dep(lnf_mem->sort_field, 0) != LNF_FLD_ZERO_ ) {
			lnf_mem_upd_calc_fields(lnf_mem);
		}

		//hash_table_sort_heap(&lnf_mem->hash_table[0]);
		hash_table_sort(&lnf_mem->hash_table[0]);
		lnf_mem->sorted = 1;
	}

	*cursor = (lnf_mem_cursor_t *)hash_table_first(&lnf_mem->hash_table[0]);

	if (*cursor == NULL) {
		return LNF_EOF;
	}

	return LNF_OK;
}

/* set cursot to next record from memory heap */
/* used later by lnf_mem_read and lnf_mem_read_raw */
int lnf_mem_next_c(lnf_mem_t *lnf_mem, lnf_mem_cursor_t **cursor) {

	if (lnf_mem->thread_status[0] == LNF_TH_EMPTY) {
		return LNF_EOF;
	}

	*cursor = (lnf_mem_cursor_t *)hash_table_next(&lnf_mem->hash_table[0], (void *)*cursor);

	if (*cursor == NULL) {
		return LNF_EOF;
	}

	return LNF_OK;
}

/* set cursot to record identified by key */
int lnf_mem_lookup_c(lnf_mem_t *lnf_mem, lnf_rec_t *rec, lnf_mem_cursor_t **cursor) {

	char *pval;
	unsigned long hash;
	char keybuf[LNF_MAX_KEY_LEN]; 

	if (lnf_mem->thread_status[0] == LNF_TH_EMPTY) {
		*cursor = NULL;
		return LNF_EOF;
	}

	/* build key */
	lnf_mem_fill_buf(lnf_mem->key_list, rec, keybuf, 0);

	*cursor = (lnf_mem_cursor_t *)hash_table_lookup(&lnf_mem->hash_table[0], keybuf, &pval, &hash);

	if (*cursor == NULL) {
		return LNF_EOF;
	}

	return LNF_OK;
}

/* store in raw format in the memory heap */
int lnf_mem_lookup_raw_c(lnf_mem_t *lnf_mem, char *buff, int buffsize, lnf_mem_cursor_t **cursor) {

	char *pval;
	unsigned long hash;

	/* compare the size of buffer - have to macht size of key + val fields */
	if (buffsize != lnf_mem->key_len + lnf_mem->val_len) {
		return LNF_ERR_OTHER;
	}

	if (lnf_mem->thread_status[0] == LNF_TH_EMPTY) {
		*cursor = NULL;
		return LNF_EOF;
	}

	*cursor = (lnf_mem_cursor_t *)hash_table_lookup(&lnf_mem->hash_table[0], buff, &pval, &hash);

	if (*cursor == NULL) {
		return LNF_EOF;
	}

	return LNF_OK;
}

/* read next record from memory heap */
int lnf_mem_read_c(lnf_mem_t *lnf_mem, lnf_mem_cursor_t *cursor, lnf_rec_t *rec) {

	char *key; 
	char *val;

	hash_table_fetch(&lnf_mem->hash_table[0], (void *)cursor, &key, &val);

	lnf_rec_clear(rec);

	/* fields from key */
	lnf_mem_fill_rec(lnf_mem->key_list, key, rec);

	/* fields from values */
	lnf_mem_fill_rec(lnf_mem->val_list, val, rec);

	return LNF_OK;
}

/* deprecated version of read */
int lnf_mem_read(lnf_mem_t *lnf_mem, lnf_rec_t *rec) {

	int ret;

	if (lnf_mem->read_cursor == NULL) {
		ret = lnf_mem_first_c(lnf_mem, &lnf_mem->read_cursor);
	} else {
		ret = lnf_mem_next_c(lnf_mem, &lnf_mem->read_cursor);
	}
	
	if (ret != LNF_OK) {
		return ret;
	}

	return  lnf_mem_read_c(lnf_mem, lnf_mem->read_cursor, rec);

}

/* read next record from memory heap in raw format */
int lnf_mem_read_raw_c(lnf_mem_t *lnf_mem, lnf_mem_cursor_t *cursor, char *buff, int *len, int buffsize) {

	char *key; 
	char *val;

	if (len != NULL) {
		*len = lnf_mem->key_len + lnf_mem->val_len;
	}

	/* check whether there is enough space in buffer */
	if (buffsize < lnf_mem->key_len + lnf_mem->val_len) {
		return LNF_ERR_NOMEM;
	}

	hash_table_fetch(&lnf_mem->hash_table[0], (void *)cursor, &key, &val);

	memcpy(buff, key, lnf_mem->key_len);
	memcpy(buff + lnf_mem->key_len, val, lnf_mem->val_len);

	return LNF_OK;
}

/* deprecated version of read */
int lnf_mem_read_raw(lnf_mem_t *lnf_mem, char *buff, int *len, int buffsize) {

	int ret;
	
	if (lnf_mem->read_cursor == NULL) {
		ret = lnf_mem_first_c(lnf_mem, &lnf_mem->read_cursor);
	} else {
		ret =  lnf_mem_next_c(lnf_mem, &lnf_mem->read_cursor);
	}

	if (ret != LNF_OK) {
		return ret;
	}

	return  lnf_mem_read_raw_c(lnf_mem, lnf_mem->read_cursor, buff, len, buffsize);
}

/* set cursor position to the first rec */
void lnf_mem_read_reset(lnf_mem_t *lnf_mem) {

	lnf_mem_first_c(lnf_mem, &lnf_mem->read_cursor);

}

void lnf_mem_free(lnf_mem_t *lnf_mem) {

	if (lnf_mem == NULL) {
		return;
	}

	if (lnf_mem->thread_status[0] != LNF_TH_EMPTY) {
		hash_table_free(&lnf_mem->hash_table[0]);
	}

	/* clean lists */
	if (lnf_mem->key_list != NULL) {
		lnf_filedlist_free(lnf_mem->key_list);
	}
	if (lnf_mem->val_list != NULL) {
		lnf_filedlist_free(lnf_mem->val_list);
	}

#ifdef LNF_THREADS
	pthread_key_delete(lnf_mem->thread_id_key);
#endif

	free(lnf_mem);

}

