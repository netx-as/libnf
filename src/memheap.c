
#define _LIBNF_C_ 1

#include "config.h"
#include "libnf.h"

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

#include "libnf_internal.h"
#include "libnf.h"


/* initialize memory heap structure */
int lnf_mem_init(lnf_mem_t **lnf_mem) {
	return LNF_OK;
}

/* 
* LNF_PREFIXv4
* LNF_PREFIXv6
*/

int lnf_mem_add_aggr(lnf_mem_t *lnf_mem, int field, int bitsize, int flags) {

}

int lnf_mem_add_sort(lnf_mem_t *lnf_mem, int field, int flags) {

}

