

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* uncommon types used by libnf */
/* IP address, MAC address, MPLS stack */
//typedef struct in6_addr lnf_ip_t;
typedef struct lnf_ip_s { uint32_t data[4]; } lnf_ip_t;
typedef struct lnf_mac_s { uint8_t data[6]; }  lnf_mac_t;
typedef struct lnf_mpls_s { uint32_t data[10]; } lnf_mpls_t;

/* basic record type 1 - contains the most commonly used fields */
typedef struct lnf_brec1_s {
	uint64_t	first;			/* LNF_FLD_FIRST */
	uint64_t	last;			/* LNF_FLD_LAST */
	lnf_ip_t	srcaddr;		/* LNF_FLD_SRCADDR */
	lnf_ip_t	dstaddr;		/* LNF_FLD_DSTADDR */
	uint8_t		prot;			/* LNF_FLD_PROT */
	uint16_t	srcport;		/* LNF_FLD_SRCPORT */
	uint16_t	dstport;		/* LNF_FLD_DSTPORT */
	uint64_t	bytes;			/* LNF_FLD_DOCTETS */
	uint64_t	pkts;			/* LNF_FLD_DPKTS */
	uint64_t	flows;			/* LNF_FLD_AGGR_FLOWS */
} lnf_brec1_t;

#define LNF_MAX_STRING		512

/* type of fields */
/* note: if the fields type allows two kind of data type  */
/* for example UINT32 and UINT64 libnf always uses the biggest one */
#define LNF_UINT8			0x08
#define LNF_UINT16			0x16
#define LNF_UINT32			0x32
#define LNF_UINT64			0x64
#define LNF_ADDR 			0xA1	/* 128 bit addr (struct in6_addr/network order) */
#define LNF_MAC				0xA2
#define LNF_STRING			0xAA	/* null terminated string */
#define LNF_MPLS			0xAB	/* mpls labels */
#define LNF_BASIC_RECORD1	0xB1


#define LNF_MASK_TYPE  		0x0000FF
#define LNF_MASK_FIELD 		0xFFFF00 

#define LNF_GET_TYPE(x) 	(x & LNF_MASK_TYPE)
#define LNF_GET_FIELD(x) 	((x & LNF_MASK_FIELD) >> 8)
//#define LNF_GET_FIELD(x) 	((x & LNF_MASK_FIELD))

/* top two bytes of field identifies data type LNF_UINT8, ... */

