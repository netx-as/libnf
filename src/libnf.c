
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
	{LNF_FLD_FIRST, 		"first",	"Timestamp of the first packet seen (in miliseconds)"},
	{LNF_FLD_LAST,			"last",		"Timestamp of the last packet seen (in miliseconds)"},
	{LNF_FLD_RECEIVED,		"received",	"Timestamp regarding when the packet was received by collector"},
// pod:
// pod:  Statistical items
// pod:  =====================
	{LNF_FLD_DOCTETS,		"bytes",	"The number of bytes"},
	{LNF_FLD_DPKTS,			"pkts",		"The number of packets"},
	{LNF_FLD_OUT_BYTES,		"outbytes",	"The number of output bytes"},
	{LNF_FLD_OUT_PKTS,		"outpkts",	"The number of output packets"},
	{LNF_FLD_AGGR_FLOWS,	"flows",	"The number of flows (aggregated)"},
// pod:
// pod:  Layer 4 information
// pod:  =====================
	{LNF_FLD_SRCPORT,		"srcport",		"Source port"},
	{LNF_FLD_DSTPORT, 		"dstport",		"Destination port"},
	{LNF_FLD_TCP_FLAGS,		"tcpflags",		"TCP flags"},
// pod:
// pod:  Layer 3 information
// pod:  =====================
	{LNF_FLD_SRCADDR,		"srcip",		"Source IP address"},
	{LNF_FLD_DSTADDR,		"dstip",		"Destination IP address"},
	{LNF_FLD_IP_NEXTHOP,	"nexthop",		"IP next hop"},
	{LNF_FLD_SRC_MASK,		"srcmask",		"Source mask"}, 
	{LNF_FLD_DST_MASK,		"dstmask",		"Destination mask"}, 
	{LNF_FLD_TOS,			"tos",			"Source type of service"}, 
	{LNF_FLD_DST_TOS,		"dsttos",		"Destination type of service"},
	{LNF_FLD_SRCAS,			"srcas",		"Source AS number"},
	{LNF_FLD_DSTAS,			"dstas",		"Destination AS number"},
	{LNF_FLD_BGPNEXTADJACENTAS,		"nextas",	"BGP Next AS"},
	{LNF_FLD_BGPPREVADJACENTAS,		"prevas",	"BGP Previous AS"},
	{LNF_FLD_BGP_NEXTHOP,			"bgpnexthop",	"BGP next hop"},
	{LNF_FLD_PROT,			"proto",		"IP protocol"}, 
// pod:
// pod:  Layer 2 information
// pod:  =====================
	{LNF_FLD_SRC_VLAN,		"srcvlan",		"Source vlan label"},
	{LNF_FLD_DST_VLAN,		"dstvlan",		"Destination vlan label"}, 
	{LNF_FLD_IN_SRC_MAC,	"insrcmac",		"In source MAC address"},
	{LNF_FLD_OUT_SRC_MAC,	"outsrcmac",	"Out destination MAC address"},
	{LNF_FLD_IN_DST_MAC,	"indstmac",		"In destination MAC address"}, 
	{LNF_FLD_OUT_DST_MAC,	"outdstmac",	"Out source MAC address"}, 
// pod:
// pod:  MPLS information
// pod:  =====================
	{LNF_FLD_MPLS_LABEL,	"mpls",		"MPLS labels"},
// pod:
// pod:  Layer 1 information
// pod:  =====================
	{LNF_FLD_INPUT,			"inif",		"SNMP input interface number"},
	{LNF_FLD_OUTPUT,		"outif",	"SNMP output interface number"},
	{LNF_FLD_DIR,			"dir",		"Flow directions ingress/egress"}, 
	{LNF_FLD_FWD_STATUS,	"fwd",		"Forwarding status"},
// pod:
// pod:  Exporter information
// pod:  =====================
	{LNF_FLD_IP_ROUTER,		"router",	"Exporting router IP"}, 
	{LNF_FLD_ENGINE_TYPE,	"systype",	"Type of exporter"},
	{LNF_FLD_ENGINE_ID,		"sysid",	"Internal SysID of exporter"},
// pod:
// pod:  NSEL fields, see: http://www.cisco.com/en/US/docs/security/asa/asa81/netflow/netflow.html
// pod:  =====================
	{LNF_FLD_EVENT_TIME,		"eventtime",	"NSEL The time that the flow was created"},
	{LNF_FLD_CONN_ID,			"connid",		"NSEL An identifier of a unique flow for the device"},
	{LNF_FLD_ICMP_CODE,			"icmpcode",		"NSEL ICMP code value"},
	{LNF_FLD_ICMP_TYPE,			"icmptype",		"NSEL ICMP type value"},
	{LNF_FLD_FW_XEVENT,			"xevent",		"NSEL Extended event code"},
	{LNF_FLD_XLATE_SRC_IP,		"xsrcip",	"NSEL Mapped source IPv4 address"},
	{LNF_FLD_XLATE_DST_IP,		"xdstip",	"NSEL Mapped destination IPv4 address"},
	{LNF_FLD_XLATE_SRC_PORT,	"xsrcport",	"NSEL Mapped source port"},
	{LNF_FLD_XLATE_DST_PORT,	"xdstport",	"NSEL Mapped destination port"},
