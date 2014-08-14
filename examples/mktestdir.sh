#!/bin/bash 

set -x

RECS=100000000
DIR=testdir

mkdir ${DIR}
for i in 01 02 03 04 05 06 07 08 09 10 11 12 ; do 
	time ./lnf_ex01_writer -n ${RECS} -r $i -f ${DIR}/tf-${i} &
done 