#define LNF_FLD_ZERO			0x00
#define LNF_FLD_FIRST			( 0x0100 | LNF_UINT64 )
#define LNF_FLD_LAST			( 0x0200 | LNF_UINT64 )
#define LNF_FLD_RECEIVED		( 0x0300 | LNF_UINT64 )
#define LNF_FLD_DOCTETS			( 0x0400 | LNF_UINT64 )
#define LNF_FLD_DPKTS			( 0x0500 | LNF_UINT64 )
#define LNF_FLD_OUT_BYTES		( 0x0600 | LNF_UINT64 )
#define LNF_FLD_OUT_PKTS		( 0x0700 | LNF_UINT64 )
#define LNF_FLD_AGGR_FLOWS		( 0x0800 | LNF_UINT64 )
#define LNF_FLD_SRCPORT 		( 0x0900 | LNF_UINT16 )
#define LNF_FLD_DSTPORT			( 0x0a00 | LNF_UINT16 )
#define LNF_FLD_TCP_FLAGS		( 0x0b00 | LNF_UINT8 )
#define LNF_FLD_SRCADDR 		( 0x0c00 | LNF_ADDR )
#define LNF_FLD_DSTADDR			( 0x0d00 | LNF_ADDR )
#define LNF_FLD_IP_NEXTHOP		( 0x0e00 | LNF_ADDR )
#define LNF_FLD_SRC_MASK		( 0x0f00 | LNF_UINT8 )
#define LNF_FLD_DST_MASK		( 0x1000 | LNF_UINT8 )
#define LNF_FLD_TOS				( 0x1100 | LNF_UINT8 )
#define LNF_FLD_DST_TOS			( 0x1300 | LNF_UINT8 )
#define LNF_FLD_SRCAS			( 0x1400 | LNF_UINT32 )
#define LNF_FLD_DSTAS			( 0x1500 | LNF_UINT32 )
#define LNF_FLD_BGPNEXTADJACENTAS	( 0x1600 | LNF_UINT32 )
#define LNF_FLD_BGPPREVADJACENTAS	( 0x1700 | LNF_UINT32 )
#define LNF_FLD_BGP_NEXTHOP			( 0x1800 | LNF_ADDR )
#define LNF_FLD_PROT	 		( 0x1900 | LNF_UINT8 )
#define LNF_FLD_SRC_VLAN		( 0x2000 | LNF_UINT16 )
#define LNF_FLD_DST_VLAN		( 0x2100 | LNF_UINT16 )
#define LNF_FLD_IN_SRC_MAC		( 0x2200 | LNF_MAC )
#define LNF_FLD_OUT_SRC_MAC		( 0x2300 | LNF_MAC )
#define LNF_FLD_IN_DST_MAC		( 0x2400 | LNF_MAC )
#define LNF_FLD_OUT_DST_MAC		( 0x2500 | LNF_MAC )
#define LNF_FLD_MPLS_LABEL		( 0x2600 | LNF_MPLS )
#define LNF_FLD_INPUT			( 0x2700 | LNF_UINT16 )
#define LNF_FLD_OUTPUT			( 0x2800 | LNF_UINT16 )
#define LNF_FLD_DIR				( 0x2900 | LNF_UINT8 )
#define LNF_FLD_FWD_STATUS		( 0x3000 | LNF_UINT8 )
#define LNF_FLD_IP_ROUTER		( 0x3100 | LNF_ADDR )
#define LNF_FLD_ENGINE_TYPE		( 0x3200 | LNF_UINT8 )
#define LNF_FLD_ENGINE_ID		( 0x3300 | LNF_UINT8 )
#define LNF_FLD_EVENT_TIME		( 0x3400 | LNF_UINT64 )
#define LNF_FLD_CONN_ID			( 0x3500 | LNF_UINT32 )
#define LNF_FLD_ICMP_CODE		( 0x3600 | LNF_UINT8 )
#define LNF_FLD_ICMP_TYPE		( 0x3700 | LNF_UINT8 )
#define LNF_FLD_FW_XEVENT		( 0x3800 | LNF_UINT16 )
#define LNF_FLD_XLATE_SRC_IP	( 0x3900 | LNF_ADDR )
#define LNF_FLD_XLATE_DST_IP	( 0x4000 | LNF_ADDR )
#define LNF_FLD_XLATE_SRC_PORT	( 0x4100 | LNF_UINT16 )
#define LNF_FLD_XLATE_DST_PORT	( 0x4200 | LNF_UINT16 )
#define LNF_FLD_INGRESS_ACL_ID	( 0x4300 | LNF_UINT32 )
#define LNF_FLD_INGRESS_ACE_ID	( 0x4400 | LNF_UINT32 )
#define LNF_FLD_INGRESS_XACE_ID	( 0x4500 | LNF_UINT32 )
#define LNF_FLD_EGRESS_ACL_ID	( 0x4600 | LNF_UINT32 )
#define LNF_FLD_EGRESS_ACE_ID	( 0x4700 | LNF_UINT32 )
#define LNF_FLD_EGRESS_XACE_ID	( 0x4800 | LNF_UINT32 )
#define LNF_FLD_USERNAME		( 0x4900 | LNF_STRING )
#define LNF_FLD_INGRESS_VRFID	( 0x5000 | LNF_UINT32 )
#define LNF_FLD_EVENT_FLAG		( 0x5100 | LNF_UINT8 )
#define LNF_FLD_EGRESS_VRFID	( 0x5200 | LNF_UINT32 )
#define LNF_FLD_BLOCK_START		( 0x5300 | LNF_UINT16 )
#define LNF_FLD_BLOCK_END		( 0x5400 | LNF_UINT16 )
#define LNF_FLD_BLOCK_STEP		( 0x5500 | LNF_UINT16 )
#define LNF_FLD_BLOCK_SIZE		( 0x5600 | LNF_UINT16 )
#define LNF_FLD_CLIENT_NW_DELAY_USEC	( 0x5700 | LNF_UINT64 )
#define LNF_FLD_SERVER_NW_DELAY_USEC	( 0x5800 | LNF_UINT64 )
#define LNF_FLD_APPL_LATENCY_USEC		( 0x5900 | LNF_UINT64 )

#define LNF_FLD_BREC1			( 0x60000 | LNF_BASIC_RECORD1 ) 	/* special field for lnf_brec1_t */

/* text description of fields */
typedef struct lnf_field_s {
	int index;			/* numerical index of field */
	char *name;			/* field name */
	char *fld_descr;	/* short description */
} lnf_field_t;

/* info structure returned by lnf_info function */
typedef struct lnf_info_s {

	/* pointer to array of lnf_field structure */
	/* fields that are supported by the libnf */
	char*		libnf_version;			
	char*		nfdump_version;			
	lnf_field_t*	libnf_fields;			

	/* file info & statistics */	
	uint64_t	version;		/* file version */
	uint64_t	blocks;			/* number of blocks */
	int			compressed;		/* is compressed */
	int			anonymized;		/* is anonymized */
	int			catalog;		/* have catalog */
	char*		ident;			/* file string identificator - NULL if not supported */

	uint64_t	first;			/* timestamp of first stored packet (in miliseconds) */
	uint64_t	last;			/* timestamp of last stored packet (in miliseconds) */
	uint64_t	failures;		/* number of sequence failures  */

	uint64_t	flows;			/* stored flow statistics */
	uint64_t	bytes;
	uint64_t	packets;

	uint64_t	flows_tcp;		/* per basic protocol statistics  */
	uint64_t	bytes_tcp;
	uint64_t	packets_tcp;
	
	uint64_t	flows_udp;		
	uint64_t	bytes_udp;
	uint64_t	packets_udp;

	uint64_t	flows_icmp;		
	uint64_t	bytes_icmp;
	uint64_t	packets_icmp;

	uint64_t	flows_other;		
	uint64_t	bytes_other;
	uint64_t	packets_other;

	/* statistics updated during processing file via lnf_read */
	uint64_t	proc_blocks;	
	uint64_t	proc_bytes;	
	uint64_t	proc_records;	

} lnf_info_t;


