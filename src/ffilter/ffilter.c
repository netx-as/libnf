/*

 Copyright (c) 2015-2017, Imrich Stoffa

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
 * \file ffilter.c
 * \brief netflow fiter implementation
 */
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <math.h>
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

#include "ffilter_internal.h"
#include "ffilter_gram.h"
#include "ffilter.h"
#include "fcore.h"

/// Formatting strings for operators
const char* ff_oper_str[FF_OP_TERM_] = {
		[FF_OP_EQ] = "EQ/=/==",
		[FF_OP_LT] = "LT/<",
		[FF_OP_GT] = "GT/>",
		[FF_OP_ISSET] = "LIKE/&"
};

/// Formatting strings for data types
const char* ff_type_str[FF_TYPE_TERM_] = {

		[FF_TYPE_UNSIGNED] = "uint",
		[FF_TYPE_TIMESTAMP] = "uint",
		[FF_TYPE_TIMESTAMP_BIG] = "uint",
		[FF_TYPE_UNSIGNED_BIG] = "net byte order uint",
		[FF_TYPE_UINT64] = "uint64",
		[FF_TYPE_UINT32] = "uint32",
		[FF_TYPE_UINT16] = "uint16",
		[FF_TYPE_UINT8] = "uint8",
		[FF_TYPE_SIGNED] = "int",
		[FF_TYPE_SIGNED_BIG] = "net byte order int",
		[FF_TYPE_INT64] = "int64",
		[FF_TYPE_INT32] = "int32",
		[FF_TYPE_INT16] = "int16",
		[FF_TYPE_INT8] = "int8",
		[FF_TYPE_DOUBLE] = "double",
		[FF_TYPE_ADDR] = "ip address",
		[FF_TYPE_MAC] = "mac address",
		[FF_TYPE_STRING] = "string",
		[FF_TYPE_MPLS] = "mpls labels"

};

int64_t saturate_int(int64_t num, ff_type_t type)
{
    switch (type) {
    case FF_TYPE_INT8:
        if (num > INT8_MAX) {
            num = INT8_MAX;
        } else if (num < INT8_MIN) {
            num = INT8_MIN;
        }
        break;

    case FF_TYPE_INT16:
        if (num > INT16_MAX) {
            num = INT16_MAX;
        } else if (num < INT16_MIN) {
            num = INT16_MIN;
        }
        break;

    case FF_TYPE_INT32:
        if (num > INT32_MAX) {
            num = INT32_MAX;
        } else if (num < INT32_MIN) {
            num = INT32_MIN;
        }
        break;

    default:
        break;
    }
    return num;
}

uint64_t saturate_uint(uint64_t num, ff_type_t type)
{
    switch (type) {
    case FF_TYPE_UINT8:
        if (num > UINT8_MAX) {
            num = UINT8_MAX;
        }
        break;

    case FF_TYPE_UINT16:
        if (num > UINT16_MAX) {
            num = UINT16_MAX;
        }
        break;

    case FF_TYPE_UINT32:
        if (num > UINT32_MAX) {
            num = UINT32_MAX;
        }
        break;

    default:
        break;
    }
    return num;
}

/**
 * \brief Convert unit character to positive power of 10
 * \param[in] unit Suffix of number
 * \return 0 on unknown, scale otherwise eg. (1k -> 1000) etc.
 */
unsigned get_unit(char *unit)
{
	if (strlen(unit) > 1)
		return 0;

	switch (*unit) {
	case 'k':
	case 'K':
		return FF_SCALING_FACTOR;
	case 'm':
	case 'M':
		return FF_SCALING_FACTOR * FF_SCALING_FACTOR;
	case 'g':
	case 'G':
		return FF_SCALING_FACTOR * FF_SCALING_FACTOR * FF_SCALING_FACTOR;
	default:
		return 0;
	}
}

/**
 * \brief Function adds conversion support for k/M/G suffixes to strtoull.
 * Also checks for successfull conversion
 * \param[in]  valstr Literal number
 * \param[out] endptr Place to store an address where conversion finised
 * \patam[out] err    Pointer to variable holding error code returned
 * \return Result of conversion.
 */
uint64_t ff_strtoull(char *valstr, char**endptr, int* err)
{
	uint64_t tmp64;
	uint64_t mult = 0;
    *err = 0;

	// Base 0 - given the string 0x is base 16 0x0 is 8 and no prefix is base 10
	if (valstr[0] == '-') {
		*err = EINVAL;
	}

    errno = 0;
	tmp64 = strtoull(valstr, endptr, 0);
        if (errno != 0) {
            *err = errno;
            *endptr = valstr;
            return 0;
        }

	if (!**endptr) {
		return tmp64;
	}

	// Allow one whitespace before unit
	if ((**endptr) == ' ') {
		(*endptr)++;
	}

	mult = get_unit(*endptr);
	if (mult != 0) {
	// Move conversion potinter after unit
		*endptr = (*endptr + 1);
	}

	if (mult != 0) {
		if (((tmp64 * mult) / mult) != tmp64) {
			*err = ERANGE;
		}
	}

	return tmp64*mult;
}

