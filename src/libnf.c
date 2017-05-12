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

#define NEED_PACKRECORD 1 
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

#include "libnf_internal.h"
#include "libnf.h"
#include "fields.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#ifdef LNF_THREADS
#include <pthread.h>
#endif

#include "nfdump_inline.c"
#include "nffile_inline.c"

/*! \brief Brief text 
* Some TEXT libnf bla bla 
*/ 

/* Global Variables */
extern extension_descriptor_t extension_descriptor[];
char error_str[LNF_MAX_STRING];
pthread_mutex_t lnf_nfdump_filter_mutex;	/* mutex for operations open/close file, filter init */


#define FLOW_RECORD_NEXT(x) x = (common_record_t *)((pointer_addr_t)x + x->size)

/* text description of the fields */
lnf_field_t lnf_fields[] = {
// pod:  =====================
	{LNF_FLD_FIRST, 		LNF_AGGR_MIN,	LNF_SORT_ASC,	"first",	
	"Timestamp of the first packet seen (in miliseconds)"},
	{LNF_FLD_LAST,			LNF_AGGR_MAX,	LNF_SORT_ASC,	"last",		
	"Timestamp of the last packet seen (in miliseconds)"},
	{LNF_FLD_RECEIVED,		LNF_AGGR_MAX,	LNF_SORT_ASC,	"received",	
	"Timestamp regarding when the packet was received by collector"},
// pod:
// pod:  Statistical items
// pod:  =====================
	{LNF_FLD_DOCTETS,		LNF_AGGR_SUM,	LNF_SORT_DESC,	"bytes",	
	"The number of bytes"},
	{LNF_FLD_DPKTS,			LNF_AGGR_SUM,	LNF_SORT_DESC,	"pkts",		
	"The number of packets"},
	{LNF_FLD_OUT_BYTES,		LNF_AGGR_SUM,	LNF_SORT_DESC,	"outbytes",	
	"The number of output bytes"},
	{LNF_FLD_OUT_PKTS,		LNF_AGGR_SUM,	LNF_SORT_DESC,	"outpkts",	
	"The number of output packets"},
	{LNF_FLD_AGGR_FLOWS,	LNF_AGGR_SUM,	LNF_SORT_DESC,	"flows",	
	"The number of flows (aggregated)"},
// pod:
// pod:  Layer 4 information
// pod:  =====================
	{LNF_FLD_SRCPORT,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"srcport",		
	"Source port"},
	{LNF_FLD_DSTPORT, 		LNF_AGGR_KEY,	LNF_SORT_ASC,	"dstport",		
	"Destination port"},
	{LNF_FLD_TCP_FLAGS,		LNF_AGGR_OR,	LNF_SORT_ASC,	"tcpflags",		
	"TCP flags"},
// pod:
// pod:  Layer 3 information
// pod:  =====================
	{LNF_FLD_SRCADDR,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"srcip",		
	"Source IP address"},
	{LNF_FLD_DSTADDR,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"dstip",		
	"Destination IP address"},
	{LNF_FLD_IP_NEXTHOP,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"nexthop",		
	"IP next hop"},
	{LNF_FLD_SRC_MASK,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"srcmask",		
	"Source mask"}, 
	{LNF_FLD_DST_MASK,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"dstmask",		
	"Destination mask"}, 
	{LNF_FLD_TOS,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"tos",			
	"Source type of service"}, 
	{LNF_FLD_DST_TOS,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"dsttos",		
	"Destination type of service"},
	{LNF_FLD_SRCAS,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"srcas",		
	"Source AS number"},
	{LNF_FLD_DSTAS,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"dstas",		
	"Destination AS number"},
	{LNF_FLD_BGPNEXTADJACENTAS,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"nextas",	
	"BGP Next AS"},
	{LNF_FLD_BGPPREVADJACENTAS,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"prevas",	
	"BGP Previous AS"},
	{LNF_FLD_BGP_NEXTHOP,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"bgpnexthop",	
	"BGP next hop"},
	{LNF_FLD_PROT,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"proto",		
	"IP protocol"}, 
// pod:
// pod:  Layer 2 information
// pod:  =====================
	{LNF_FLD_SRC_VLAN,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"srcvlan",		
	"Source vlan label"},
	{LNF_FLD_DST_VLAN,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"dstvlan",		
	"Destination vlan label"}, 
	{LNF_FLD_IN_SRC_MAC,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"insrcmac",		
	"In source MAC address"},
	{LNF_FLD_OUT_SRC_MAC,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"outsrcmac",	
	"Out destination MAC address"},
	{LNF_FLD_IN_DST_MAC,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"indstmac",		
	"In destination MAC address"}, 
	{LNF_FLD_OUT_DST_MAC,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"outdstmac",	
	"Out source MAC address"}, 
// pod:
// pod:  MPLS information
// pod:  =====================
	{LNF_FLD_MPLS_LABEL,	LNF_AGGR_KEY,	LNF_SORT_NONE,	"mpls",		
	"MPLS labels"},
// pod:
// pod:  Layer 1 information
// pod:  =====================
	{LNF_FLD_INPUT,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"inif",		
	"SNMP input interface number"},
	{LNF_FLD_OUTPUT,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"outif",	
	"SNMP output interface number"},
	{LNF_FLD_DIR,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"dir",		
	"Flow directions ingress/egress"}, 
	{LNF_FLD_FWD_STATUS,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"fwd",		
	"Forwarding status"},
// pod:
// pod:  Exporter information
// pod:  =====================
	{LNF_FLD_IP_ROUTER,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"router",	
	"Exporting router IP"}, 
	{LNF_FLD_ENGINE_TYPE,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"systype",	
	"Type of exporter"},
	{LNF_FLD_ENGINE_ID,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"sysid",	
	"Internal SysID of exporter"},
// pod:
// pod:  NSEL fields, see: http://www.cisco.com/en/US/docs/security/asa/asa81/netflow/netflow.html
// pod:  =====================
	{LNF_FLD_EVENT_TIME,		LNF_AGGR_MIN,	LNF_SORT_ASC,	"eventtime",	
	"NSEL The time that the flow was created"},
	{LNF_FLD_CONN_ID,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"connid",		
	"NSEL An identifier of a unique flow for the device"},
	{LNF_FLD_ICMP_CODE,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"icmpcode",		
	"NSEL ICMP code value"},
	{LNF_FLD_ICMP_TYPE,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"icmptype",		
	"NSEL ICMP type value"},
	{LNF_FLD_FW_XEVENT,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"xevent",		
	"NSEL Extended event code"},
	{LNF_FLD_XLATE_SRC_IP,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"xsrcip",	
	"NSEL Mapped source IPv4 address"},
	{LNF_FLD_XLATE_DST_IP,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"xdstip",	
	"NSEL Mapped destination IPv4 address"},
	{LNF_FLD_XLATE_SRC_PORT,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"xsrcport",	
	"NSEL Mapped source port"},
	{LNF_FLD_XLATE_DST_PORT,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"xdstport",	
	"NSEL Mapped destination port"},
// pod: NSEL The input ACL that permitted or denied the flow
	{LNF_FLD_INGRESS_ACL_ID,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"iacl",		
	"Hash value or ID of the ACL name"},
	{LNF_FLD_INGRESS_ACE_ID,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"iace", 	
	"Hash value or ID of the ACL name"},
	{LNF_FLD_INGRESS_XACE_ID,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"ixace",	
	"Hash value or ID of an extended ACE configuration"},
// pod: NSEL The output ACL that permitted or denied a flow  
	{LNF_FLD_EGRESS_ACL_ID,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"eacl",		
	"Hash value or ID of the ACL name"},
	{LNF_FLD_EGRESS_ACE_ID,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"eace",		
	"Hash value or ID of the ACL name"},
	{LNF_FLD_EGRESS_XACE_ID,	LNF_AGGR_KEY,	LNF_SORT_ASC,	"exace",	
	"Hash value or ID of an extended ACE configuration"},
	{LNF_FLD_USERNAME,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"username",	
	"NSEL username"},
// pod:
// pod:  NEL (NetFlow Event Logging) fields
// pod:  =====================
	{LNF_FLD_INGRESS_VRFID,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"ingressvrfid",		
	"NEL NAT ingress vrf id"},
	{LNF_FLD_EVENT_FLAG,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"eventflag",		
	"NAT event flag (always set to 1 by nfdump)"},
	{LNF_FLD_EGRESS_VRFID,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"egressvrfid",		
	"NAT egress VRF ID"},
	{LNF_FLD_BLOCK_START,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"blockstart",		
	"NAT pool block start"},
// pod:
// pod:  NEL Port Block Allocation (added 2014-04-19)
// pod:  =====================
	{LNF_FLD_BLOCK_END,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"blockend",			
	"NAT pool block end"},
	{LNF_FLD_BLOCK_STEP,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"blockstep",		
	"NAT pool block step"},
	{LNF_FLD_BLOCK_SIZE,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"blocksize",		
	"NAT pool block size"},
// pod:
// pod:  Extra/special fields
// pod:  =====================
	{LNF_FLD_CLIENT_NW_DELAY_USEC,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"cl",	
	"nprobe latency client_nw_delay_usec"},
	{LNF_FLD_SERVER_NW_DELAY_USEC,		LNF_AGGR_KEY,	LNF_SORT_ASC,	"sl",	
	"nprobe latency server_nw_delay_usec"},
	{LNF_FLD_APPL_LATENCY_USEC,			LNF_AGGR_KEY,	LNF_SORT_ASC,	"al",	
	"nprobe latency appl_latency_usec"},
	{LNF_FLD_ZERO_,						0,				0,				NULL,	
	NULL}
};


