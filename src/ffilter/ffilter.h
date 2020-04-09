/*

 Copyright (c) 2015-2017, Tomas Podermanski, Lukas Hutak, Imrich Stoffa

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

/**
 * \file ffilter.h
 * \brief netflow fiter implementation - C interface
 */

#ifndef _FLOW_FILTER_H_
#define _FLOW_FILTER_H_//

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllexport))
    #else
      #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllimport))
    #else
      #define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define DLL_LOCAL
#else
#if __GNUC__ >= 4
#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define DLL_PUBLIC
    #define DLL_LOCAL
#endif
#endif

#include <stdint.h>
#include <stddef.h>

#define FF_MAX_STRING  1024
#define FF_SCALING_FACTOR  1000LL
#define FF_MULTINODE_MAX 4


#ifndef HAVE_HTONLL
#ifdef WORDS_BIGENDIAN
#   define ntohll(n)    (n)
#   define htonll(n)    (n)
#else
#   define ntohll(n)    ((((uint64_t)ntohl(n)) << 32) | ntohl(((uint64_t)(n)) >> 32))
#   define htonll(n)    ((((uint64_t)htonl(n)) << 32) | htonl(((uint64_t)(n)) >> 32))
#endif
#define HAVE_HTONLL 1
#endif



/**
 * \brief Supported data types
 */
typedef enum {
	FF_TYPE_UNSUPPORTED = 0x0,	/** for unsupported data types */

	FF_TYPE_UNSIGNED,
	FF_TYPE_UNSIGNED_BIG,
	FF_TYPE_SIGNED,
	FF_TYPE_SIGNED_BIG,
	FF_TYPE_UINT8,			/** 1Byte unsigned - fixed size */
	FF_TYPE_UINT16,
	FF_TYPE_UINT32,
	FF_TYPE_UINT64,
	FF_TYPE_INT8,			/** 1Byte unsigned - fixed size */
	FF_TYPE_INT16,
	FF_TYPE_INT32,
	FF_TYPE_INT64,

	FF_TYPE_DOUBLE,			/** we can be sure that IEEE 754 fomrat is used */
	FF_TYPE_ADDR,
	FF_TYPE_MAC,
	FF_TYPE_STRING,
	FF_TYPE_MPLS,
	FF_TYPE_TIMESTAMP,      /** uint64_t bit timestamp eval as unsigned, milliseconds from 1-1-1970 00:00:00 */
	FF_TYPE_TIMESTAMP_BIG,  /** uint64_t bit timestamp eval as unsigned, to host byte order conversion required */
    FF_TYPE_TERM_
} ff_type_t;

/**
 * \brief Enumerator for validation of type, operator combinations
 * 1 Dim switch uses this to tidy up a code and utilize possible optimizations
 * EQ/GT/LT/IS are operators,
 * I/UI - integers followed by size in bytes, no size means one of 1/2/4/8 bytes,
 *        BE means big endian variant expected from wrapper
 * ADP - addres with prefix
 * AD4 - v4 address
 * AD6 - v6 address
 * RE - double
 * TS - timestamp which is equivalent to UI/UIB only string convertor differs
 * M__ - mpls operators L - label one of 10, LX - label x is requested,
 * MEX - exp bits on top of stack, MES - check which label is top of stack
 */
