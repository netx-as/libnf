#!/bin/bash 

set -x

RECS=50000000
DIR=testdir

mkdir ${DIR}
for i in 1 2 3 4 5 6 7 8 9 ; do 
	time ./lnf_ex01_writer -n ${RECS} -r $i -f ${DIR}/tf-${i}
done 

