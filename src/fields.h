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
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

*/

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
	int pair_field[2];
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

/* return field type - smae as lnf_fld_type but no checks */
static int inline __lnf_fld_type(int field) {

    return  lnf_fields_def[field].type;
}

/* return field ID for field set 1 - first field id, 2 - second field id */
static int inline __lnf_fld_pair(int field, int pairset) {

    return  lnf_fields_def[field].pair_field[pairset - 1];
}