typedef enum ff_attr_e{
	FFAT_ERR,
	FFAT_EQ_UI,
	FFAT_EQ_UIBE,
	FFAT_EQ_UI8,
	FFAT_EQ_UI4,
	FFAT_EQ_UI2,
	FFAT_EQ_UI1,
	FFAT_EQ_I,
	FFAT_EQ_IBE,
	FFAT_EQ_I8,
	FFAT_EQ_I4,
	FFAT_EQ_I2,
	FFAT_EQ_I1,
	FFAT_EQ_RE,
	FFAT_EQ_STR,
	FFAT_EQ_TSB,
	FFAT_EQ_TS,
	FFAT_EQ_MAC,
	FFAT_EQ_AD4,
	FFAT_EQ_AD6,
	FFAT_EQ_ADP,
	FFAT_EQ_ML,
	FFAT_EQ_MLX,
	FFAT_EQ_MEX,
	FFAT_EQ_MES,

	FFAT_GT_UI,
	FFAT_GT_UIBE,
	FFAT_GT_UI8,
	FFAT_GT_UI4,
	FFAT_GT_UI2,
	FFAT_GT_UI1,
	FFAT_GT_I,
	FFAT_GT_IBE,
	FFAT_GT_I8,
	FFAT_GT_I4,
	FFAT_GT_I2,
	FFAT_GT_I1,
	FFAT_GT_RE,
	FFAT_GT_STR,
	FFAT_GT_TSB,
	FFAT_GT_TS,
	FFAT_GT_MAC,
	FFAT_GT_AD4,
	FFAT_GT_AD6,
	FFAT_GT_ADP,
	FFAT_GT_ML,
	FFAT_GT_MLX,
	FFAT_GT_MEX,
	FFAT_GT_MES,

	FFAT_LT_UI,
	FFAT_LT_UIBE,
	FFAT_LT_UI8,
	FFAT_LT_UI4,
	FFAT_LT_UI2,
	FFAT_LT_UI1,
	FFAT_LT_I,
	FFAT_LT_IBE,
	FFAT_LT_I8,
	FFAT_LT_I4,
	FFAT_LT_I2,
	FFAT_LT_I1,
	FFAT_LT_RE,
	FFAT_LT_STR,
	FFAT_LT_TSB,
	FFAT_LT_TS,
	FFAT_LT_MAC,
	FFAT_LT_AD4,
	FFAT_LT_AD6,
	FFAT_LT_ADP,
	FFAT_LT_ML,
	FFAT_LT_MLX,
	FFAT_LT_MEX,
	FFAT_LT_MES,

	FFAT_IS_UI,
	FFAT_IS_UIBE,
	FFAT_IS_UI8,
	FFAT_IS_UI4,
	FFAT_IS_UI2,
	FFAT_IS_UI1,
	FFAT_IS_I,
	FFAT_IS_IBE,
	FFAT_IS_I8,
	FFAT_IS_I4,
	FFAT_IS_I2,
	FFAT_IS_I1,
	FFAT_IS_RE,
	FFAT_IS_STR,
	FFAT_IS_TSB,
	FFAT_IS_TS,
	FFAT_IS_MAC,
	FFAT_IS_AD4,
	FFAT_IS_AD6,
	FFAT_IS_ADP,
	FFAT_IS_ML,
	FFAT_IS_MLX,
	FFAT_IS_MEX,
	FFAT_IS_MES,

	FFAT_INS_UI,
	FFAT_INS_UIBE,
	FFAT_INS_UI8,
	FFAT_INS_UI4,
	FFAT_INS_UI2,
	FFAT_INS_UI1,
	FFAT_INS_I,
	FFAT_INS_IBE,
	FFAT_INS_I8,
	FFAT_INS_I4,
	FFAT_INS_I2,
	FFAT_INS_I1,
	FFAT_INS_RE,
	FFAT_INS_STR,
	FFAT_INS_TSB,
	FFAT_INS_TS,
	FFAT_INS_MAC,
	FFAT_INS_AD4,
	FFAT_INS_AD6,
	FFAT_INS_ADP,
	FFAT_INS_ML,
	FFAT_INS_MLX,
	FFAT_INS_MEX,
	FFAT_INS_MES,

	FFAT_EXIST,
	FFAT_IN

} ff_attr_t;

// Some of the types here are useless - why define another fixed size types ?
typedef void ff_unsup_t;
typedef char* ff_uint_t;
typedef char* ff_int_t;
typedef uint8_t ff_uint8_t;
typedef uint16_t ff_uint16_t;
typedef uint32_t ff_uint32_t;
typedef uint64_t ff_uint64_t;
typedef int8_t ff_int8_t;
typedef int16_t ff_int16_t;
typedef int32_t ff_int32_t;
typedef int64_t ff_int64_t;
typedef double ff_double_t;
typedef char ff_mac_t[6];
typedef uint64_t ff_timestamp_t;

/**
 * IPv4/IPv6 address
 */
typedef struct ff_ip_s {
	uint32_t data[4];
} ff_ip_t;

typedef struct ff_net_s {
	ff_ip_t ip;
	ff_ip_t mask;
	int ver;
} ff_net_t;

/**
 * Type for mpls used in abstarct tree \see ff_note_t value
 */
typedef struct ff_mpls_s {
	uint32_t val;       /** Expected value of specified mpls field */
	uint32_t label;     /** Identification of label which to evaluate if relevant */
} ff_mpls_t;

/**
 * Type for mpls expected from adapter, this is structure of one label !
 */
typedef union {
	//TODO: Test on big-endian machine
	struct {
		unsigned eos : 1;
		unsigned exp : 3;
		unsigned label : 20;
		unsigned none : 8;
	};
	uint32_t data;
} ff_mpls_label_t;

