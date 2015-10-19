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

#define _HAVE_LIBNF_STRUCT_H_ 1

/*
#include <stdio.h>
#include <stdlib.h>
#include <nffile.h>
#include <rbtree.h>
#include <nftree.h>
*/

#include <nfx.h>


typedef int (*lnf_fld_func_t)(lnf_rec_t *rec, void *p);

typedef const struct lnf_field_def {
    int type;
    int size;
    int default_aggr;
    int default_sort;
#define MAX_CALC_DEPS 4					/* max items for calc dependency */
	int calc_dependency[MAX_CALC_DEPS];		/* list of fields that the item is dependend on */
	int pair_field[2];
    char *name;
    char *fld_descr;
	char *ipfix_name;				/* IPFIX element name - http://www.iana.org/assignments/ipfix/ipfix.xhtml */
	int ipfix_enterprise;			/* IPFIX enterprise ID */
	int ipfix_id;					/* IPFIX element ID */
	char *ipfix_name6;				/* elements for IPv6 items (IP address type */
	int ipfix_enterprise6;	
	int ipfix_id6;					
    lnf_fld_func_t fget_func;
	lnf_fld_func_t fset_func;
} lnf_field_def_t;
//
//  /* text description of the fields */
extern lnf_field_def_t lnf_fields_def[];

//extern lnf_fget_fld_func_t lnf_rec_fget_jmptbl[];

static int inline __lnf_rec_fget(lnf_rec_t *rec, int field, void * p) {

    return  lnf_fields_def[field].fget_func(rec, p);
//    return  lnf_fields_def[field].fget_func(rec->master_record, p, rec->extensions_arr);
}

static int inline __lnf_rec_fset(lnf_rec_t *rec, int field, void * p) {

	return  lnf_fields_def[field].fset_func(rec, p);
//    return  lnf_fields_def[field].fset_func(rec->master_record, p, rec->extensions_arr);
}

/* return field type - smae as lnf_fld_type but no checks */
static int inline __lnf_fld_type(int field) {

    return  lnf_fields_def[field].type;
}

/* return field ID for field set 1 - first field id, 2 - second field id */
static int inline __lnf_fld_pair(int field, int pairset) {

    return  lnf_fields_def[field].pair_field[pairset - 1];
}

/* return field ID of calc dependency item */
static int inline __lnf_fld_calc_dep(int field, int calcnum) {

    return  calcnum >= MAX_CALC_DEPS ? 0 : lnf_fields_def[field].calc_dependency[calcnum];

}


