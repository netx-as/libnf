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

/*! \file libnf.h
	\brief libnf C interface
    The libnf.h is complete public application interface for accessing all
	libnf functions. The API is divided into several section where each section
	represents specific operation for file, record, filter, in memory aggregation.

	For examples how to use library please visit examples directory in the root of
	source files.
*/
#ifndef _LIBNF_H
#define _LIBNF_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* uncommon types used by libnf */
/* IP address, MAC address, MPLS stack */
//typedef struct in6_addr lnf_ip_t;
typedef struct lnf_ip_s { uint32_t data[4]; } lnf_ip_t; /*!< IPv4/IPv6 address */
typedef struct lnf_mac_s { uint8_t data[6]; }  lnf_mac_t; /*!< MAC address */
typedef struct lnf_mpls_s { uint32_t data[10]; } lnf_mpls_t; /*!< MPLS tags */
typedef struct lnf_acl_s {  /*!< ACL ID (http://www.cisco.com/c/en/us/td/docs/security/asa/asa93/netflow/netflow.html) */
	uint32_t acl_id;
	uint32_t ace_id;
	uint32_t xace_id;
} lnf_acl_t;


/* basic record type 1 - contains the most commonly used fields */
typedef struct lnf_brec1_s {
	uint64_t	first;			/*!< LNF_FLD_FIRST */
	uint64_t	last;			/*!< LNF_FLD_LAST */
	lnf_ip_t	srcaddr;		/*!< LNF_FLD_SRCADDR */
	lnf_ip_t	dstaddr;		/*!< LNF_FLD_DSTADDR */
	uint8_t		prot;			/*!< LNF_FLD_PROT */
	uint16_t	srcport;		/*!< LNF_FLD_SRCPORT */
	uint16_t	dstport;		/*!< LNF_FLD_DSTPORT */
	uint64_t	bytes;			/*!< LNF_FLD_DOCTETS */
	uint64_t	pkts;			/*!< LNF_FLD_DPKTS */
	uint64_t	flows;			/*!< LNF_FLD_AGGR_FLOWS */
} lnf_brec1_t;

#define LNF_MAX_STRING		512
#define LNF_MAX_RAW_LEN 	1024
#define LNF_MAX_FIELD_LEN 	512		/* maximum length of data */

/* type of fields */
/* note: if the fields type allows two kind of data type  */
/* for example UINT32 and UINT64 libnf always uses the biggest one */
#define LNF_NONE			0x00
#define LNF_NONE_T			void
#define LNF_UINT8			0x08
#define LNF_UINT8_T			uint8_t
#define LNF_UINT16			0x16
#define LNF_UINT16_T		uint16_t
#define LNF_UINT32			0x32
#define LNF_UINT32_T		uint32_t
#define LNF_UINT64			0x64
#define LNF_UINT64_T		uint64_t
#define LNF_DOUBLE			0x70
#define LNF_DOUBLE_T		double
#define LNF_ADDR 			0xA1	/* 128 bit addr (struct in6_addr/network order) */
#define LNF_ADDR_T 			lnf_ip_t
#define LNF_MAC				0xA2
#define LNF_MAC_T			lnf_mac_t
#define LNF_STRING			0xAA	/* null terminated string, max 255 characters */
#define LNF_STRING_T		char[256]
#define LNF_MPLS			0xAB	/* mpls labels */
#define LNF_MPLS_T			lnf_mpls_t
#define LNF_ACL				0xAC	/* ACL  */
#define LNF_ACL_T			lnf_acl_t
#define LNF_BASIC_RECORD1	0xB1
#define LNF_BASIC_RECORD1_T	lnf_brec1_t


#define LNF_MASK_TYPE  		0x0000FF
#define LNF_MASK_FIELD 		0xFFFF00

#define LNF_GET_TYPE(x) 	(x & LNF_MASK_TYPE)
//#define LNF_GET_FIELD(x) 	((x & LNF_MASK_FIELD) >> 8)
#define LNF_GET_FIELD(x) 	x