/**
 * Type for 10 pack of labels, this is expected to get from adapter
 * when evaluating mpls fields
 */
typedef struct ff_mpls_stack_s {
    ff_mpls_label_t id[10];
} ff_mpls_stack_t;

/**
 * \brief Union used to reinterpret values in node of abstract tree.
 * For more detail \see ff_node_t member - value.
 */
typedef union ff_val_u {
	uint64_t ui;
	uint32_t ui4;
	uint16_t ui2;
	uint8_t ui1;
	int64_t i;
	int32_t i4;
	int16_t i2;
	int8_t i1;
	double real;
	ff_mpls_t mpls;

	char str[1];
	ff_net_t net;
	ff_ip_t ip;
} ff_val_t;

/**
 * \brief Union used to reinterpret data from wrapper, char* is casted to trec*
 */
typedef union ff_rec_u {
	uint64_t ui;
	uint32_t ui4;
	uint16_t ui2;
	uint8_t ui1;
	int64_t i;
	int32_t i4;
	int16_t i2;
	int8_t i1;
	double real;

	char str[1];
	ff_mpls_stack_t mpls;
	ff_net_t net;
	ff_ip_t ip;
} ff_rec_t;

/**
 * \brief ffilter interface return codes
 */
typedef enum {
	FF_OK = 0x1,				/** No error occuried */
	FF_ERR_NOMEM = -0x1,
	FF_ERR_UNKN  = -0x2,
	FF_ERR_UNSUP  = -0x3,
	FF_ERR_OTHER  = -0xE,
	FF_ERR_OTHER_MSG  = -0xF, 	/** Closer description of fault can be received from ff_error */
} ff_error_t;

/**
 * \brief ffilter lvalue options
 */
typedef enum {
	FF_OPTS_NONE = 0,
	FF_OPTS_FLAGS = 0x01,		/** Item is of flag type, this change behaviour when no operator is set to bit compare */
	FF_OPTS_MPLS_LABEL = 0x08,
	FF_OPTS_MPLS_EOS = 0x04,
	FF_OPTS_MPLS_EXP = 0x02,
	FF_OPTS_CONST = 0x10,

} ff_opts_t;

/**
 * \brief Supported operations
 */
typedef enum {
	FF_OP_UNDEF = -1,
	FF_OP_NOT = 1,
	FF_OP_OR,
	FF_OP_AND,
	FF_OP_IN,
	FF_OP_YES,

	FF_OP_NOOP,
	FF_OP_EQ,
	FF_OP_LT,
	FF_OP_GT,
	FF_OP_ISSET,
	FF_OP_ISNSET,    // Nfdump compat operator, for flags exclusion
	FF_OP_EXIST,
    FF_OP_TERM_
} ff_oper_t;

extern const char* ff_oper_str[FF_OP_TERM_];
extern const char* ff_type_str[FF_TYPE_TERM_];

/**
 * \brief External identification of value
 */
typedef union {
	uint64_t index;       /** Index mapping */
	const void *ptr;      /** Direct mapping */
} ff_extern_id_t;

/**
 * \brief Identification of left value
 */
typedef struct ff_lvalue_s {
	ff_type_t type;                       /** Type of left value */
	ff_extern_id_t id[FF_MULTINODE_MAX];  /** External identification of data field */
	int options;                          /** Extra options that modiflies evaluation of data */
	int n;                                /** 0 for not set */
	const char * literal;

} ff_lvalue_t;

/**
 * \brief node of syntax tree
 */
typedef struct ff_node_s {
	ff_extern_id_t field;         /** field ID */
	char *value;                  /** buffer allocated for data */
	size_t vsize;                 /** size of data in value */
	//Future TODO: could be ommited in future if pointer to function to evaluate is used instead
	ff_type_t type;               /** data type for value */
	ff_oper_t oper;               /** operation */
	ff_attr_t opcode;             /** translated data type + operation */

	//Future TODO: transform to heap data structure - no pointers
	struct ff_node_s *left;
	struct ff_node_s *right;

} ff_node_t;

//typedef struct ff_s ff_t;
struct ff_s;

/**{@
 * \section ff_options_t
 *	Clarify purpose of options object in filter
 */

