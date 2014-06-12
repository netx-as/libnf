#!/bin/sh 

#set -x

SUM=$1
FILE=$2

if [ -x /bin/md5 ] ; then 
	RES=$(/bin/md5 $FILE)
elif [ -x /sbin/md5 ] ; then 
	RES=$(/sbin/md5 $FILE)
elif [ -x /usr/bin/md5sum ] ; then 
	RES=$(/usr/bin/md5sum $FILE)
elif [ -x /usr/bin/openssl ] ; then 
	RES=$(/usr/bin/openssl dgst -md5 $FILE)
elif [ -x /usr/local/bin/openssl ] ; then 
	RES=$(/usr/local/bin/openssl dgst -md5 $FILE)
else 
	echo "No MD5 checksum utility found"
	uname -a 
	exit 1
fi 

if [ "$(echo $RES | grep  $SUM)" != "" ] ; then 
	echo "Checksum for $FILE is OK"
	exit 0
else 
	echo "Invalid checksum for $FILE"
	echo "Excpected: $SUM"
	echo "Result: $RES"
	exit 1;
fi 