// pod: NSEL The input ACL that permitted or denied the flow
	{LNF_FLD_INGRESS_ACL_ID,	"iacl",		"Hash value or ID of the ACL name"},
	{LNF_FLD_INGRESS_ACE_ID,	"iace", 	"Hash value or ID of the ACL name"},
	{LNF_FLD_INGRESS_XACE_ID,	"ixace",	"Hash value or ID of an extended ACE configuration"},
// pod: NSEL The output ACL that permitted or denied a flow  
	{LNF_FLD_EGRESS_ACL_ID,		"eacl",		"Hash value or ID of the ACL name"},
	{LNF_FLD_EGRESS_ACE_ID,		"eace",		"Hash value or ID of the ACL name"},
	{LNF_FLD_EGRESS_XACE_ID,	"exace",	"Hash value or ID of an extended ACE configuration"},
	{LNF_FLD_USERNAME,			"username",	"NSEL username"},
// pod:
// pod:  NEL (NetFlow Event Logging) fields
// pod:  =====================
	{LNF_FLD_INGRESS_VRFID,		"ingressvrfid",		"NEL NAT ingress vrf id"},
	{LNF_FLD_EVENT_FLAG,		"eventflag",		"NAT event flag (always set to 1 by nfdump)"},
	{LNF_FLD_EGRESS_VRFID,		"egressvrfid",		"NAT egress VRF ID"},
	{LNF_FLD_BLOCK_START,		"blockstart",		"NAT pool block start"},
// pod:
// pod:  NEL Port Block Allocation (added 2014-04-19)
// pod:  =====================
	{LNF_FLD_BLOCK_END,			"blockend",			"NAT pool block end"},
	{LNF_FLD_BLOCK_STEP,		"blockstep",		"NAT pool block step"},
	{LNF_FLD_BLOCK_SIZE,		"blocksize",		"NAT pool block size"},
