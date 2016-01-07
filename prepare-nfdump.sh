#!/bin/sh 

#
# Copyright (c) 2013-2015, Tomas Podermanski
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


LIBNF_VERSION="1.10"

NFDUMP_VERSION="1.6.14-b2"
NFDUMP="nfdump-$NFDUMP_VERSION"
NFDUMP_MD5=" e0b5421ef542d4dc340498b9c4447477"
NFDUMP_SRC="$NFDUMP.tar.gz"
NFDUMP_URL="https://github.com/phaag/nfdump/archive/v$NFDUMP_VERSION.tar.gz"


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
(cd nfdump && ./configure && make clean) || exit 1
(cd nfdump && patch -p1 < ../nfdump-bugs.patch && cd .. ) || exit 1
#(cd nfdump && patch -p1 < ../nfdump-leak.patch && cd .. ) || exit 1
#(cd nfdump && patch -p1 < ../nfdump-thread.patch && cd .. ) || exit 1

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


FILES="nffile.c nfx.c nftree.c minilzo.c nf_common.c grammar.y scanner.l \
		  ipconv.c"
echo "Creating symlinks for $FILES"
for f in $FILES ; do
	(cd src && rm -f $f && ln -s ../nfdump/bin/$f && cd ..) || exit 1
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