/**
 * \brief Function adds support for k/M/G suffixes to strtoll. Checks successfull conversion.
 * \param[in]  valstr Literal number
 * \param[out] endptr Place to store an address where conversion finised
 * \patam[out] err    Pointer to variable holding error code returned
 * \return Result of conversion.
 */
int64_t ff_strtoll(char *valstr, char**endptr, int* err)
{
	int64_t tmp64;
	int mult = 0;
    *err = 0;

    // Base 0 - given the string prefixes: 0x is base 16 0x0 is 8 and no prefix is base 10
    errno = 0;
	tmp64 = (int64_t) strtoll(valstr, endptr, 0);
	if (errno != 0) {
		*err = errno;
		*endptr = valstr;
		return 0;
	}

	if (!**endptr) {
		return tmp64;
	}

	// Allow one whitespace after unit
	if (*(*endptr) == ' ') {
		(*endptr)++;
	}

	mult = get_unit(*endptr);
	if (mult != 0) {
	// Move conversion potinter by one
		*endptr = (*endptr + 1);
	}

	if (mult != 0) {
		if (((tmp64 * mult) / mult) != tmp64) {
			*err = ERANGE;
		}
	}

	return tmp64*mult;
}

/// Convert string into uint64_t, also converts string with units (64k -> 64000)
int str_to_uint(ff_t *filter, char *str, ff_type_t type, char **res, size_t *vsize)
{
	uint64_t tmp64;
	void *ptr;
    int err = 0;

	char* endptr;
        tmp64 = ff_strtoull(str, &endptr, &err);
        if (err != 0) {
            if (err == ERANGE) {
                ff_set_error(filter, "Conversion failed, number \"%s\" out of range", str);
            } else if (err == EINVAL)
                ff_set_error(filter, "Conversion failed, bad characters in \"%s\"", str);
            return 1;
        }
	if (*endptr) {
		return 1;
	}

    uint64_t val = tmp64;

	switch (type) {
	case FF_TYPE_UINT64:
		*vsize = sizeof(uint64_t);
		break;
	case FF_TYPE_UINT32:
		*vsize = sizeof(uint32_t);
        val = saturate_uint(tmp64, type);
		break;
	case FF_TYPE_UINT16:
		*vsize = sizeof(uint16_t);
        val = saturate_uint(tmp64, type);
		break;
	case FF_TYPE_UINT8:
        *vsize = sizeof(uint8_t);
        val = saturate_uint(tmp64, type);
		break;
	default: return 1;
	}

    if (tmp64 != val) {
        ff_set_error(filter, "Conversion failed, value \"%s\" out of range for type %s", str, ff_type_str[type]);
        return 1;
    }

	ptr = malloc(*vsize);

	if (ptr == NULL) {
		return 1;
	}

    *res = ptr;

    switch (type) {
    case FF_TYPE_UINT64:
        *((uint64_t *) ptr) = tmp64;
        break;
    case FF_TYPE_UINT32:
        *((uint32_t *) ptr) = tmp64;
        break;
    case FF_TYPE_UINT16:
        *((uint16_t *) ptr) = tmp64;
        break;
    case FF_TYPE_UINT8:
        *((uint8_t *) ptr) = tmp64;
        break;
    default:
        free(ptr);
        return 1;
    }

	return 0;
}

/* convert string into int64_t */
/* also converts string with units (64k -> 64000) */
int str_to_int(ff_t *filter, char *str, ff_type_t type, char **res, size_t *vsize)
{

	int64_t tmp64;
	void *ptr;
	int err;

	char *endptr;
	tmp64 = ff_strtoll(str, &endptr, &err);
        if (err != 0) {
            if (err == ERANGE) {
                ff_set_error(filter, "Conversion failed, number \"%s\" out of range", str);
            } else if (err == EINVAL)
                ff_set_error(filter, "Conversion failed, bad characters in \"%s\"", str);
            return 1;
        }
        if (*endptr){
		return 1;
	}

    int64_t val = tmp64;

	switch (type) {
	case FF_TYPE_INT64:
		*vsize = sizeof(uint64_t);
		break;
	case FF_TYPE_INT32:
		*vsize = sizeof(uint32_t);
        val = saturate_int(tmp64, type);
		break;
	case FF_TYPE_INT16:
		*vsize = sizeof(uint16_t);
        val = saturate_int(tmp64, type);
		break;
	case FF_TYPE_INT8:
        *vsize = sizeof(uint8_t);
        val = saturate_int(tmp64, type);
		break;
	default: return 1;
	}

    if (tmp64 != val) {
        ff_set_error(filter, "Conversion failed, value \"%s\" out of range for type %s", str, ff_type_str[type]);
        return 1;
    }

    ptr = malloc(*vsize);

    if (ptr == NULL) {
        return 1;
    }

    *res = ptr;

    switch (type) {
    case FF_TYPE_INT64:
        *((int64_t *) ptr) = tmp64;
        break;
    case FF_TYPE_INT32:
        *((int32_t *) ptr) = tmp64;
        break;
    case FF_TYPE_INT16:
        *((int16_t *) ptr) = tmp64;
        break;
    case FF_TYPE_INT8:
        *((int8_t *) ptr) = tmp64;
        break;
    default:
        free(ptr);
        return 1;
    }

    return 0;
}