// pod:
// pod:  Extra/special fields
// pod:  =====================
	{LNF_FLD_CLIENT_NW_DELAY_USEC,		"cl",	"nprobe latency client_nw_delay_usec"},
	{LNF_FLD_SERVER_NW_DELAY_USEC,		"sl",	"nprobe latency server_nw_delay_usec"},
	{LNF_FLD_APPL_LATENCY_USEC,			"al",	"nprobe latency appl_latency_usec"},
	{LNF_FLD_ZERO,						NULL,	NULL}
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
		bit_array_set(lnf_rec->extensions_arr, lnf_rec->master_record->map_ref->ex_id[i], 1);
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

	master_record_t *m = rec->master_record;
	bit_array_t *e = rec->extensions_arr;
	int i;

	switch (LNF_GET_FIELD(field)) {

		case LNF_GET_FIELD(LNF_FLD_FIRST): 
			m->first = *((uint64_t *)p) / 1000LL;
			m->msec_first = *((uint64_t *)p) - m->first * 1000LL;	
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_LAST): 
			m->last = *((uint64_t *)p) / 1000LL;
			m->msec_last = *((uint64_t *)p) - m->last * 1000LL;	
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_RECEIVED):
			m->received = *((uint64_t *)p);
			bit_array_set(e, EX_RECEIVED, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_DPKTS):
			m->dPkts = *((uint64_t *)p);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_DOCTETS):
			m->dOctets = *((uint64_t *)p);
			return LNF_OK;

		// EX_OUT_PKG_4 not used 
		case LNF_GET_FIELD(LNF_FLD_OUT_PKTS):
			m->out_pkts = *((uint64_t *)p);
			bit_array_set(e, EX_OUT_PKG_8, 1);
			return LNF_OK;
		// EX_OUT_BYTES_4 not used
		case LNF_GET_FIELD(LNF_FLD_OUT_BYTES):
			m->out_bytes = *((uint64_t *)p);
			bit_array_set(e, EX_OUT_BYTES_8, 1);
			return LNF_OK;
		// EX_AGGR_FLOWS_4 not used 
		case LNF_GET_FIELD(LNF_FLD_AGGR_FLOWS):
			m->aggr_flows = *((uint64_t *)p);
			bit_array_set(e, EX_AGGR_FLOWS_8, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_SRCPORT):
			m->srcport = *((uint16_t *)p);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_DSTPORT):
			m->dstport = *((uint16_t *)p);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_TCP_FLAGS):
			m->tcp_flags = *((uint8_t *)p);
			return LNF_OK;

		// Required extension 1 - IP addresses 
		// NOTE: srcaddr and dst addr do not uses ip_addr_t union/structure 
		// however the structures are compatible so we will pretend 
		// that v6.srcaddr and v6.dst addr points to same structure 
		case LNF_GET_FIELD(LNF_FLD_SRCADDR): {
			ip_addr_t *d = (ip_addr_t *)&m->v6.srcaddr;
	
			d->v6[0] = ntohll( ((ip_addr_t *)p)->v6[0] );
			d->v6[1] = ntohll( ((ip_addr_t *)p)->v6[1] );

			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)p)) {
				ClearFlag(m->flags, FLAG_IPV6_ADDR);
			} else {
				SetFlag(m->flags, FLAG_IPV6_ADDR);
			}
			return LNF_OK;
		}
		case LNF_GET_FIELD(LNF_FLD_DSTADDR): {
			ip_addr_t *d = (ip_addr_t *)&m->v6.dstaddr;
	
			d->v6[0] = ntohll( ((ip_addr_t *)p)->v6[0] );
			d->v6[1] = ntohll( ((ip_addr_t *)p)->v6[1] );

			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)p)) {
				ClearFlag(m->flags, FLAG_IPV6_ADDR);
			} else {
				SetFlag(m->flags, FLAG_IPV6_ADDR);
			}
			return LNF_OK;
		}

		case LNF_GET_FIELD(LNF_FLD_IP_NEXTHOP): {
			ip_addr_t *d = &m->ip_nexthop;
	
			d->v6[0] = ntohll( ((ip_addr_t *)p)->v6[0] );
			d->v6[1] = ntohll( ((ip_addr_t *)p)->v6[1] );

			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)p)) {
				ClearFlag(m->flags, FLAG_IPV6_NH);
				bit_array_set(e, EX_NEXT_HOP_v4, 1);
			} else {
				SetFlag(m->flags, FLAG_IPV6_NH);
				bit_array_set(e, EX_NEXT_HOP_v6, 1);
			}
			return LNF_OK;
		}

		case LNF_GET_FIELD(LNF_FLD_SRC_MASK):
			m->src_mask = *((uint8_t *)p);
			bit_array_set(e, EX_MULIPLE, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_DST_MASK):
			m->dst_mask = *((uint8_t *)p);
			bit_array_set(e, EX_MULIPLE, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_TOS):
			m->tos = *((uint8_t *)p);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_DST_TOS):
			m->dst_tos = *((uint8_t *)p);
			bit_array_set(e, EX_MULIPLE, 1);
			return LNF_OK;

		// EX_AS_2 - no used
		case LNF_GET_FIELD(LNF_FLD_SRCAS):
			m->srcas = *((uint32_t *)p);
			bit_array_set(e, EX_AS_4, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_DSTAS):
			m->dstas = *((uint32_t *)p);
			bit_array_set(e, EX_AS_4, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_BGPNEXTADJACENTAS):
			m->bgpNextAdjacentAS = *((uint32_t *)p);
			bit_array_set(e, EX_BGPADJ, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_BGPPREVADJACENTAS):
			m->bgpPrevAdjacentAS = *((uint32_t *)p);
			bit_array_set(e, EX_BGPADJ, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_BGP_NEXTHOP): {
			ip_addr_t *d = &m->bgp_nexthop;
	
			d->v6[0] = ntohll( ((ip_addr_t *)p)->v6[0] );
			d->v6[1] = ntohll( ((ip_addr_t *)p)->v6[1] );

			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)p)) {
				ClearFlag(m->flags, FLAG_IPV6_NHB);
				bit_array_set(e, EX_NEXT_HOP_BGP_v4, 1);
			} else {
				SetFlag(m->flags, FLAG_IPV6_NHB);
				bit_array_set(e, EX_NEXT_HOP_BGP_v6, 1);
			}
			return LNF_OK;
		}
		
		case LNF_GET_FIELD(LNF_FLD_PROT):
			m->prot = *((uint8_t *)p);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_SRC_VLAN):
			m->src_vlan = *((uint32_t *)p);
			bit_array_set(e, EX_VLAN, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_DST_VLAN):
			m->dst_vlan = *((uint32_t *)p);
			bit_array_set(e, EX_VLAN, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_IN_SRC_MAC): 
			m->in_src_mac = 0x0;
			for (i = 0; i < 6; i++) {
				((uint8_t *)(&m->in_src_mac))[5 - i] = ((uint8_t *)p)[i];
		    } 
			bit_array_set(e, EX_MAC_1, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_OUT_DST_MAC): 
			m->out_dst_mac = 0x0;
			for (i = 0; i < 6; i++) {
				((uint8_t *)(&m->out_dst_mac))[5 - i] = ((uint8_t *)p)[i];
		    } 
			bit_array_set(e, EX_MAC_1, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_OUT_SRC_MAC): 
			m->out_src_mac = 0x0;
			for (i = 0; i < 6; i++) {
				((uint8_t *)(&m->out_src_mac))[5 - i] = ((uint8_t *)p)[i];
		    } 
			bit_array_set(e, EX_MAC_2, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_IN_DST_MAC): 
			m->in_dst_mac = 0x0;
			for (i = 0; i < 6; i++) {
				((uint8_t *)(&m->in_dst_mac))[5 - i] = ((uint8_t *)p)[i];
		    } 
			bit_array_set(e, EX_MAC_2, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_MPLS_LABEL): 
			memcpy(m->mpls_label, p, sizeof(lnf_mpls_t));
			bit_array_set(e, EX_MPLS, 1);
			return LNF_OK;
	
		// EX_IO_SNMP_2 not used 	
		case LNF_GET_FIELD(LNF_FLD_INPUT):
			m->input = *((uint32_t *)p);
			bit_array_set(e, EX_IO_SNMP_4, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_OUTPUT):
			m->output = *((uint32_t *)p);
			bit_array_set(e, EX_IO_SNMP_4, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_DIR):
			m->dir = *((uint32_t *)p);
			bit_array_set(e, EX_MULIPLE, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_FWD_STATUS):
			m->fwd_status = *((uint32_t *)p);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_IP_ROUTER): {
			ip_addr_t *d = &m->ip_router;
	
			d->v6[0] = ntohll( ((ip_addr_t *)p)->v6[0] );
			d->v6[1] = ntohll( ((ip_addr_t *)p)->v6[1] );

			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)p)) {
				ClearFlag(m->flags, FLAG_IPV6_EXP);
				bit_array_set(e,  EX_ROUTER_IP_v4, 1);
			} else {
				SetFlag(m->flags, FLAG_IPV6_EXP);
				bit_array_set(e,  EX_ROUTER_IP_v6, 1);
			}
			return LNF_OK;
		}

		case LNF_GET_FIELD(LNF_FLD_ENGINE_TYPE):
			m->engine_type = *((uint8_t *)p);
			bit_array_set(e, EX_ROUTER_ID, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_ENGINE_ID):
			m->engine_id = *((uint8_t *)p);
			bit_array_set(e, EX_ROUTER_ID, 1);
			return LNF_OK;
