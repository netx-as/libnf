
#include "libnf.h"
#include "config.h"


int main (void) {

	int i = 0;
	lnf_field_t *f = lnf_fields;

	printf("libnf version: %s\n", VERSION);
	printf("libnf based on nfdump: %s\n", NFDUMP_VERSION);

	printf("Supported items:\n");
	while (f->index != 0) {

		printf("  0x%08x  %-20s %s\n", f->index, f->name, f->fld_descr);
		f++;
	}

	
	return 0;
}