int str_to_uint64(ff_t *filter, char *str, char **res, size_t *vsize)
{
	return str_to_uint(filter, str, FF_TYPE_UINT64, res, vsize);
}

int str_to_int64(ff_t *filter, char *str, char **res, size_t *vsize)
{
	return str_to_int(filter, str, FF_TYPE_INT64, res, vsize);
}

/**
 *
 * \param filter
 * \param str
 * \param res
 * \param vsize
 * \return
 */
int str_to_real(ff_t *filter, char *str, char **res, size_t *vsize)
{
	double tmp64;
	void *ptr;

	char *endptr;
    int err = 0;
	tmp64 = strtod(str, &endptr);
    err = errno;

    if (*endptr) {
        ff_set_error(filter, "Conversion failed, bad characters in \"%s\"", str);
        return 1;
    }
    if (err == ERANGE) {
        if (tmp64 == HUGE_VALF || tmp64 == HUGE_VALL) {
            ff_set_error(filter, "Conversion failed, to real number, "
                "due to overflow/underflow \"%s\"", str);
            return 1;
        }
//        ff_set_error(filter, "Conversion warning, to real number, "
//           "value too small \"%s\"", str);
    }

	*vsize = sizeof(double);

	ptr = malloc(*vsize);

	if (ptr == NULL) {
		return 1;
	}

    *((double*) ptr) = tmp64;
	*res = ptr;

	return 0;
}

/**
 * \brief Transform mask representation from number of network bits format to full bit-mask.
 * \param numbits   Number of network portion bits
 * \param[out] mask Ip address containing full mask
 * \return Zero on succes
 */
int int_to_netmask(int numbits, ff_ip_t *mask)
{
	int retval = 0;
	if (numbits > 128 || numbits < 0) {
        retval = 1;
    }

	int x;
	for (x = 0; x < (numbits >> 5); x++) {
		mask->data[x] = ~0U;
	}

	if (x < 4) {
		uint32_t bitmask = ~0U;
		mask->data[x] = htonl(~(bitmask >> (numbits & 0x1f)));
	}
	return retval;
}

/**
 * \brief Pad necessary zeros to shortened ipv4 address to make it valid.
 * \param[in] ip_str  Shortened ip string
 * \param     numbits Number of network portion bits
 * \return Autocompleted network address in new string (must be freed) \see strdup
 */
char* unwrap_ip(char *ip_str, int numbits)
{
	char *endptr = ip_str;
	char suffix[8] = {0};
	int octet = 0;

	/* Check for required octets, note that inet_pton does the job of conversion
	   this is just to allow shortened notation of ip addresses eg 172.168/16 */

	int min_octets = (numbits >> 3) + ((numbits & 0x7) > 0);

	for (endptr = ip_str; endptr != NULL; octet++) {
		endptr = strchr(++endptr, '.');
	}

	if (octet < min_octets) {
		return NULL;
	}

	for (suffix[0] = 0 ; octet < 4; octet++) {
		strcat(suffix, ".0");
	}

	char *ip = NULL;
    size_t size;
	ip = malloc(size = (strlen(ip_str)+strlen(suffix)+1));

    if (ip == NULL) {
        return NULL;
    }

    snprintf(ip, size, "%s%s", ip_str, suffix);

	return ip;
}

/**
 * \brief Extended conversion from ip string to internal representation.
 * \param filter Filter object
 * \param str    String to convert
 * \param res    Data in converted from
 * \param size   Size of memory allocated for result
 * \return Zero on success
 */
