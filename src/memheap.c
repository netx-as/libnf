
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


	/* XXX !!!! */
	if (hash_table_init(&lnf_mem->hash_table, 65535,
			&lnf_mem_aggr_callback, &lnf_mem_sort_callback, lnf_mem) == NULL) {

		free(lnf_mem);
		return LNF_ERR_NOMEM;

	}
	
	hash_table_entry_len(&lnf_mem->hash_table, 0, 0);

	*lnf_memp =  lnf_mem;

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


	hash_table_entry_len(&lnf_mem->hash_table, lnf_mem->key_len, lnf_mem->val_len);

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
	char keybuf[LNF_MAX_KEY_LEN]; 
	char valbuf[LNF_MAX_VAL_LEN];


	/* build key */
	keylen = lnf_mem_fill_buf(lnf_mem->key_list, rec, keybuf);

	/* build values */
	vallen = lnf_mem_fill_buf(lnf_mem->val_list, rec, valbuf);

	/* insert record */
	if (hash_table_insert(&lnf_mem->hash_table, keybuf, valbuf) == NULL) {
		return LNF_ERR_NOMEM;
	}

	return LNF_OK;
}


/* read netx record from memory heap */
int lnf_mem_read(lnf_mem_t *lnf_mem, lnf_rec_t *rec) {

	char *key; 
	char *val;

	if (!lnf_mem->sorted) {
		hash_table_sort(&lnf_mem->hash_table);
		lnf_mem->sorted = 1;
		lnf_mem->hash_ptr = hash_table_first(&lnf_mem->hash_table);
	} else {
		lnf_mem->hash_ptr = hash_table_next(&lnf_mem->hash_table, lnf_mem->hash_ptr);
	}

	if (lnf_mem->hash_ptr == NULL) {
		//lnf_mem->hash_index = 0;
		return LNF_EOF;
	}

	hash_table_fetch(&lnf_mem->hash_table, lnf_mem->hash_ptr, &key, &val);

	lnf_rec_clear(rec);

	/* fields from key */
	lnf_mem_fill_rec(lnf_mem->key_list, key, rec);

	/* fields from values */
	lnf_mem_fill_rec(lnf_mem->val_list, val, rec);

	return LNF_OK;
}

void lnf_mem_free(lnf_mem_t *lnf_mem) {

	hash_table_free(&lnf_mem->hash_table);

	/* clean lists */
	lnf_filedlist_free(lnf_mem->key_list);
	lnf_filedlist_free(lnf_mem->val_list);

	free(lnf_mem);

}

