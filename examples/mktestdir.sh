#!/bin/bash 

#set -x

FILES=50			# num files 
RECS=10000000		# records per file 
DIR=testdir			# test dir 

rm -rf ${DIR}
mkdir ${DIR}

for i in $(seq 1 ${FILES}) ; do 
	i=$(printf %04d ${i})
#	time ./lnf_ex01_writer -n ${RECS} -r $i -f ${DIR}/tf-${i} & 
	echo $i
	./lnf_ex01_writer -n ${RECS} -r $i -f ${DIR}/tf-${i}  
done 