//TODO: solve error blending...
int str_to_addr(ff_t *filter, char *str, char **res, size_t *size)
{
	ff_net_t *ptr;
	char *saveptr;

	char *ip;
	char *mask;
    // Guess ip version
	int ip_ver = 0;

	int numbits;

    char *ip_str = strdup(str);
	ptr = malloc(sizeof(ff_net_t));

    if (ip_str == NULL) {
        return 1;
    }
	if (ptr == NULL) {
		return 1;
	}

	memset(ptr, 0x0, sizeof(ff_net_t));
	numbits = 0;	//Not specified
	*res = (char *)ptr;

	ip = strtok_r(ip_str, "\\/ ", &saveptr);
	mask = strtok_r(NULL, "", &saveptr);

	if (mask == NULL) {
		// Mask was not given -> compare whole ip */
		memset(&ptr->mask, ~0, sizeof(ff_ip_t));

	} else {
		numbits = strtoul(mask, &saveptr, 10);

		// Conversion does not end after first number maybe full mask was given
		if (*saveptr) {
			numbits = 0;
			if (inet_pton(AF_INET, mask, &(ptr->mask.data[0]))) {
				ip_ver = 4;
			} else if (inet_pton(AF_INET6, mask, &ptr->mask.data)) {
				ip_ver = 6;
			} else {
				// Invalid mask
                ff_set_error(filter, "Conversion failed, invalid address mask \"%s\"", str);
				free(ptr);
				free(ip_str);
				return 1;
			}
        // Convert number to fill mask
		} else {
			// for ip v6 require ::0 if address is shortened;
			if (int_to_netmask(numbits, &(ptr->mask))) {
                ff_set_error(filter, "Conversion failed, invalid form of address/bits \"%s\"", str);
				free(ptr);
				free(ip_str);
				return 1;
			}
			// Try to unwrap ipv4 address
			ip = unwrap_ip(ip_str, numbits);
			if (ip) {
				ip_ver = 4;
				free(ip_str);
				ip_str = ip;
			} else {
				ip_ver = 6;
			}
		}
	}

	if (inet_pton(AF_INET, ip_str, &(ptr->ip.data[3])) && (numbits <= 32) && ip_ver != 6 ) {
		ptr->mask.data[3] = ptr->mask.data[0];
		ptr->mask.data[0] = 0;
		ptr->mask.data[1] = 0;
		ptr->mask.data[2] = 0;
		ptr->ver = 4;
	} else if (inet_pton(AF_INET6, ip_str, &ptr->ip)) {
		ptr->ver = 6;
	} else {
        ff_set_error(filter, "Conversion failed, bad character in address \"%s\"", str);
		free(ptr);
		free(ip_str);
		return 1;
	}

	for (int x = 0; x < 4; x++) {
		ptr->ip.data[x] &= ptr->mask.data[x];
	}

	free(ip_str);

	*res = (char*)&(ptr->ip);

	*size = sizeof(ff_net_t);
	return 0;
}

/**
 * \brief str_to_mac Decodes mac from string to array of chars function expects xx:xx:xx:xx:xx:xx
 * \param     filter
 * \param[in] str    literal containing mac address
 * \param     res
 * \param     size   number of bits allocated
 * \return zero on success
 */
int str_to_mac(ff_t *filter, char *str, char **res, size_t *size)
{
	char *ptr;

	ptr = malloc(sizeof(ff_mac_t));
	if (ptr == NULL) {
		return 1;
	}

	char *endptr = str;

	int ret = 1;
	uint32_t num = 0;
	for (int x = 0; x < 6; x++) {

		num = strtoul(endptr, &endptr, 16);
		if (num > 255) {
			break;
		}
		((char *)ptr)[x] = num;

		while (isspace(*endptr)) {
			endptr++;
		}

		if (*endptr == ':') {
			endptr++;
			while (isspace(*endptr)) {
				endptr++;
			}
		}
		if (isxdigit(*endptr)) { ;
		} else if (x == 5 && !*endptr) {
			ret = 0;
		} else {
			break;
		}
	}
	if (ret) {
        ff_set_error(filter, "Conversion failed, bad characters in mac address \"%s\"", str);
		free(ptr);
		*size = 0;
	} else {
		*res = ptr;
		*size = sizeof(ff_mac_t);
	}
	return ret;
}

int str_to_timestamp(ff_t *filter, char* str, char** res, size_t *size)
{
	struct tm tm;
	ff_timestamp_t timest;
	const char *res_ptr;

    memset(&tm, 0, sizeof(struct tm));

	res_ptr = strptime(str, "%F%n%T", &tm);

	if (res_ptr == NULL || *res_ptr != '\0') {
        ff_set_error(filter, "Conversion failed, bad characters in timestamp \"%s\"", str);
		return 1;
	}

	timest = (ff_timestamp_t)mktime(&tm);

    if (timest == -1) {
        ff_set_error(filter, "Conversion failed, cannot represent timestamp \"%s\"", str);
        return 1;
    }

	char *ptr = malloc(sizeof(ff_timestamp_t));
	if (!ptr) {
		return 1;
	}

	timest *= 1000;

    *size = sizeof(ff_timestamp_t);
    *((ff_timestamp_t *) ptr) = timest;
	*res = ptr;

	return 0;
}

