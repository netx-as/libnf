BUILT_SOURCES = grammar.h
AM_YFLAGS = -d

INCLUDES = $(DEPS_CFLAGS)
LDADD = $(DEPS_LIBS)

#Add extra debug info for gdb and nfdump includes
AM_CFLAGS = -ggdb -I$(top_srcdir)/include -I$(top_srcdir)/nfdump/bin

EXTRA_DIST = inline.c collector_inline.c nffile_inline.c nfdump_inline.c heapsort_inline.c applybits_inline.c

CLEANFILES = lex.yy.c grammar.c grammar.h scanner.c scanner.h

# Target for the libnfdump library
lib_LTLIBRARIES=libnf.la
libnf_la_SOURCES=libnf.c bit_array.c \
	nffile.c nfx.c nftree.c minilzo.c \
	nf_common.c grammar.y scanner.l 

# Test program
#bin_PROGRAMS = dumpflds lnf_example1 lnf_example2

#dumpflds_SOURCES = dumpflds.c
#dumpflds_LDADD = -lnf

#lnf_example1_SOURCES = lnf_example1.c
#lnf_example1_LDADD = -lnf

#lnf_example2_SOURCES = lnf_example2.c
#lnf_example2_LDADD = -lnf

