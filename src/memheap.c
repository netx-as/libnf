
#define _LIBNF_C_ 1

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
	lnf_mem->key_size = 0;
	lnf_mem->val_list = NULL;
	lnf_mem->val_size= 0;
	lnf_mem->sort_list = NULL;
	lnf_mem->sort_size = 0;

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

int lnf_mem_addf(lnf_mem_t *lnf_mem, int field, int flags, int numbits, int numbits6) {

	lnf_fieldlist_t fld;
	
	fld.field = field;
	switch (LNF_GET_TYPE(field)) { 
		case LNF_UINT8: fld.size = sizeof(uint8_t); break;
		case LNF_UINT16: fld.size = sizeof(uint16_t); break;
		case LNF_UINT32: fld.size = sizeof(uint32_t); break;
		case LNF_UINT64: fld.size = sizeof(uint64_t); break;
		case LNF_ADDR: fld.size = sizeof(lnf_ip_t); break;
		case LNF_MAC: fld.size = sizeof(lnf_mac_t); break;
		delafult : 
			return LNF_ERR_UKNFLD;
	}
	fld.numbits = numbits;
	fld.numbits6 = numbits6;
	fld.aggr_flag = flags & LNF_AGGR_FLAGS;
	fld.sort_flag = flags & LNF_SORT_FLAGS;

	/* add to key list */

	if ((flags & LNF_AGGR_FLAGS) == LNF_AGGR_KEY) {
		if ( lnf_filedlist_add(&lnf_mem->key_list, &fld, &lnf_mem->key_size) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
	} else { /* add to value list */
		if ( lnf_filedlist_add(&lnf_mem->val_list, &fld, &lnf_mem->val_size) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
	}

	/* add to sort list */
	if ((flags & LNF_SORT_FLAGS) != LNF_SORT_NONE) {
		if ( lnf_filedlist_add(&lnf_mem->sort_list, &fld, &lnf_mem->sort_size) != LNF_OK ) {
			return LNF_ERR_NOMEM;
		}
	}
}


/* store record in memory heap */
int lnf_mem_write(lnf_mem_t *lnf_mem, lnf_rec_t *rec) {

	lnf_fieldlist_t *fld = lnf_mem->key_list;
	char keybuf[1024];

	printf("XXX: %d, %d, %d\n", lnf_mem->key_size, lnf_mem->val_size, lnf_mem->sort_size);
	printf("KEY:\n");

	/* build key */
	while (fld != NULL) {

		printf(" %x : size %d, offset %d, masklen4 %d, masklen6 %d, aggr: %x, sort: %x\n",
			fld->field, fld->size, fld->offset, fld->numbits, fld->numbits6, fld->aggr_flag, fld->sort_flag);

		lnf_rec_fget(rec, fld->field, (char *)keybuf + fld->offset);

		fld = fld->next;
	}

	fld = lnf_mem->val_list;

	printf("VAL:\n");
	while (fld != NULL) {

		printf(" %x : size %d, offset %d, masklen4 %d, masklen6 %d, aggr: %x, sort: %x\n",
			fld->field, fld->size, fld->offset, fld->numbits, fld->numbits6, fld->aggr_flag, fld->sort_flag);
		fld = fld->next;
	}

	fld = lnf_mem->sort_list;

	printf("SORT:\n");
	while (fld != NULL) {

		printf(" %x : size %d, offset %d, masklen4 %d, masklen6 %d, aggr: %x, sort: %x\n",
			fld->field, fld->size, fld->offset, fld->numbits, fld->numbits6, fld->aggr_flag, fld->sort_flag);
		fld = fld->next;
	}
}


void lnf_mem_free(lnf_mem_t *lnf_mem) {
	free(lnf_mem);
}