/* open existing nfdump file and prepare for reading records */
/* only simple wrapper to nfdump function */
int lnf_open(lnf_file_t **lnf_filep, const char * filename, unsigned int flags, const char * ident) {
	int i;
	lnf_file_t *lnf_file;
	int comp;

	lnf_file = malloc(sizeof(lnf_file_t));
	
	if (lnf_file == NULL) {
		return LNF_ERR_NOMEM;
	}


	lnf_file->flags = flags;
	/* open file in either read only or write only mode */
#ifdef LNF_THREADS
   	pthread_mutex_lock(&lnf_nfdump_filter_mutex);
#endif
	if (flags & LNF_APPEND) {

		lnf_file->nffile = AppendFile((char *)filename);

	} else if (flags & LNF_WRITE) {

		if (flags & LNF_COMP_LZO) {
			comp = 1; 
		} else if (flags & LNF_COMP_BZ2) { 
			comp = 2; 
		} else {
			comp = 0;
		}

		lnf_file->nffile = OpenNewFile((char *)filename, NULL, comp, 
								flags & LNF_ANON, (char *)ident);

	} else {
		/* set file name in LOOP mode */
		if (flags & LNF_READ_LOOP) {
			if (filename == NULL) {
				free(lnf_file);
				return LNF_ERR_OTHER;;
			}

			lnf_file->filename = malloc(strlen(filename) + 1);
			if (lnf_file->filename == NULL) {
				free(lnf_file);
				return LNF_ERR_OTHER;;
			}

			strcpy(lnf_file->filename, filename);
		
		}

		lnf_file->nffile = OpenFile((char *)filename, NULL);

	}
#ifdef LNF_THREADS
   	pthread_mutex_unlock(&lnf_nfdump_filter_mutex);
#endif

	if (lnf_file->nffile == NULL) {
		free(lnf_file);
		return LNF_ERR_OTHER;;
	}

	lnf_file->blk_record_remains = 0;
	lnf_file->processed_blocks  = 0;
	lnf_file->processed_bytes  = 0;
	lnf_file->skipped_blocks  = 0;
#ifdef LNF_THREADS
    pthread_mutex_lock(&lnf_nfdump_filter_mutex);
#endif
	lnf_file->extension_map_list = InitExtensionMaps(NEEDS_EXTENSION_LIST);

	lnf_file->v1convert_buffer = NULL;
	lnf_file->lnf_map_list = NULL;
	lnf_file->exporters = NULL;
	lnf_file->samplers = NULL;

	lnf_file->num_exporters = 0; 

	i = 1;
	lnf_file->max_num_extensions = 0;
	while ( extension_descriptor[i++].id )
		lnf_file->max_num_extensions++;

#ifdef LNF_THREADS
    pthread_mutex_unlock(&lnf_nfdump_filter_mutex);
#endif


	*lnf_filep = lnf_file;

	return LNF_OK;
}