/* top two bytes of field identifies data type LNF_UINT8, ... */
/* 01 - 9F - ordinary fields */
/* A0 - DF - extra fields - computed etc. */
/* EF - FF - reserved */
#define LNF_FLD_ZERO_			0x00
#define LNF_FLD_FIRST			0x01
#define LNF_FLD_LAST			0x02
#define LNF_FLD_RECEIVED		0x03
#define LNF_FLD_DOCTETS			0x04
#define LNF_FLD_DPKTS			0x05
#define LNF_FLD_OUT_BYTES		0x06
#define LNF_FLD_OUT_PKTS		0x07
#define LNF_FLD_AGGR_FLOWS		0x08
#define LNF_FLD_SRCPORT 		0x09
#define LNF_FLD_DSTPORT			0x0a
#define LNF_FLD_TCP_FLAGS		0x0b
#define LNF_FLD_SRCADDR 		0x0c
#define LNF_FLD_DSTADDR			0x0d
#define LNF_FLD_IP_NEXTHOP		0x0e
#define LNF_FLD_SRC_MASK		0x0f
#define LNF_FLD_DST_MASK		0x10
#define LNF_FLD_TOS				0x11
#define LNF_FLD_DST_TOS			0x12
#define LNF_FLD_SRCAS			0x13
#define LNF_FLD_DSTAS			0x14
#define LNF_FLD_BGPNEXTADJACENTAS	0x15
#define LNF_FLD_BGPPREVADJACENTAS	0x16
#define LNF_FLD_BGP_NEXTHOP			0x17
#define LNF_FLD_PROT	 		0x18
#define LNF_FLD_SRC_VLAN		0x19
#define LNF_FLD_DST_VLAN		0x1a
#define LNF_FLD_IN_SRC_MAC		0x1b
#define LNF_FLD_OUT_SRC_MAC		0x1c
#define LNF_FLD_IN_DST_MAC		0x1d
#define LNF_FLD_OUT_DST_MAC		0x1e
#define LNF_FLD_MPLS_LABEL		0x1f
#define LNF_FLD_INPUT			0x20
#define LNF_FLD_OUTPUT			0x21
#define LNF_FLD_DIR				0x22
#define LNF_FLD_FWD_STATUS		0x23
#define LNF_FLD_IP_ROUTER		0x24
#define LNF_FLD_ENGINE_TYPE		0x25
#define LNF_FLD_ENGINE_ID		0x26
#define LNF_FLD_EVENT_TIME		0x27
#define LNF_FLD_CONN_ID			0x28
#define LNF_FLD_ICMP_CODE		0x29
#define LNF_FLD_ICMP_TYPE		0x2a
#define LNF_FLD_FW_XEVENT		0x2b
#define LNF_FLD_XLATE_SRC_IP	0x2c
#define LNF_FLD_XLATE_DST_IP	0x2d
#define LNF_FLD_XLATE_SRC_PORT	0x2e
#define LNF_FLD_XLATE_DST_PORT	0x2f
#define LNF_FLD_INGRESS_ACL_ID	0x30
#define LNF_FLD_INGRESS_ACE_ID	0x31
#define LNF_FLD_INGRESS_XACE_ID	0x32
#define LNF_FLD_EGRESS_ACL_ID	0x33
#define LNF_FLD_EGRESS_ACE_ID	0x34
#define LNF_FLD_EGRESS_XACE_ID	0x35
#define LNF_FLD_USERNAME		0x36
#define LNF_FLD_INGRESS_VRFID	0x37
#define LNF_FLD_EVENT_FLAG		0x38
#define LNF_FLD_EGRESS_VRFID	0x39
#define LNF_FLD_BLOCK_START		0x3a
#define LNF_FLD_BLOCK_END		0x3b
#define LNF_FLD_BLOCK_STEP		0x3c
#define LNF_FLD_BLOCK_SIZE		0x3d
#define LNF_FLD_CLIENT_NW_DELAY_USEC	0x3e
#define LNF_FLD_SERVER_NW_DELAY_USEC	0x3f
#define LNF_FLD_APPL_LATENCY_USEC		0x40
#define LNF_FLD_FW_EVENT		0x41
#define LNF_FLD_INGRESS_ACL		0x42
#define LNF_FLD_EGRESS_ACL		0x43
#define LNF_FLD_INET_FAMILY		0x44
#define LNF_FLD_EXPORTER_IP		0x45
#define LNF_FLD_EXPORTER_ID		0x46
#define LNF_FLD_EXPORTER_VERSION	0x47
#define LNF_FLD_SEQUENCE_FAILURES	0x48
#define LNF_FLD_SAMPLER_MODE		0x49
#define LNF_FLD_SAMPLER_INTERVAL	0x4a
#define LNF_FLD_SAMPLER_ID			0x4b
/* Added stub for compatibility, bears same identification as ADDR variants */

/* computed and extra fields */
#define LNF_FLD_CALC_DURATION	 0xA0 			/* computed : duration in msec  */
#define LNF_FLD_CALC_BPS		 0xA1 			/* computed : Bytes per second  */
#define LNF_FLD_CALC_PPS		 0xA2 			/* computed : packets per second  */
#define LNF_FLD_CALC_BPP		 0xA3 			/* computed : bytes per packet */

/* special fields */
#define LNF_FLD_BREC1			 0xB0 			/* special field for lnf_brec1_t */

/* pair fields - special use */
#define LNF_FLD_PAIR_PORT		 0xC0			/* src port + dst port */
#define LNF_FLD_PAIR_ADDR		 0xC1			/* src ip + dst ip */
#define LNF_FLD_PAIR_AS			 0xC2			/* src as + dst as */
#define LNF_FLD_PAIR_IF			 0xC3			/* in if + out if */
#define LNF_FLD_PAIR_VLAN		 0xC4			/* src vlan + dst vlan */
#define LNF_FLD_PAIR_ADDR_ALIAS		 0xC5		/* src ip + dst ip compatibility*/

/* alias fields - for backward compatibility */
#define LNF_FLD_DPKTS_ALIAS         0x60
#define LNF_FLD_OUT_PKTS_ALIAS      0x61
#define LNF_FLD_AGGR_FLOWS_ALIAS    0x62
#define LNF_FLD_IP_NEXTHOP_ALIAS    0x63
#define LNF_FLD_IP_ROUTER_ALIAS     0x64
#define LNF_FLD_ENGINE_TYPE_ALIAS   0x65
#define LNF_FLD_ENGINE_ID_ALIAS     0x66
#define LNF_FLD_ICMP_CODE_ALIAS     0x67
#define LNF_FLD_ICMP_TYPE_ALIAS     0x68
#define LNF_FLD_TCP_FLAGS_ALIAS     0x69
#define LNF_FLD_SRCADDR_ALIAS       0x70
#define LNF_FLD_DSTADDR_ALIAS	    0x71

#define LNF_FLD_TERM_			 0xFF  			/* ID of last field */

/* text description of fields */
typedef struct lnf_field_s {
	int index;			/*!< numerical index of field */
	int default_aggr;	/*!< default aggregation function */
	int default_sort;	/*!< default sort order */
	char *name;			/**< field name */
	char *fld_descr;	/**< short description */
} lnf_field_t;


#ifndef _HAVE_LIBNF_STRUCT_H_
/* dummy portable handles - the complete definition is */
/* available at libnf_struct.h in lnf sources */
typedef void lnf_file_t;
typedef void lnf_rec_t;
typedef void lnf_filter_t;
typedef void lnf_mem_t;
typedef void lnf_mem_cursor_t;
typedef void lnf_ring_t;
#endif


#define LNF_OK				0x0001	/* OK status */
#define LNF_EOF 			0x0000	/* end of file */

