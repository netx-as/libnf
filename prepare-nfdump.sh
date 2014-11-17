#!/bin/sh 

LIBNF_VERSION="1.04"

NFDUMP="nfdump-1.6.12"
NFDUMP_MD5="e55a9130c93cfb9ed24b01bccd691bcb"
NFDUMP_SRC="$NFDUMP.tar.gz"
NFDUMP_URL="http://downloads.sourceforge.net/project/nfdump/stable/$NFDUMP/$NFDUMP_SRC"


echo ""
echo "##########################################################"
echo "# STAGE 1: getting and patching nfdump sources           #"
echo "##########################################################"

if [ ! -f $NFDUMP_SRC ] ; then 
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	echo "!!                                                     !!"
	echo "!!  If automatic download fails please get nfdump      !!"
	echo "!!  sources manually                                   !!"
	echo "!!                                                     !!"
	echo "!!  VERSION: $NFDUMP                             !!"
	echo "!!  URL:     http://sourceforge.net/projects/nfdump/   !!"
	echo "!!  FILE:    $NFDUMP.tar.gz                      !!"
	echo "!!                                                     !!"
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	wget -nv $NFDUMP_URL || curl -L -o $NFDUMP_SRC $NFDUMP_URL || exit 1
fi 

rm -rf nfdump/ $NFDUMP 

./md5sum.sh $NFDUMP_MD5 $NFDUMP_SRC || exit 1

tar xzf $NFDUMP_SRC || exit 1
mv $NFDUMP nfdump  || exit 1
(cd nfdump && patch -p1 < ../nfdump-bugs.patch && cd .. ) || exit 1

echo ""
echo "##########################################################"
echo "# STAGE 2: preparing configure and nfdump sources        #"
echo "##########################################################"

echo "Getting configure.in from original nfdump"
cp nfdump/configure.in configure.ac

echo "Adding LT_INT into configure.ac"
perl -pi -w -e 's/(AM_INIT_AUTOMAKE.*)/$1\nLT_INIT()\n/g;' configure.ac 

#echo "Removing nel/nsel/compat/fixmi options"
#perl -pi -w -e 's/^AC_ARG_ENABLE\(compat15.*/XXX/m;' configure.ac 

echo "Replacing AC_REVISIONin configure.ac" 
sed -i -e 's/AC_REVISION.*//g' configure.ac 	#makefiles

echo "Changing AC_INIT to AC_DEFINE in configure.ac" 
perl -pi -w -e 's/AC_INIT\((.*)\)/AC_DEFINE\(NFDUMP_VERSION,["$1"],[nfdump]\)/g;' configure.ac 

echo "Adding own AC_INIT in configure.ac" 
perl -pi -w -e 's/(AC_PREREQ.*)/$1\nAC_INIT\(libnf, '${LIBNF_VERSION}', tpoder\@cis.vutbr.cz\)/g;' configure.ac 

echo "Enabling NEL/NSEL in configure.ac" 
perl -pi -w -e 's/(AC_PROG_YACC)/CFLAGS="\$CFLAGS -DNSEL"\n\n$1/g;' configure.ac 

echo "Removing AC_OUTPUT and text from configure.ac" 
sed -i -e 's/AC_OUTPUT.*//g' configure.ac 	#makefiles
sed -i -e 's/echo ".*//g' configure.ac 		#comments

echo "Adding pthread.h checj into configure.ac" 
perl -pi -w -e 's/AC_HEADER_STDC/AC_HEADER_STDC\nAC_CHECK_HEADERS(pthread.h)\nLIBS="\$LIBS -lpthread"/g;' configure.ac 

echo "Adding extra configuration into configure.ac"
cat >> configure.ac << EOT 
AC_OUTPUT(Makefile include/Makefile src/Makefile examples/Makefile)

echo ""
echo "The libnf extends the source code of nfdump tool"
echo "developed by Peter Haag. Unmodified nfdump sources are"
echo "placed in the nfudump directory that is distributed"
echo "together with libnf package. Thanks for using libnf."
echo "For more info visit http://libnf.net."
echo ""
EOT


FILES="nffile.c nfx.c nftree.c minilzo.c nf_common.c grammar.y scanner.l \
		  ipconv.c"
echo "Creating symlinks for $FILES"
for f in $FILES ; do
	(cd src && rm -f $f && ln -s ../nfdump/bin/$f && cd ..) || exit 1
done

echo ""
echo "##########################################################"
echo "# STAGE 3: checking definitions of all items in libnf    #"
echo "##########################################################"
echo ""
./check_items_map.pl || exit 1

echo ""
echo "##########################################################"
echo "# STAGE 4: creating final ./configure and Makefiles      #"
echo "##########################################################"
./bootstrap || exit 1

echo ""
echo "##########################################################"
echo "# OK: it seems that all steps went well                  #"
echo "##########################################################"

