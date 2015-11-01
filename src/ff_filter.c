
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
#include <arpa/inet.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "nffile.h"
#include "nfx.h"
#include "nfnet.h"
#include "bookkeeper.h"
#include "nfxstat.h"
#include "nf_common.h"
#include "rbtree.h"
#include "nftree.h"
#include "nfprof.h"
#include "nfdump.h"
#include "nflowcache.h"
#include "nfstat.h"
#include "nfexport.h"
#include "ipconv.h"
#include "flist.h"
#include "util.h"

#include "lnf_filter.h"
#include "libnf_internal.h"
#include "libnf.h" 
#include "ff_filter.h" 
#include "ff_filter_internal.h" 
#include "lnf_filter_gram.h"


/* convert string into uint64_t */
/* FIXME: also converst string with units (64k -> 64000) */
int str_to_uint(char *str, int type, char **res, int *vsize) {

	uint64_t tmp64;
	uint32_t tmp32;
	uint32_t tmp16;
	uint32_t tmp8;
	void *tmp, *ptr;

	tmp64 = atol(str);

	switch (type) {
		case FF_TYPE_UINT64:
				*vsize = sizeof(uint64_t);
				tmp = &tmp64;
				break;
		case FF_TYPE_UINT32:
				*vsize = sizeof(uint32_t);
				tmp32 = tmp64;
				tmp = &tmp32;
				break;
		case FF_TYPE_UINT16:
				*vsize = sizeof(uint16_t);
				tmp16 = tmp64;
				tmp = &tmp16;
				break;
		case FF_TYPE_UINT8:
				*vsize = sizeof(uint16_t);
				tmp8 = tmp64;
				tmp = &tmp8;
				break;
		default: return 0;
	}

	ptr = malloc(*vsize);

	if (ptr == NULL) {
		return 0;
	}

	memcpy(ptr, tmp, *vsize);

	*res = ptr;

	return 1;	
}

/* convert string into lnf_ip_t */
/* FIXME: also converst string with units (64k -> 64000) */
int str_to_addr(char *str, char **res, int *numbits) {

	lnf_ip_t *ptr;

	ptr = malloc(sizeof(lnf_ip_t));

	memset(ptr, 0x0, sizeof(lnf_ip_t));

	if (ptr == NULL) {
		return 0;
	}
	
	*numbits = 0;

	*res = (char *)ptr;

	if (inet_pton(AF_INET, str, &((*ptr).data[3]))) {
		return 1;
	}

	if (inet_pton(AF_INET6, str, ptr)) {
		return 1;
	}

	lnf_seterror("Can't convert '%s' into IP address", str);

	return 0;
}

/* set error to error buffer */
/* set error string */
void ff_filter_seterr(ff_filter_t *filter, char *format, ...) {
va_list args;

	va_start(args, format);
	vsnprintf(filter->error_str, FF_MAX_STRING - 1, format, args);
	va_end(args);
}

/* get error string */
void ff_filter_error(ff_filter_t *filter, const char *buf, int buflen) {

	strncpy((char *)buf, filter->error_str, buflen - 1);

}



/* add leaf entry into expr tree */
ff_filter_node_t* ff_filter_new_leaf(yyscan_t scanner, ff_filter_t *filter,char *fieldstr, ff_oper_t oper, char *valstr) {
	int field;
	ff_filter_node_t *node;
	ff_lvalue_t lvalue;


	/* callback to fetch field type and additional info */
	if (filter->ff_lookup_func == NULL) {
		ff_filter_seterr(filter, "Filter lookup function not defined for %s", fieldstr);
		return NULL;
	}

	memset(&lvalue, 0x0, sizeof(ff_lvalue_t));
	if (filter->ff_lookup_func(filter, fieldstr, &lvalue) != FF_OK) {
		ff_filter_seterr(filter, "Can't lookup field type for %s", fieldstr);
		return NULL;
	}

	/* fieldstr is set - trie to find field id and relevant _fget function */
	//if ( fieldstr != NULL ) {
	//	field = lnf_fld_parse(fieldstr, NULL, NULL); 
	//	if (field == LNF_FLD_ZERO_) {
	//		lnf_seterror("Unknown or unsupported field %s", fieldstr); 
	//		return NULL;
	//	}
	//}

	node = malloc(sizeof(ff_filter_node_t));

	if (node == NULL) {
		return NULL;
	}

	//node->type = lnf_fld_type(field);
	node->type = lvalue.type;;
	//node->field = field;
	node->field = lvalue.id;
	node->oper = oper;

	/* determine field type and assign data to lvalue */
	switch (node->type) {
	//switch (lnf_fld_type(field)) {

		case FF_TYPE_UINT64:
		case FF_TYPE_UINT32:
		case FF_TYPE_UINT16:
		case FF_TYPE_UINT8:
//		case FF_TYPE_UNSIGNED:
				if (str_to_uint(valstr, node->type, &node->value, &node->vsize) == 0) {
					ff_filter_seterr(filter, "Can't convert '%s' into numeric value", valstr);
					return NULL;
				}
				break;
		case FF_TYPE_ADDR:
				if (str_to_addr(valstr, &node->value, &node->numbits) == 0) {
					return NULL;
				}
				node->vsize = sizeof(lnf_ip_t);
				break;
	}

	node->left = NULL;
	node->right = NULL;

	return node;
}