ff_error_t ff_type_cast(yyscan_t *scanner, ff_t *filter, char *valstr, ff_node_t* node)
{
    // determine field type and assign data to lvalue */
	ff_val_t* tmp;
	switch (node->type) {
    /*
    case FF_TYPE_UINT64:
    case FF_TYPE_UINT32:
    case FF_TYPE_UINT16:
    case FF_TYPE_UINT8:
		if (str_to_uint(filter, valstr, FF_TYPE_UINT64, &node->value, &node->vsize)) {
			ff_set_error(filter, "Can't convert '%s' into numeric value", valstr);
			return FF_ERR_OTHER_MSG;
		}
		break;

	case FF_TYPE_INT64:
	case FF_TYPE_INT32:
	case FF_TYPE_INT16:
	case FF_TYPE_INT8:
		if (str_to_int(filter, valstr, FF_TYPE_INT64, &node->value, &node->vsize)) {
			ff_set_error(filter, "Can't convert '%s' into numeric value", valstr);
			return FF_ERR_OTHER_MSG;
		}
		break;
    */
	case FF_TYPE_MPLS:
		if (str_to_uint(filter, valstr, FF_TYPE_UINT32, &node->value, &node->vsize)) {
			ff_set_error(filter, "Can't convert '%s' into numeric value", valstr);
			return FF_ERR_OTHER_MSG;
		}
		tmp = calloc(1, sizeof(ff_mpls_t));
		if (!tmp) {
			return FF_ERR_NOMEM;
		}

		memcpy(&tmp->mpls.val, node->value, sizeof(uint32_t));
		free(node->value);
		node->value = (char*)tmp;
		node->vsize = sizeof(ff_mpls_t);
		break;

	case FF_TYPE_DOUBLE:
		if (str_to_real(filter, valstr, &node->value, &node->vsize)) {
			ff_set_error(filter, "Can't convert '%s' to real number", valstr);
			return FF_ERR_OTHER_MSG;
		}
		break;

	case FF_TYPE_ADDR:
		if (str_to_addr(filter, valstr, &node->value, &node->vsize)) {
			ff_set_error(filter, "Can't convert '%s' into IP address", valstr);
			return FF_ERR_OTHER_MSG;
		}
		break;

    // unsigned with undefined data size (internally mapped to uint64_t in network order) */
    case FF_TYPE_UINT64:
    case FF_TYPE_UINT32:
    case FF_TYPE_UINT16:
    case FF_TYPE_UINT8:
	case FF_TYPE_UNSIGNED_BIG:
	case FF_TYPE_UNSIGNED:
		if (str_to_uint64(filter, valstr, &node->value, &node->vsize)) {
			node->value = calloc(1, sizeof(uint64_t));
            if (!node->value) {
                return FF_ERR_NOMEM;
            }
			node->vsize = sizeof(uint64_t);

			if (filter->options.ff_rval_map_func == NULL) {
				node->vsize = 0;
				ff_set_error(filter, "Can't convert '%s' into numeric value, "
                    "missing literal mapping function", valstr);
				return FF_ERR_OTHER_MSG;

			} else if (filter->options.ff_rval_map_func(filter, valstr, node->type, node->field,
                node->value, &node->vsize) != FF_OK) {

				free(node->value);
				node->vsize = 0;
				ff_set_error(filter, "Can't map '%s' to numeric value", valstr);
				return FF_ERR_OTHER_MSG;
			}
		}
		break;

    case FF_TYPE_INT64:
    case FF_TYPE_INT32:
    case FF_TYPE_INT16:
    case FF_TYPE_INT8:
	case FF_TYPE_SIGNED_BIG:
	case FF_TYPE_SIGNED:
        if (str_to_int64(filter, valstr, &node->value, &node->vsize)) {
            ff_set_error(filter, "Can't convert '%s' into numeric value", valstr);
            return FF_ERR_OTHER_MSG;
        }
        break;

	case FF_TYPE_STRING:
		if ((node->value = strdup(valstr)) == NULL) {
			ff_set_error(filter, "Failed to duplicate string");
			return FF_ERR_NOMEM;
		}
		node->vsize = strlen(valstr);
		break;

	case FF_TYPE_MAC:
		if (str_to_mac(filter, valstr, &node->value, &node->vsize)) {
			ff_set_error(filter, "Can't convert '%s' into mac address", valstr);
			return FF_ERR_OTHER_MSG;
		}
		break;

	case FF_TYPE_TIMESTAMP_BIG:
	case FF_TYPE_TIMESTAMP:
		if (str_to_timestamp(filter, valstr, &node->value, &node->vsize)) {
			ff_set_error(filter, "Can't convert '%s' to timestamp", valstr);
			return FF_ERR_OTHER_MSG;
		}
		break;

	default:
		ff_set_error(filter, "Can't convert '%s' type is unsupported", valstr);
		return FF_ERR_OTHER_MSG;
	}

	return FF_OK;
}

ff_error_t ff_type_validate(yyscan_t *scanner, ff_t *filter, char *valstr, ff_node_t* node,
    ff_lvalue_t* info)
{
	ff_error_t retval;
	ff_attr_t valid;

	if ((retval = ff_type_cast(scanner, filter, valstr, node)) != FF_OK) {
		return retval;
	}

	if ((valid = ff_validate((ff_type_t)node->type, node->oper, node->value, info)) == FFAT_ERR) {

		ff_set_error(filter, "Semantic error: Operator %s is not valid for type %s",
            ff_oper_str[node->oper], ff_type_str[node->type]);

		return FF_ERR_OTHER_MSG;
	}
	node->type = info->type;
	node->opcode = valid;
	return FF_OK;
}

