

#include <libnf.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <flist.h>
#include <time.h>
#include <pthread.h>
#include "output.h"
#include "output_line.h"

#define UNIT_1K (double)(1000.0)
#define UNIT_1M (double)(1000.0 * 1000.0)
#define UNIT_1G (double)(1000.0 * 1000.0 * 1000.0)
#define UNIT_1T (double)(1000.0 * 1000.0 * 1000.0 * 1000.0)

/* format big bumber with units */
static void num_unit(char *buff, double num) {

	if (num >= UNIT_1T) {
		sprintf(buff, "%.1f T", num / UNIT_1T);
	} else if (num >= UNIT_1G) {
		sprintf(buff, "%.1f G", num / UNIT_1G);
	} else if (num >= UNIT_1M) {
		sprintf(buff, "%.1f M", num / UNIT_1M);
	} else if (num >= UNIT_1K) {
		sprintf(buff, "%.1f K", num / UNIT_1K);
	} else {
		sprintf(buff, "%.1f", num);
	}
}

/* function for print number */
static void format_uint64_unit(char *buff, char *data) {
	num_unit(buff, *((uint64_t *)data));
}

/* basic functions for print number */
static void format_uint64(char *buff, char *data) {
	sprintf(buff, " %llu", (long long unsigned)*((uint64_t *)data));
}

static void format_uint32(char *buff, char *data) {
	sprintf(buff, " %u", *((uint32_t *)data));
}

static void format_uint16(char *buff, char *data) {
	sprintf(buff, " %hu", *((uint16_t *)data));
}

static void format_uint8(char *buff, char *data) {
	sprintf(buff, " %hhu", *((uint8_t *)data));
}

/* function for print float */
static void format_double(char *buff, char *data) {
	num_unit(buff, *((double *)data));
}

/* function for print duration */
static void format_duration(char *buff, char *data) {
	sprintf(buff, " %1.3f", *((uint64_t *)data) / 1000.0);
}

/* format date/time */
static void format_date(char *buff, char *data) {
	time_t sec;
	int msec;
	struct tm *ts;
	char buff2[MAX_STR];

	sec = *((uint64_t *)data) / 1000LL;	
	msec = *((uint64_t *)data) - sec * 1000LL;
	ts = localtime(&sec);
	strftime(buff, MAX_STR, "%Y-%m-%d %H:%M:%S", ts);
	sprintf(buff2, ".%03d", msec);
	strcat(buff, buff2);
}

/* function for IPv4/IPv6 address */
static void format_addr(char *buff, char *data) {
	lnf_ip_t *addr = (lnf_ip_t *)data;

	if (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)addr)) {
		inet_ntop(AF_INET, (char *)&(addr->data[3]), buff, MAX_STR);
	} else {
		inet_ntop(AF_INET6, addr, buff, MAX_STR);
	}
}


/* defines format ptions for types and fields */
/* type 0 and field 0 defines default value  */
typedef struct format_ent_s {
	int type;
	int field;
	char format[MAX_STR];		/* column width */
	format_func_t format_func;
} format_ent_t;

const format_ent_t formats[] = { 
	{ 0,          0, "%s ", NULL }, 				/* default for all types */
	{ LNF_UINT64, 0, "%8s ", format_uint64_unit },	/* default for uint64_t */
	{ LNF_UINT32, 0, "%8s ", format_uint32 },	/* default for uint32_t */
	{ LNF_UINT16, 0, "%8s ", format_uint16 },	/* default for uint16_t */
	{ LNF_UINT8,  0, "%8s ", format_uint8 },	/* default for uint8_t */
	{ LNF_ADDR,   0, "%17s ", format_addr }, 				/* default for all LNF_ADDR */
	{ LNF_MAC,    0, "%10s ", NULL },  				/* default for all LNF_MAC */
	{ LNF_UINT64, LNF_FLD_FIRST, "%23s ", format_date }, 
	{ LNF_UINT64, LNF_FLD_LAST, "%10s ", format_date }, 
	{ LNF_UINT64, LNF_FLD_CALC_DURATION, "%9s ", format_duration }, 
	{ LNF_UINT64, LNF_FLD_DPKTS, "%9s ", format_uint64_unit }, 
	{ LNF_DOUBLE, LNF_FLD_CALC_BPS, "%8s ", format_double }, 
	{ LNF_DOUBLE, LNF_FLD_CALC_PPS, "%8s ", format_double }, 
	{ LNF_DOUBLE, LNF_FLD_CALC_BPP, "%7s ", format_double }, 
	{ LNF_UINT64, LNF_FLD_AGGR_FLOWS, "%5s ", format_uint64 } /* aggr flow withou unit */
};

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

