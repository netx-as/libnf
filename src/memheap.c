
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
	lnf_mem->sort_list = NULL;
	lnf_mem->sort_len = 0;

	lnf_mem->hash_table_init = 0;

	*lnf_memp =  lnf_mem;



	return LNF_OK;
}

/* add item to linked list */
int lnf_filedlist_add(lnf_fieldlist_t **list, lnf_fieldlist_t *snode, int *sizep) {

	lnf_fieldlist_t *node, *tmp_node;
	int offset = 0;

	node = malloc(sizeof(lnf_fieldlist_t));

	if (node == NULL) {
		return LNF_ERR_NOMEM;
	}

	memcpy(node, snode, sizeof(lnf_fieldlist_t));
	
	if (*list == NULL) {
		*list = node;	
	} else {
		tmp_node = *list;
		offset = tmp_node->size;
		while (tmp_node->next != NULL) {
			tmp_node = tmp_node->next;
			offset = tmp_node->offset + tmp_node->size;
		}
		tmp_node->next = node;
	}

	node->offset = offset;
	node->next = NULL;
	*sizep = node->offset + node->size;

	return LNF_OK;
}

int lnf_mem_fadd(lnf_mem_t *lnf_mem, int field, int flags, int numbits, int numbits6) {

	lnf_fieldlist_t fld;
	
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
		if ( lnf_filedlist_add(&lnf_mem->key_list, &fld, &lnf_mem->key_len) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
	} else { /* add to value list */
		if ( lnf_filedlist_add(&lnf_mem->val_list, &fld, &lnf_mem->val_len) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
	}

	/* add to sort list */
	if ((flags & LNF_SORT_FLAGS) != LNF_SORT_NONE) {
		if ( lnf_filedlist_add(&lnf_mem->sort_list, &fld, &lnf_mem->sort_len) != LNF_OK ) {
			return LNF_ERR_NOMEM;
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

		printf(" %x : size %d, offset %d, masklen4 %d, masklen6 %d, aggr: %x, sort: %x\n",
			fld->field, fld->size, fld->offset, fld->numbits, fld->numbits6, fld->aggr_flag, fld->sort_flag);

		/* put contenf of the field to the keybuf + offset */
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

/* callback for updating items in hash table */
void *lnf_mem_callback(char *key, char *hval, char *uval, void *lnf_mem) {

	lnf_fieldlist_t *fld = ((lnf_mem_t *)lnf_mem)->val_list;

	while (fld != NULL) {
		char *hckb = (char *)hval + fld->offset;
		char *uckb = (char *)uval + fld->offset;
	/*	
		switch (LNF_GET_TYPE(fld->field)) {
			case LNF_UINT8: fld.size = sizeof(uint8_t); break;
			case LNF_UINT16: fld.size = sizeof(uint16_t); break;
			case LNF_UINT32: fld.size = sizeof(uint32_t); break;
			case LNF_UINT64: fld.size = sizeof(uint64_t); break;
			case LNF_ADDR: fld.size = sizeof(lnf_ip_t); break;
			case LNF_MAC: fld.size = sizeof(lnf_mac_t); break;
       		 default :
            return LNF_ERR_UKNFLD;
		}


		switch (

		switch (fld->aggr_flag) {
			case LNF_AGGR_MIN: MAX, SUM, OR 
				break;
		}

		printf("CALL  %x : size %d, offset %d, masklen4 %d, masklen6 %d, aggr: %x, sort: %x\n",
			fld->field, fld->size, fld->offset, fld->numbits, fld->numbits6, fld->aggr_flag, fld->sort_flag);
*/
		fld = fld->next;
	
	}


	printf("HASH CALLBACK \n");
}


/* store record in memory heap */
int lnf_mem_write(lnf_mem_t *lnf_mem, lnf_rec_t *rec) {

	lnf_fieldlist_t *fld = lnf_mem->key_list;
	int keylen, vallen;
	char keybuf[1024]; /* XXX !!! */
	char valbuf[1024]; /* XXX !!! */


	/* build key */
	keylen = lnf_mem_fill_buf(lnf_mem->key_list, rec, keybuf);

	/* build values */
	vallen = lnf_mem_fill_buf(lnf_mem->val_list, rec, valbuf);


	/* first record - initialise hash table */
	if ( lnf_mem->hash_table_init == 0 ) {
		if (hash_table_init(&lnf_mem->hash_table, keylen, vallen, &lnf_mem_callback) == NULL) {
			return LNF_ERR_NOMEM;
		}
		lnf_mem->hash_table_init = 1;
	}

	/* insert record */
	if (hash_table_insert(&lnf_mem->hash_table, keybuf, valbuf, lnf_mem) == NULL) {
		return LNF_ERR_NOMEM;
	}

	return LNF_OK;
}


void lnf_mem_free(lnf_mem_t *lnf_mem) {
	free(lnf_mem);
}