/**
 * Lookup callback signature.
 * \brief Lookup the field name found in filter expresson and identify its type one of and associated data elements
 * Callback fills in information about field into ff_lvalue_t sturcture. Required information are external
 * identification of field as understood by data function, data type of filed as one of ff_type_t enum
 * \param[in]  filter   Filter object
 * \param[in]  fieldstr Name of element to identify
 * \param[out] lvalue   Identification representing field
 * \return FF_OK on success
 */
typedef ff_error_t (*ff_lookup_func_t) (struct ff_s *filter, const char *fieldstr, ff_lvalue_t *lvalue);

/**
 * Data Callback signature
 * \brief Select requested data from record.
 * Callback copies data associated with external identification, as set by lookup callback, from evaluated record
 * to buffer and marks length of these data. Structure of record must be known to data function.
 * \param ff_s Filter object
 * \param[in]     record General data pointer to record
 * \param[in]     id     Indentfication of data field in recrod
 * \param[in/out] buf    in - Pointer to buffer, out - Pointer to buffer with retrieved data
 * \param[in/out] vsize  in - Size of passed buffer, out - size of valid data in buffer
 */
typedef ff_error_t (*ff_data_func_t) (struct ff_s*, void *, ff_extern_id_t, char**, size_t *);

/**
 * Rval_map Callback signature
 * \brief Translate constant values unresolved by filter convertors.
 * Callback is used to transform literal value to given ff_type_t when internal conversion function fails.
 * \param ff_s Filter object
 * \param[in]  valstr String representation of value
 * \param[in]  type   Required ffilter internal type
 * \param[in]  id     External identification of field (form transforming exceptions like flags)
 * \param[out] buf    Buffer to copy data
 * \param[out] size   Length of valid data in buffer
 */
typedef ff_error_t (*ff_rval_map_func_t) (struct ff_s *, const char *, ff_type_t, ff_extern_id_t, char*, size_t* );

/**
 * \brief Filter options callbacks
 */
typedef struct ff_options_s {
	/** Element lookup function */
	ff_lookup_func_t ff_lookup_func;
	/** Value comparation function */
	ff_data_func_t ff_data_func;
	/** Literal constants translation function eg. TCP->6 */
	ff_rval_map_func_t ff_rval_map_func;
} ff_options_t;

/**@}*/

/**
 * \brief Filter object instance
 */
typedef struct ff_s {

	ff_options_t    options;	/**< Callback functions */
	ff_node_t       *root;		/**< Internal representation of filter expression */
	char            error_str[FF_MAX_STRING];	/**< Last error set */

} ff_t;

/**
 * \brief Options constructor allocates options structure
 * \param[in] ff_options
 * \return FF_OK on success
 */
ff_error_t ff_options_init(ff_options_t **ff_options);

/**
 * \brief Options destructor frees options structure
 * \param[out] ff_options Address of pointer to options
 * \return FF_OK on success
 */
ff_error_t ff_options_free(ff_options_t *ff_options);

/**
 * \brief Create filter structure and compile filter expression using callbacks in options
 * First filter object is created then expr is compiled to internal representation.
 * Options callbacks provides following:
 * Lookup identifies the valid lvalue field names and associated filed data types.
 * Data callback sellects associated data for each identificator during evaluation
 * Rval_map callback provides translations to literal constants in value fileds eg.: "SSH"->22 etc.
 * \param[out] ff_filter  Address of pointer to filter object
 * \param[in]  expr       Filter expression
 * \param[in]  ff_options Associated options containig callbacks
 * \return FF_OK on success
 */
ff_error_t ff_init(ff_t **filter, const char *expr, ff_options_t *ff_options);

/**
 * \brief Evaluate filter on data
 * \param[in] ff_filter Compiled filter object
 * \param[in] rec       Data record in form readable to data callback
 * \return Nonzero on match
 */
int ff_eval(ff_t *filter, void *rec);

/**
 * \brief Release memory allocated for filter object and destroy it
 * \param[out] filter Compiled filter object
 * \return FF_OK on success
 */
ff_error_t ff_free(ff_t *filter);

/**
 * \brief Set error string to filter object
 * \param[in] filter Compiled filter object
 * \param[in] format Format string as used in printf
 * \param[in] ...
 */
void ff_set_error(ff_t *filter, char *format, ...);

/**
 * \brief Retrive last error set form filter object
 * \param[in]  filter Compiled filter object
 * \param[out] buf    Place where to copy error string
 * \param[in]  buflen Length of buffer available for error string
 * \return Pointer to copied error string
 */
const char* ff_error(ff_t *filter, const char *buf, int buflen);


#endif /* _LNF_FILTER_H */
