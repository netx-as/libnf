

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


void output_init(output_t *output) {

	memset(output, 0x0, sizeof(output_t));

	output->output_start_func = output_start_line;
	output->output_row_func = output_row_line;
	output->output_finish_func = output_finish_line;
}


void output_set_fmt(output_t *output, output_fmt_t output_fmt, char *filename) {

	output->output_fmt = output_fmt;

	switch (output_fmt) {

		case OFMT_BIN_NFDUMP:
				output->output_start_func = output_start_nfdump;
				output->output_row_func = output_row_nfdump;
				output->output_finish_func = output_finish_nfdump;
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


int output_start(output_t *output) {

	return output->output_start_func(output);

}


int output_row(output_t *output, lnf_rec_t *rec) {

	return output->output_row_func(output, rec);

}


int output_finish(output_t *output) {

	return output->output_finish_func(output);
}


int output_field_add(output_t *output, int field) {


	if (field < LNF_FLD_TERM_) {
		output->fields[output->numfields++].field = field;
		return 1;
	} else {
		return 0;
	}
}

/* parse argument given by -A */
int parse_aggreg(output_t *output, lnf_mem_t *memp, char *str) {

	char *token = str;
	int field, numbits, numbits6;

	/* default fields on the begining of the list */
	lnf_mem_fastaggr(memp, LNF_FAST_AGGR_BASIC);
//	fields_add(LNF_FLD_FIRST);
//	fields_add(LNF_FLD_CALC_DURATION);

	while ( (token = strsep(&str, ",")) != NULL ) {
		/* parse field */
		field = lnf_fld_parse(token, &numbits, &numbits6);

		if (field == LNF_FLD_ZERO_) {
			fprintf(stderr, "Cannot parse %s in -A \n", token);
			exit(1);
		}

		if (lnf_fld_type(field) == LNF_ADDR && (numbits > 32 || numbits6 > 128)) {
			fprintf(stderr, "Invalid bit size (%d/%d) for %s in -A \n", 
			numbits, numbits6, token);
			exit(1);
		}	
		
		lnf_mem_fadd(memp, field, LNF_AGGR_KEY, numbits, numbits6);
		output_field_add(output, field);

		token = NULL;
	}

	/* default fields on the ond of the list */
/*
	fields_add(LNF_FLD_DPKTS);
	fields_add(LNF_FLD_DOCTETS);
	fields_add(LNF_FLD_CALC_BPS);
	fields_add(LNF_FLD_CALC_BPP);
	fields_add(LNF_FLD_AGGR_FLOWS);
*/
	return 1;
}

