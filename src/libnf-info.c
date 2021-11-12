/* 

 Copyright (c) 2013-2015, Tomas Podermanski
    
 This file is part of libnf.net project.

 Libnf is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Libnf is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libnf.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libnf.h"



int main (int argc, char **argv) {

	int fields[LNF_FLD_TERM_];
	int aggr, sort, ipfix_id, ipfix_eid;
	int idx = 0;
	int ipfix_info = 0;
	char name[LNF_INFO_BUFSIZE];
	char descr[LNF_INFO_BUFSIZE];
	char buf[LNF_INFO_BUFSIZE];
	int c;
	

	while ((c = getopt (argc, argv, "i")) != -1) {
		switch (c) {
			case 'i': 	
				ipfix_info = 1; 
				break;
			case '?': 	
				printf(" -i - print additional IPFIX mapping info\n"); 
				exit(1);;
		}
	}


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

		if (ipfix_info) {
			if (lnf_fld_info(fields[idx], LNF_FLD_INFO_IPFIX_NAME, &name, sizeof(name)) == LNF_OK) {
				lnf_fld_info(fields[idx], LNF_FLD_INFO_IPFIX_EID, &ipfix_eid, sizeof(ipfix_eid));
				lnf_fld_info(fields[idx], LNF_FLD_INFO_IPFIX_ID, &ipfix_id, sizeof(ipfix_id));
				printf("                        %-30s  %04d %04d\n", name, ipfix_eid, ipfix_id);
			}
			if (lnf_fld_info(fields[idx], LNF_FLD_INFO_IPFIX_NAME6, &name, sizeof(name)) == LNF_OK) {
				lnf_fld_info(fields[idx], LNF_FLD_INFO_IPFIX_EID6, &ipfix_eid, sizeof(ipfix_eid));
				lnf_fld_info(fields[idx], LNF_FLD_INFO_IPFIX_ID6, &ipfix_id, sizeof(ipfix_id));
				printf("                        %-30s  %04d %04d\n", name, ipfix_eid, ipfix_id);
			}
		}
		idx++;
	}
	
	return 0;
}