/* fill info structure */
int lnf_info(lnf_file_t *lnf_file, int info, void *data, size_t size) {

	file_header_t *h;
	stat_record_t *s;
	size_t reqsize = 0;
	char buf[LNF_INFO_BUFSIZE];

	/* for NULL lnf_mem */
	switch (info) {
		case LNF_INFO_VERSION: 
			strcpy(buf, VERSION);
			reqsize = strlen(VERSION) + 1;
			break;
		case LNF_INFO_NFDUMP_VERSION: 
			strcpy(buf, NFDUMP_VERSION);
			reqsize = strlen(NFDUMP_VERSION) + 1;
			break;
	}

	/* the requested item was one of the above */
	if ( reqsize != 0 ) {
		if ( reqsize <= size ) {
			memcpy(data, buf, reqsize);
			return LNF_OK;
		} else {
			 return LNF_ERR_NOMEM;
		}
	} 

	/* file specific request */
	if (lnf_file == NULL || lnf_file->nffile == NULL || lnf_file->nffile->file_header == NULL) {
		return LNF_ERR_OTHER;	
	}

	h = lnf_file->nffile->file_header;

	switch (info) {
		case LNF_INFO_FILE_VERSION:
			*((uint64_t *)buf) = h->version;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_BLOCKS:
			*((uint64_t *)buf) = h->NumBlocks;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_COMPRESSED:
			*((int *)buf) = FILE_IS_LZO_COMPRESSED(lnf_file->nffile) || FILE_IS_BZ2_COMPRESSED(lnf_file->nffile) ;
			reqsize = sizeof(int);
			break;
		case LNF_INFO_LZO_COMPRESSED:
			*((int *)buf) = FILE_IS_LZO_COMPRESSED(lnf_file->nffile);
			reqsize = sizeof(int);
			break;
		case LNF_INFO_BZ2_COMPRESSED:
			*((int *)buf) = FILE_IS_BZ2_COMPRESSED(lnf_file->nffile);
			reqsize = sizeof(int);
			break;
		case LNF_INFO_ANONYMIZED:
			*((int *)buf) = h->flags & FLAG_ANONYMIZED;
			reqsize = sizeof(int);
			break;
		case LNF_INFO_CATALOG: 
			*((int *)buf) = h->flags & FLAG_CATALOG; 
			reqsize = sizeof(int);
			break;
		case LNF_INFO_IDENT: 
			strcpy(buf, h->ident);
			reqsize = strlen(h->ident) + 1;
			break;
		case LNF_INFO_PROC_BLOCKS: 
			*((uint64_t *)buf) = lnf_file->processed_blocks;
			reqsize = sizeof(uint64_t);
			break;
	}

	/* the requested item was one of the above */
	if ( reqsize != 0 ) {
		if ( reqsize <= size ) {
			memcpy(data, buf, reqsize);
			return LNF_OK;
		} else {
			 return LNF_ERR_NOMEM;
		}
	} 

	/* get stat record */
	if (lnf_file->nffile->stat_record == NULL) {
		return LNF_ERR_OTHER;	
	}

	s = lnf_file->nffile->stat_record;

	switch (info) {
		case LNF_INFO_FIRST:
			*((uint64_t *)buf) = s->first_seen * 1000LL + s->msec_first;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_LAST:
			*((uint64_t *)buf) = s->last_seen * 1000LL + s->msec_last;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_FAILURES:
			*((uint64_t *)buf) = s->sequence_failure;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_FLOWS:
			*((uint64_t *)buf) = s->numflows;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_BYTES:
			*((uint64_t *)buf) = s->numbytes;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_PACKETS:
			*((uint64_t *)buf) = s->numpackets;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_FLOWS_TCP:
			*((uint64_t *)buf) = s->numflows_tcp;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_FLOWS_UDP:
			*((uint64_t *)buf) = s->numflows_udp;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_FLOWS_ICMP:
			*((uint64_t *)buf) = s->numflows_icmp;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_FLOWS_OTHER:
			*((uint64_t *)buf) = s->numflows_other;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_BYTES_TCP:
			*((uint64_t *)buf) = s->numbytes_tcp;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_BYTES_UDP:
			*((uint64_t *)buf) = s->numbytes_udp;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_BYTES_ICMP:
			*((uint64_t *)buf) = s->numbytes_icmp;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_BYTES_OTHER:
			*((uint64_t *)buf) = s->numbytes_other;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_PACKETS_TCP:
			*((uint64_t *)buf) = s->numpackets_tcp;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_PACKETS_UDP:
			*((uint64_t *)buf) = s->numpackets_udp;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_PACKETS_ICMP:
			*((uint64_t *)buf) = s->numpackets_icmp;
			reqsize = sizeof(uint64_t);
			break;
		case LNF_INFO_PACKETS_OTHER:
			*((uint64_t *)buf) = s->numpackets_other;
			reqsize = sizeof(uint64_t);
			break;
	}

	if ( reqsize != 0 ) {
		if ( reqsize <= size ) {
			memcpy(data, buf, reqsize);
			return LNF_OK;
		} else {
			 return LNF_ERR_NOMEM;
		}
	} else {
		return LNF_ERR_OTHER;
	}
}


