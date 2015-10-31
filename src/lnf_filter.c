
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
//#include "lnf_filter_gram.h"

pthread_mutex_t lnf_nfdump_filter_match_mutex;    /* mutex for operations match filter  */


/* initialise filter */
int lnf_filter_init_v2(lnf_filter_t **filterp, char *expr) {

    lnf_filter_t *filter;
	int ff_ret;

    filter = malloc(sizeof(lnf_filter_t));

    if (filter == NULL) {
        return LNF_ERR_NOMEM;
    }

	filter->v2filter = 1;	/* nitialised as V2 - lnf pure filter */

	/* call ff_filter code */
	memset(&filter->ff_filter, 0x0, sizeof(ff_filter_t));

	ff_ret = ff_filter_parse(&filter->ff_filter, expr);

	/* error in parsing */
	if (ff_ret == FF_ERR_OTHER_MSG) {
		ff_filter_free(&filter->ff_filter);
		free(filter);
		/* handle error message */
		return LNF_ERR_OTHER_MSG;
	} else if (ff_ret != FF_OK) {
		ff_filter_free(&filter->ff_filter);
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
		return ff_filter_eval(&filter->ff_filter, (void *)rec);
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
		ff_filter_free(&filter->ff_filter);
	}

	free(filter);
}

