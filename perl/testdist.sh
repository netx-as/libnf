#!/bin/bash 

#set -x

export COPYFILE_DISABLE=1


#	r104
#	r120
HOSTS="
	r120-tp
	r104
	r120
	tpoder@ox.netx.as
	root@localhost
	root@irbis.cis.vutbr.cz 
"

# get lastets version of package 
VER=$(cat lib/Net/NfDump.pm | grep VERSIO | grep our | cut -f2 -d"'")
DIST="Net-NfDump-$VER"
EXT=".tar.gz"

#echo "y" | make dist 
rm Net-NfDump-*.tar
rm Net-NfDump-*.tar.gz

echo "Y" | make dist 

for h in $HOSTS; do 
	echo "***********************************************"
	echo ${h}
	echo "***********************************************"
	echo "Press Enter to start test or 0 to skip..."
	read a 
	if [ "$a" != "0" ]; then
		scp ${DIST}${EXT} ${h}:/tmp/
		#ssh ${h} "export AUTOMATED_TESTING=1 ; cd /tmp/ && (rm -rf ${DIST}/ ; tar xzf ${DIST}${EXT}) && cd ${DIST} && perl Makefile.PL && make -j 8 -j 8 && make test ; rm -rf Net-Dump-*"
		ssh ${h} "export AUTOMATED_TESTING=1 ; cd /tmp/ && (rm -rf ${DIST}/ ; tar xzf ${DIST}${EXT}) && cd ${DIST} && perl Makefile.PL && make && make test ; rm -rf Net-Dump-*"
		echo FINISHED: ${h}
		echo 
	fi

done



