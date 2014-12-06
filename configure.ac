#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libnf, 1.07, tpoder@cis.vutbr.cz)

AC_DEFINE(NFDUMP_VERSION,["nfdump, 1.6.13, phaag@users.sourceforge.net"],[nfdump])
# $Date: 2014-11-16 14:10:20 +0100 (Sun, 16 Nov 2014) $
#AC_CONFIG_SRCDIR([grammar.y])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
LT_INIT()

 
# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

dnl get the flags
CFLAGS="${CFLAGS=}"
if test $ac_cv_prog_gcc = yes -a "x$CFLAGS" = "x-g -O2"; then
	CFLAGS="-g -O2 -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn -fno-strict-aliasing"
fi

AC_ARG_ENABLE(devel,
[  --enable-devel       compile debug and development code into libnf and nfdump; default is NO])

# Which way is better?
if test "${enable_devel}" = "yes" ; then
	CFLAGS="$CFLAGS -DDEVEL"
cat >>config.h <<_ACEOF
#define dbg_printf(...) printf(__VA_ARGS__)
_ACEOF
else
cat >>config.h <<_ACEOF
#define dbg_printf(...) /* printf(__VA_ARGS__) */
_ACEOF
fi

AC_ARG_ENABLE(threads,
[  --enable-threads       enable thread support for libnf_mem - see documentation; default is NO])

if test "${enable_threads}" = "yes" ; then
	LIBS="$LIBS -lpthread"
	CFLAGS="$CFLAGS -DLNF_THREADS"
	AC_CHECK_HEADERS(pthread.h)
	AC_CHECK_FUNCS(pthread_mutex_init pthread_key_create pthread_mutex_lock pthread_mutex_unlock)
	AC_CHECK_FUNCS(pthread_setspecific pthread_getspecific) 
	AM_CONDITIONAL(LNF_THREADS, true)
else
	AM_CONDITIONAL(LNF_THREADS, false)
fi


CFLAGS="$CFLAGS -DNSEL"

AC_PROG_YACC
AC_PROG_LEX
#which $LEX > /dev/null 2>&1
#if test $? = 1; then
#	AC_MSG_ERROR(No lex or flex found on system)
#fi
AC_PROG_INSTALL

if test "x$ac_cv_prog_LEX" = "xflex"; then
	LFLAGS=-i
fi


# Check for structures
AC_CHECK_MEMBER([struct sockaddr.sa_len],
 AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, define if socket address structures have length fields),,[
#include <sys/types.h>
#include <sys/socket.h>])

AC_CHECK_MEMBERS([struct sockaddr_storage.ss_family,
		  struct sockaddr_storage.__ss_family,
		  struct sockaddr_storage.ss_len,
		  struct sockaddr_storage.__ss_len,
		  struct sockaddr.sa_len], [], [], [
#include <sys/types.h>
#include <sys/socket.h>
])

#AC_SUBST(opt_objects)
AC_SUBST(FT_INCLUDES)
AC_SUBST(FT_LDFLAGS)
AC_SUBST(LFLAGS)


# Checks for libraries.
AC_CHECK_FUNCS(gethostbyname,,[AC_CHECK_LIB(nsl,gethostbyname,,[AC_CHECK_LIB(socket,gethostbyname)])])
AC_CHECK_FUNCS(setsockopt,,[AC_CHECK_LIB(socket,setsockopt)])

dnl checks for fpurge or __fpurge
AC_CHECK_FUNCS(fpurge __fpurge)

AC_MSG_CHECKING([if htonll is defined])

dnl # Check for htonll
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM(
     [[
        #include <arpa/inet.h>
     ]],
     [[
       return htonll(0);
     ]]
  )],
  [
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_HTONLL, 1, [Define to 1 if the function (or macro) htonll exists.])
  ],
  [
    AC_MSG_RESULT(no)
  ]
)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([nameser8_compat.h])
AC_CHECK_HEADERS([features.h arpa/inet.h fcntl.h netinet/in.h fts.h stdint.h stdlib.h stddef.h string.h sys/socket.h syslog.h unistd.h iso/limits_iso.h])
#AC_CHECK_HEADERS(pcap-bpf.h net/bpf.h)

AC_CHECK_HEADERS(sys/types.h netinet/in.h arpa/nameser.h arpa/nameser_compat.h netdb.h resolv.h,
                 [], [],
[[#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>   /* inet_ functions / structs */
#endif
#ifdef HAVE_ARPA_NAMESER_H
#  include <arpa/nameser.h> /* DNS HEADER struct */
#endif
#ifdef HAVE_NETDB_H
#  include <netdb.h>
#endif]])

AC_SUBST(FTS_OBJ)

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_SIZEOF(void *)

AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(inet_ntoa socket strchr strdup strerror strrchr strstr scandir)

dnl The res_search may be in libsocket as well, and if it is
dnl make sure to check for dn_skipname in libresolv, or if res_search
dnl is in neither of these libs, still check for dn_skipname in libresolv
AC_CHECK_LIB(socket, res_search, [
 AC_CHECK_LIB(resolv, dn_skipname)
 AC_CHECK_LIB(resolv, __dn_skipname)
 LIBS="$LIBS -lsocket"
 AC_DEFINE(HAVE_LIBSOCKET,1,[ ]) ], [
 AC_CHECK_LIB(resolv, res_search, [
  LIBS="$LIBS -lresolv"
  AC_DEFINE(HAVE_LIBRESOLV,1,[ ]) 
 ], [
  AC_CHECK_LIB(resolv, dn_skipname)
  AC_CHECK_LIB(resolv, __dn_skipname)
 ]) 
])

# lzo compression requirements
AC_CHECK_TYPE(ptrdiff_t, long)
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(__int64)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(ptrdiff_t)
AC_C_CONST
AC_CHECK_FUNCS(memcmp memcpy memmove memset)

AC_MSG_CHECKING([for union semun])
AC_CACHE_VAL(ac_cv_struct_semun,
	[
		AC_TRY_COMPILE(
		[
			#include <sys/types.h>
	 		#include <sys/ipc.h>
			#include <sys/sem.h>;
		],
		[
			union semun semdat;
		],
		ac_cv_struct_semun=yes, ac_cv_struct_semun=no
		)
	]
)

AC_MSG_RESULT($ac_cv_struct_semun)
if test "$ac_cv_struct_semun" = "yes"; then
	AC_DEFINE(HAVE_SEMUN, 1, [Define if sys/sem.h defines struct semun])
fi


AC_OUTPUT(Makefile include/Makefile src/Makefile examples/Makefile)

echo ""
echo "The libnf extends the source code of nfdump tool"
echo "developed by Peter Haag. Unmodified nfdump sources are"
echo "placed in the nfudump directory that is distributed"
echo "together with libnf package. Thanks for using libnf."
echo "For more info visit http://libnf.net."
echo ""
