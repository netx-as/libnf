
AM_CFLAGS = -I$(top_srcdir)/include 

bin_PROGRAMS = lnf_ex01_writer lnf_ex02_reader lnf_ex03_aggreg lnf_ex03_coreperf lnf_ex05_memtrans lnf_ex06_readreset \
			lnf_ex07_filter lnf_ex08_statistics lnf_ex09_memlookup lnf_ex10_listmode lnf_ex11_memclean \
			lnf_ex12_rec_raw lnf_ex13_ring_write lnf_ex14_ring_read 
			

lnf_ex01_writer_SOURCES = lnf_ex01_writer.c
lnf_ex01_writer_LDADD = ../src/.libs/libnf.a


lnf_ex02_reader_SOURCES = lnf_ex02_reader.c
lnf_ex02_reader_LDADD = ../src/.libs/libnf.a
#lnf_example2_LDADD = -lnf

lnf_ex03_aggreg_SOURCES = lnf_ex03_aggreg.c
lnf_ex03_aggreg_LDADD = ../src/.libs/libnf.a

lnf_ex03_coreperf_SOURCES = lnf_ex03_coreperf.c
lnf_ex03_coreperf_LDADD = ../src/.libs/libnf.a

if LNF_THREADS
bin_PROGRAMS += lnf_ex04_threads
lnf_ex04_threads_SOURCES = lnf_ex04_threads.c
lnf_ex04_threads_LDADD = ../src/.libs/libnf.a -lpthread
endif 

lnf_ex05_memtrans_SOURCES = lnf_ex05_memtrans.c
lnf_ex05_memtrans_LDADD = ../src/.libs/libnf.a

lnf_ex06_readreset_SOURCES = lnf_ex06_readreset.c
lnf_ex06_readreset_LDADD = ../src/.libs/libnf.a

lnf_ex07_filter_SOURCES = lnf_ex07_filter.c
lnf_ex07_filter_LDADD = ../src/.libs/libnf.a

lnf_ex08_statistics_SOURCES = lnf_ex08_statistics.c
lnf_ex08_statistics_LDADD = ../src/.libs/libnf.a

lnf_ex09_memlookup_SOURCES = lnf_ex09_memlookup.c
lnf_ex09_memlookup_LDADD = ../src/.libs/libnf.a

lnf_ex10_listmode_SOURCES = lnf_ex10_listmode.c
lnf_ex10_listmode_LDADD = ../src/.libs/libnf.a

lnf_ex11_memclean_SOURCES = lnf_ex11_memclean.c
lnf_ex11_memclean_LDADD = ../src/.libs/libnf.a

lnf_ex12_rec_raw_SOURCES = lnf_ex12_rec_raw.c
lnf_ex12_rec_raw_LDADD = ../src/.libs/libnf.a

lnf_ex13_ring_write_SOURCES = lnf_ex13_ring_write.c
lnf_ex13_ring_write_LDADD = ../src/.libs/libnf.a -lrt

lnf_ex14_ring_read_SOURCES = lnf_ex14_ring_read.c
lnf_ex14_ring_read_LDADD = ../src/.libs/libnf.a -lrt