/* set error to error buffer */
/* set error string */
void ff_set_error(ff_t *filter, char *format, ...)
{
va_list args;

	va_start(args, format);
	vsnprintf(filter->error_str, FF_MAX_STRING - 1, format, args);
	va_end(args);
}

/* get error string */
const char* ff_error(ff_t *filter, const char *buf, int buflen)
{
	strncpy((char *)buf, filter->error_str, buflen - 1);
	return buf;
}

/**
 * \brief Build node tree with leaf nodes for each ff_external_id
 * \param[in] node - leaf used as template
 * \param     oper - FF_OP_AND or FF_OP_OR defines how structure will evaluate \see ff_oper_t
 * \param[in] lvalue - Info about field
 * \return Root node of new subtree or NULL on error
 */
ff_node_t* ff_branch_node(ff_node_t *node, ff_oper_t oper, ff_lvalue_t* lvalue)
{
	ff_node_t *leaf[FF_MULTINODE_MAX] = {NULL};
	int err;
    int forked;

    err = 0;
    forked = 1;
    /// Save template node
	leaf[0] = node;

	for (int x = 1; (x < FF_MULTINODE_MAX && lvalue->id[x].index); x++) {
        leaf[x] = NULL;
		leaf[x] = ff_duplicate_node(node);
		if (leaf[x]) {
			leaf[x]->field = lvalue->id[x];
            forked++;
		} else {
			err = 1;
			;
		}
	}

    if (err) {
        for (int x = 0; (x < FF_MULTINODE_MAX && lvalue->id[x].index); x++) {
            ff_free_node(leaf[x]);
        }
        return NULL;
    }

    /// Build tree
	while (forked > 1) {
		for (int i = 0; i < forked; i += 2) {
			node = ff_new_node(NULL, NULL, leaf[i], oper, leaf[i+1]);
			if (!node) {
                /* Failed to create new node, kill also children, tree structure however is
                   preserved for later full free */
				ff_free_node(leaf[i]);
				ff_free_node(leaf[i+1]);
                err = 1;
			}
			leaf[i >> 1] = node;
		}
		forked = forked >> 1;
	}

    if (err) {
        /// Free form root of newly allocated subtree if error occured
        ff_free_node(leaf[0]);
        return NULL;
    }

	return leaf[0];
}

ff_node_t* ff_duplicate_node(ff_node_t* original)
{
	ff_node_t *copy, *lc, *rc;
	lc = NULL;
    rc = NULL;

	if (original->left) {
		lc = ff_duplicate_node(original->left);
		if (!lc) {
			return NULL;
		}
	}
	if (original->right) {
		rc = ff_duplicate_node(original->right);
		if (!rc) {
			ff_free_node(lc);
			return NULL;
		}
	}

	copy = malloc(sizeof(ff_node_t));

	if (copy == NULL) {
		ff_free_node(lc);
		ff_free_node(rc);
		return NULL;
	}

	memcpy(copy, original, sizeof(ff_node_t));

	if (original->vsize > 0) {
		copy->value = malloc(original->vsize);

		if (!copy->value) {
            copy->vsize = 0;
            ff_free_node(copy);
            return NULL;
		} else {
            memcpy(copy->value, original->value, original->vsize);
		}
	}

	copy->left = lc;
	copy->right = rc;

	return copy;
}

ff_node_t* ff_transform_mval(yyscan_t *scanner, ff_t* filter, ff_node_t *node, ff_node_t *list,
    ff_lvalue_t* lvalue)
{
    void* tmp;
    int err;

    // Connect it to right subtree
    node->right = list;
    // Terminate other subtree, nothing should be there anyway
    node->left = NULL;

    // Now process all items
    do {
        // Copy type and field or original
        list->type = node->type;
        list->field = node->field;
        // Cast stringstmp = list->value
        tmp = list->value;
        err = ff_type_validate(scanner, filter, list->value, list, lvalue);
        if(err == FF_OK) {
            list = list->right;
        } else {
            // Failed to transform data in mval list node, do not free them
            list->vsize = 0;
            free(tmp);
            break;
        }
        // Free string from mval list
        free(tmp);
    } while (list);

    if (err != FF_OK) {
        return NULL;
    }

    return node;
}

