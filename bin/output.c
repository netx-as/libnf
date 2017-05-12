

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
#include "output_nfdump.h"
#include "output_ringbuf.h"


void output_init(output_t *output) {

	memset(output, 0x0, sizeof(output_t));

	output->output_start_func = output_start_line;
	output->output_row_func = output_row_line;
	output->output_finish_func = output_finish_line;

	output->recp = NULL;
	output->memp = NULL;
	output->sortfield = 0;
	pthread_mutex_init(&output->write_lock, NULL);

}


void output_set_fmt(output_t *output, output_fmt_t output_fmt, char *filename) {

	output->output_fmt = output_fmt;

	switch (output_fmt) {

		case OFMT_BIN_RINGBUF:
				output->output_start_func = output_start_ringbuf;
				output->output_row_func = output_row_ringbuf;
				output->output_finish_func = output_finish_ringbuf;
				output->ringname = RINGBUF_NAME;
				break;
		case OFMT_BIN_NFDUMP:
				output->output_start_func = output_start_nfdump;
				output->output_row_func = output_row_nfdump;
				output->output_finish_func = output_finish_nfdump;
				break;
		case OFMT_RAW:
				output->output_start_func = output_start_line;
				output->output_row_func = output_row_raw;
				output->output_finish_func = output_finish_line;
				break;
		default:
				output->output_start_func = output_start_line;
				output->output_row_func = output_row_line;
				output->output_finish_func = output_finish_line;
				break;
	}

	if (filename != NULL) {
		output->filename = malloc(strlen(filename) + 1);
		if (output->filename != NULL) {
			strcpy(output->filename, filename);
		}
	}

}

void output_set_sort(output_t *output, int sortfield, int sortbits4, int sortbits6) {

	output->sortfield = sortfield;
	output->sortbits4 = sortbits4;
	output->sortbits6 = sortbits6;

}

void output_set_limit(output_t *output, int limit) {

	output->limit = limit;

}

int output_merge_threads(output_t *output) {

	if ( output->memp != NULL ) {
		return lnf_mem_merge_threads(output->memp);
	}

	return 1;
}



int output_start(output_t *output) {

	/* aggregated or not aggregated records */
	if (output->memp == NULL) {
		/* not aggregated, but sorted */
		if (output->sortfield > 0) {
			if (lnf_mem_init(&output->memp) != LNF_OK) {
				return 0;
			}
			/* switch memp into list mode */
			lnf_mem_setopt(output->memp, LNF_OPT_LISTMODE, NULL, 0);
			lnf_mem_fastaggr(output->memp, LNF_FAST_AGGR_BASIC);
			lnf_mem_fadd(output->memp, LNF_FLD_PROT, LNF_AGGR_KEY, 0, 0);
			lnf_mem_fadd(output->memp, LNF_FLD_SRCADDR, LNF_AGGR_KEY, 24, 128);
			lnf_mem_fadd(output->memp, LNF_FLD_SRCPORT, LNF_AGGR_KEY, 0, 0);
			lnf_mem_fadd(output->memp, LNF_FLD_DSTADDR, LNF_AGGR_KEY, 24, 128);
			lnf_mem_fadd(output->memp, LNF_FLD_DSTPORT, LNF_AGGR_KEY, 0, 0);
		}
		output_field_add(output, LNF_FLD_PROT);
		output_field_add(output, LNF_FLD_SRCADDR);
		output_field_add(output, LNF_FLD_SRCPORT);
		output_field_add(output, LNF_FLD_DSTADDR);
		output_field_add(output, LNF_FLD_DSTPORT);
	}

    /* default fields on the ond of the list */
    output_field_add(output, LNF_FLD_DPKTS);
    output_field_add(output, LNF_FLD_DOCTETS);
    output_field_add(output, LNF_FLD_CALC_PPS);
    output_field_add(output, LNF_FLD_CALC_BPS);
    output_field_add(output, LNF_FLD_CALC_BPP);
    output_field_add(output, LNF_FLD_AGGR_FLOWS);

    /* set sort firld */
    if (output->sortfield > 0) {
        int defaultaggr = 0;
        int defaultsort = 0;
        lnf_fld_info(output->sortfield, LNF_FLD_INFO_AGGR, &defaultaggr, sizeof(int));
        lnf_fld_info(output->sortfield, LNF_FLD_INFO_SORT, &defaultsort, sizeof(int));
        lnf_mem_fadd(output->memp, output->sortfield, defaultaggr|defaultsort, output->sortbits4, output->sortbits6);
    }

	return output->output_start_func(output);

}