#ifdef NSEL
	
		case LNF_GET_FIELD(LNF_FLD_EVENT_TIME):
			m->event_time = *((uint64_t *)p);
			bit_array_set(e, EX_NSEL_COMMON, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_CONN_ID):
			m->conn_id = *((uint32_t *)p);
			bit_array_set(e, EX_NSEL_COMMON, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_ICMP_CODE):
			m->icmp_code = *((uint8_t *)p);
			bit_array_set(e, EX_NSEL_COMMON, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_ICMP_TYPE):
			m->icmp_type = *((uint8_t *)p);
			bit_array_set(e, EX_NSEL_COMMON, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_FW_XEVENT):
			m->fw_xevent = *((uint16_t *)p);
			bit_array_set(e, EX_NSEL_COMMON, 1);
			return LNF_OK;

		// m->xlate_flags not used
		case LNF_GET_FIELD(LNF_FLD_XLATE_SRC_IP): {
			ip_addr_t *d = &m->xlate_src_ip;
	
			d->v6[0] = ntohll( ((ip_addr_t *)p)->v6[0] );
			d->v6[1] = ntohll( ((ip_addr_t *)p)->v6[1] );

			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)p)) {
				bit_array_set(e,  EX_NSEL_XLATE_IP_v4, 1);
			} else {
				bit_array_set(e,  EX_NSEL_XLATE_IP_v6, 1);
			}
			return LNF_OK;
		}
		case LNF_GET_FIELD(LNF_FLD_XLATE_DST_IP): {
			ip_addr_t *d = &m->xlate_dst_ip;
	
			d->v6[0] = ntohll( ((ip_addr_t *)p)->v6[0] );
			d->v6[1] = ntohll( ((ip_addr_t *)p)->v6[1] );

			if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)p)) {
				bit_array_set(e,  EX_NSEL_XLATE_IP_v4, 1);
			} else {
				bit_array_set(e,  EX_NSEL_XLATE_IP_v6, 1);
			}
			return LNF_OK;
		}
		case LNF_GET_FIELD(LNF_FLD_XLATE_SRC_PORT):
			m->xlate_src_port = *((uint16_t *)p);
			bit_array_set(e, EX_NSEL_XLATE_PORTS, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_XLATE_DST_PORT):
			m->xlate_dst_port = *((uint16_t *)p);
			bit_array_set(e, EX_NSEL_XLATE_PORTS, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_INGRESS_ACL_ID):
			m->ingress_acl_id[0] = *((uint32_t *)p);
			bit_array_set(e, EX_NSEL_ACL, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_INGRESS_ACE_ID):
			m->ingress_acl_id[1] = *((uint32_t *)p);
			bit_array_set(e, EX_NSEL_ACL, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_INGRESS_XACE_ID):
			m->ingress_acl_id[2] = *((uint32_t *)p);
			bit_array_set(e, EX_NSEL_ACL, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_EGRESS_ACL_ID):
			m->egress_acl_id[0] = *((uint32_t *)p);
			bit_array_set(e, EX_NSEL_ACL, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_EGRESS_ACE_ID):
			m->egress_acl_id[1] = *((uint32_t *)p);
			bit_array_set(e, EX_NSEL_ACL, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_EGRESS_XACE_ID):
			m->egress_acl_id[2] = *((uint32_t *)p);
			bit_array_set(e, EX_NSEL_ACL, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_USERNAME): {

			int len;
			
			len = strlen((char *)p);
			if ( len > sizeof(m->username) -  1 ) {
				len = sizeof(m->username) - 1;
			}

			memcpy(m->username, p, len );
			m->username[len] = '\0';
		
			if ( len < sizeof(((struct tpl_ext_42_s *)0)->username) - 1 ) {	
				bit_array_set(e, EX_NSEL_USER, 1);
			} else {
				bit_array_set(e, EX_NSEL_USER_MAX, 1);
			}
			return LNF_OK;
		}

		case LNF_GET_FIELD(LNF_FLD_INGRESS_VRFID):
			m->ingress_vrfid = *((uint32_t *)p);
			bit_array_set(e, EX_NEL_COMMON, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_EVENT_FLAG):
			m->event_flag = *((uint8_t *)p);
			bit_array_set(e, EX_NEL_COMMON, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_EGRESS_VRFID):
			m->egress_vrfid = *((uint32_t *)p);
			bit_array_set(e, EX_NEL_COMMON, 1);
			return LNF_OK;

		// EX_PORT_BLOCK_ALLOC added 2014-04-19
		case LNF_GET_FIELD(LNF_FLD_BLOCK_START):
			m->block_start = *((uint16_t *)p);
			bit_array_set(e, EX_PORT_BLOCK_ALLOC, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_BLOCK_END):
			m->block_end = *((uint16_t *)p);
			bit_array_set(e, EX_PORT_BLOCK_ALLOC, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_BLOCK_STEP):
			m->block_step = *((uint16_t *)p);
			bit_array_set(e, EX_PORT_BLOCK_ALLOC, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_BLOCK_SIZE):
			m->block_size = *((uint16_t *)p);
			bit_array_set(e, EX_PORT_BLOCK_ALLOC, 1);
			return LNF_OK;

