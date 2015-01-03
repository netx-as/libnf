
AM_CFLAGS = -I$(top_srcdir)/include 

bin_PROGRAMS = nfdumpp

if LNF_THREADS
nfdumpp_SOURCES = nfdumpp.c flist.c cpucores.c screen.c progress.c 
nfdumpp_LDADD = ../src/.libs/libnf.a -lpthread
endif 