/* Add leaf entry into expr tree */
ff_node_t* ff_new_leaf(yyscan_t scanner, ff_t *filter, char *fieldstr, ff_oper_t oper, char *valstr)
{
	ff_node_t *node;
	ff_node_t *retval;
	ff_lvalue_t lvalue;

	ff_oper_t root_oper;

	retval = NULL;

	/* callback to fetch field type and additional info */
	if (filter->options.ff_lookup_func == NULL) {
		ff_set_error(filter, "Filter lookup function not defined");
		return NULL;
	}

	memset(&lvalue, 0x0, sizeof(ff_lvalue_t));

	switch (*fieldstr) {
	case '|':
		root_oper = FF_OP_OR;
		fieldstr++;
		break;
	case '&':
		root_oper = FF_OP_AND;
		fieldstr++;
		break;
	default:
        root_oper = FF_OP_UNDEF;
		break;
	}

	do { /* Break on error */
		if (filter->options.ff_lookup_func(filter, fieldstr, &lvalue) != FF_OK) {
			ff_set_error(filter, "Can't lookup field type for \"%s\"", fieldstr);
			retval = NULL;
			break;
		}

			/* Change evaluation operator when no operator was specified */
		if (oper == FF_OP_NOOP) {
			if (lvalue.options & FF_OPTS_FLAGS) {
				oper = FF_OP_ISSET;
			} else if (lvalue.type == FF_TYPE_STRING ) {
				oper = FF_OP_ISSET;
			} else {
				oper = FF_OP_EQ;
			}
		}

		node = ff_new_node(scanner, filter, NULL, oper, NULL);
		if (node == NULL) {
			retval = NULL;
			break;
		}

		node->type = lvalue.type;
		node->field = lvalue.id[0];

		retval = node;

		// If node contains in list
		if (oper == FF_OP_IN) {

            retval = ff_transform_mval(scanner, filter, node, (ff_node_t*)valstr, &lvalue);
            // Transformation failed
            if (retval == NULL) {
                ff_free_node(node);
                break;
            }

		// Normal behavior, convert one value
		} else if (*valstr == '\0' || (ff_type_validate(scanner, filter, valstr, node, &lvalue) != FF_OK)) {

			if (oper == FF_OP_EXIST) {
				// OP exist does not need value
                ;
			} else if (lvalue.options & FF_OPTS_CONST) {
                // Also pass if for constant there is a default value
                if (lvalue.literal == NULL) {
                    ff_set_error(filter, "No constant set for field \"%s\"", fieldstr);
                    ff_free_node(node);
                    retval = NULL;
                    break;
                }
                if (ff_type_validate(scanner, filter, (char *)lvalue.literal, node, &lvalue) != FF_OK) {
                    char errstr[32];
                    ff_set_error(filter, "Constant conversion failed for field \"%s\", %s", fieldstr,
                        ff_error(filter, errstr, 32 ));
                    ff_free_node(node);
                    retval = NULL;
                    break;
                }
                ;
			} else {
                ff_free_node(node);
				retval = NULL;
				break;
			}
		}

		if (lvalue.id[1].index != 0) {
			// Setup nodes in or configuration for pair fields (src/dst etc.)
			ff_node_t* new_root;
			new_root = ff_branch_node(node, root_oper == FF_OP_UNDEF ? FF_OP_OR : root_oper,
                &lvalue);

			if (new_root == NULL) {
                ff_set_error(filter, "Failed to allocate node!");
				ff_free_node(node);
				break;
			}
			retval = new_root;
		}
	} while (0);

	return retval;
}

/* add node entry into expr tree */
ff_node_t* ff_new_node(yyscan_t scanner, ff_t *filter, ff_node_t* left, ff_oper_t oper, ff_node_t* right) {

	ff_node_t *node;
    node = NULL;

    // not has only one child
    if (oper == FF_OP_NOT) {
        if (right && !left) {
            node = right;
        } else if (left && !right) {
            node = left;
        } else {
	        ff_set_error(filter, "Failed, not operator node requires only one child.");
            return NULL;
        }

        if (node) {
            ff_attr_t neg;
            if (FFAT_ERR != (neg = ff_negate(node->opcode))) {
                node->opcode = neg;
                return node;
            }
        }
    }

	node = malloc(sizeof(ff_node_t));

	if (node == NULL) {
        ff_set_error(filter, "Failed to allocate node!");
		return NULL;
	}

	node->vsize = 0;
	node->type = 0;
	node->oper = oper;

	node->left = left;
	node->right = right;

	return node;
}

/* add new item to list */
//TODO: Solve leak from line 1090
ff_node_t* ff_new_mval(yyscan_t scanner, ff_t *filter, char *valstr, ff_oper_t oper, ff_node_t* nptr)
{

	ff_node_t *node;

	node = malloc(sizeof(ff_node_t));

	if (node == NULL) {
        ff_set_error(filter, "Failed to allocate node!");
		return NULL;
	}
    node->value = NULL;

	node->vsize = strlen(valstr);
	node->value = (char*)malloc(sizeof(char) * (node->vsize + 1));
	node->type = FF_TYPE_STRING;
	node->oper = oper;
	node->left = NULL;
	node->right = nptr;

    if (!node->value) {
        ff_set_error(filter, "Failed to allocate node!");
        node->vsize = 0;
        ff_free_node(node);
        return NULL;
    }

    snprintf(node->value, node->vsize + 1, "%s", valstr);

	return node;
}