#endif
		// extra fields
		case LNF_GET_FIELD(LNF_FLD_CLIENT_NW_DELAY_USEC):
			m->client_nw_delay_usec = *((uint64_t *)p);
			bit_array_set(e, EX_LATENCY, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_SERVER_NW_DELAY_USEC):
			m->server_nw_delay_usec = *((uint64_t *)p);
			bit_array_set(e, EX_LATENCY, 1);
			return LNF_OK;
		case LNF_GET_FIELD(LNF_FLD_APPL_LATENCY_USEC):
			m->appl_latency_usec = *((uint64_t *)p);
			bit_array_set(e, EX_LATENCY, 1);
			return LNF_OK;

		case LNF_GET_FIELD(LNF_FLD_BREC1): {
			lnf_brec1_t *brec1 = p;

			lnf_rec_fset(rec, LNF_FLD_FIRST, &brec1->first);
			lnf_rec_fset(rec, LNF_FLD_LAST, &brec1->last);
			lnf_rec_fset(rec, LNF_FLD_SRCADDR, &brec1->srcaddr);
			lnf_rec_fset(rec, LNF_FLD_DSTADDR, &brec1->dstaddr);
			lnf_rec_fset(rec, LNF_FLD_PROT, &brec1->prot);
			lnf_rec_fset(rec, LNF_FLD_SRCPORT, &brec1->srcport);
			lnf_rec_fset(rec, LNF_FLD_DSTPORT, &brec1->dstport);
			lnf_rec_fset(rec, LNF_FLD_DOCTETS, &brec1->bytes);
			lnf_rec_fset(rec, LNF_FLD_DPKTS, &brec1->pkts);
			lnf_rec_fset(rec, LNF_FLD_AGGR_FLOWS, &brec1->flows);
			return LNF_OK;
		}
	}

	return LNF_ERR_UKNFLD;
}

