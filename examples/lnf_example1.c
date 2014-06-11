
#include <libnf.h>

int main (void) {

	int i;	
	lnf_file_t *fp;
	lnf_rec_t *rp;
	uint16_t port = 666;

	lnf_open(&fp, "testfile", LNF_WRITE | LNF_COMP, "myfile");

	lnf_rec_init(&rp);

	for (i = 0; i < 10; i++) {
		port += i;
		lnf_rec_fset(rp, LNF_FLD_SRCPORT, &port);
		lnf_rec_fset(rp, LNF_FLD_DSTPORT, &port);
		lnf_write(fp, rp);
	}

	lnf_rec_free(rp);
	lnf_close(fp);
}