/* XXX should be redesigned */
void lnf_update_exporter_stats(lnf_file_t *lnf_file, nffile_t *nffile) {

	generic_exporter_t *exporter; 
	exporter_stats_record_t *estats;
	size_t size;
	int i = 0;

	size = sizeof(exporter_stats_record_t) + (lnf_file->num_exporters - 1 ) * sizeof(struct exporter_stat_s);
	estats = malloc(size);

	if (estats == NULL) {
		return; 
	}

	memset(estats, 0x0, size); 

	/* prepare header */
	estats->header.type = ExporterStatRecordType;
	estats->header.size = size;
	estats->stat_count = lnf_file->num_exporters;

	/* fill statistics */
	exporter = lnf_file->exporters; 
	while (exporter != NULL) {
	
		estats->stat[i].sysid = exporter->info.sysid; 	
		estats->stat[i].sequence_failure = exporter->sequence_failure; 	
		estats->stat[i].packets = exporter->packets; 	
		estats->stat[i].flows = exporter->flows; 	
		i++;

		exporter = exporter->next;
	}

	AppendToBuffer(lnf_file->nffile, (void *)&estats, estats->header.size);

	free(estats);

}

/* close file handler and release related structures */
void lnf_close(lnf_file_t *lnf_file) {

	lnf_map_list_t *map_list, *tmp_map_list;
	generic_exporter_t *exporter; 
	generic_sampler_t *sampler;
	void *tmp; 

	if (lnf_file == NULL || lnf_file->nffile == NULL) {
		return ;
	}

	if (lnf_file->flags & LNF_WRITE) {

		/* append exporter statistics into file */
		lnf_update_exporter_stats(lnf_file, lnf_file->nffile);

		// write the last records in buffer
		if (lnf_file->nffile->block_header->NumRecords ) {
			if ( WriteBlock(lnf_file->nffile) <= 0 ) {
				fprintf(stderr, "Failed to write output buffer: '%s'" , strerror(errno));
			}
		}
		CloseUpdateFile(lnf_file->nffile, NULL );

	} else {
		CloseFile(lnf_file->nffile);
	}

	DisposeFile(lnf_file->nffile); 

	PackExtensionMapList(lnf_file->extension_map_list);
	FreeExtensionMaps(lnf_file->extension_map_list);

	map_list = lnf_file->lnf_map_list; 
	while (map_list != NULL) {
		tmp_map_list = map_list;
		map_list = map_list->next;
		bit_array_release(&tmp_map_list->bit_array);
		if (tmp_map_list->map != NULL) {
			free(tmp_map_list->map);
		}
		free(tmp_map_list);
	}

	if (lnf_file->v1convert_buffer != NULL) {
		free(lnf_file->v1convert_buffer);
	}

	/* free exporter list */
	exporter = lnf_file->exporters;
	while (exporter != NULL) {
		tmp = exporter;
		exporter = exporter->next;	
		free(tmp);
	}

	/* free sampler list */
	sampler = lnf_file->samplers;
	while (sampler != NULL) {
		tmp = sampler;
		sampler = sampler->next;	
		free(tmp);
	}

	free(lnf_file);
}

