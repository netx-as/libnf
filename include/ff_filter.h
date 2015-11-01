/* 

 Copyright (c) 2015, Tomas Podermanski, Lukas Hutak 
    
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

/*! \file ff_filter.h
    \brief netflow fiter implementation - C interface
*/
#ifndef _FLOW_FILTER_H_
#define _FLOW_FILTER_H_

#include <stdint.h>
#include <stddef.h>

#define FF_MAX_STRING  1024


/*! \brief Supported data types */
typedef enum {
	FF_TYPE_UNSUPPORTED,  // for unsupported data types
#define FF_TYPE_UNSUPPORTED_T void
//	FF_TYPE_SIGNED,
//	FF_TYPE_UNSIGNED,
	FF_TYPE_UINT8,
	FF_TYPE_UINT16,
	FF_TYPE_UINT32,
	FF_TYPE_UINT64,
#define FF_TYPE_UNSIGNED_T unit64_t
	FF_TYPE_FLOAT,        // TODO: muzeme si byt jisti, ze se bude pouzivat format IEEE 754?
#define FF_TYPE_FLOAT_T double
	FF_TYPE_ADDR,
#define FF_TYPE_ADDR_T ?
	FF_TYPE_MAC,
#define FF_TYPE_MAC_T char[8]
	FF_TYPE_STRING,
#define FF_TYPE_STRING_T char*
	FF_TYPE_MPLS,
#define FF_TYPE_MPLS_T unit32_t[10]
	FF_TYPE_TIMESTAMP     // jaky format??
#define FF_TYPE_TIMESTAMP_T unit64_t
} ff_type_t;

typedef enum {
	FF_OK = 0x1,
	FF_ERR_NOMEM = -0x1,
	FF_ERR_UNKN  = -0x2,
	FF_ERR_UNSUP  = -0x3,
	FF_ERR_OTHER  = -0xE,
	FF_ERR_OTHER_MSG  = -0xF,
} ff_error_t;


/*! \brief External identification of value */
typedef union {
	uint64_t index;       /**< Index mapping      */
	const void *ptr;      /**< Direct mapping     */
} ff_extern_id_t;



/** \brief Identification of left value */
typedef struct ff_lvalue_s {
	/** Type of left value */
	ff_type_t type;
	/** External identification */
	ff_extern_id_t id;
	ff_extern_id_t id2;	/* for pair fields  */

	int options;

	
	// POZN: velikost datoveho typu nemuze byt garantovana IPFIXcolem a muze
	//       se lisit v zavislosti na velikostech dat posilanych exporterem
	//       -> velikost dat si bude muset zjistit komparacni funkce a podle 
	//       toho se bude muset zachovat
} ff_lvalue_t;


typedef struct ff_filter_s ff_filter_t;

/** \brief Function pointer on element lookup function
 *
 * - first: Name of the element
 * - returns: lvalue identification
 */
typedef ff_error_t (*ff_lookup_func_t) (ff_filter_t *, const char *, ff_lvalue_t *);
typedef ff_error_t (*ff_data_func_t) (ff_filter_t *, void *, ff_extern_id_t, char*, size_t);



/** \brief Filter instance */
typedef struct ff_filter_s {
	
	/** Element lookup function */
	ff_lookup_func_t ff_lookup_func;
	/** Value comparation function */
	ff_data_func_t ff_data_func;

	/** Root node */
	void *root;

	char error_str[FF_MAX_STRING];

} ff_filter_t;



//ff_error_t ff_filter_parse(ff_filter_t *ff_filter, const char *expr, ff_lookup_func func_lookup, ff_data_func func_data);
ff_error_t ff_filter_init(ff_filter_t *ff_filter);
ff_error_t ff_filter_parse(ff_filter_t *ff_filter, const char *expr);
int ff_filter_eval(ff_filter_t *filter, void *rec);
ff_error_t ff_filter_free(ff_filter_t *filter);

void ff_filter_seterr(ff_filter_t *filter, char *format, ...);
void ff_filter_error(ff_filter_t *filter, const char *buf, int buflen);


#endif /* _LNF_FILTER_H */
