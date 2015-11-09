
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
#include "ffilter.h" 
//#include "lnf_filter_gram.h"

pthread_mutex_t lnf_nfdump_filter_match_mutex;    /* mutex for operations match filter  */

/* callback from ffilter to lookup field */
ff_error_t lnf_ff_lookup_func(ff_t *filter, const char *fieldstr, ff_lvalue_t *lvalue) {

	/* fieldstr is set - trie to find field id and relevant _fget function */
	if ( fieldstr != NULL ) {

		lvalue->id.index = lnf_fld_parse(fieldstr, NULL, NULL);

		if (lvalue->id.index == LNF_FLD_ZERO_) {
			return FF_ERR_UNKN;
		}

		switch (lnf_fld_type(lvalue->id.index)) {
			case LNF_UINT8: 
					lvalue->type = FF_TYPE_UINT8;
					break;
			case LNF_UINT16: 
					lvalue->type = FF_TYPE_UINT16;
					break;
			case LNF_UINT32: 
					lvalue->type = FF_TYPE_UINT32;
					break;
			case LNF_UINT64: 
					lvalue->type = FF_TYPE_UINT64;
					break;
			case LNF_ADDR: 
					lvalue->type = FF_TYPE_ADDR;
					break;
			case LNF_MAC: 
					lvalue->type = FF_TYPE_MAC;
					break;
			default: 
					return FF_ERR_UNSUP;
		}

		return FF_OK;
	} 

	return FF_ERR_OTHER;	
}


/* getting data callback */
ff_error_t lnf_ff_data_func(ff_t *filter, void *rec, ff_extern_id_t id, char *data, size_t *size) { 

	switch ( lnf_rec_fget((lnf_rec_t *)rec, id.index, data) ) {
		case LNF_OK:
		case LNF_ERR_NAN:
				*size = 0; 		/* only for variable length items */
				return FF_OK;
				break;
	}

	
 	return FF_ERR_OTHER;

}


/* initialise filter */
int lnf_filter_init_v2(lnf_filter_t **filterp, char *expr) {

    lnf_filter_t *filter;
	int ff_ret;
	ff_options_t *ff_options;
	char errbuf[FF_MAX_STRING];

    filter = malloc(sizeof(lnf_filter_t));

    if (filter == NULL) {
        return LNF_ERR_NOMEM;
    }

	filter->v2filter = 1;	/* nitialised as V2 - lnf pure filter */

	/* init ff_filter code */
	if (ff_options_init(&ff_options) != FF_OK)  {
		free(filter);
        return LNF_ERR_NOMEM;
	}

	/* set callback functions */
	ff_options->ff_lookup_func = lnf_ff_lookup_func;
	ff_options->ff_data_func = lnf_ff_data_func;


	ff_ret = ff_init(&filter->ff_filter, expr, ff_options);

	ff_options_free(ff_options);

	/* error in parsing */
	if (ff_ret == FF_ERR_OTHER_MSG) {
		ff_error(filter->ff_filter, errbuf, FF_MAX_STRING);
		lnf_seterror("%s", errbuf);

		ff_free(filter->ff_filter);
		free(filter);
		/* handle error message */
		return LNF_ERR_OTHER_MSG;
	} else if (ff_ret != FF_OK) {
		ff_free(filter->ff_filter);
		free(filter);
		/* handle error message */
		return LNF_ERR_OTHER;
	}

	*filterp = filter;

    return LNF_OK;
}

/* matches the record agains filter */
/* returns 1 - record was matched, 0 - record wasn't matched */
int lnf_filter_match(lnf_filter_t *filter, lnf_rec_t *rec) {

    /* call proper version of match function depends on initialised filter version */
    if (filter->v2filter) {
		return ff_eval(filter->ff_filter, (void *)rec);
		//return lnf_filter_eval(filter->root, rec);
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
	if (filter->v2filter) { 	/* nitialised as V2 - lnf pure filter */
		ff_free(filter->ff_filter);
	}

	free(filter);
}

