#!/bin/bash 

#set -x

export COPYFILE_DISABLE=1

echo "y" | make dist 
echo "Y" | make dist 

HOSTS="
	root@hawk.cis.vutbr.cz 
	root@coyote.cis.vutbr.cz 
	tpoder@147.229.3.110
	root@test-ubuntu.net.vutbr.cz 
	root@r101.cis.vutbr.cz 
	root@test-freebsd.net.vutbr.cz 
	root@test-gnukfreebsd.net.vutbr.cz 
	root@test-solaris.net.vutbr.cz 
	root@test-openbsd.net.vutbr.cz
"

DIST="Net-NfDump-1.25"
EXT=".tar.gz"

for h in $HOSTS; do 
	echo "***********************************************"
	echo ${h}
	echo "***********************************************"
	echo "Press Enter to start test or 0 to skip..."
	read a 
	if [ "$a" != "0" ]; then
		scp ${DIST}${EXT} ${h}:/tmp/
		ssh ${h} "export AUTOMATED_TESTING=1 ; cd /tmp/ && (rm -rf ${DIST}/ ; tar xzf ${DIST}${EXT}) && cd ${DIST} && perl Makefile.PL && make -j 8 -j 8 && make test ; rm -rf Net-Dump-*"
		echo FINISHED: ${h}
		echo 
	fi

done