/* return next record in file */
/* status of read and fill pre-prepared structure lnf_rec */
int lnf_read_record(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec) {

//master_record_t	*master_record;
int ret;
uint32_t map_id;
extension_map_t *map;
int i;
common_record_t *common_record_ptr;

#ifdef COMPAT15
int	v1_map_done = 0;
#endif

begin:

	if (lnf_file->blk_record_remains == 0) {
	/* all records in block have been processed, we are going to load nex block */

		// get next data block from file
		if (lnf_file->nffile) {
			ret = ReadBlock(lnf_file->nffile);
			lnf_file->processed_blocks++;
		} else {	
			ret = NF_EOF;		/* the firt file in the list */
		}

		switch (ret) {
			case NF_CORRUPT:
				return LNF_ERR_CORRUPT;
			case NF_ERROR:
				return LNF_ERR_READ;
			case NF_EOF: 
				return LNF_EOF;
			default:
				// successfully read block
				lnf_file->processed_bytes += ret;
		}

		/* block types to be skipped  -> goto begin */
		/* block types that are unknown -> return */
		switch (lnf_file->nffile->block_header->id) {
			case DATA_BLOCK_TYPE_1:		/* old record type - nfdump 1.5 */
					lnf_file->skipped_blocks++;
					goto begin;
					return LNF_ERR_COMPAT15;
					break;
			case DATA_BLOCK_TYPE_2:		/* common record type - normally processed */
					break;
			case Large_BLOCK_Type:
					lnf_file->skipped_blocks++;
					goto begin;
					break;
			default: 
					lnf_file->skipped_blocks++;
					return LNF_ERR_UNKBLOCK;
		}

		lnf_file->flow_record = lnf_file->nffile->buff_ptr;
		lnf_file->blk_record_remains = lnf_file->nffile->block_header->NumRecords;
	} /* reading block */

	/* there are some records to process - we are going continue reading next record */
	lnf_file->blk_record_remains--;

	switch (lnf_file->flow_record->type) {
		case ExporterRecordType:
		case SamplerRecordype:
		case ExporterInfoRecordType:
		case ExporterStatRecordType:
		case SamplerInfoRecordype:
				/* just skip */
				FLOW_RECORD_NEXT(lnf_file->flow_record);	
				goto begin;
				break;
		case ExtensionMapType: 
				map = (extension_map_t *)lnf_file->flow_record;
				//Insert_Extension_Map(&instance->extension_map_list, map);
				Insert_Extension_Map(lnf_file->extension_map_list, map);
				FLOW_RECORD_NEXT(lnf_file->flow_record);	
				goto begin;
				break;
			
		case CommonRecordV0Type:

				// convert common record v0
				if ( lnf_file->v1convert_buffer == NULL) {
					lnf_file->v1convert_buffer = malloc(65536); /* very suspisous part of code taken from nfdump */
					if ( lnf_file->v1convert_buffer == NULL ) {
						return LNF_ERR_NOMEM;
					}
				}
				ConvertCommonV0((void *)lnf_file->flow_record, (common_record_t *)lnf_file->v1convert_buffer);
				common_record_ptr = (common_record_t *)lnf_file->v1convert_buffer;

				break;

		case CommonRecordType:
				/* data record type - go ahead */
				common_record_ptr = lnf_file->flow_record;
				break;

		default:
				FLOW_RECORD_NEXT(lnf_file->flow_record);	
				return LNF_ERR_UNKREC;

	}

	/* we are sure that record is CommonRecordType */
	map_id = lnf_file->flow_record->ext_map;
	if ( map_id >= MAX_EXTENSION_MAPS ) {
		FLOW_RECORD_NEXT(lnf_file->flow_record);	
		return LNF_ERR_EXTMAPB;
	}
	if ( lnf_file->extension_map_list->slot[map_id] == NULL ) {
		FLOW_RECORD_NEXT(lnf_file->flow_record);	
		return LNF_ERR_EXTMAPM;
	} 


	// changed in 1.6.8 - added exporter info 
//	ExpandRecord_v2( flow_record, extension_map_list.slot[map_id], master_record);
	ExpandRecord_v2(common_record_ptr, lnf_file->extension_map_list->slot[map_id], NULL, lnf_rec->master_record);

	// update number of flows matching a given map
	lnf_file->extension_map_list->slot[map_id]->ref_count++;

	// Move pointer by number of bytes for netflow record
	FLOW_RECORD_NEXT(lnf_file->flow_record);	
/*
	{
		char *s;
		PrintExtensionMap(instance->extension_map_list.slot[map_id]->map);
		format_file_block_record(master_record, &s, 0);
		printf("READ: %s\n", s);
	}
*/

	// processing map 
	//bit_array_clear(&lnf_file->extensions_arr);
	bit_array_clear(lnf_rec->extensions_arr);

	i = 0;
	while (lnf_rec->master_record->map_ref->ex_id[i]) {
		__bit_array_set(lnf_rec->extensions_arr, lnf_rec->master_record->map_ref->ex_id[i], 1);
		i++;
	}

//	lnf_rec->extensions_arr = &(lnf_file->extensions_arr);

	/* the record seems OK. We prepare hash reference with items */
//	lnf_file->lnf_rec = lnf_rec; /* XXX temporary */

	return LNF_OK;

} /* end of _readfnction */

/* return next record in file */
int lnf_read(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec) {

	int ret;
	
	if (lnf_file->flags & LNF_READ_LOOP) {
NEW_READ:
		ret = lnf_read_record(lnf_file, lnf_rec);

		/* we are at the end of the file, we will wait for new records or inode change */
		if (ret == LNF_EOF) {
			struct stat stat_buf;
			/* wait for while */
			sleep(1);

			/* check inode change -> indicastes new file */
			if (stat(lnf_file->filename, &stat_buf) != 0) {
				return LNF_EOF;
			}

			/* inode change - close odl file and open a new one */
			if (lnf_file->inode != 0 && lnf_file->inode != stat_buf.st_ino) {

				if (lnf_file->nffile != NULL) {
					CloseFile(lnf_file->nffile);
				}

				lnf_file->nffile = OpenFile((char *)lnf_file->filename, NULL);

				if (lnf_file->nffile == NULL) {
					return LNF_ERR_OTHER;
				}
			}

			lnf_file->inode = stat_buf.st_ino;

			/* sleep for while and try to read again */
			goto NEW_READ;

		} else {
			return ret;
		}

	} else {
		/* normal read -> map function to lnf_read_file */
		return lnf_read_record(lnf_file, lnf_rec);
	}
}

extension_map_t * lnf_lookup_map(lnf_file_t *lnf_file, bit_array_t *ext ) {
extension_map_t *map; 
lnf_map_list_t *map_list;
int i = 0;
int is_set = 0;
int id = 0;
int map_id = 0;

	// find whether the template already exist 
	map_id = 0;

	map_list = lnf_file->lnf_map_list; 
	if (map_list == NULL) {
		// first map 
		map_list =  malloc(sizeof(lnf_map_list_t));
		if (map_list == NULL) {
			return NULL;
		}
		lnf_file->lnf_map_list = map_list;
	} else {
		if (bit_array_cmp(&(map_list->bit_array), ext) == 0) {
			return map_list->map;
		}
		map_id++;
		while (map_list->next != NULL ) {
			if (bit_array_cmp(&(map_list->bit_array), ext) == 0) {
				return map_list->map;
			} else {
				map_id++;
				map_list = map_list->next;
			}
		}
		map_list->next = malloc(sizeof(lnf_map_list_t));
		if (map_list->next == NULL) {
			return NULL;
		}
		map_list = map_list->next;
	}
	
	// allocate memory potentially for all extensions 
	map = malloc(sizeof(extension_map_t) + (lnf_file->max_num_extensions + 1) * sizeof(uint16_t));
	if (map == NULL) {
		free(map_list);
		return NULL;
	}

	map_list->map = map;
	map_list->next = NULL;

	bit_array_init(&map_list->bit_array, lnf_file->max_num_extensions + 1);
	bit_array_copy(&map_list->bit_array, ext);

	map->type   = ExtensionMapType;
	map->map_id = map_id; 
			
	// set extension map according the bits set in ext structure 
	id = 0;
	i = 0;
	while ( (is_set = bit_array_get(ext, id)) != -1 ) {
//		fprintf(stderr, "i: %d, bit %d, val: %d\n", i, id, is_set);
		if (is_set) 
			map->ex_id[i++]  = id;
		id++;
	}
	map->ex_id[i++] = 0;

	// determine size and align 32bits
	map->size = sizeof(extension_map_t) + ( i - 1 ) * sizeof(uint16_t);
	if (( map->size & 0x3 ) != 0 ) {
		map->size += (4 - ( map->size & 0x3 ));
	}

	map->extension_size = 0;
	i=0;
	while (map->ex_id[i]) {
		int id = map->ex_id[i];
		map->extension_size += extension_descriptor[id].size;
		i++;
	}

	//Insert_Extension_Map(&instance->extension_map_list, map); 
	Insert_Extension_Map(lnf_file->extension_map_list, map); 
	AppendToBuffer(lnf_file->nffile, (void *)map, map->size);

	return map;
}