/* add node entry into expr tree */
ff_filter_node_t* ff_filter_new_node(yyscan_t scanner, ff_filter_t *filter, ff_filter_node_t* left, ff_oper_t oper, ff_filter_node_t* right) {

	ff_filter_node_t *node;

	node = malloc(sizeof(ff_filter_node_t));

	if (node == NULL) {
		return NULL;
	}

	node->vsize = 0;
	node->type = 0;
	node->oper = oper;

	node->left = left;
	node->right = right;

	return node;
}

/* evaluate node in tree or proces subtree */
/* return 0 - false; 1 - true; -1 - error  */
int ff_filter_eval_node(ff_filter_t *filter, ff_filter_node_t *node, void *rec) {
	int buf[LNF_MAX_STRING];
	int left, right, res;
	size_t size;

	if (node == NULL) {
		return -1;
	}

	left = 0;

	/* go deeper into tree */
	if (node->left != NULL ) { 
		left = ff_filter_eval_node(filter, node->left, rec); 

		/* do not evaluate if the result is obvious */
		if (node->oper == FF_OP_NOT)              { return !left; };
		if (node->oper == FF_OP_OR  && left == 1) { return 1; };
		if (node->oper == FF_OP_AND && left == 0) { return 0; };
	}

	if (node->right != NULL ) { 
		right = ff_filter_eval_node(filter, node->right, rec); 

		switch (node->oper) {
			case FF_OP_NOT: return !right; break;
			case FF_OP_OR:  return left || right; break;
			case FF_OP_AND: return left && right; break;
			default: break;
		}
	}

	/* operations on leaf -> compare values  */
	/* going to be callback */
	if (filter->ff_data_func(filter, rec, node->field, &buf, &size) != FF_OK) {
		ff_filter_seterr(filter, "Can't get data");
		return -1;
	}

//	lnf_rec_fget((lnf_rec_t *)rec, node->field, &buf);

	switch (node->type) {
		case FF_TYPE_UINT64: res = *(uint64_t *)&buf - *(uint64_t *)node->value; break;
		case FF_TYPE_UINT32: res = *(uint32_t *)&buf - *(uint32_t *)node->value; break;
		case FF_TYPE_UINT16: res = *(uint16_t *)&buf - *(uint16_t *)node->value; break; 
		case FF_TYPE_UINT8:  res = *(uint8_t *)&buf - *(uint8_t *)node->value; break;
		case FF_TYPE_FLOAT: res = *(double *)&buf - *(double *)node->value; break;
		case FF_TYPE_STRING: res = strcmp((char *)&buf, node->value); break;
		default: res = memcmp(buf, node->value, node->vsize); break;
	}

	/* simple comparsion */
	switch (node->oper) {
		case FF_OP_NOT: 
		case FF_OP_OR:  
		case FF_OP_AND: return -1 ; break; 
		case FF_OP_EQ:  return res == 0; break;
		case FF_OP_NE:  return res != 0; break;
		case FF_OP_GT:  return res > 0; break;
		case FF_OP_LT:  return res < 0; break;
	}

	return -1;
}


/* initialise filter */
ff_error_t ff_filter_init(ff_filter_t *filter) {

	memset(filter, 0x0, sizeof(ff_filter_t));
	
	return FF_OK;

}

 //   filter = malloc(sizeof(lnf_filter_t));
ff_error_t ff_filter_parse(ff_filter_t *filter, const char *expr) {

//    lnf_filter_t *filter;
	yyscan_t scanner;
	YY_BUFFER_STATE buf;
	int parse_ret;
	char errbuf[FF_MAX_STRING] = "\0";

 //   filter = malloc(sizeof(lnf_filter_t));

 //  if (filter == NULL) {
//        return LNF_ERR_NOMEM;
 //   }

//	filter->v2filter = 1;	/* nitialised as V2 - lnf pure filter */

	filter->root = NULL;

	v2_lex_init(&scanner);
    buf = v2__scan_string(expr, scanner);
    parse_ret = v2_parse(scanner, filter);

//   if (buf != NULL) {
//        v2__delete_buffer(buf, scanner);
//    }

	v2_lex_destroy(scanner);

	/* error in parsing */
	if (parse_ret != 0) {
//		free(filter);
		ff_filter_error(filter, errbuf, FF_MAX_STRING);
		lnf_seterror("%s", errbuf);
		return FF_ERR_OTHER_MSG;
	}

//	*filterp = filter;

    return FF_OK;
}

/* matches the record agains filter */
/* returns 1 - record was matched, 0 - record wasn't matched */
int ff_filter_eval(ff_filter_t *filter, void *rec) {

	/* call eval node on root node */
	return ff_filter_eval_node(filter, filter->root, rec);

}

/* release all resources allocated by filter */
ff_error_t ff_filter_free(ff_filter_t *filter) {

	/* !!! memory clenaup */
	return FF_OK;

}

