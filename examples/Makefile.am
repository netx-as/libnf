
AM_CFLAGS = -I$(top_srcdir)/include 

bin_PROGRAMS = lnf_ex01_writer lnf_ex02_reader lnf_ex03_aggreg lnf_ex05_aggreg lnf_ex06_readreset lnf_ex07_filter

lnf_ex01_writer_SOURCES = lnf_ex01_writer.c
lnf_ex01_writer_LDADD = ../src/.libs/libnf.a


lnf_ex02_reader_SOURCES = lnf_ex02_reader.c
lnf_ex02_reader_LDADD = ../src/.libs/libnf.a
#lnf_example2_LDADD = -lnf

lnf_ex03_aggreg_SOURCES = lnf_ex03_aggreg.c
lnf_ex03_aggreg_LDADD = ../src/.libs/libnf.a

if LNF_THREADS
bin_PROGRAMS += lnf_ex04_threads
lnf_ex04_threads_SOURCES = lnf_ex04_threads.c
lnf_ex04_threads_LDADD = ../src/.libs/libnf.a -lpthread
endif 

lnf_ex05_aggreg_SOURCES = lnf_ex05_memtrans.c
lnf_ex05_aggreg_LDADD = ../src/.libs/libnf.a

lnf_ex06_readreset_SOURCES = lnf_ex06_readreset.c
lnf_ex06_readreset_LDADD = ../src/.libs/libnf.a

lnf_ex07_filter_SOURCES = lnf_ex07_filter.c
lnf_ex07_filter_LDADD = ../src/.libs/libnf.a