#define LNF_READ			0x00
#define LNF_WRITE			0x01
#define LNF_APPEND			0x10
#define LNF_ANON			0x02
#define LNF_COMP			0x04
#define LNF_COMP_LZO		0x04
#define LNF_COMP_BZ2		0x20
#define LNF_WEAKERR			0x08
#define LNF_READ_LOOP		0x40

#define LNF_ERR_UNKBLOCK	-0x0001	/* weak error: unknown block type */
#define LNF_ERR_UNKREC		-0x0002	/* weak error: unknown record type */
#define LNF_ERR_COMPAT15	-0x0004	/* weak error: old block type supported by nfdump 1.5 */
#define LNF_ERR_WEAK		-0x000F	/* all weak errors (errors to skip) */

#define LNF_ERR_READ		-0x0010	/* read error (IO) */
#define LNF_ERR_CORRUPT		-0x0020	/* corrupted file */
#define LNF_ERR_EXTMAPB		-0x0040	/* too big extension map */
#define LNF_ERR_EXTMAPM		-0x0080	/* missing extension map */
#define LNF_ERR_WRITE		-0x00F0	/* write error */

#define LNF_ERR_NOTSET		-0x0100	/* item is not set  */
#define LNF_ERR_UNKFLD		-0x0200	/* unknown field  */
#define LNF_ERR_FILTER		-0x0400	/* cannot compile filter  */
#define LNF_ERR_NOMEM		-0x0800	/* cannot allocate memory  */
#define LNF_ERR_OTHER		-0x0F00	/* some other error */
#define LNF_ERR_OTHER_MSG	-0x1000	/* other error and more descriptive information can be get from lnf_error */
#define LNF_ERR_NAN			-0x2000	/* attempt to divide by 0 */


/* constants for lnf_info function */
#define LNF_INFO_VERSION		0x02	/* string with lbnf version - char* */
#define LNF_INFO_NFDUMP_VERSION	0x04	/* string with nfdump version that libnf is based on - char**/
#define LNF_INFO_FILE_VERSION	0x06	/* nfdump file version  - int*/
#define LNF_INFO_BLOCKS			0x08	/* number of block in file - unit64_t */
#define LNF_INFO_COMPRESSED		0x0A	/* is file compressed - int */
#define LNF_INFO_LZO_COMPRESSED	0x0B	/* is file LZO compressed - int */
#define LNF_INFO_BZ2_COMPRESSED	0x0D	/* is file BZ2 compressed - int */
#define LNF_INFO_ANONYMIZED		0x0C	/* is file anonymized - int */
#define LNF_INFO_CATALOG		0x0E	/* file have catalog - int */
#define LNF_INFO_IDENT			0x10	/* string ident - char* */
#define LNF_INFO_FIRST			0x12	/* msec of first packet in file - unit64_t */
#define LNF_INFO_LAST			0x14	/* msec of last packet in file - uint64_t */
#define LNF_INFO_FAILURES		0x16	/* number of sequence failures - uint64_t */
#define LNF_INFO_FLOWS			0x18	/* summary of stored flows - uint64_t */
#define LNF_INFO_BYTES			0x1A	/* summary of stored bytes - uint64_t */
#define LNF_INFO_PACKETS		0x1C	/* summary of stored packets - uint64_t */
#define LNF_INFO_PROC_BLOCKS	0x1E	/* number of processed blocks - uint64_t */
#define LNF_INFO_FLOWS_TCP		0x20	/* number of stored TCP flows - uint64_t */
#define LNF_INFO_FLOWS_UDP		0x22	/* number of stored UDP flows - uint64_t */
#define LNF_INFO_FLOWS_ICMP		0x24	/* number of stored ICMP flows - uint64_t */
#define LNF_INFO_FLOWS_OTHER	0x26	/* number of stored other flows - uint64_t */
#define LNF_INFO_BYTES_TCP		0x28	/* number of stored TCP bytes - uint64_t */
#define LNF_INFO_BYTES_UDP		0x2A	/* number of stored UDP bytes - uint64_t */
#define LNF_INFO_BYTES_ICMP		0x2C	/* number of stored ICMP bytes - uint64_t */
#define LNF_INFO_BYTES_OTHER	0x2E	/* number of stored other bytes - uint64_t */
#define LNF_INFO_PACKETS_TCP	0x30	/* number of stored TCP packets - uint64_t */
#define LNF_INFO_PACKETS_UDP	0x32	/* number of stored UDP packets - uint64_t */
#define LNF_INFO_PACKETS_ICMP	0x34	/* number of stored ICMP packets - uint64_t */
#define LNF_INFO_PACKETS_OTHER	0x36	/* number of stored other packets - uint64_t */


/*!
	\defgroup file Basic file operations (red/create/write)
	\defgroup record  Record operations, fields extraction
	\defgroup filter  Filter operations
	\defgroup memheap In memory aggregation and sorting module
	\defgroup error Error handling
	\defgroup deprecated Deprecated functions
*/

/*!
\ingroup error
\brief  Get error message of last error.

Fills error buf with message of the last error. The error
buffer is only set when a function returns LNF_ERR_OTHER_MSG.
The recommended size of buffer is defined as LNF_MAX_STRING

\param buf	 	buffer where the message will be copied
\param buflen	available space in the buffer
*/
void lnf_error(const char *buf, int buflen);

/****************************************************
* file module                                       *
*****************************************************/

/*!
	\ingroup file

	This module provides basic operations on file. The file can
	be open in either read or write mode. In write mode the
	new one is created and if the file exists it is overwritten.
*/