/**
 * \brief Evaluate node in tree or proces subtree.
 * \param filter
 * \param node   Root of abstract syntax tree of expression
 * \param rec    One "line" of record in format known to adapter \see ff_data_func
 * \return 0 - false; 1 - true; -1 - error  */
int ff_eval_node(ff_t *filter, ff_node_t *node, void *rec)
{
#define EVAL_BUF_MAX 128

    // Since function is reentrant, alocate own buffer
	char buf[EVAL_BUF_MAX];
    size_t size = EVAL_BUF_MAX;
    // Data from callback can be returned in provided buffer or from adapter
    char* data = &buf[0];

	int left, right, res, exist;

	if (node == NULL) {
		return -1;
	}

	exist = 1;
	left = 0;

	if (node->oper == FF_OP_YES) return 1;

	// go deeper into tree
	if (node->left != NULL ) {
		left = ff_eval_node(filter, node->left, rec);

		// do not evaluate if the result is obvious
		if (node->oper == FF_OP_NOT)			    { return left <= 0; };
		if (node->oper == FF_OP_OR  && left > 0)	{ return 1; };
		if (node->oper == FF_OP_AND && left <= 0)	{ return 0; };
	}

	if (node->right != NULL ) {
		right = ff_eval_node(filter, node->right, rec);

		switch (node->oper) {
		case FF_OP_NOT: return right <= 0;
		case FF_OP_OR:  return left > 0 || right > 0;
		case FF_OP_AND: return left > 0 && right > 0;
		default: break;
		}
	}

	// operations on leaf -> compare values
	// going to be callback
	if (filter->options.ff_data_func(filter, rec, node->field, &data, &size) != FF_OK) {
		// ff_set_error(filter, "Can't get data");
		// On no data mimic zero
		memset(buf, 0, node->vsize);
        data = buf;
        size = node->vsize;
		exist = 0;
	}

	switch (node->oper) {
    default:
        return ff_oper_eval_V2(data, size, node);

    // Check for presence of item
	case FF_OP_EXIST:
        return exist;

	// Compare against list (right branch is NULL) data retireved once
	case FF_OP_IN:
		node = node->right;
		do {
			res = ff_oper_eval_V2(data, size, node);
			node = node->right;
		 } while (res <= 0 && node);
        return res;

	case FF_OP_NOT:
	case FF_OP_OR:
	case FF_OP_AND:
        return -1;
	}
}

ff_error_t ff_options_init(ff_options_t **poptions) {

	ff_options_t *options;

	options = calloc(1, sizeof(ff_options_t));

	if (options == NULL) {
		*poptions = NULL;
		return FF_ERR_NOMEM;
	}

	*poptions = options;

	return FF_OK;
}

/**
 * \brief Release all resources allocated by filter options
 * \param[in/out] options Option callbacks structure
 */
ff_error_t ff_options_free(ff_options_t *options) {

	// !!! memory cleanup
	free(options);

	return FF_OK;
}


ff_error_t ff_init(ff_t **pfilter, const char *expr, ff_options_t *options) {

	yyscan_t scanner;
	//YY_BUFFER_STATE buf;
	int parse_ret;
	ff_t *filter;

	filter = malloc(sizeof(ff_t));
	*pfilter = NULL;

	if (filter == NULL) {
		return FF_ERR_NOMEM;
	}

	filter->root = NULL;

	if (options == NULL) {
		free(filter);
		return FF_ERR_OTHER;
	}

	memcpy(&filter->options, options, sizeof(ff_options_t));

	ff_set_error(filter, "No Error.");

	ff2_lex_init(&scanner);
    // Buff is unused for now
	//buf = ff2__scan_string(expr, scanner);
    ff2__scan_string(expr, scanner);
	parse_ret = ff2_parse(scanner, filter);


	ff2_lex_destroy(scanner);

	/* error in parsing */
	if (parse_ret != 0) {
		*pfilter = filter;
		return FF_ERR_OTHER_MSG;
	}

	*pfilter = filter;

	return FF_OK;
}

/* matches the record against filter */
/* returns 1 - record was matched, 0 - record wasn't matched */
int ff_eval(ff_t *filter, void *rec) {

	/* call eval node on root node */
	return ff_eval_node(filter, filter->root, rec) > 0;
}

/* recursively release all resources allocated in filter tree */
void ff_free_node(ff_node_t* node) {

	if (node == NULL) {
		return;
	}

	ff_free_node(node->left);
	ff_free_node(node->right);

	if(node->vsize > 0) {
		free(node->value);
	}

	free(node);
}

/* release all resources allocated by filter */
ff_error_t ff_free(ff_t *filter) {

	/* !!! memory cleanup */
	if (filter != NULL) {
		ff_free_node(filter->root);
	}
	free(filter);

	return FF_OK;
}