/* returns LN_OK or LNF_ERR_UKNFLD */
/* TAG for check_items_map.pl: lnf_rec_fget */
int lnf_rec_fget(lnf_rec_t *rec, int field, void * p) {

	master_record_t *m = rec->master_record;
	bit_array_t *e = rec->extensions_arr;
	int i;

	switch (LNF_GET_FIELD(field)) {

		case LNF_GET_FIELD(LNF_FLD_FIRST): 
			*((uint64_t *)p) = m->first * 1000LL + m->msec_first;
			return LNF_OK;
			break;
		case LNF_GET_FIELD(LNF_FLD_LAST): 
			*((uint64_t *)p) = m->last * 1000LL + m->msec_last;
			return LNF_OK;
			break;

		case LNF_GET_FIELD(LNF_FLD_RECEIVED): 
			*((uint64_t *)p) = m->received;
			return bit_array_get(e, EX_RECEIVED) ? LNF_OK : LNF_ERR_NOTSET;
			break;
			
		case LNF_GET_FIELD(LNF_FLD_DPKTS):
			*((uint64_t *)p) = m->dPkts;
			return LNF_OK;
			break;
		case LNF_GET_FIELD(LNF_FLD_DOCTETS):
			*((uint64_t *)p) = m->dOctets;
			return LNF_OK;
			break;

		// EX_OUT_PKG_4 not used 
		case LNF_GET_FIELD(LNF_FLD_OUT_PKTS):
			*((uint64_t *)p) = m->out_pkts;
			return bit_array_get(e, EX_OUT_PKG_8) ||  bit_array_get(e, EX_OUT_PKG_4)  ? LNF_OK : LNF_ERR_NOTSET;
			break;
		// EX_OUT_BYTES_4 not used
		case LNF_GET_FIELD(LNF_FLD_OUT_BYTES):
			*((uint64_t *)p) = m->out_bytes;
			return bit_array_get(e, EX_OUT_BYTES_8) || bit_array_get(e, EX_OUT_BYTES_4) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		// EX_AGGR_FLOWS_4 not used 
		case LNF_GET_FIELD(LNF_FLD_AGGR_FLOWS):
			*((uint64_t *)p) = m->aggr_flows;
			return bit_array_get(e, EX_AGGR_FLOWS_8) || bit_array_get(e, EX_AGGR_FLOWS_4) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_SRCPORT):
			*((uint16_t *)p) = m->srcport;
			return LNF_OK;
			break;
		case LNF_GET_FIELD(LNF_FLD_DSTPORT):
			*((uint16_t *)p) = m->dstport;
			return LNF_OK;
			break;
		case LNF_GET_FIELD(LNF_FLD_TCP_FLAGS):
			*((uint16_t *)p) = m->tcp_flags;
			return LNF_OK;
			break;

		// Required extension 1 - IP addresses 
		// NOTE: srcaddr and dst addr do not uses ip_addr_t union/structure 
		// however the structures are compatible so we will pretend 
		// that v6.srcaddr and v6.dst addr points to same structure 
		case LNF_GET_FIELD(LNF_FLD_SRCADDR): {
			ip_addr_t *d = (ip_addr_t *)&m->v6.srcaddr;
	
			((ip_addr_t *)p)->v6[0] = htonll(d->v6[0]);
			((ip_addr_t *)p)->v6[1] = htonll(d->v6[1]);

			return LNF_OK;
			break;
		}
		case LNF_GET_FIELD(LNF_FLD_DSTADDR): {
			ip_addr_t *d = (ip_addr_t *)&m->v6.dstaddr;
	
			((ip_addr_t *)p)->v6[0] = htonll(d->v6[0]);
			((ip_addr_t *)p)->v6[1] = htonll(d->v6[1]);

			return LNF_OK;
			break;
		}

		case LNF_GET_FIELD(LNF_FLD_IP_NEXTHOP): {
			ip_addr_t *d = (ip_addr_t *)&m->ip_nexthop;
	
			((ip_addr_t *)p)->v6[0] = htonll(d->v6[0]);
			((ip_addr_t *)p)->v6[1] = htonll(d->v6[1]);

			return bit_array_get(e, EX_NEXT_HOP_v4) || bit_array_get(e, EX_NEXT_HOP_v6) ? LNF_OK : LNF_ERR_NOTSET;
			return LNF_OK;
			break;
		}

		case LNF_GET_FIELD(LNF_FLD_SRC_MASK):
			*((uint8_t *)p) = m->src_mask;
			return bit_array_get(e, EX_MULIPLE) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_DST_MASK):
			 *((uint8_t *)p) = m->dst_mask;
			return bit_array_get(e, EX_MULIPLE) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_TOS):
			*((uint8_t *)p) = m->tos;
			return LNF_OK;
			break;
		case LNF_GET_FIELD(LNF_FLD_DST_TOS):
			*((uint8_t *)p) = m->dst_tos;
			return bit_array_get(e, EX_MULIPLE) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_SRCAS):
			*((uint32_t *)p) = m->srcas;
			return bit_array_get(e, EX_AS_2) || bit_array_get(e, EX_AS_4) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_DSTAS):
			*((uint32_t *)p) = m->dstas;
			return bit_array_get(e, EX_AS_2) || bit_array_get(e, EX_AS_4) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_BGPNEXTADJACENTAS):
			*((uint32_t *)p) = m->bgpNextAdjacentAS;
			return bit_array_get(e, EX_BGPADJ) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_BGPPREVADJACENTAS):
			*((uint32_t *)p) = m->bgpPrevAdjacentAS;
			return bit_array_get(e, EX_BGPADJ) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_BGP_NEXTHOP): {
			ip_addr_t *d = (ip_addr_t *)&m->bgp_nexthop;
	
			((ip_addr_t *)p)->v6[0] = htonll(d->v6[0]);
			((ip_addr_t *)p)->v6[1] = htonll(d->v6[1]);

			return bit_array_get(e, EX_NEXT_HOP_BGP_v4) || bit_array_get(e, EX_NEXT_HOP_BGP_v6) ? LNF_OK : LNF_ERR_NOTSET;
			return LNF_OK;
			break;
		}

		case LNF_GET_FIELD(LNF_FLD_PROT): 
			*((uint8_t *)p) = m->prot;
			return LNF_OK;
			break;

		case LNF_GET_FIELD(LNF_FLD_SRC_VLAN):
			*((uint32_t *)p) = m->src_vlan;
			return bit_array_get(e, EX_VLAN) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_DST_VLAN):
			*((uint32_t *)p) = m->dst_vlan;
			return bit_array_get(e, EX_VLAN) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_IN_SRC_MAC): 
			for (i = 0; i < 6; i++) {
				((uint8_t *)p)[5 - i] = ((uint8_t *)(&m->in_src_mac))[i];
		    } 
			return bit_array_get(e, EX_MAC_1) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_OUT_DST_MAC): 
			for (i = 0; i < 6; i++) {
				((uint8_t *)p)[5 - i] = ((uint8_t *)(&m->out_dst_mac))[i];
		    } 
			return bit_array_get(e, EX_MAC_1) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_OUT_SRC_MAC): 
			for (i = 0; i < 6; i++) {
				((uint8_t *)p)[5 - i] = ((uint8_t *)(&m->out_src_mac))[i];
		    } 
			return bit_array_get(e, EX_MAC_2) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_IN_DST_MAC): 
			for (i = 0; i < 6; i++) {
				((uint8_t *)p)[5 - i] = ((uint8_t *)(&m->in_dst_mac))[i];
		    } 
			return bit_array_get(e, EX_MAC_2) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_MPLS_LABEL): 
			memcpy(p, m->mpls_label, sizeof(lnf_mpls_t));
			return bit_array_get(e, EX_MPLS) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_INPUT):
			*((uint32_t *)p) = m->input;
			return bit_array_get(e, EX_IO_SNMP_2)  || bit_array_get(e, EX_IO_SNMP_4) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_OUTPUT):
			*((uint32_t *)p) =  m->output;
			return bit_array_get(e, EX_IO_SNMP_2)  || bit_array_get(e, EX_IO_SNMP_4) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_DIR):
			*((uint32_t *)p) =  m->dir;
			return bit_array_get(e, EX_MULIPLE) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_FWD_STATUS):
			*((uint32_t *)p) = m->fwd_status;
			return LNF_OK;
			break;

		case LNF_GET_FIELD(LNF_FLD_IP_ROUTER): {
			ip_addr_t *d = (ip_addr_t *)&m->ip_router;
	
			((ip_addr_t *)p)->v6[0] = htonll(d->v6[0]);
			((ip_addr_t *)p)->v6[1] = htonll(d->v6[1]);

			return bit_array_get(e, EX_ROUTER_IP_v4) || bit_array_get(e, EX_ROUTER_IP_v6) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		}

		case LNF_GET_FIELD(LNF_FLD_ENGINE_TYPE):
			*((uint8_t *)p) = m->engine_type;
			return bit_array_get(e, EX_ROUTER_ID) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_ENGINE_ID):
			*((uint8_t *)p) = m->engine_id;
			return bit_array_get(e, EX_ROUTER_ID) ? LNF_OK : LNF_ERR_NOTSET;
			break;

