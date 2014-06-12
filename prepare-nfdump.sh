#!/bin/sh 

NFDUMP="nfdump-1.6.12"
NFDUMP_MD5="e55a9130c93cfb9ed24b01bccd691bcb"
NFDUMP_SRC="$NFDUMP.tar.gz"
NFDUMP_URL="http://downloads.sourceforge.net/project/nfdump/stable/$NFDUMP/$NFDUMP_SRC"

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

echo "##########################################################"
echo "# STAGE 2: preparing configure and nfdump sources        #"
echo "##########################################################"

echo "Getting configure.in from original nfdump"
cp nfdump/configure.in configure.ac

echo "Removing AC_OUTPUT and text in configure.ac" 
sed -i -e 's/AC_OUTPUT.*//g' configure.ac 
sed -i -e 's/echo ".*//g' configure.ac 

echo "Adding extra configuration to configure.ac"
echo "AC_OUTPUT(Makefile src/Makefile)" >> configure.ac 
echo "LT_INIT()" >> configure.ac 