/*! \ingroup file

\brief initialize lnf_filep structure and opens file in read or write mode

After file is open the lnf_read/lnf_write operations can read/write records
structure (see record operations).

The special mode LNF_READ_LOOP is available since libnf > 1.26. When this
mode is used the file is read in the endless loop. Is is useful for
real time reading of files that are being written - for exaple open nfcapd.nnnn
files produced by nfcapd. This mode also detects whether the original file was
replaced by a new one and if so it reopens file handle on the new
file. The LNF_READ_LOOP ends (means lnf_read return LNF_EOF) when the
file in the file system is permanently deleted.

\param **lnf_filep 	double pointer to lnf_filep_t structure
\param *filename 	path and file name to open
\param flags 		flags, described above \n
	LNF_READ - open file for reading  \n
	LNF_READ_LOOP - open for reading in endless loop (see note above)\n
	LNF_APPEND - open file for reading in append mode (!not implemented yet)\n
	LNF_WRITE - open file for for writing  \n
	LNF_ANON - set anon flag on the file (only for write mode) \n
	LNF_COMP - set the output file to be compressed (using LZO)\n
	LNF_COMP_LZO - set the output file to be compressed (using LZO)\n
	LNF_COMP_BZ2 - set the output file to be compressed (using BZ2)\n
	LNF_WEAKERR - when reading reports also weak errors (unknown block, ...) \n
\param *ident 		file ident for newly created files, can be set to NULL
\return 			LNF_OK, LNF_ERR_NOMEM
*/
int lnf_open(lnf_file_t **lnf_filep, const char *filename, unsigned int flags, const char *ident);


/*!	\ingroup file
\brief 	Read next record from file

Read next record from file. The record is stored in lnf_rec object.

\param *lnf_file 	pointer to lnf_filep_t structure
\param *lnf_rec 	pointer to initialized record structure
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_read(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec);

/*!	\ingroup file
\brief Write record to file.

Write record to file. The record is stored in lnf_rec object.

\param *lnf_file 	pointer to lnf_filep_t structure
\param *lnf_rec 	pointer to initialized record structure
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_WRITE
*/
int lnf_write(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec);

/*!	\ingroup file
\brief Get file info.

Get detailed information and statistics related to the open file.

\param *lnf_file 	pointer to lnf_filep_t structure
\param info 		info required from file - content returned in *data\n
	LNF_INFO_VERSION - string with lbnf version (char*) \n
	LNF_INFO_NFDUMP_VERSION - string with nfdump version that libnf is based on (char*) \n
	LNF_INFO_FILE_VERSION - nfdump file version (int*)
	LNF_INFO_BLOCKS - number of block in file (unit64_t) \n
	LNF_INFO_COMPRESSED - is file compressed (int) \n
	LNF_INFO_ANONYMIZED - is file anonymized (int) \n
	LNF_INFO_CATALOG - file have catalog (int) \n
	LNF_INFO_IDENT - string ident (char*) \n
	LNF_INFO_FIRST - msec of first packet in file (unit64_t) \n
	LNF_INFO_LAST - msec of last packet in file (uint64_t) \n
	LNF_INFO_FAILURES - number of sequence failures (uint64_t) \n
	LNF_INFO_FLOWS - summary of stored flows (uint64_t) \n
	LNF_INFO_BYTES - summary of stored bytes (uint64_t) \n
	LNF_INFO_PACKETS - summary of stored packets (uint64_t) \n
	LNF_INFO_PROC_BLOCKS - number of processed blocks (uint64_t) \n
	LNF_INFO_FLOWS_TCP - number of stored TCP flows (uint64_t) \n
	LNF_INFO_FLOWS_UDP - number of stored UDP flows (uint64_t) \n
	LNF_INFO_FLOWS_ICMP - number of stored ICMP flows (uint64_t) \n
	LNF_INFO_FLOWS_OTHER - number of stored other flows (uint64_t) \n
	LNF_INFO_BYTES_TCP - number of stored TCP bytes (uint64_t) \n
	LNF_INFO_BYTES_UDP - number of stored UDP bytes (uint64_t) \n
	LNF_INFO_BYTES_ICMP - number of stored ICMP bytes (uint64_t) \n
	LNF_INFO_BYTES_OTHER - number of stored other bytes (uint64_t) \n
	LNF_INFO_PACKETS_TCP - number of stored TCP packets (uint64_t) \n
	LNF_INFO_PACKETS_UDP - number of stored UDP packets (uint64_t) \n
	LNF_INFO_PACKETS_ICMP - number of stored ICMP packets (uint64_t) \n
	LNF_INFO_PACKETS_OTHER - number of stored other packets (uint64_t) \n
\param *data 		pointer initialized and zeroed data structure
\param size			maximum size allocated for *data structure
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_OTHER
*/
int lnf_info(lnf_file_t *lnf_file, int info, void *data, size_t size);

/*!	\ingroup file
\brief Close file and release resources.

Close previously open file, flush buffer and release all relevant resources.

\param *lnf_file 	pointer to lnf_filep_t structure
*/
void lnf_close(lnf_file_t *lnf_file);


/*!	\ingroup record
\brief Initialize empty record object.

Initialize empty record object and allocate all necessary resources.

\param **recp	 	double pointer to lnf_rec_t structure
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_OTHER
*/
int lnf_rec_init(lnf_rec_t **recp);

/*!	\ingroup record
\brief Zero all fields in initialized record object.

\param *rec 	pointer to lnf_rec_t structure
*/
void lnf_rec_clear(lnf_rec_t *rec);

/*!	\ingroup record
\brief Copy content of the record between two record objects

\param *dst 		pointer to destination lnf_rec_t structure
\param *src 		pointer to source lnf_rec_t structure
\return 			LNF_OK, LNF_ERR_OTHER
*/
int lnf_rec_copy(lnf_rec_t *dst, lnf_rec_t *src);

/*!	\ingroup record
\brief Get the value of specific field (item) from the record object

\param *rec 		pointer to lnf_rec_t structure
\param field 		field ID - see "supported fields" for supported fields
\param *data 		pointer to the data buffer
\return 			LNF_OK, LNF_ERR_NOTSET, LNF_ERR_UNKFLD
*/
int lnf_rec_fset(lnf_rec_t *rec, int field, void *data);