/* lookup exporter ID by exporter ID an eporter IP 
fileds: LNF_FLD_EXPORTER_ID LNF_FLD_EXPORTER_IP 
returns - internal exporter ID (sysid)

Exporters are organizes in linked lst. If the ID and IP is not 
found in the list the new entry is created */
generic_exporter_t* lnf_lookup_exporter(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec) {

	generic_exporter_t *exporter;
	generic_exporter_t *tmp;
	ip_addr_t ip;

	/* no exporter set in the record */
	if ((lnf_rec->flags & LNF_REC_EXPORTER) == 0) {
		return NULL;
	}

	exporter = lnf_file->exporters;

	/* walk via all exporters */
	while (exporter != NULL) {

		if (lnf_rec->exporter->info.id == exporter->info.id && memcmp(&lnf_rec->exporter->info.ip, &exporter->info.ip, sizeof(ip_addr_t)) == 0) {
			return exporter;
		}

		exporter = exporter->next;
	}

	exporter = calloc(sizeof(generic_exporter_t), 1);

	if (exporter == NULL) {
		return NULL;
	}

	/* add new exporter into linked list */
	tmp = lnf_file->exporters;
	lnf_file->exporters = exporter;
	exporter->next = tmp;

	/* copy exporter info data */
	exporter->info.id = lnf_rec->exporter->info.id;
	memcpy(&exporter->info.ip, &lnf_rec->exporter->info.ip, sizeof(ip_addr_t));

	/* assign sysid */
	lnf_file->num_exporters++;
	exporter->info.sysid = lnf_file->num_exporters;

	/* additional exporter_info_record_t fields */
	exporter->info.version = lnf_rec->exporter->info.version; 

	ip.v6[0] = htonll(exporter->info.ip.v6[0]);
	ip.v6[1] = htonll(exporter->info.ip.v6[1]);

	if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)&ip)) {
		exporter->info.sa_family = AF_INET;
	} else {
		exporter->info.sa_family = AF_INET6;
	}

	exporter->info.header.size = sizeof(exporter_info_record_t);
	exporter->info.header.type =  ExporterInfoRecordType;

	AppendToBuffer(lnf_file->nffile, (void *)&exporter->info, exporter->info.header.size);


	return exporter;
}



/* return next record in file */
/* status of read and fill pre-prepared structure lnf_rec */
int lnf_write(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec) {

	extension_map_t *map;
	generic_exporter_t *exporter;

	/* lookup and add map into file if it it is nescessary */
	map = lnf_lookup_map(lnf_file, lnf_rec->extensions_arr);

	if (map == NULL) {
		return LNF_ERR_WRITE;
	}

	lnf_rec->master_record->map_ref = map;
	lnf_rec->master_record->ext_map = map->map_id;
	lnf_rec->master_record->type = CommonRecordType;

	/* lookup and add exporter record into file if it is nescessary */
	exporter = lnf_lookup_exporter(lnf_file, lnf_rec);

	/* assign exporter sysid and update statistics */
	if (exporter != NULL) {
		lnf_rec->master_record->exporter_sysid = exporter->info.sysid;
		exporter->packets += lnf_rec->master_record->dPkts;
		exporter->flows += lnf_rec->master_record->aggr_flows;
		exporter->sequence_failure += lnf_rec->sequence_failures;
	}


	UpdateStat(lnf_file->nffile->stat_record, lnf_rec->master_record);

	PackRecord(lnf_rec->master_record, lnf_file->nffile);


	return LNF_OK;
}

