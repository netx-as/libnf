
#include "libnf.h"

int main (void) {

	lnf_info_t i;
	lnf_field_t *f;

	lnf_info(NULL, &i);

	printf("libnf version: %s\n", i.libnf_version);
	printf("libnf based on nfdump: %s\n", i.nfdump_version);

	f = i.libnf_fields;

	printf("Supported items:\n");
	while (f->index != 0) {

		printf("  0x%08x  %-20s %s\n", f->index, f->name, f->fld_descr);
		f++;
	}

	
	return 0;
}
