
#include <libnf.h>

int main (void) {

	int match;	
	lnf_file_t *filep;
	lnf_rec_t *recp;
	lnf_filter_t *filterp;
	uint16_t sport,dport;

	lnf_open(&filep, "testfile", LNF_READ, NULL);


	lnf_filter_init(&filterp, "src port > 700");

	lnf_rec_init(&recp);

	while (lnf_read(filep, recp) != LNF_EOF) {

		lnf_rec_fget(recp, LNF_FLD_SRCPORT, &sport);
		lnf_rec_fget(recp, LNF_FLD_DSTPORT, &dport);

		match = lnf_filter_match(filterp, recp);

		printf(" %d -> %d %d\n", sport, dport, match);
	}

	lnf_rec_free(recp);
	lnf_filter_free(filterp);
	lnf_close(filep);
}


