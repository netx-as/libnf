
[![Maintained by NetX Networks](https://img.shields.io/badge/maintained%20by-NetX%20Networks-blue)](https://netx.as/)

libnf - C interface for processing nfdump files 


IMPORTANT LINKS
===============================================

Official homepage:
	http://libnf.net

C API interface documentation: 
	http://libnf.net/doc/api/

Package directory: 
	http://libnf.net/packages/

Github repository:
	https://github.com/NETX-AS/libnf

Libnf interface for perl (Net::NfDump package) available via cpan:
	http://search.cpan.org/~tpoder/Net-NfDump/lib/Net/NfDump.pm
	https://github.com/netx-as/libnf/tree/master/perl


INSTALLATION FROM TARBALL 
===============================================

1. Download libnf tarball from http://libnf.net/packages/
```
# wget http://libnf.net/packages/libnf-x.xx.tar.gz
```
2. Unzip package and enter the package dir 
```
# tar xzf libnf-x.xx.tar.gz 
# cd libnf-x.xx
```
3. Run configure script 
```
# ./configure 
```
4. Run make and install files 
```
# make && make install 
```   
5. Look at the examples directory how to use libnf 


INSTALLATION FROM GIT REPOSITORY
===============================================

1. Get libnf git repository 
```
# git clone https://github.com/NETX-AS/libnf
```
2. Go into libnf source directory 
```
# cd libnf
```
3. Fetch nfdump sources an prepare for using in libnf
```
# ./prepare-nfdump.sh
```
4. Follow instructions 3., 4. and 5. from TARBALL INSTALLATION




