
.NOTPARALLEL:

AM_YFLAGS = -d

INCLUDES = $(DEPS_CFLAGS)
LDADD = $(DEPS_LIBS)
#AM_LDFLAGS=-pthread

# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info
libnf_la_LDFLAGS = -version-info 1:35:0

#Add extra debug info for gdb and nfdump includes
AM_CFLAGS = -ggdb -I$(top_srcdir)/include -I$(top_srcdir)/nfdump/bin -I$(top_srcdir)/src/ffilter

if LNF_BZ2STATIC
AM_CFLAGS += -I$(top_srcdir)/bzip2 
endif

#SUBDIRS = ffilter

EXTRA_DIST = libnf_internal.h bit_array.h hash_table.h heap_sort.h list_sort.h xxhash.h fields.h \
    lnf_filter.h literals.h 


CLEANFILES = lex.yy.c 
#grammar.c grammar.h scanner.c scanner.h

# Target for the libnfdump library
lib_LTLIBRARIES = libnf.la
libnf_la_SOURCES = libnf.c bit_array.c heap_sort.c list_sort.c hash_table.c xxhash.c memheap.c fields.c \
	nffile.c nfx.c minilzo.c ipconv.c lnf_filter.c literals.c lz4.c

# Nfdump filter sources
BUILT_SOURCES = grammar.h 
EXTRA_DIST += scanner.c grammar.c
libnf_la_SOURCES += nftree.c output_util.c grammar.y scanner.l 

# Libnf flow filter sources 
EXTRA_DIST += ffilter/ffilter_gram.c ffilter/ffilter_lex.c ffilter/ffilter_gram.h ffilter/ffilter_internal.h ffilter/ffilter.h ffilter/fcore.h
BUILT_SOURCES += ffilter/ffilter_gram.h 
libnf_la_SOURCES += ffilter/ffilter_gram.y ffilter/ffilter_lex.l ffilter/ffilter.c ffilter/fcore.c
include_HEADERS = ffilter/ffilter.h

# rinbuffer code
EXTRA_DIST += lnf_ringbuf.h
libnf_la_SOURCES += lnf_ringbuf.c

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
libnf_info_DEPENDENCIES = libnf.la
libnf_info_LDADD = ../src/.libs/libnf.a