/*!	\ingroup record
\brief Set the value of specific field (item) from the record object

\param *rec 		pointer to lnf_rec_t structure
\param field 		field ID - see "supported fields" for supported fields
\param *data 		pointer to the data buffer (must be initialized before use)
\return 			LNF_OK, LNF_ERR_UNKFLD
*/
int lnf_rec_fget(lnf_rec_t *rec, int field, void *data);

#define LNF_REC_RAW_TLV 1				/* TLV version identification */
#define LNF_REC_RAW_TLV_BUFSIZE 1024	/* recomended size of the buffer */
/*!	\ingroup record
\brief Fill buffer with binnary representation of record.

Returns binary representation of the record. The format of data
depend on the type of the record. The data can be converted
back to the lnf_rec_t by the lnf_rec_set_raw function.

Currently on LNF_REC_RAW_TLV records are supported. This formats
encodes set fields in TLV (Type-Length-Value) format. The format
is platform independent so it can be used to transfer data between
different platforms.

The data are filled to the pre-prepared buffer. If there is not enough
space in the buffer the LNF_ERR_NOMEM error code is returned. The recommended
size of the buffer is defined in LNF_REC_RAW_TLV_BUFSIZE

The example of use of lnf_rec_get_raw and lnf_rec_set_raw can be
found in examples/lnf_ex12_rec_raw.c.

\param *rec 	pointer to lnf_rec_t structure
\param type 	currently only LNF_REC_RAW_TLV is supported
\param *buf 	pointer to allocated buffer
\param size 	size of the buffer
\param ret_size the size of returned data in buffer
\return 			LNF_OK, LNF_ERR_OTHER, LNF_ERR_NOMEM
*/
int lnf_rec_get_raw(lnf_rec_t *rec, int type, char *buf, size_t size, size_t *ret_size);

/*!	\ingroup record
\brief Fill lnf_rec_t withe data from buffer gotten by lnf_rec_get_raw.

The opposite function to lnf_rec_get_raw. For more information see lnf_rec_get_raw.

\param *rec 	pointer to lnf_rec_t structure
\param *buf 	pointer to buffer with raw data
\param size 	size of the data/buffer
\return 			LNF_OK, LNF_ERR_OTHER, LNF_ERR_NOMEM
*/
int lnf_rec_set_raw(lnf_rec_t *rec, char *buf, size_t size);


/*!	\ingroup record
\brief Close file and release resources.

Close previously record object and release all relevant resources.

\param *rec 	pointer to lnf_rec_t structure
*/
void lnf_rec_free(lnf_rec_t *rec);



/*!	\ingroup filter
\brief Initialize empty filter object.

Compile filter expression, initialize filter object.

Currently libnf supports 2 version of filter code. The original one
that is embedded in the nfdump source and the new (experimental) one
that is independent on the original nfdump.

The reason for create a new filter code is that the original code
contains a lot of memory leaks (which are not problem for
nfdump command line, bud big issue in non-stop running
applications) and is almost impossible to use it in the multithread
environment. Another disadvantage is that the filter have hard-coded
fields directly into filters grammar. The advantage of this filter
that behavior is just the same as the nfdump's filter.

The new filter code is designed from the scratch does proper
memory clean ups and designed to be used in the multithread
environment. The news code is also designed to be more flexible
what means that adding new items (fields) into libnf code do not
require updating of filter code. The disadvantage of this code
is that do not support all functionality of the old (nfdump)
filter so far.

The old (nfdump) filter code is used when lnf_filter_t object is
initialized with lnf_filter_init_v1 and the new (libnf) filter code
is used when the lnf_filter_t object is initialized with
lnf_filter_init_v2 function. Remaining function (lnf_filter_match and
lnf_filter_free) are same for both version of the filter.

The function lnf_filter_init actually call the old (nfdump)
filter initialization function (lnf_filter_init_v1). In
future when the new filter code will have all features
implemented the lnf_filter_init will call lnf_filter_init_v2.

Examples:
   examples/lnf_ex02_reader.c - basic filter operations
   examples/lnf_ex07_filter.c - using both types of filter in one program


\param **filterp 	double pointer to lnf_filter_t structure
\param *expr	 	pointer to string representing filter expression
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_FILTER
*/
int	lnf_filter_init(lnf_filter_t **filterp, char *expr);

/*!	\ingroup filter
\brief Initialize empty filter object - legacy filter using nfdump code

See comment in lnf_filter_init function for more information.

\param **filterp 	double pointer to lnf_filter_t structure
\param *expr	 	pointer to string representing filter expression
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_FILTER
*/
int	lnf_filter_init_v1(lnf_filter_t **filterp, char *expr);

/*!	\ingroup filter
\brief Initialize empty filter object - new filter using libnf code

See comment in lnf_filter_init function for more information.

\param **filterp 	double pointer to lnf_filter_t structure
\param *expr	 	pointer to string representing filter expression
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_FILTER, LNF_ERR_OTHER_MSG
*/
int	lnf_filter_init_v2(lnf_filter_t **filterp, char *expr);

/*!	\ingroup filter
\brief Returns pointer to libnf-ffilter (ff_t) internal structure.

In cases when the internal structure of ffilter must be accessed
from the outside of the library (index optimization etc.).

The pointer is returned only if filter was properly initialized as
libnf v2 filter {via. lnf_filter_init_v2). For other cases the NULL
pointer is returned.

\param *filter		double pointer to lnf_filter_t structure
\return 			void pointer to ffilter internal structure (ff_t).
*/
void *lnf_filter_ffilter_ptr(lnf_filter_t *filter);

/*!	\ingroup filter
\brief Match record object against filter.

Match the record object. If the record object matched the
filter syntax the return value is 1 else the return value is 0.

\param *filter		double pointer to lnf_filter_t structure
\param *rec		 	pointer to record object
\return 			0, 1
*/
int	lnf_filter_match(lnf_filter_t *filter, lnf_rec_t *rec);

