/*

 Copyright (c) 2015-2017, Imrich Stoffa, Tomas Podermanski

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
 * \file ffilter_internal.h
 * \brief netflow fiter private interface for helper functions and abstract syntax tree constructors
 */

#ifndef _FFILTER_INTERNAL_H
#define _FFILTER_INTERNAL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ffilter.h"

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

/* scanner instance */
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

YY_BUFFER_STATE ff_yy_scan_string(const char *str, yyscan_t yyscanner);
int ff_yyparse(yyscan_t yyscanner, ff_t *filter);
//int lnf_filter_yylex(YYSTYPE *yylval, void *scanner);

// error function for scanner
void yyerror(yyscan_t yyscanner, ff_t *filter, char *);

// conversion from string to numeric/bit value
unsigned get_unit(char *unit);
int64_t ff_strtoll(char *num, char**endptr, int *err);
uint64_t ff_strtoull(char *num, char**endptr, int *err);
int64_t saturate_int(int64_t num, ff_type_t type);
uint64_t saturate_uint(uint64_t num, ff_type_t type);

ff_error_t ff_type_validate(yyscan_t *scanner, ff_t *filter, char *valstr, ff_node_t* node, ff_lvalue_t* info);
ff_node_t* ff_transform_mval(yyscan_t *scanner, ff_t* filter, ff_node_t *node, ff_node_t *list, ff_lvalue_t* lvalue);

int str_to_uint(ff_t *filter, char *str, ff_type_t type, char **res, size_t *vsize);
int str_to_int(ff_t *filter, char *str, ff_type_t type, char **res, size_t *vsize);

int str_to_uint64(ff_t *filter, char *str, char **res, size_t *vsize);
int str_to_int64(ff_t *filter, char *str, char **res, size_t *vsize);
int str_to_real(ff_t *filter, char *str, char **res, size_t *vsize);
int str_to_mac(ff_t *filter, char *str, char **res, size_t *vsize);
int str_to_addr(ff_t *filter, char *str, char **res, size_t *vsize);
int str_to_timestamp(ff_t *filter, char *str, char **res, size_t *vsize);
int int_to_netmask(int numbits, ff_ip_t *mask);
char* unwrap_ip(char *ip_str, int numbits);

ff_error_t ff_type_cast(yyscan_t *scanner, ff_t *filter, char *valstr, ff_node_t* node);

// add new node into parse tree
ff_node_t* ff_duplicate_node(ff_node_t* original);
ff_node_t* ff_new_mval(yyscan_t scanner, ff_t *filter, char *valstr, ff_oper_t oper,  ff_node_t* nextptr);
ff_node_t* ff_new_leaf(yyscan_t scanner, ff_t *filter, char *fieldstr, ff_oper_t oper, char *valstr);
ff_node_t* ff_new_node(yyscan_t scanner, ff_t *filter, ff_node_t* left, ff_oper_t oper, ff_node_t* right);
ff_node_t* ff_branch_node(ff_node_t *node, ff_oper_t oper, ff_lvalue_t* lvalue);
int ff_oper_eval(char* buf, size_t size, ff_node_t *node);

// evaluate filter
int ff_eval_node(ff_t *filter, ff_node_t *node, void *rec);

// release memory allocated by nodes
void ff_free_node(ff_node_t* node);

// lex bison prototypes
int ff2_get_column(yyscan_t yyscanner);
void ff2_set_column(int , yyscan_t);
int ff2_lex_init(yyscan_t *yyscanner);
YY_BUFFER_STATE ff2__scan_string(const char *, yyscan_t yyscanner);
int ff2_parse(yyscan_t yyscanner, ff_t*);
int ff2_lex_destroy(yyscan_t yyscanner);

#endif