/* initialise empty record */
int lnf_rec_init(lnf_rec_t **recp) {

	lnf_rec_t *rec;
	int i, numext;

	rec = malloc(sizeof(lnf_rec_t)); 

	if (rec == NULL) {
		*recp = NULL;
		return LNF_ERR_NOMEM;
	}

	rec->extensions_arr = NULL;
	rec->field_data = NULL;

	rec->master_record = malloc(sizeof(master_record_t));

	if (rec->master_record == NULL) {
		lnf_rec_free(rec);
		*recp = NULL;
		return LNF_ERR_NOMEM;
	}

	rec->extensions_arr = malloc(sizeof(bit_array_t));

	if (rec->extensions_arr == NULL) {
		lnf_rec_free(rec);
		*recp = NULL;
		return LNF_ERR_NOMEM;
	}


	/* exporter and sampler structure initialisation */
	rec->exporter = malloc(sizeof(generic_exporter_t));
	rec->sampler = malloc(sizeof(generic_sampler_t));

	if (rec->exporter == NULL || rec->sampler == NULL) {
		lnf_rec_free(rec);
		*recp = NULL;
		return LNF_ERR_NOMEM;
	}

	memset(rec->exporter, 0x0, sizeof(generic_exporter_t));
	memset(rec->sampler, 0x0, sizeof(generic_sampler_t));

	rec->exporter->info.version = LNF_DEFAULT_EXPORTER_VERSION;


	/* initialise nfdump extension list */
	i = 1;
	numext = 0;
	while ( extension_descriptor[i++].id ) {
		numext++;
	}

	if (!bit_array_init(rec->extensions_arr, numext + 1)) {
		lnf_rec_free(rec);
		*recp = NULL;
		return LNF_ERR_NOMEM;
	}

	/* initialise (non nfdump) field data */
	rec->field_data = malloc( LNF_FLD_TERM_ * sizeof(void*) );
	if (rec->field_data == NULL) {
		lnf_rec_free(rec);
		*recp = NULL;
		return LNF_ERR_NOMEM;
	}

	memset(rec->field_data, 0x0, LNF_FLD_TERM_ * sizeof(void*));

	for (i = LNF_FLD_ZERO_; i < LNF_FLD_TERM_; i++) {
		if (lnf_fields_def[i].type !=  LNF_FLD_ZERO_) {
			rec->field_data[i] = malloc(lnf_fields_def[i].size);
			if (rec->field_data[i] == NULL) { 
				lnf_rec_free(rec);
				*recp = NULL;
				return LNF_ERR_NOMEM;
			} 
		}
	}

	lnf_rec_clear(rec);

	*recp = rec; 

	return LNF_OK;
}

/* clear record */
void lnf_rec_clear(lnf_rec_t *rec) {

	if (rec != NULL) {
		bit_array_clear(rec->extensions_arr);
		memset(rec->master_record, 0x0, sizeof(master_record_t));
		rec->sequence_failures = 0;
	}

	memset(rec->exporter, 0x0, sizeof(generic_exporter_t));
	memset(rec->sampler, 0x0, sizeof(generic_sampler_t));

	rec->exporter->info.version = LNF_DEFAULT_EXPORTER_VERSION;
}

/* copy record */
int lnf_rec_copy(lnf_rec_t *dst, lnf_rec_t *src) {

	if (dst == NULL || src == NULL) {
		return LNF_ERR_OTHER;
	}

	memcpy(dst->master_record, src->master_record, sizeof(master_record_t));
	memcpy(dst->exporter, src->exporter, sizeof(generic_exporter_t));
	memcpy(dst->sampler, src->sampler, sizeof(generic_sampler_t));

	dst->sequence_failures = src->sequence_failures;
	dst->flags = src->flags;

	dst->exporter->info.version = LNF_DEFAULT_EXPORTER_VERSION;

	if ( bit_array_copy(dst->extensions_arr, src->extensions_arr)) {
		return LNF_OK;
	} else {
		return LNF_ERR_OTHER;
	}
}

/* get data from the record in binnary representation */
int lnf_rec_get_raw(lnf_rec_t *rec, int type, char *buf, size_t size, size_t *ret_size) {

	int i, offset, data_size;
	char data[LNF_MAX_FIELD_LEN];

	lnf_rec_raw_t *raw = (lnf_rec_raw_t *)buf;			/* map buffer to lnf_rec_raw_t */
	lnf_rec_raw_entry_t *current_entry;


	if (rec == NULL) {
		return LNF_ERR_OTHER;
	}

	if (type != LNF_REC_RAW_TLV) {
		lnf_seterror("%s: unsupported version in TLV (0x%x)", __func__, type);
		return LNF_ERR_OTHER_MSG;
	}

	raw->version = LNF_REC_RAW_TLV;
	raw->size = 0;
	offset = sizeof(lnf_rec_raw_t);
	*ret_size = 0;

	/* walk via all items */
	for (i = LNF_FLD_ZERO_; i < LNF_FLD_TERM_; i++) {

		if ( lnf_rec_fget(rec, i, data) == LNF_OK ) {

			data_size = __lnf_fld_size(i);

			switch ( __lnf_fld_type(i) ) {
				case LNF_UINT16:  
					*(uint16_t *)data = htons(*(uint16_t *)data);
					break;
				case LNF_UINT32:  
					*(uint32_t *)data = htonl(*(uint32_t *)data);
					break;
				case LNF_UINT64:  
					*(uint64_t *)data = htonll(*(uint64_t *)data);
					break;
			}

			current_entry = (lnf_rec_raw_entry_t *)(buf + offset);
			
			current_entry->field = i;
			current_entry->data_size = data_size;

			memcpy(&buf[offset + sizeof(lnf_rec_raw_entry_t)], data, data_size);

			offset += sizeof(lnf_rec_raw_entry_t) + data_size;
			raw->size += sizeof(lnf_rec_raw_entry_t) + data_size;

			if (offset > size) {
				return LNF_ERR_NOMEM;
			}
		}
	}

	*ret_size = offset;
	return LNF_OK;
}