/*!	\ingroup filter
\brief Close filter and release resources.

\param *filter	pointer to lnf_filter_t structure
*/
void lnf_filter_free(lnf_filter_t *filter);


#define LNF_MAX_THREADS 128		/* maximum threads */

/*!	\ingroup memheap
\brief Initialize empty memheap object.

Initialize empty memheap object and allocate all necessary resources.
Memheap is the set of functions that allows to aggregate and sort
records data. The uses of the lnf_mem is usually done in five steps:

1. Initialize lnf_mem_t structure
2. Set key, aggregation and sort key via lnf_mem_fadd function
3. Fill internal structures with input records via lnf_mem_write
4. Read aggregated and sorted result via lnf_mem_read
5. Release lnf_mem_t structure and all relevant resources.

Examples:
	examples/lnf_ex03_aggreg.c - simple aggregation example

\param **lnf_mem 	double pointer to lnf_mem_t structure
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_OTHER
*/
int lnf_mem_init(lnf_mem_t **lnf_mem);

/* options for lnf_mem_t */
#define LNF_OPT_HASHBUCKETS	0x0001	/* number of buckets in hash  */
#define LNF_OPT_LISTMODE	0x0002	/* switch lnf_mem_t to list mode */
#define LNF_OPT_COMP_STATSCMP	0x0003	/* switch lnf_mem_t compatible statistics mode */

/*!	\ingroup memheap
\brief Set lnf_mem_t options

Set some specific lnf_mem_t options

In in the list mode lnf_mem_t acts as list of records. There is no aggregation performed
and for each written record new entry in   is created. This mode is suitable for
sorting items without aggregation.

\param *lnf_mem 	pointer to lnf_mem_t structure
\param info 		option to set - value of option is stored in *data\n
	LNF_OPT_HASHBUCKETS - the number of buckets in hash table (int) (default 500000)
	LNF_OPT_LISTMODE - switch lnf_mem into linked list mode (NULL)
	LNF_OPT_COMP_STATSCMP - switch lnf_mem into mode where pair statistics compatible mode with nfdump

In nfdump when the statistics i computed for the part item (for example port) the record is counted
twice - first for src port and then for destination port. However if the source and
destination port is the same nfdump counts the record only once. By default libnf always
counts the records with pair fields twice. This option switches libnf to the nfdump behavior.

Example:
We have input flow
SRC           DST          PKTS BYTES
1.1.1.1:53 -> 2.2.2.2:53      1   20
3.3.3.3:80 -> 4.4.4.4:1222    3   80

and we have statistics via port field

In the nfdump and libnf with LNF_OPT_COMP_STATSCMP option enabled
the result will be:
PORT    PKTS BYTES
53         1    20
80         3    80
1222       3    80

but in libnf  without LNF_OPT_COMP_STATSCMP option the result will be:
PORT    PKTS BYTES
53         2    40
80         3    80
1222       3    80


\param *data 		pointer to data structure
\param size			data size
\return 			LNF_OK, LNF_ERR_OTHER
*/
int lnf_mem_setopt(lnf_mem_t *lnf_mem, int opt, void *data, size_t size);

/* flags for lnf_mem_addf */
#define LNF_AGGR_AUTO	0x0000	/* autodetect aggregation key */
#define LNF_AGGR_MIN	0x0001	/* min value - for LNF_FLD_FIRST */
#define LNF_AGGR_MAX	0x0002	/* max value - for LNF_FLD_LAST */
#define LNF_AGGR_SUM	0x0003	/* summary of values - for all counters */
#define LNF_AGGR_OR		0x0004	/* OR operation - for LNF_TCP_FLAGS */
#define LNF_AGGR_KEY	0x0008	/* key field */
#define LNF_AGGR_FLAGS	0x000F

#define LNF_SORT_NONE	0x0000	/* do not sort by this field */
#define LNF_SORT_ASC	0x0010	/* sort by item ascending */
#define LNF_SORT_DESC	0x0020	/* sort by item descending */
#define LNF_SORT_FLAGS	0x00F0

/*!	\ingroup memheap
\brief Set aggregation and sort option for memheap.

Set fields for lnf_mem_t to be used in aggregation process.

\param *lnf_mem 	pointer to lnf_mem_t structure
\param field 		the id if the field that the options are set on
\param flags 		aggregation and sort flags for field
	LNF_AGGR_KEY	use the field as the aggregation key
	LNF_AGGR_MIN	use the minimum value (useful for LNF_FLD_FIRST)
	LNF_AGGR_MAX	use the max value (useful for LNF_FLD_LAST)
	LNF_AGGR_SUM	make summary of all aggregated values
	LNF_AGGR_OR		make OR operation of all values  (useful for LNF_TCP_FLAGS)
	LNF_SORT_ASC	sort the result by the in ascending order
	LNF_SORT_DESC	sort the result by the in descending order
\param numbits		in case the field type is LNF_ADDR use only first numbits as the aggregation key
\param numbits6 	same as numbits but the number of bits applied on IPv6 address
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_OTHER
*/
int lnf_mem_fadd(lnf_mem_t *lnf_mem, int field, int flags, int numbits, int numbits6);

#define LNF_FAST_AGGR_NONE	0x0000	/* do not set fast aggregation mode */
#define LNF_FAST_AGGR_BASIC	0x0001	/* perform aggregation on items FIRST,LAST,BYTES,PKTS */
#define LNF_FAST_AGGR_ALL	0x0002	/* aggregation on all items */

/*!	\ingroup memheap
\brief Set fast aggregation mode

TO BE REDESIGNED

\param *lnf_mem 	pointer to lnf_mem_t structure
\param flags 	xxx
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_OTHER
*/
int lnf_mem_fastaggr(lnf_mem_t *lnf_mem, int flags);

/*!	\ingroup memheap
\brief Write record to memheap object.

\param *lnf_mem 	pointer to lnf_mem_t structure
\param *rec 		pointer to initialized record structure
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_OTHER
*/
int lnf_mem_write(lnf_mem_t *lnf_mem, lnf_rec_t *rec);

