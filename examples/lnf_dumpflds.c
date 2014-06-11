
#include "libnf.h"


int main (void) {

	int i = 0;
	lnf_field_t *f = lnf_fields;

	while (f->index != 0) {

		printf("0x%x\t%s\t%s\n", f->index, f->name, f->fld_descr);
		f++;
	}

	
	return 0;
}
