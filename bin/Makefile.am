
AM_CFLAGS = -I$(top_srcdir)/include 

EXTRA_DIST = cpucores.h flist.h output.h progress.h output_line.h output_nfdump.h output_ringbuf.h

if LNF_THREADS
bin_PROGRAMS = nfdumpp

nfdumpp_SOURCES = nfdumpp.c flist.c cpucores.c output.c output_line.c output_nfdump.c output_ringbuf.c progress.c 
nfdumpp_LDADD = ../src/.libs/libnf.a -lpthread
endif 