/*!	\ingroup memheap
\brief Write raw data obtained from lnf_mem_read_raw to memheap object.

The function stores buffer data directly into lnf_mem structure. The
data can be obtained from different lnf_mem instance by lnf_mem_read_raw
function. See lnf_mem_read_raw for more information.

\param *lnf_mem 	pointer to lnf_mem_t structure
\param *buff 		pointer to the buffer with data
\param buffsize 	size of data in bufer - must match internal size of data
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_OTHER
*/
int lnf_mem_write_raw(lnf_mem_t *lnf_mem, char *buff, int buffsize);

/*!	\ingroup memheap
\brief Merge data from multiple threads into one thread.

This function merge data from all threads into one structure. If the
lnf_mem_t is switched into linked list mode this function do nit perform
merging data but only joins liked lists from all threads into one
long list.

\param *lnf_mem 	pointer to lnf_mem_t structure
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_mem_merge_threads(lnf_mem_t *lnf_mem);

/*!	\ingroup memheap
\brief 	Read next record from memheap

\param *lnf_mem 	pointer to lnf_mem_t structure
\param *rec 		pointer to initialized record structure
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_mem_read(lnf_mem_t *lnf_mem, lnf_rec_t *rec);

/*!	\ingroup memheap
\brief Set the cursor position to the first record

This function set memheap cursor to the first record in the memheap.
\param *lnf_mem 	pointer to lnf_mem_t structure
\param **cursor 	double pointer to lnf_mem_cursor_t structure
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_mem_first_c(lnf_mem_t *lnf_mem, lnf_mem_cursor_t **cursor);

/*!	\ingroup memheap
\brief Set the cursor position to the next record

This function set memheap cursor to the next record in the memheap.
\param *lnf_mem 	pointer to lnf_mem_t structure
\param **cursor 	double pointer to lnf_mem_cursor_t structure
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_mem_next_c(lnf_mem_t *lnf_mem, lnf_mem_cursor_t **cursor);

/*!	\ingroup memheap
\brief Set the cursor position to the record identified by key fields

This function set memheap cursor  to the position given by the key fields (added by
lnf_mem_fadd(..,..,LNF_AGGR_KEY) if the record. If the proper record is found returns
LNF_OK else LNF_EOF.

\param *lnf_mem 	pointer to lnf_mem_t structure
\param *rec 		pointer to initialized record structure
\param **cursor 	double pointer to lnf_mem_cursor_t structure
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_mem_lookup_c(lnf_mem_t *lnf_mem, lnf_rec_t *rec, lnf_mem_cursor_t **cursor);

/*!	\ingroup memheap
\brief Set the cursor position to the record identified by key in raw record

Same meaning as lnf_mem_lookup_c, but instead lnf_rec_t record works with raw buffer
taken from lnf_mem_read_raw function.

Lookup function works only if the lnf_mem_t was NOT switched into link list mode.

\param *lnf_mem 	pointer to lnf_mem_t structure
\param *buff 		pointer to raw buffer
\param buffsize		size of data in raw buffer
\param **cursor 	double pointer to lnf_mem_cursor_t structure
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_mem_lookup_raw_c(lnf_mem_t *lnf_mem, char *buff, int buffsize, lnf_mem_cursor_t **cursor);

/*!	\ingroup memheap
\brief 	Read next record on the position given by cursor

\param *lnf_mem 	pointer to lnf_mem_t structure
\param *cursor 		pointer to valid lnf_mem_cursor_t structure
\param *rec 		pointer to initialized record structure
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_mem_read_c(lnf_mem_t *lnf_mem, lnf_mem_cursor_t *cursor, lnf_rec_t *rec);

/*!	\ingroup memheap
\brief 	Read next record from memheap in binary format

The function reads data from internal lnf_mem structure in binary format.
The data obtained by this function shouldn't be processed
in any way except of lnf_mem_write_raw function.

For the maximum size of buffer the macro LNF_MAX_RAW_LEN can be used. See
to examples/lnf_ex05_memtrans.c for example of use lnf_mem_readi/write_raw.

The data obtained by lnf_mem_read are platform dependent!

\param *lnf_mem 	pointer to lnf_mem_t structure
\param *buff 		buffer where data will be filled in - there must be enough space for data
\param *len 		returned data size in Bytes
\param buffsize 	space available in buff
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_mem_read_raw_c(lnf_mem_t *lnf_mem, lnf_mem_cursor_t *cursor, char *buff, int *len, int buffsize);

/*!	\ingroup deprecated
\brief 	DEPRECATED Read next record from memheap in binary format

This function is deprecated and might be removed in the future version.
Use lnf_mem_read_raw_c instead.

The data obtained by lnf_mem_read are platform dependent!

\param *lnf_mem 	pointer to lnf_mem_t structure
\param *buff 		buffer where data will be filled in - there must be enough space for data
\param *len 		returned data size in Bytes
\param buffsize 	space available in buff
\return 			LNF_OK, LNF_EOF, LNF_ERR_NOMEM
*/
int lnf_mem_read_raw(lnf_mem_t *lnf_mem, char *buff, int *len, int buffsize);

/*!	\ingroup deprecated
\brief 	DEPRECATED Re-set the position of the internal cursor to the beginning

This function is deprecated and might be removed in the future version.
Use lnf_mem_first_c instead.
*/
void lnf_mem_read_reset(lnf_mem_t *lnf_mem);

/*!	\ingroup memheap
\brief Clean all data in memheap.

The memheap will be in the same state as it was after the initialization.

\param *lnf_mem 	pointer to lnf_mem_t structure
*/
void lnf_mem_clean(lnf_mem_t *lnf_mem);

/*!	\ingroup memheap
\brief Close memheap and release resources.

\param *lnf_mem 	pointer to lnf_mem_t structure
*/
void lnf_mem_free(lnf_mem_t *lnf_mem);