/* add record to output (either add to lnf_mem or orint out */
int output_write(output_t *output, lnf_rec_t *rec) {

	pthread_mutex_lock(&output->write_lock);
	if (output->memp != NULL) {
		if ( lnf_mem_write(output->memp, rec) == LNF_OK ) {
			pthread_mutex_unlock(&output->write_lock);
			return 1;
		}
	} else {
		if ( output_row(output, rec) == LNF_OK ) {
			output->outputflows++;
			pthread_mutex_unlock(&output->write_lock);
			return 1;
		}
	}	

	pthread_mutex_unlock(&output->write_lock);
	return 0;

}


int output_row(output_t *output, lnf_rec_t *rec) {

	return output->output_row_func(output, rec);

}

int output_output_rows(output_t *output) {

	int i;

	/* print the records out */
	if (output->memp != NULL) {
		i = 0;
		lnf_rec_init(&output->recp);
		while (lnf_mem_read(output->memp, output->recp) != LNF_EOF) {
			i++;
			output->outputflows++;
			output_row(output, output->recp);

			/* if the limit of output rows is set */
			if (output->limit > 0 && output->outputflows >= output->limit) {
				break;
			}
		}
	}

	return 0;
}



int output_finish(output_t *output) {

	int ret;

	ret =  output->output_finish_func(output);

	if (ret) {
		lnf_mem_free(output->memp);
		lnf_rec_free(output->recp);
		return ret;
	} else {
		return 0;
	}

}


int output_field_add(output_t *output, int field) {

	int i;

	/* check whether the field is in list */
	for (i = 0; i < output->numfields; i++) {
		if (output->fields[i].field == field ) {
			return 0;
		}
	}

	if (field < LNF_FLD_TERM_) {
		output->fields[output->numfields++].field = field;
		return 1;
	} else {
		return 0;
	}
}

/* parse argument given by -A */
int output_parse_aggreg(output_t *output, char *str) {

	char *token = str;
	int field, numbits, numbits6;

	if (output->memp == NULL) {
		if (lnf_mem_init(&output->memp) != LNF_OK) {
			return 0;
		}
	}

	/* default fields on the begining of the list */
//	lnf_mem_fastaggr(memp, LNF_FAST_AGGR_BASIC);
	output_field_add(output, LNF_FLD_FIRST);
	output_field_add(output, LNF_FLD_CALC_DURATION);
	lnf_mem_fadd(output->memp, LNF_FLD_FIRST, LNF_AGGR_MIN, 0, 0);
	lnf_mem_fadd(output->memp, LNF_FLD_LAST, LNF_AGGR_MAX, 0, 0);
	lnf_mem_fadd(output->memp, LNF_FLD_CALC_DURATION, LNF_AGGR_SUM, 0, 0);

	while ( (token = strsep(&str, ",")) != NULL ) {
		/* parse field */
		field = lnf_fld_parse(token, &numbits, &numbits6);

		if (field == LNF_FLD_ZERO_) {
			fprintf(stderr, "Cannot parse %s in -A \n", token);
			return 0;
		}

		if (lnf_fld_type(field) == LNF_ADDR && (numbits > 32 || numbits6 > 128)) {
			fprintf(stderr, "Invalid bit size (%d/%d) for %s in -A \n", 
			numbits, numbits6, token);
			return 0;
		}	
	
		lnf_mem_fadd(output->memp, field, LNF_AGGR_KEY, numbits, numbits6);
		output_field_add(output, field);

		token = NULL;
	}

	/* default fields on the ond of the list */
	lnf_mem_fadd(output->memp, LNF_FLD_DPKTS, LNF_AGGR_SUM, 0, 0);
	lnf_mem_fadd(output->memp, LNF_FLD_DOCTETS, LNF_AGGR_SUM, 0, 0);
	lnf_mem_fadd(output->memp, LNF_FLD_AGGR_FLOWS, LNF_AGGR_SUM, 0, 0);
/*
	fields_add(LNF_FLD_DPKTS)M;
	fields_add(LNF_FLD_DOCTETS);
	fields_add(LNF_FLD_CALC_BPS);
	fields_add(LNF_FLD_CALC_BPP);
	fields_add(LNF_FLD_AGGR_FLOWS);
*/
	return 1;
}