#ifdef NSEL
	
		case LNF_GET_FIELD(LNF_FLD_EVENT_TIME):
			*((uint64_t *)p) = m->event_time;
			return bit_array_get(e, EX_NSEL_COMMON) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_CONN_ID):
			*((uint32_t *)p) = m->conn_id;
			return bit_array_get(e, EX_NSEL_COMMON) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_ICMP_CODE):
			*((uint8_t *)p) = m->icmp_code;
			return bit_array_get(e, EX_NSEL_COMMON) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_ICMP_TYPE):
			*((uint8_t *)p) = m->icmp_type;
			return bit_array_get(e, EX_NSEL_COMMON) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_FW_XEVENT):
			*((uint16_t *)p) = m->fw_xevent;
			return bit_array_get(e, EX_NSEL_COMMON) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		 // m->xlate_flags not used
		case LNF_GET_FIELD(LNF_FLD_XLATE_SRC_IP): {
			ip_addr_t *d = (ip_addr_t *)&m->xlate_src_ip;
	
			((ip_addr_t *)p)->v6[0] = htonll(d->v6[0]);
			((ip_addr_t *)p)->v6[1] = htonll(d->v6[1]);

			return bit_array_get(e, EX_NSEL_XLATE_IP_v4) || bit_array_get(e, EX_NSEL_XLATE_IP_v6) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		}
		case LNF_GET_FIELD(LNF_FLD_XLATE_DST_IP): {
			ip_addr_t *d = (ip_addr_t *)&m->xlate_dst_ip;
	
			((ip_addr_t *)p)->v6[0] = htonll(d->v6[0]);
			((ip_addr_t *)p)->v6[1] = htonll(d->v6[1]);

			return bit_array_get(e, EX_NSEL_XLATE_IP_v4) || bit_array_get(e, EX_NSEL_XLATE_IP_v6) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		}
		case LNF_GET_FIELD(LNF_FLD_XLATE_SRC_PORT):
			*((uint16_t *)p) = m->xlate_src_port;
			return bit_array_get(e, EX_NSEL_XLATE_PORTS) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_XLATE_DST_PORT):
			*((uint16_t *)p) = m->xlate_dst_port;
			return bit_array_get(e, EX_NSEL_XLATE_PORTS) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_INGRESS_ACL_ID):
			*((uint32_t *)p) = m->ingress_acl_id[0];
			return bit_array_get(e, EX_NSEL_ACL) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_INGRESS_ACE_ID):
			*((uint32_t *)p) = m->ingress_acl_id[1];
			return bit_array_get(e, EX_NSEL_ACL) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_INGRESS_XACE_ID):
			*((uint32_t *)p) = m->ingress_acl_id[2];
			return bit_array_get(e, EX_NSEL_ACL) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_EGRESS_ACL_ID):
			*((uint32_t *)p) = m->egress_acl_id[0];
			return bit_array_get(e, EX_NSEL_ACL) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_EGRESS_ACE_ID):
			*((uint32_t *)p) = m->egress_acl_id[1];
			return bit_array_get(e, EX_NSEL_ACL) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_EGRESS_XACE_ID):
			*((uint32_t *)p) = m->egress_acl_id[2];
			return bit_array_get(e, EX_NSEL_ACL) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_USERNAME): {
			memcpy(p, m->username, strlen(m->username) + 1);
			return bit_array_get(e, EX_NSEL_USER) || bit_array_get(e, EX_NSEL_USER_MAX) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		}

		case LNF_GET_FIELD(LNF_FLD_INGRESS_VRFID):
			*((uint32_t *)p) = m->ingress_vrfid;
			return bit_array_get(e, EX_NEL_COMMON) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_EVENT_FLAG):
			*((uint8_t *)p) = m->event_flag;
			return bit_array_get(e, EX_NEL_COMMON) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_EGRESS_VRFID):
			*((uint32_t *)p) = m->egress_vrfid;
			return bit_array_get(e, EX_NEL_COMMON) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		// EX_PORT_BLOCK_ALLOC added 2014-04-19
		case LNF_GET_FIELD(LNF_FLD_BLOCK_START):
			*((uint16_t *)p) = m->block_start;
			return bit_array_get(e, EX_PORT_BLOCK_ALLOC) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_BLOCK_END):
			*((uint16_t *)p) = m->block_end;
			return bit_array_get(e, EX_PORT_BLOCK_ALLOC) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_BLOCK_STEP):
			*((uint16_t *)p) = m->block_step;
			return bit_array_get(e, EX_PORT_BLOCK_ALLOC) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_BLOCK_SIZE):
			*((uint16_t *)p) = m->block_size;
			return bit_array_get(e, EX_PORT_BLOCK_ALLOC) ? LNF_OK : LNF_ERR_NOTSET;
			break;

