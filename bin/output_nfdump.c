

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
#include "output.h"
#include "output_nfdump.h"



int output_start_nfdump(output_t *output) {

	if ( lnf_open(&output->filep, output->filename, LNF_WRITE | LNF_COMP, "myfile") != LNF_OK )  {
		return 1;
	} else {
		return 0;
	}
}


int output_row_nfdump(output_t *output, lnf_rec_t *rec) {

	if (lnf_write(output->filep, rec) != LNF_OK) { 
		return 1;
	} else {
		return 0;
	}
}

int output_finish_nfdump(output_t *output) {

	lnf_close(output->filep);
	return 1;
}