/*!	\ingroup ringbuf
\brief Initialize empty ring buffer object

Ring bugger object is designed for exchanging libnf records between different
processes. There is function that adds (writes) record to the ring buffer and
read function that retrieves record from ring buffer. The data between
processes are handled via shared memory that is allocated initialized when the
first instance is initialized and removed when the last instance freed. This behavior
can be modified via LNF_RING_FORCE_INIT and LNF_RING_FORCE_RELEASE during the
initialization.

NOTE: If process is ended without calling lnf_ring_free then the shared memory
remains allocated forever or until some process is called with one of
LNF_RING_FORCE_* option. It is recommended to use LNF_RING_FORCE_* ate leas in one
process (typically the main writer process).

The ringbuf supports sharing between multiple writers and readers that can be
in separate process or separate threads.

\param **ringp 		double pointer to lnf_ring_t structure
\param *filename 	pointer to string representing filename of the shared memory segment (eg. libnf-shm)
\param flags 		additional flags
	LNF_RING_FORCE_INIT	- reinitialize shared buffer during initialization
	LNF_RING_FORCE_RELEASE - remove shared buffer during lnf_ring_free
	LNF_RING_NO_BLOCK - perform non blocking reading
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_OTHER, LNF_ERR_OTHER_MSG
*/
#define LNF_RING_FORCE_INIT 0x01
#define LNF_RING_FORCE_RELEASE 0x02
#define LNF_RING_NO_BLOCK 0x04
int lnf_ring_init(lnf_ring_t **ringp, char *filename, int flags);

/*!	\ingroup ringbuf
\brief Get various information about ring buffer internal status

The function returns internal information about ring buffer.

\param *ringp 		pointer to lnf_ring_t structure
\param info 		additional flags
	LNF_RING_TOTAL	- total records properly received since initialization (uint64_t)
	LNF_RING_LOST   - number of records that couldn't be read due to ringif overflow (uint64_t)
	LNF_RING_STUCK  - counter of lock stuck states (uint64_t)
\param *data 		pointer initialized and zeroed data structure
\param size			maximum size allocated for *data structure
\return 			LNF_OK, LNF_ERR_OTHER
*/
#define LNF_RING_TOTAL 0x01
#define LNF_RING_LOST  0x02
#define LNF_RING_STUCK 0x03
int lnf_ring_info(lnf_ring_t *ring, int info, void *data, size_t size);

/*!	\ingroup ringbuf
\brief Read record from ring buffer

The function reds record from ring buffer and moves internal cursor to next
position. The function has following behavior:
- Multiple readers can retrieve data from one shared ring buffer. In that case
all readers will retrieve all data.
- If there is no more records in ring buffer to read the function returns
LNF_EOF. Is up to your code to wait for proper data in ring buffer.
- If the reader don't read data too fast the data are lost. The amount of lost
records during the ring buffer initialization can be retrieved by
lnf_ring_info(... LNF_RING_LOST) function.

\param *ringp 		pointer to lnf_ring_t structure
\param *recp 		pointer to lnf_rec_t structure
\return 			LNF_OK, LNF_EOF, LNF_ERR_OTHER,
*/
int lnf_ring_read(lnf_ring_t *ring, lnf_rec_t *rec);

/*!	\ingroup ringbuf
\brief Write (add) record to ring buffer

Teh function writes / adds record into ring buffer.

\param *ringp 		pointer to lnf_ring_t structure
\param *recp 		pointer to lnf_rec_t structure
\return 			LNF_OK, LNF_ERR_NOMEM, LNF_ERR_OTHER,
*/
int lnf_ring_write(lnf_ring_t *ring, lnf_rec_t *rec);

/*!	\ingroup ringbuf
\brief Close ring buffer and release resources.

\param *lnf_ring 	pointer to lnf_ringt structure
*/
void lnf_ring_free(lnf_ring_t *ring);


/* fields management */
int lnf_fld_type(int field);
#define LNF_FLD_INFO_FIELDS	0x01	/* fill array of ints ended with LNF_FLD_TERM_  */
#define LNF_FLD_INFO_TYPE	0x02	/* type - int */
#define LNF_FLD_INFO_SIZE	0x1F	/* field size in Bytes, 0 - for dynamics size fields */
#define LNF_FLD_INFO_NAME	0x04	/* name - char* */
#define LNF_FLD_INFO_DESCR	0x08	/* description - char * */
#define LNF_FLD_INFO_AGGR	0x0B	/* default aggregation - int */
#define LNF_FLD_INFO_SORT	0x0E	/* default sort - int */
#define LNF_FLD_INFO_IPFIX_NAME		0x11	/* ipfix name  - char */
#define LNF_FLD_INFO_IPFIX_EID		0x12	/* enterprise ID - int */
#define LNF_FLD_INFO_IPFIX_ID		0x14	/* element ID - int */
#define LNF_FLD_INFO_IPFIX_NAME6	0x18	/* ipfix name - char */
#define LNF_FLD_INFO_IPFIX_EID6		0x1B	/* enterprise ID - int */
#define LNF_FLD_INFO_IPFIX_ID6		0x1E	/* element ID - int */

#define LNF_INFO_BUFSIZE 4096		/* maximum buffer size for data lnf_*_fields */
/* return LNF_OK or LNF_ERR_UNKFLD or LNF_ERR_OTHER */
int lnf_fld_info(int field, int info, void *data, size_t size);
int lnf_fld_parse(const char *str, int *numbits, int *numbits6);


#ifndef IN6_IS_ADDR_V4COMPAT
#define IN6_IS_ADDR_V4COMPAT(a) \
   ((((uint32_t *) (a))[0] == 0) && (((uint32_t *) (a))[1] == 0) && \
   (((uint32_t *) (a))[2] == 0) && (ntohl (((uint32_t *) (a))[3]) > 1))
#endif

#endif /* _LIBNF_H */
