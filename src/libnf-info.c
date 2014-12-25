
#include "libnf.h"

int main (void) {

	lnf_info_t i;
	int fields[LNF_FLD_TERM_];
	int aggr, sort;
	int idx = 0;
	char name[1024];
	char descr[1024];

	lnf_info(NULL, &i);

	printf("libnf version: %s\n", i.libnf_version);
	printf("libnf based on nfdump: %s\n", i.nfdump_version);


	lnf_fld_info(LNF_FLD_ZERO_, LNF_FLD_INFO_FIELDS, &fields);

	printf("Supported items:\n");
	while (fields[idx] != LNF_FLD_TERM_) {
		lnf_fld_info(fields[idx], LNF_FLD_INFO_AGGR, &aggr);
		lnf_fld_info(fields[idx], LNF_FLD_INFO_SORT, &sort);
		lnf_fld_info(fields[idx], LNF_FLD_INFO_NAME, &name);
		lnf_fld_info(fields[idx], LNF_FLD_INFO_DESCR, &descr);

		printf("  0x%08x 0x%02x 0x%02x  %-20s %s\n", fields[idx], aggr, sort, name, descr);
		idx++;
	}
	
	return 0;
}