#ifndef _HAVE_LIBNF_STRUCT_H_ 
/* dummy portable handles - the comlete definition is */
/* available at libnf_struct.h in lnf sources */
typedef void lnf_file_t;	
typedef void lnf_rec_t;		
typedef void lnf_filter_t;
typedef void lnf_mem_t;
#endif

 
#define LNF_OK				0x0001	/* OK status */
#define LNF_EOF 			0x0000	/* end of file */

#define LNF_ERR_UNKBLOCK	-0x0001	/* weak error: unknown block type */
#define LNF_ERR_UNKREC		-0x0002	/* weak error: unknown record type */
#define LNF_ERR_COMPAT15	-0x0004	/* weak error: old blok type suppoerted by nfdump 1.5 */
#define LNF_ERR_WEAK		-0x000F	/* all weak errors (errors to skip) */

#define LNF_ERR_READ		-0x0010	/* read error (IO) */
#define LNF_ERR_CORRUPT		-0x0020	/* coruprted file */
#define LNF_ERR_EXTMAPB		-0x0040	/* too big extension map */
#define LNF_ERR_EXTMAPM		-0x0080	/* missing extension map */
#define LNF_ERR_WRITE		-0x00F0	/* write error */

#define LNF_ERR_NOTSET		-0x0100	/* item is not set  */
#define LNF_ERR_UKNFLD		-0x0200	/* unknown field  */
#define LNF_ERR_FILTER		-0x0400	/* cannot compile filter  */
#define LNF_ERR_NOMEM		-0x0800	/* cannot allocate memory  */
#define LNF_ERR_OTHER		-0x0F00	/* some other error */


/* flags for file open */
#define LNF_READ	0x0		/* open file for reading */
#define LNF_WRITE	0x1		/* open file for for writing */
#define LNF_ANON	0x2		/* set anon flag on the file */
#define LNF_COMP	0x4		/* the file is compressed */
#define LNF_WEAKERR	0x8		/* return weak erros $(unknow block, record) */

/* other functions */
void lnf_error(char *buf, int buflen);

/* file operations */
int lnf_open(lnf_file_t **lnf_filep, char *filename, unsigned int flags, char *ident);
void lnf_info(lnf_file_t *lnf_file, lnf_info_t *lnf_info);
int lnf_read(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec);
int lnf_write(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec);
void lnf_close(lnf_file_t *lnf_file);


/* record operations */
int lnf_rec_init(lnf_rec_t **recp);
void lnf_rec_clear(lnf_rec_t *rec);
int lnf_rec_copy(lnf_rec_t *dst, lnf_rec_t *src);
int lnf_rec_fset(lnf_rec_t *rec, int field, void *data);
int lnf_rec_fget(lnf_rec_t *rec, int field, void *data);
void lnf_rec_free(lnf_rec_t *rec);


/* filter operations */
int	lnf_filter_init(lnf_filter_t **filterp, char *expr);
int	lnf_filter_match(lnf_filter_t *filter, lnf_rec_t *rec);
void lnf_filter_free(lnf_filter_t *filter);


/* memory heap operations */
int lnf_mem_init(lnf_mem_t **lnf_mem);

/* flags for lnf_mem_addf */
#define LNF_AGGR_KEY	0x0000	/* the key item */
#define LNF_AGGR_MIN	0x0001	/* min value - for LNF_FLD_FIRST */
#define LNF_AGGR_MAX	0x0002	/* max value - for LNF_FLD_LAST */
#define LNF_AGGR_SUM	0x0003	/* summary of values - for all counters */
#define LNF_AGGR_OR		0x0004	/* OR operation - for LNF_TCP_FLAGS */
#define LNF_AGGR_FLAGS	0x000F

#define LNF_SORT_NONE	0x0000	/* do not sort by this field */
#define LNF_SORT_ASC	0x0010	/* sort by item ascending */
#define LNF_SORT_DESC	0x0020	/* sort by item descending */
#define LNF_SORT_FLAGS	0x00F0

int lnf_mem_fadd(lnf_mem_t *lnf_mem, int field, int flags, int numbits, int numbits6);
int lnf_mem_write(lnf_mem_t *lnf_mem, lnf_rec_t *rec);
void lnf_mem_free(lnf_mem_t *lnf_mem);


#ifndef IN6_IS_ADDR_V4COMPAT
#define IN6_IS_ADDR_V4COMPAT(a) \
   ((((uint32_t *) (a))[0] == 0) && (((uint32_t *) (a))[1] == 0) && \
   (((uint32_t *) (a))[2] == 0) && (ntohl (((uint32_t *) (a))[3]) > 1))
#endif


