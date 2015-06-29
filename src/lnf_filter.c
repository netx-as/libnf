
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
#include "lnf_filter_gram.h"

pthread_mutex_t lnf_nfdump_filter_match_mutex;    /* mutex for operations match filter  */

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
		case LNF_UINT64:
				*vsize = sizeof(uint64_t);
				tmp = &tmp64;
				break;
		case LNF_UINT32:
				*vsize = sizeof(uint32_t);
				tmp32 = tmp64;
				tmp = &tmp32;
				break;
		case LNF_UINT16:
				*vsize = sizeof(uint16_t);
				tmp16 = tmp64;
				tmp = &tmp16;
				break;
		case LNF_UINT8:
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

/* add leaf entry into expr tree */
lnf_filter_node_t* lnf_filter_new_leaf(yyscan_t scanner, char *fieldstr, lnf_oper_t oper, char *valstr) {
	int field;
	lnf_filter_node_t *node;

	/* fieldstr is set - trie to find field id and relevant _fget function */
	if ( fieldstr != NULL ) {
		field = lnf_fld_parse(fieldstr, NULL, NULL); 
		if (field == LNF_FLD_ZERO_) {
			lnf_seterror("Unknown or unsupported field %s", fieldstr); 
			return NULL;
		}
	}

	node = malloc(sizeof(lnf_filter_node_t));

	if (node == NULL) {
		return NULL;
	}

	node->type = lnf_fld_type(field);
	node->field = field;
	node->oper = oper;

	/* determine field type and assign data to lvalue */
	switch (lnf_fld_type(field)) {

		case LNF_UINT64:
		case LNF_UINT32:
		case LNF_UINT16:
		case LNF_UINT8:
				if (str_to_uint(valstr, lnf_fld_type(field), &node->value, &node->vsize) == 0) {
					lnf_seterror("Can't convert '%s' into numeric value", valstr);
					return NULL;
				}
				break;
		case LNF_ADDR:
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
lnf_filter_node_t* lnf_filter_new_node(yyscan_t scanner, lnf_filter_node_t* left, lnf_oper_t oper, lnf_filter_node_t* right) {

	lnf_filter_node_t *node;

	node = malloc(sizeof(lnf_filter_node_t));

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
int lnf_filter_eval(lnf_filter_node_t *node, lnf_rec_t *rec) {
	int buf[LNF_MAX_STRING];
	int left, right, res;

	if (node == NULL) {
		return -1;
	}

	left = 0;

	/* go deeper into tree */
	if (node->left != NULL ) { 
		left = lnf_filter_eval(node->left, rec); 

		/* do not evaluate if the result is obvious */
		if (node->oper == LNF_OP_NOT)              { return !left; };
		if (node->oper == LNF_OP_OR  && left == 1) { return 1; };
		if (node->oper == LNF_OP_AND && left == 0) { return 0; };
	}

	if (node->right != NULL ) { 
		right = lnf_filter_eval(node->right, rec); 

		switch (node->oper) {
			case LNF_OP_NOT: return !right; break;
			case LNF_OP_OR:  return left || right; break;
			case LNF_OP_AND: return left && right; break;
			default: break;
		}
	}

	/* operations on leaf -> compare values  */
	lnf_rec_fget(rec, node->field, &buf);

	switch (node->type) {
		case LNF_UINT64: res = *(uint64_t *)&buf - *(uint64_t *)node->value; break;
		case LNF_UINT32: res = *(uint32_t *)&buf - *(uint32_t *)node->value; break;
		case LNF_UINT16: res = *(uint16_t *)&buf - *(uint16_t *)node->value; break; 
		case LNF_UINT8:  res = *(uint8_t *)&buf - *(uint8_t *)node->value; break;
		case LNF_DOUBLE: res = *(double *)&buf - *(double *)node->value; break;
		case STRING: res = strcmp((char *)&buf, node->value); break;
		default: res = memcmp(buf, node->value, node->vsize); break;
	}

	/* simple comparsion */
	switch (node->oper) {
		case LNF_OP_NOT: 
		case LNF_OP_OR:  
		case LNF_OP_AND: return -1 ; break; 
		case LNF_OP_EQ:  return res == 0; break;
		case LNF_OP_NE:  return res != 0; break;
		case LNF_OP_GT:  return res > 0; break;
		case LNF_OP_LT:  return res < 0; break;
	}

	return -1;
}


/* initialise filter */
int lnf_filter_init_v2(lnf_filter_t **filterp, char *expr) {

    lnf_filter_t *filter;
	yyscan_t scanner;
	YY_BUFFER_STATE buf;
	int parseret;

    filter = malloc(sizeof(lnf_filter_t));

    if (filter == NULL) {
        return LNF_ERR_NOMEM;
    }

	filter->v2filter = 1;	/* nitialised as V2 - lnf pure filter */

	filter->root = NULL;

	v2_lex_init(&scanner);
    buf = v2__scan_string(expr, scanner);
    parseret = v2_parse(scanner, filter);

    if (buf != NULL) {
        v2__delete_buffer(buf, scanner);
    }

	v2_lex_destroy(scanner);

	/* error in parsing */
	if (parseret != 0) {
		free(filter);
		return LNF_ERR_OTHER_MSG;
	}

	*filterp = filter;

    return LNF_OK;
}

/* matches the record agains filter */
/* returns 1 - record was matched, 0 - record wasn't matched */
int lnf_filter_match(lnf_filter_t *filter, lnf_rec_t *rec) {

    /* call proper version of match function depends on initialised filter version */
    if (filter->v2filter) {
		return lnf_filter_eval(filter->root, rec);
    } else {
		int res;
#ifdef LNF_THREADS
		 pthread_mutex_lock(&lnf_nfdump_filter_match_mutex);
#endif
    	filter->engine->nfrecord = (uint64_t *)rec->master_record;
        res =  (*filter->engine->FilterEngine)(filter->engine);
#ifdef LNF_THREADS
		pthread_mutex_unlock(&lnf_nfdump_filter_match_mutex);
#endif
		return res;
    }
}

/* release all resources allocated by filter */
void lnf_filter_free(lnf_filter_t *filter) {

	if (filter == NULL) {
		return;
	}

	/* cleanup V2 filter */


	free(filter);
}

