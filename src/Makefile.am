BUILT_SOURCES = grammar.h
AM_YFLAGS = -d

INCLUDES = $(DEPS_CFLAGS)
LDADD = $(DEPS_LIBS)
AM_LDFLAGS=-pthread

#Add extra debug info for gdb and nfdump includes
AM_CFLAGS = -ggdb -I$(top_srcdir)/include -I$(top_srcdir)/nfdump/bin

EXTRA_DIST = libnf_internal.h bit_array.h hash_table.h heap_sort.h 
#util.h

CLEANFILES = lex.yy.c grammar.c grammar.h scanner.c scanner.h

# Target for the libnfdump library
lib_LTLIBRARIES=libnf.la
libnf_la_SOURCES=libnf.c bit_array.c heap_sort.c hash_table.c xxhash.c memheap.c \
	nffile.c nfx.c nftree.c minilzo.c \
	nf_common.c grammar.y scanner.l \
	ipconv.c 

# Simple infor program 
bin_PROGRAMS = libnf-info

libnf_info_SOURCES = libnf-info.c
libnf_info_LDADD = -lnf


