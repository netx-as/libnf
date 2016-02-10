
AM_CFLAGS = -I$(top_srcdir)/include 

EXTRA_DIST = cpucores.h flist.h screen.h progress.h output_line.h

if LNF_THREADS
bin_PROGRAMS = nfdumpp

nfdumpp_SOURCES = nfdumpp.c flist.c cpucores.c output.c output_line.c progress.c 
nfdumpp_LDADD = ../src/.libs/libnf.a -lpthread
endif 