/* get data from the record in binnary representation */
int lnf_rec_set_raw(lnf_rec_t *rec, char *buf, size_t size) {

	int offset;
	char data[LNF_MAX_FIELD_LEN];

	lnf_rec_raw_t *raw = (lnf_rec_raw_t *)buf;			/* map buffer to lnf_rec_raw_t */
	lnf_rec_raw_entry_t *current_entry;


	if (rec == NULL) {
		return LNF_ERR_OTHER;
	}

	/* check size in buffer */
	if (size < sizeof(lnf_rec_raw_t)) {
		lnf_seterror("%s: invalid buffer size");
		return LNF_ERR_OTHER_MSG;
	}

	/* check version */
	if (raw->version != LNF_REC_RAW_TLV) {
		lnf_seterror("%s: unsupported version in TLV (0x%x)", __func__, raw->version);
		return LNF_ERR_OTHER_MSG;
	}

	/* check if we have all data in buffer */
	if (raw->size + sizeof(lnf_rec_raw_t) > size) {
		lnf_seterror("%s: the size of data (%dB) is slaller than buffer size (%dB)", __func__, raw->size + sizeof(lnf_rec_raw_t), size);
		return LNF_ERR_OTHER_MSG;
	}

	lnf_rec_clear(rec);

	offset = sizeof(lnf_rec_raw_t);

	/* walk via all entiries in buffer */	
	while(offset < raw->size) {

		current_entry = (lnf_rec_raw_entry_t *)(buf + offset);

		if ( current_entry->data_size >= LNF_MAX_FIELD_LEN ) {
			return LNF_ERR_NOMEM;
		}

		memcpy(data, current_entry->data, current_entry->data_size);

		switch ( lnf_fld_type(current_entry->field) ) {
			case LNF_UINT16:  
				*(uint16_t *)data = ntohs(*(uint16_t *)data);
				break;
			case LNF_UINT32:  
				*(uint32_t *)data = ntohl(*(uint32_t *)data);
				break;
			case LNF_UINT64:  
				*(uint64_t *)data = ntohll(*(uint64_t *)data);
				break;
		}

		lnf_rec_fset(rec, current_entry->field, data); 

		offset += sizeof(lnf_rec_raw_entry_t) + current_entry->data_size;

	}

	return LNF_OK;
}

/* free record */
void lnf_rec_free(lnf_rec_t *rec) {

	int i;

	if (rec == NULL) {
		return;
	}

	if (rec->extensions_arr != NULL) {
		bit_array_release(rec->extensions_arr);
		free(rec->extensions_arr);
	}

	if (rec->master_record != NULL) {
		free(rec->master_record);
	}

	if (rec->exporter != NULL) {
		free(rec->exporter);
	}

	if (rec->sampler != NULL) {
		free(rec->sampler);
	}

	if (rec->field_data != NULL) {
		for (i = LNF_FLD_ZERO_; i < LNF_FLD_TERM_; i++) {
			if (rec->field_data[i] != NULL) {
				free(rec->field_data[i]);
			}
		}
		free(rec->field_data);
	}
	free(rec);
}


/* returns LN_OK or LNF_ERR_UNKFLD */
/* TAG for check_items_map.pl: lnf_rec_fset */
int lnf_rec_fset(lnf_rec_t *rec, int field, void * p) {

	if (lnf_fld_type(field) == LNF_NONE) {

		return LNF_ERR_UNKFLD;

	}

	return __lnf_rec_fset(rec, field, p);

}


/* returns LN_OK or LNF_ERR_UNKFLD */
/* TAG for check_items_map.pl: lnf_rec_fget */
int lnf_rec_fget(lnf_rec_t *rec, int field, void * p) {

	if (lnf_fld_type(field) == LNF_NONE) {

		return LNF_ERR_UNKFLD;

	}

	return __lnf_rec_fget(rec, field, p);

}

/* initialize filter */
/* returns LNF_OK or LNF_ERR_FILTER */
/* there is a new code for lnf_filter, however it is experimental so far */
int lnf_filter_init_v1(lnf_filter_t **filterp, char *expr) {

	lnf_filter_t *filter;	

	filter = malloc(sizeof(lnf_filter_t));

	if (filter == NULL) {
		return LNF_ERR_NOMEM;
	}

#ifdef LNF_THREADS
    pthread_mutex_lock(&lnf_nfdump_filter_mutex);
#endif

	filter->v2filter = 0;	/* nitialised as V1 - nfdump filter */	

	filter->engine = CompileFilter(expr);

#ifdef LNF_THREADS
    pthread_mutex_unlock(&lnf_nfdump_filter_mutex);
#endif

	if ( !filter->engine ) {
		free(filter);
		return LNF_ERR_FILTER;
	}

	*filterp = filter;
	
	return LNF_OK;
}

/* call the proper version of the filter initialisation */
/* So far we use V1 - original nfdump filter. In future */
/* it will be replaced by new code V2 - libnf filter */
int lnf_filter_init(lnf_filter_t **filterp, char *expr) {
	return lnf_filter_init_v1(filterp, expr);
}

/************************************************************/
/* Fields        */
/************************************************************/
/* returns data type for field ir LNF_NONE if field does not exist  */
int lnf_fld_type(int field) {

	if (field > LNF_FLD_TERM_) {

		return LNF_NONE;

	}

	return __lnf_fld_type(field);
}


/************************************************************/
/* Errrr handling - needs to be updated for threades        */
/************************************************************/

/* get error string */
void lnf_error(const char *buf, int buflen) {

	strncpy((char *)buf, error_str, buflen - 1);

}

/* set error string */
void lnf_seterror(char *format, ...) {
va_list args;

	va_start(args, format);
	vsnprintf(error_str, LNF_MAX_STRING - 1, format, args);
	va_end(args);

}

/* compatibility with nfdump */
/* cannot be a macro - is reverence from other files */
void LogError(char *format, ...) {
va_list args;
	va_start(args, format);
	lnf_seterror(format, args);
	va_end(args);
}

/* empry functions - required by nfdump */
void LogInfo(char *format, ...) { }
void format_number(uint64_t num, char *s, int scale, int fixed_width) { } 
/* dummy functions from flist.c */
char *GetCurrentFilename(void) { return NULL; } 
nffile_t *GetNextFile(nffile_t *nffile, time_t twin_start, time_t twin_end) { return NULL; }
void SetupInputFileSequence(char *multiple_dirs, char *single_file, char *multiple_files) { }