#endif
		// extra fields
		case LNF_GET_FIELD(LNF_FLD_CLIENT_NW_DELAY_USEC):
			*((uint64_t *)p) = m->client_nw_delay_usec;
			return bit_array_get(e, EX_LATENCY) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_SERVER_NW_DELAY_USEC):
			*((uint64_t *)p) = m->server_nw_delay_usec;
			return bit_array_get(e, EX_LATENCY) ? LNF_OK : LNF_ERR_NOTSET;
			break;
		case LNF_GET_FIELD(LNF_FLD_APPL_LATENCY_USEC):
			*((uint64_t *)p) = m->appl_latency_usec;
			return bit_array_get(e, EX_LATENCY) ? LNF_OK : LNF_ERR_NOTSET;
			break;

		case LNF_GET_FIELD(LNF_FLD_BREC1): {
			lnf_brec1_t *brec1 = p;
			lnf_rec_fget(rec, LNF_FLD_FIRST, &brec1->first);
			lnf_rec_fget(rec, LNF_FLD_LAST, &brec1->last);
			lnf_rec_fget(rec, LNF_FLD_SRCADDR, &brec1->srcaddr);
			lnf_rec_fget(rec, LNF_FLD_DSTADDR, &brec1->dstaddr);
			lnf_rec_fget(rec, LNF_FLD_PROT, &brec1->prot);
			lnf_rec_fget(rec, LNF_FLD_SRCPORT, &brec1->srcport);
			lnf_rec_fget(rec, LNF_FLD_DSTPORT, &brec1->dstport);
			lnf_rec_fget(rec, LNF_FLD_DOCTETS, &brec1->bytes);
			lnf_rec_fget(rec, LNF_FLD_DPKTS, &brec1->pkts);
			lnf_rec_fget(rec, LNF_FLD_AGGR_FLOWS, &brec1->flows);
			return LNF_OK;
			break;
		}
	}

	return LNF_ERR_UKNFLD;

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
