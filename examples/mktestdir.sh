#!/bin/bash 

set -x

RECS=20000000
DIR=testdir

mkdir ${DIR}
./lnf_ex01_writer -n ${RECS} -r 2 -f ${DIR}/tf-02 &
./lnf_ex01_writer -n ${RECS} -r 4 -f ${DIR}/tf-04 &
./lnf_ex01_writer -n ${RECS} -r 8 -f ${DIR}/tf-08 &
./lnf_ex01_writer -n ${RECS} -r 16 -f ${DIR}/tf-16 &
./lnf_ex01_writer -n ${RECS} -r 10 -f ${DIR}/tf-10 &
./lnf_ex01_writer -n ${RECS} -r 5 -f ${DIR}/tf-5
./lnf_ex01_writer -n ${RECS} -r 6 -f ${DIR}/tf-6
./lnf_ex01_writer -n ${RECS} -r 7 -f ${DIR}/tf-7
./lnf_ex01_writer -n ${RECS} -r 3 -f ${DIR}/tf-3
./lnf_ex01_writer -n ${RECS} -r 11 -f ${DIR}/tf-11


