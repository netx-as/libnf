

#define _HAVE_LIBNF_STRUCT_H_ 1

/*
#include <stdio.h>
#include <stdlib.h>
#include <nffile.h>
#include <rbtree.h>
#include <nftree.h>
*/

#include <nfx.h>


typedef int (*lnf_fld_func_t)(master_record_t *m, void *p, bit_array_t *e);

typedef const struct lnf_field_def {
    int type;
    int default_aggr;
    int default_sort;
    char *name;
    char *fld_descr;
    lnf_fld_func_t fget_func;
	lnf_fld_func_t fset_func;
} lnf_field_def_t;
//
//  /* text description of the fields */
extern lnf_field_def_t lnf_fields_def[];

//extern lnf_fget_fld_func_t lnf_rec_fget_jmptbl[];

static int inline __lnf_rec_fget(lnf_rec_t *rec, int field, void * p) {

    return  lnf_fields_def[field].fget_func(rec->master_record, p, rec->extensions_arr);
}

static int inline __lnf_rec_fset(lnf_rec_t *rec, int field, void * p) {

    return  lnf_fields_def[field].fset_func(rec->master_record, p, rec->extensions_arr);
}

static int inline __lnf_fld_type(int field) {

    return  lnf_fields_def[field].type;
}

