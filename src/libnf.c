
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

#include "nfdump_inline.c"
#include "nffile_inline.c"


/* Global Variables */
extern extension_descriptor_t extension_descriptor[];
char error_str[LNF_MAX_STRING];

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
	{LNF_FLD_ZERO,						0,				0,				NULL,	
	NULL}
};


/* open existing nfdump file and prepare for reading records */
/* only simple wrapper to nfdump function */
int lnf_open(lnf_file_t **lnf_filep, const char * filename, unsigned int flags, const char * ident) {
	int i;
	lnf_file_t *lnf_file;

	lnf_file = malloc(sizeof(lnf_file_t));
	
	if (lnf_file == NULL) {
		return LNF_ERR_NOMEM;
	}

	lnf_file->flags = flags;
	/* open file in either read only or write only mode */
	if (flags & LNF_WRITE) {
		lnf_file->nffile = OpenNewFile((char *)filename, NULL, flags & LNF_COMP, 
								flags & LNF_ANON, (char *)ident);
	} else {
		lnf_file->nffile = OpenFile((char *)filename, NULL);
	}

	if (lnf_file->nffile == NULL) {
		free(lnf_file);
		return LNF_ERR_OTHER;;
	}

	lnf_file->blk_record_remains = 0;
	lnf_file->extension_map_list = InitExtensionMaps(NEEDS_EXTENSION_LIST);

	lnf_file->lnf_map_list = NULL;


	i = 1;
	lnf_file->max_num_extensions = 0;
	while ( extension_descriptor[i++].id )
		lnf_file->max_num_extensions++;


	*lnf_filep = lnf_file;

	return LNF_OK;
}

/* fill info structure */
void lnf_info(lnf_file_t *lnf_file, lnf_info_t *i) {

	file_header_t *h;
	stat_record_t *s;

	memset(i, 0x0, sizeof(lnf_info_t));

	i->libnf_version = VERSION;
	i->nfdump_version = NFDUMP_VERSION;
	i->libnf_fields = lnf_fields;


	if (lnf_file == NULL || lnf_file->nffile == NULL || lnf_file->nffile->file_header == NULL) {
		return ;
	}

	h = lnf_file->nffile->file_header;

	i->version = h->version;
	i->blocks = h->NumBlocks;
	i->compressed = h->flags & FLAG_COMPRESSED;
	i->anonymized = h->flags & FLAG_ANONYMIZED;
	i->catalog = h->flags & FLAG_CATALOG;
	i->ident = h->ident;


	if (lnf_file->nffile->stat_record == NULL) {
		return ;
	}

	s = lnf_file->nffile->stat_record;

	i->first =  s->first_seen * 1000LL + s->msec_first;
	i->last = s->last_seen * 1000LL + s->msec_last;
	i->failures = s->sequence_failure;

	i->flows = s->numflows;
	i->bytes = s->numbytes;
	i->packets = s->numpackets;

	i->flows_tcp = s->numflows_tcp;
	i->bytes_tcp = s->numbytes_tcp;
	i->packets_tcp = s->numpackets_tcp;

	i->flows_udp = s->numflows_udp;
	i->bytes_udp = s->numbytes_udp;
	i->packets_udp = s->numpackets_udp;

	i->flows_icmp = s->numflows_icmp;
	i->bytes_icmp = s->numbytes_icmp;
	i->packets_icmp = s->numpackets_icmp;

	i->flows_other = s->numflows_other;
	i->bytes_other = s->numbytes_other;
	i->packets_other = s->numpackets_other;

	return ;
}


