
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
#include "nfnet.h"
#include "bookkeeper.h"
//#include "nfxstat.h"
//#include "nf_common.h"
#include "rbtree.h"
#include "nftree.h"
#include "nfprof.h"
#include "nfdump.h"
#include "nfx.h"
#include "nflowcache.h"
#include "nfstat.h"
#include "nfexport.h"
#include "ipconv.h"
#include "flist.h"
#include "util.h"


#include "lnf_filter.h"
#include "libnf_internal.h"
#include "libnf.h"
#include "fields.h"
#include "ffilter.h"
#include "literals.h"
//#include "lnf_filter_gram.h"

pthread_mutex_t lnf_nfdump_filter_match_mutex;    /* mutex for operations match filter  */

static char lnfc_inet[4];
static char lnfc_inet6[4];

/* callback from ffilter to lookup field */
ff_error_t lnf_ff_lookup_func(ff_t *filter, const char *fieldstr, ff_lvalue_t *lvalue) {


	/* fieldstr is set - try to find field id and relevant _fget function */
    if ( fieldstr != NULL ) {

        // Quick support for those few compatibility constants needed
        if (!strcmp(fieldstr, "inet") || !strcmp(fieldstr, "ipv4")) {
            snprintf(lnfc_inet, 4, "%d", AF_INET);
            lvalue->literal = &lnfc_inet[0];
            lvalue->options = FF_OPTS_CONST;
            fieldstr = "inetfamily";

        } else if (!strcmp(fieldstr, "inet6") || !strcmp(fieldstr, "ipv6")) {
            snprintf(lnfc_inet6, 4, "%d", AF_INET6);
            lvalue->literal = &lnfc_inet6[0];
            lvalue->options = FF_OPTS_CONST;
            fieldstr = "inetfamily";
        }

        int x = lvalue->id[0].index = lnf_fld_parse(fieldstr, NULL, NULL);

        if (lvalue->id[0].index == LNF_FLD_ZERO_) {
            return FF_ERR_UNKN;
        }

        if (x == LNF_FLD_TCP_FLAGS) {
            lvalue->options = FF_OPTS_FLAGS;
        }

        if (lnf_fields_def[x].pair_field[0] != LNF_FLD_ZERO_) {
            lvalue->id[0].index = lnf_fields_def[x].pair_field[0];
            lvalue->id[1].index = lnf_fields_def[x].pair_field[1];
        }

        switch (lnf_fld_type(lvalue->id[0].index)) {
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
        case LNF_DOUBLE:
            lvalue->type = FF_TYPE_DOUBLE;
            break;
        case LNF_ADDR:
            lvalue->type = FF_TYPE_ADDR;
            break;
        case LNF_MAC:
            lvalue->type = FF_TYPE_MAC;
            break;
        case LNF_MPLS:
            lvalue->type = FF_TYPE_MPLS;
            break;
        default:
            return FF_ERR_UNSUP;
        }

        return FF_OK;
    }

	return FF_ERR_OTHER;	
}


/* getting data callback */
/** Copy data to buffer, note data are always passed to filter by reference
 * If coping is done no matter what, then copy data to buffer offseted, and as first element
 * save the pointer.
 */
ff_error_t lnf_ff_data_func(ff_t *filter, void *rec, ff_extern_id_t id, char **data, size_t *size) {

    //TODO WARNING!: not guaranteed that copied data will fit into buffer
	switch (lnf_rec_fget((lnf_rec_t *)rec, id.index, *data)) {
		case LNF_OK:
			return FF_OK;
		case LNF_ERR_NAN:
			*size = 0; 		/* only for variable length items */
			break;
	}

 	return FF_ERR_OTHER;
}

ff_error_t lnf_rval_map_func(ff_t *filter, const char *valstr, ff_type_t type, ff_extern_id_t id,
    char *buf, size_t *size)
{
    struct nff_literal_s *dict = NULL;
    char *tcp_ctl_bits = "FSRPAUECNX";
    char *hit = NULL;
    *size = 0;

    id.index;

    if (id.index == LNF_FLD_ZERO_ || valstr == NULL) {
        return FF_ERR_OTHER;
    }

    int x;

    *size = sizeof(ff_uint64_t);
    ff_uint64_t val;

    switch (id.index) {

        /** Protocol */
    case LNF_FLD_PROT:
        dict = nff_get_protocol_map();
        break;

        /** Translate tcpControlFlags */
    case LNF_FLD_TCP_FLAGS:
    case LNF_FLD_TCP_FLAGS_ALIAS:
        if (strlen(valstr) > 9) {
            return FF_ERR_OTHER;
        }

        for (x = val = 0; x < strlen(valstr); x++) {
            if ((hit = strchr(tcp_ctl_bits, valstr[x])) == NULL) {
                return FF_ERR_OTHER;
            }
            val |= 1 << (hit - tcp_ctl_bits);
            /* If X was in string set all flags */
            if (*hit == 'X') {
                val = 1 << (hit - tcp_ctl_bits);
                val--;
            }
        }
        memcpy(buf, &val, sizeof(val));
        return FF_OK;
        break;

        /** Src/dst ports */
    case LNF_FLD_SRCPORT:
    case LNF_FLD_DSTPORT:
        dict = nff_get_port_map();
        break;
    default:
        return FF_ERR_UNSUP;
    }

    // Universal processing for literals
    nff_literal_t *item = NULL;

    for (int x = 0; dict[x].name != NULL; x++) {
        if (!strcasecmp(valstr, dict[x].name)) {
            item = &dict[x];
            break;
        }
    }

    if (item != NULL) {
        memcpy(buf, &item->value, sizeof(item->value));
        *size = sizeof(item->value);
        return FF_OK;
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

	filter->v2filter = 1;	/* initialised as V2 - lnf pure filter */

	/* init ff_filter code */
	if (ff_options_init(&ff_options) != FF_OK)  {
		free(filter);
        return LNF_ERR_NOMEM;
	}

	/* set callback functions */
	ff_options->ff_lookup_func = lnf_ff_lookup_func;
	ff_options->ff_data_func = lnf_ff_data_func;
	ff_options->ff_rval_map_func = lnf_rval_map_func;

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

/* returns pointer to internal ffilter (ff_t) structure */
void *lnf_filter_ffilter_ptr(lnf_filter_t *filter) {

	if (filter->v2filter == 1) {
		return filter->ff_filter;
	} 

	return NULL;

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
	} else {
		if (filter->engine != NULL) {
			if (filter->engine->filter != NULL) {
				free(filter->engine->filter);
			}
			free(filter->engine);
		}
	}
	free(filter);
}

