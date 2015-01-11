
#include "libnf.h"

int main (void) {

	int fields[LNF_FLD_TERM_];
	int aggr, sort;
	int idx = 0;
	char name[LNF_INFO_BUFSIZE];
	char descr[LNF_INFO_BUFSIZE];
	char buf[LNF_INFO_BUFSIZE];


	lnf_info(NULL, LNF_INFO_VERSION, buf, LNF_INFO_BUFSIZE);
	printf("libnf version: %s\n", buf);
	lnf_info(NULL, LNF_INFO_NFDUMP_VERSION, buf, LNF_INFO_BUFSIZE);
	printf("libnf based on nfdump: %s\n", buf);


	lnf_fld_info(LNF_FLD_ZERO_, LNF_FLD_INFO_FIELDS, &fields, sizeof(fields));

	printf("Supported items:\n");
	while (fields[idx] != LNF_FLD_TERM_) {
		lnf_fld_info(fields[idx], LNF_FLD_INFO_AGGR, &aggr, sizeof(aggr));
		lnf_fld_info(fields[idx], LNF_FLD_INFO_SORT, &sort, sizeof(sort));
		lnf_fld_info(fields[idx], LNF_FLD_INFO_NAME, &name, sizeof(name));
		lnf_fld_info(fields[idx], LNF_FLD_INFO_DESCR, &descr, sizeof(descr));

		printf("  0x%08x 0x%02x 0x%02x  %-20s %s\n", fields[idx], aggr, sort, name, descr);
		idx++;
	}
	
	return 0;
}