/* close file handler and release related structures */
void lnf_close(lnf_file_t *lnf_file) {

	if (lnf_file == NULL || lnf_file->nffile == NULL) {
		return ;
	}

	if (lnf_file->flags & LNF_WRITE) {

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

	free(lnf_file);
}

/* return next record in file */
/* status of read and fill pre-prepared structure lnf_rec */
int lnf_read(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec) {

//master_record_t	*master_record;
int ret;
uint32_t map_id;
extension_map_t *map;
int i;

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
			lnf_file->current_processed_blocks++;
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
		case CommonRecordType:
				/* data record type - go ahead */
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

	lnf_file->processed_records++;

//	lnf_file->master_record = &(lnf_file->extension_map_list->slot[map_id]->master_record);
//	lnf_rec->master_record = lnf_file->master_record;

	// changed in 1.6.8 - added exporter info 
//	ExpandRecord_v2( flow_record, extension_map_list.slot[map_id], master_record);
	ExpandRecord_v2(lnf_file->flow_record, lnf_file->extension_map_list->slot[map_id], NULL, lnf_rec->master_record);

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



/* return next record in file */
/* status of read and fill pre-prepared structure lnf_rec */
int lnf_write(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec) {
extension_map_t *map;

	/* lookup and add map into file it it is nescessary */
	map = lnf_lookup_map(lnf_file, lnf_rec->extensions_arr);

	if (map == NULL) {
		return LNF_ERR_WRITE;
	}

	lnf_rec->master_record->map_ref = map;
	lnf_rec->master_record->ext_map = map->map_id;
	lnf_rec->master_record->type = CommonRecordType;

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
		return LNF_ERR_NOMEM;
	}

	rec->master_record = malloc(sizeof(master_record_t));

	if (rec->master_record == NULL) {
		free(rec);
		return LNF_ERR_NOMEM;
	}

	rec->extensions_arr = malloc(sizeof(bit_array_t));

	if (rec->extensions_arr == NULL) {
		free(rec->master_record);
		free(rec);
		return LNF_ERR_NOMEM;
	}

	i = 1;
	numext = 0;
	while ( extension_descriptor[i++].id ) {
		numext++;
	}

	if (!bit_array_init(rec->extensions_arr, numext + 1)) {
		free(rec->extensions_arr);
		free(rec->master_record);
		free(rec);
		return LNF_ERR_NOMEM;
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
	}
}

/* copy record */
int lnf_rec_copy(lnf_rec_t *dst, lnf_rec_t *src) {

	if (dst == NULL || src == NULL) {
		return LNF_ERR_OTHER;
	}

	memcpy(dst->master_record, src->master_record, sizeof(master_record_t));
	if ( bit_array_copy(dst->extensions_arr, src->extensions_arr)) {
		return LNF_OK;
	} else {
		return LNF_ERR_OTHER;
	}
}

/* free record */
void lnf_rec_free(lnf_rec_t *rec) {

	bit_array_release(rec->extensions_arr);
	free(rec->master_record);
	free(rec->extensions_arr);
	free(rec);
}


/* returns LN_OK or LNF_ERR_UKNFLD */
/* TAG for check_items_map.pl: lnf_rec_fset */
int lnf_rec_fset(lnf_rec_t *rec, int field, void * p) {

	if (lnf_fld_type(field) == LNF_NONE) {

		return LNF_ERR_UKNFLD;

	}

	return __lnf_rec_fset(rec, field, p);

}


/* returns LN_OK or LNF_ERR_UKNFLD */
/* TAG for check_items_map.pl: lnf_rec_fget */
int lnf_rec_fget(lnf_rec_t *rec, int field, void * p) {

	if (lnf_fld_type(field) == LNF_NONE) {

		return LNF_ERR_UKNFLD;

	}

	return __lnf_rec_fget(rec, field, p);

}

/* initialize filter */
/* returns LNF_OK or LNF_ERR_FILTER */
int lnf_filter_init(lnf_filter_t **filterp, char *expr) {

	lnf_filter_t *filter;	

	filter = malloc(sizeof(lnf_filter_t));

	if (filter == NULL) {
		return LNF_ERR_NOMEM;
	}	

	filter->engine = CompileFilter(expr);
	
	if ( !filter->engine ) {
		free(filter);
		return LNF_ERR_FILTER;
	}

	*filterp = filter;
	
	return LNF_OK;
}

/* matches the record agains filter */
/* returns 1 - record was matched, 0 - record wasn't matched */
int lnf_filter_match(lnf_filter_t *filter, lnf_rec_t *rec) {

	filter->engine->nfrecord = (uint64_t *)rec->master_record;

	return  (*filter->engine->FilterEngine)(filter->engine);
}

void lnf_filter_free(lnf_filter_t *filter) {

	/* TODO nfdump do not have code to release filter :-( */
//	free(filter->engine);
	free(filter);
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

/* get error string */
void lnf_error(const char *buf, int buflen) {

	strncpy((char *)buf, error_str, buflen - 1);

}
