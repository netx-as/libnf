
.NOTPARALLEL:

AM_YFLAGS = -d

INCLUDES = $(DEPS_CFLAGS)
LDADD = $(DEPS_LIBS)
#AM_LDFLAGS=-pthread

#Add extra debug info for gdb and nfdump includes
AM_CFLAGS = -ggdb -I$(top_srcdir)/include -I$(top_srcdir)/nfdump/bin -I$(top_srcdir)/src/ffilter

if LNF_BZ2STATIC
AM_CFLAGS += -I$(top_srcdir)/bzip2 
endif

#SUBDIRS = ffilter

EXTRA_DIST = libnf_internal.h bit_array.h hash_table.h heap_sort.h list_sort.h xxhash.h fields.h lnf_filter.h 
#util.h

CLEANFILES = lex.yy.c 
#grammar.c grammar.h scanner.c scanner.h

# Target for the libnfdump library
lib_LTLIBRARIES = libnf.la
libnf_la_SOURCES = libnf.c bit_array.c heap_sort.c list_sort.c hash_table.c xxhash.c memheap.c fields.c \
	nffile.c nfx.c minilzo.c nf_common.c ipconv.c lnf_filter.c 

# Nfdump filter sources
BUILT_SOURCES = grammar.h
EXTRA_DIST += scanner.c grammar.c
libnf_la_SOURCES += nftree.c grammar.y scanner.l 

# Libnf flow filter sources 
EXTRA_DIST += ffilter/ffilter_gram.c ffilter/ffilter_lex.c ffilter/ffilter_gram.h ffilter/ffilter_internal.h
BUILT_SOURCES += ffilter/ffilter_gram.h 
libnf_la_SOURCES += ffilter/ffilter_gram.y ffilter/ffilter_lex.l ffilter/ffilter.c

# if the bz2 is staticaly linked to libnf 
if LNF_BZ2STATIC
libnf_la_SOURCES += ../bzip2/bzlib.c ../bzip2/bzip2.c ../bzip2/blocksort.c ../bzip2/huffman.c \
					../bzip2/crctable.c ../bzip2/randtable.c ../bzip2/compress.c \
					../bzip2/decompress.c
endif


# Simple info program 
bin_PROGRAMS = libnf-info

libnf_info_SOURCES = libnf-info.c
#libnf_info_LDADD = -lnf
libnf_info_LDADD = ../src/.libs/libnf.a

