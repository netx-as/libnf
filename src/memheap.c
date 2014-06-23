
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

	lnf_mem->aggr_list = NULL;
	lnf_mem->sort_list = NULL;

	*lnf_memp =  lnf_mem;
	return LNF_OK;
}

int lnf_mem_add_aggr(lnf_mem_t *lnf_mem, int field, int numbits, int numbits6) {

	lnf_fieldlist_t *fld, *tmp_fld;
	int offset;
	
	fld = malloc(sizeof(lnf_fieldlist_t));

	if (fld == NULL) {
		return LNF_ERR_NOMEM;
	}

	fld->field = field;
	switch (LNF_GET_TYPE(field)) { 
		case LNF_UINT8: fld->size = sizeof(uint8_t); break;
		case LNF_UINT16: fld->size = sizeof(uint16_t); break;
		case LNF_UINT32: fld->size = sizeof(uint32_t); break;
		case LNF_UINT64: fld->size = sizeof(uint64_t); break;
		case LNF_ADDR: fld->size = sizeof(lnf_ip_t); break;
		case LNF_MAC: fld->size = sizeof(lnf_mac_t); break;
		delafult : 
			free(fld);
			return LNF_ERR_UKNFLD;
	}
	fld->numbits = numbits;
	fld->numbits6 = numbits6;
	fld->next = NULL;

		
	if (lnf_mem->aggr_list == NULL) {
		lnf_mem->aggr_list = fld;	
	} else {
		tmp_fld = lnf_mem->aggr_list;
		offset = tmp_fld->size;
		while (tmp_fld->next != NULL) {
			tmp_fld = tmp_fld->next;
			offset += tmp_fld->size;
		}
		fld->offset = offset;
		tmp_fld->next = fld;
	}
}

int lnf_mem_add_sort(lnf_mem_t *lnf_mem, int field, int flags) {

}

/* store record in memory heap */
int lnf_mem_write(lnf_mem_t *lnf_mem, lnf_rec_t *rec) {

	lnf_fieldlist_t *fld = lnf_mem->aggr_list;

	while (fld != NULL) {

		printf(" %x : size %d, offset %d, masklen4 %d, masklen6 %d, sortorder: %d\n",
			fld->field, fld->size, fld->offset, fld->numbits, fld->numbits6, fld->sortorder);
		fld = fld->next;
	}
}


void lnf_mem_free(lnf_mem_t *lnf_mem) {
	free(lnf_mem);
}