int output_start_line(output_t *output) {
	int i,j;
	char buf[LNF_INFO_BUFSIZE];
	field_ent_t *fe;
	format_ent_t fmte;

	for (i = 0; i < output->numfields; i++) {

		fe = &output->fields[i];	/* get field entry for better manipulation */

		/* set output formating function and formating callback */
		for (j = 0; j < sizeof(formats) / sizeof(format_ent_t); j++) {
			/* default for all */
			if ( formats[j].type == 0 ) {
				memcpy(&fmte, &formats[j], sizeof(format_ent_t));
			}

			/* default for type */
			if ( formats[j].type == lnf_fld_type(fe->field) && formats[j].field == 0) {
				memcpy(&fmte, &formats[j], sizeof(format_ent_t));
			}

			/* particular type and field */
			if ( formats[j].type == lnf_fld_type(fe->field) && formats[j].field == fe->field) {
				memcpy(&fmte, &formats[j], sizeof(format_ent_t));
			}
		}

		strncpy(fe->format, fmte.format, MAX_STR);
		fe->format_func = fmte.format_func;

		/* print header */
		lnf_fld_info(fe->field, LNF_FLD_INFO_NAME, &buf, LNF_INFO_BUFSIZE);
		strncpy(fe->name, buf, MAX_STR);
		printf(output->fields[i].format, &buf);

	}

	printf("\n");

	return 1;
}

int output_row_line(output_t *output, lnf_rec_t *rec) {
	int i; 
	char buf[MAX_STR];
	char str[MAX_STR];
	char str2[MAX_STR];
	char row[MAX_STR_LONG];

	row[0] = '\0';

	for (i = 0; i < output->numfields; i++) {
		lnf_rec_fget(rec, output->fields[i].field, buf);
		if (output->fields[i].format_func != NULL) {
			output->fields[i].format_func(str, buf);
		} else {
			strcpy(str, "<?>");
		}
		sprintf(str2, output->fields[i].format, str);
		strcat(row, str2);
	}

	strcat(row, "\n");

	pthread_mutex_lock(&print_mutex);
	//printf("%s\n", row);
	fputs(row, stdout);
	pthread_mutex_unlock(&print_mutex);

	return 1;
}

int output_row_raw(output_t *output, lnf_rec_t *rec) {
	int i; 
	char buf[MAX_STR];
	char str[MAX_STR];
	char str2[MAX_STR];
	char row[MAX_STR_LONG];

	pthread_mutex_lock(&print_mutex);
	fputs("\nFlow Record:\n", stdout);
	pthread_mutex_unlock(&print_mutex);

	for (i = 0; i < output->numfields; i++) {

		row[0] = '\0';

		lnf_rec_fget(rec, output->fields[i].field, buf);
		if (output->fields[i].format_func != NULL) {
			output->fields[i].format_func(str, buf);
		} else {
			strcpy(str, "<?>");
		}
		sprintf(str2, "  %-10s = ", output->fields[i].name);
		strcat(row, str2);
		sprintf(str2, output->fields[i].format, str);
		strcat(row, str2);

		strcat(row, "\n");

		pthread_mutex_lock(&print_mutex);
		fputs(row, stdout);
		pthread_mutex_unlock(&print_mutex);

	}

	return 1;
}

int output_finish_line(output_t *output) {

	return 1;

}

