#!/bin/sh 

#
# Copyright (c) 2013-2023, Tomas Podermanski
#    
# This file is part of libnf.net project.
#
# Libnf is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Libnf is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libnf.  If not, see <http://www.gnu.org/licenses/>.
#
#


<<<<<<< HEAD
NFDUMP_VERSION="1.7.1"
NFDUMP="nfdump-$NFDUMP_VERSION"
NFDUMP_MD5="14000174cadb0b6230ef930f1a8c7c71"
=======
NFDUMP_VERSION="1.6.25"
NFDUMP="nfdump-$NFDUMP_VERSION"
NFDUMP_MD5="9c79e52b5b00c7ae7a31e8c681cf17cb"
>>>>>>> 95bc0aabf6921af99d117c84df376dbe5e32f5f9
NFDUMP_SRC="$NFDUMP.tar.gz"
NFDUMP_URL="https://github.com/phaag/nfdump/archive/v$NFDUMP_VERSION.tar.gz"

BZIP2_VERSION="1.0.6"
BZIP2="bzip2-$BZIP2_VERSION"
BZIP2_MD5="00b516f4704d4a7cb50a1d97e6e8e15b"
BZIP2_SRC="$BZIP2.tar.gz"
#BZIP2_URL="http://www.bzip.org/1.0.6/$BZIP2_SRC"
BZIP2_URL="https://netcologne.dl.sourceforge.net/project/bzip2/$BZIP2_SRC"


echo ""
echo "##########################################################"
echo "# STAGE 1.1: getting and patching nfdump sources         #"
echo "##########################################################"

if [ ! -f $NFDUMP_SRC ] ; then 
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	echo "!!                                                     !!"
	echo "!!  If automatic download fails please get nfdump      !!"
	echo "!!  sources manually                                   !!"
	echo "!!                                                     !!"
	echo "!!  VERSION: $NFDUMP                             !!"
	echo "!!  URL:     $NFDUMP_URL  !!"
	echo "!!  FILE:    $NFDUMP.tar.gz                      !!"
	echo "!!                                                     !!"
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	wget -nv -O $NFDUMP_SRC $NFDUMP_URL || curl -L -o $NFDUMP_SRC $NFDUMP_URL || exit 1
fi 

rm -rf nfdump/ $NFDUMP 

./md5sum.sh $NFDUMP_MD5 $NFDUMP_SRC || exit 1

tar xzf $NFDUMP_SRC || exit 1
mv $NFDUMP nfdump  || exit 1
(cd nfdump && ./bootstrap && ./configure && make clean) || exit 1
# for version < 1.6.18
#(cd nfdump && patch -p1 < ../nfdump-bugs.patch && cd .. ) || exit 1
# for version < 1.6.4
#(cd nfdump && patch -p1 < ../nfdump-thread.patch && cd .. ) || exit 1
# for version >= 1.6.4
#(cd nfdump && patch -p1 < ../nfdump-thread-nffile.patch && cd .. ) || exit 1
#(cd nfdump && patch -p1 < ../nfdump-thread-nfx.patch && cd .. ) || exit 1
if [ ! -f nfdump/README ]; then
	echo > nfdump/README
fi

echo ""
echo "##########################################################"
echo "# STAGE 1.1: fetching BZ2 source codes                   #"
echo "##########################################################"
wget -nv -O $BZIP2_SRC $BZIP2_URL || curl -L -o $BZIP2_SRC $BZIP2_URL || exit 1
rm -rf bzip2/ $BZIP2

./md5sum.sh $BZIP2_MD5 $BZIP2_SRC || exit 1

tar xzf $BZIP2_SRC || exit 1
mv $BZIP2 bzip2  || exit 1


echo ""
echo "##########################################################"
echo "# STAGE 2: preparing configure and nfdump sources        #"
echo "##########################################################"

##echo "Getting configure.in from original nfdump"
##cp nfdump/configure.ac configure.ac

##echo "Adding LT_INT into configure.ac"
##perl -pi -w -e 's/(AM_INIT_AUTOMAKE.*)/$1\nLT_INIT()\n/g;' configure.ac 

##echo "Replacing AC_REVISIONin configure.ac" 
##sed -i -e 's/AC_REVISION.*//g' configure.ac 	#makefiles

##echo "Changing AC_INIT to AC_DEFINE in configure.ac" 
##perl -pi -w -e 's/AC_INIT\((.*)\)/AC_DEFINE\(NFDUMP_VERSION,["$1"],[nfdump]\)/g;' configure.ac 

##echo "Adding own AC_INIT in configure.ac" 
##perl -pi -w -e 's/(AC_PREREQ.*)/$1\nAC_INIT\(libnf, '${LIBNF_VERSION}', tpoder\@cis.vutbr.cz\)/g;' configure.ac 

##echo "Enabling NEL/NSEL in configure.ac" 
##perl -pi -w -e 's/(AC_PROG_YACC)/CFLAGS="\$CFLAGS -DNSEL"\n\n$1/g;' configure.ac 

##echo "Removing AC_OUTPUT and text from configure.ac" 
##sed -i -e 's/AC_OUTPUT.*//g' configure.ac 	#makefiles
##sed -i -e 's/echo ".*//g' configure.ac 		#comments

##echo "Adding pthread.h checj into configure.ac" 
##perl -pi -w -e 's/AC_HEADER_STDC/AC_HEADER_STDC\nAC_CHECK_HEADERS(pthread.h)\nLIBS="\$LIBS -lpthread"/g;' configure.ac 

##echo "Adding extra configuration into configure.ac"
##cat >> configure.ac << EOT 
##AC_OUTPUT(Makefile include/Makefile src/Makefile examples/Makefile)

##echo ""
##echo "The libnf extends the source code of nfdump tool"
##echo "developed by Peter Haag. Unmodified nfdump sources are"
##echo "placed in the nfudump directory that is distributed"
##echo "together with libnf package. Thanks for using libnf."
##echo "For more info visit http://libnf.net."
##echo ""
##EOT


FILES="src/lib/nffile.c src/lib/nfx.c src/lib/nftree.c src/lib/minilzo.c src/lib/lz4.c \
		src/lib/grammar.y src/lib/scanner.l \
		  src/lib/ipconv.c src/lib/output_util.c src/lib/sgregex/sgregex.c"
echo "Creating symlinks for $FILES"
for f in $FILES ; do
	bf=$(basename $f)
	(cd src && rm -f $bf && ln -s ../nfdump/$f && cd ..) || exit 1
done

rm src/grammar.c
rm src/scanner.c

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

