
.NOTPARALLEL:

AM_YFLAGS = -d

INCLUDES = $(DEPS_CFLAGS)
LDADD = $(DEPS_LIBS)
#AM_LDFLAGS=-pthread

#Add extra debug info for gdb and nfdump includes
AM_CFLAGS = -ggdb -I$(top_srcdir)/include -I$(top_srcdir)/nfdump/bin

EXTRA_DIST = libnf_internal.h bit_array.h hash_table.h heap_sort.h xxhash.h fields.h scanner.c grammar.c 
#util.h

CLEANFILES = lex.yy.c 
#grammar.c grammar.h scanner.c scanner.h

# Target for the libnfdump library
lib_LTLIBRARIES = libnf.la
libnf_la_SOURCES = libnf.c bit_array.c heap_sort.c hash_table.c xxhash.c memheap.c fields.c \
	nffile.c nfx.c nftree.c minilzo.c nf_common.c ipconv.c 

if LNF_EXPERIMENTAL
BUILT_SOURCES = lnf_filter_gram.h
libnf_la_SOURCES += lnf_filter_gram.y lnf_filter_lex.l lnf_filter.c
else
BUILT_SOURCES = grammar.h
libnf_la_SOURCES += grammar.y scanner.l 
endif 

# Simple info program 
bin_PROGRAMS = libnf-info

libnf_info_SOURCES = libnf-info.c
libnf_info_LDADD = -lnf

