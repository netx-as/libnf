
#include <stdio.h>
#include <stdlib.h>

/* uncommon types used by libnf */
/* IP address, MAC address, MPLS stack */
//typedef struct in6_addr lnf_ip_t;
typedef struct lnf_ip { uint32_t data[4]; } lnf_ip_t;
typedef struct lnf_mac { uint8_t data[6]; }  lnf_mac_t;
typedef struct lnf_mpls { uint32_t data[10]; } lnf_mpls_t;

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
#define LNF_BASIC_RECORD	0xB1


#define LNF_MASK_TYPE  		0x000000FF
#define LNF_GET_TYPE(x) 	(x & LNF_MASK_TYPE)

/* top two bytes of field identifies data type LNF_UINT8, ... */

#define LNF_FLD_ZERO			0x00
#define LNF_FLD_FIRST			0x010000 | LNF_UINT64
#define LNF_FLD_LAST			0x020064
#define LNF_FLD_RECEIVED		0x030064
#define LNF_FLD_DOCTETS			0x040064
#define LNF_FLD_DPKTS			0x050064
#define LNF_FLD_OUT_BYTES		0x060064
#define LNF_FLD_OUT_PKTS		0x070064
#define LNF_FLD_AGGR_FLOWS		0x080064
#define LNF_FLD_SRCPORT 		0x090016
#define LNF_FLD_DSTPORT			0x0a0016
#define LNF_FLD_TCP_FLAGS		0x0b0008
#define LNF_FLD_SRCADDR 		0x0c00a1
#define LNF_FLD_DSTADDR			0x0d00a1
#define LNF_FLD_IP_NEXTHOP		0x0e00a1
#define LNF_FLD_SRC_MASK		0x0f0008
#define LNF_FLD_DST_MASK		0x100008
#define LNF_FLD_TOS				0x110008
#define LNF_FLD_DST_TOS			0x130008
#define LNF_FLD_SRCAS			0x140032
#define LNF_FLD_DSTAS			0x150032
#define LNF_FLD_BGPNEXTADJACENTAS	0x160032
#define LNF_FLD_BGPPREVADJACENTAS	0x170032
#define LNF_FLD_BGP_NEXTHOP			0x1800a1
#define LNF_FLD_PROT	 		0x190008
#define LNF_FLD_SRC_VLAN		0x200016
#define LNF_FLD_DST_VLAN		0x210016
#define LNF_FLD_IN_SRC_MAC		0x2200a2
#define LNF_FLD_OUT_SRC_MAC		0x2300a2
#define LNF_FLD_IN_DST_MAC		0x2400a2
#define LNF_FLD_OUT_DST_MAC		0x2500a2
#define LNF_FLD_MPLS_LABEL		0x2600ab
#define LNF_FLD_INPUT			0x270016
#define LNF_FLD_OUTPUT			0x280016
#define LNF_FLD_DIR				0x290008
#define LNF_FLD_FWD_STATUS		0x300008
#define LNF_FLD_IP_ROUTER		0x3100a1
#define LNF_FLD_ENGINE_TYPE		0x320008
#define LNF_FLD_ENGINE_ID		0x330008
#define LNF_FLD_EVENT_TIME		0x340064
#define LNF_FLD_CONN_ID			0x350032
#define LNF_FLD_ICMP_CODE		0x360008
#define LNF_FLD_ICMP_TYPE		0x370008
#define LNF_FLD_FW_XEVENT		0x380016
#define LNF_FLD_XLATE_SRC_IP	0x3900a1
#define LNF_FLD_XLATE_DST_IP	0x4000a1
#define LNF_FLD_XLATE_SRC_PORT	0x410016
#define LNF_FLD_XLATE_DST_PORT	0x420016
#define LNF_FLD_INGRESS_ACL_ID	0x430032
#define LNF_FLD_INGRESS_ACE_ID	0x440032
#define LNF_FLD_INGRESS_XACE_ID	0x450032
#define LNF_FLD_EGRESS_ACL_ID	0x460032
#define LNF_FLD_EGRESS_ACE_ID	0x470032
#define LNF_FLD_EGRESS_XACE_ID	0x480032
#define LNF_FLD_USERNAME		0x4900AA
#define LNF_FLD_INGRESS_VRFID	0x500032
#define LNF_FLD_EVENT_FLAG		0x510008
#define LNF_FLD_EGRESS_VRFID	0x520032
#define LNF_FLD_BLOCK_START		0x530016
#define LNF_FLD_BLOCK_END		0x540016
#define LNF_FLD_BLOCK_STEP		0x550016
#define LNF_FLD_BLOCK_SIZE		0x560016
#define LNF_FLD_CLIENT_NW_DELAY_USEC	0x570064
#define LNF_FLD_SERVER_NW_DELAY_USEC	0x580064
#define LNF_FLD_APPL_LATENCY_USEC		0x590064

/* text description of fields */
typedef struct lnf_field_f {
	int index;			/* numerical index of field */
	char *name;			/* field name */
	char *fld_descr;	/* short description */
} lnf_field_t;

extern lnf_field_t lnf_fields[];


#ifndef _HAVE_LIBNF_STRUCT_H_ 
/* dummy portable handles - the comlete definition is */
/* available at libnf_struct.h in lnf sources */
typedef void lnf_file_t;	
typedef void lnf_rec_t;		
typedef void lnf_filter_t;
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
#define LNF_ERR_WRITE		-0x00F0	/* missing extension map */

#define LNF_ERR_NOTSET		-0x0100	/* item is not set  */
#define LNF_ERR_UKNFLD		-0x0200	/* inknown field  */
#define LNF_ERR_FILTER		-0x0400	/* cannot compile filter  */
#define LNF_ERR_NOMEM		-0x0800	/* cannot allocate memory  */
#define LNF_ERR_OTHER		-0x0F00	/* cannot allocate memory  */


/* flags for file open */
#define LNF_READ	0x0		
#define LNF_WRITE	0x1		/* file is open for writing */
#define LNF_ANON	0x2		/* set anon flag on the file */
#define LNF_COMP	0x4		/* the file is compressed */
#define LNF_WEAKERR	0x8		/* return weak erros $(unknow block, record) */


/* file operations */
int lnf_open(lnf_file_t **lnf_filep, char * filename, unsigned int flags, char * ident);
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


#ifndef IN6_IS_ADDR_V4COMPAT
#define IN6_IS_ADDR_V4COMPAT(a) \
   ((((uint32_t *) (a))[0] == 0) && (((uint32_t *) (a))[1] == 0) && \
   (((uint32_t *) (a))[2] == 0) && (ntohl (((uint32_t *) (a))[3]) > 1))
#endif


