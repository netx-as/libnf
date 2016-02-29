
.NOTPARALLEL:

#EXTRA_DIST = md5sum.sh nfdump-bugs.patch nfdump-thread.patch check_items_map.pl bootstrap prepare-nfdump.sh nfdump
EXTRA_DIST = bootstrap nfdump bzip2


SUBDIRS = src include bin examples 

#main_SOURCES = nfdump

.PHONY: rpm
rpm:
	make dist; \
	rm -rf RPMBUILD; \
	mkdir -p RPMBUILD/{SOURCES,SPECS,BUILD,BUILDROOT,RPMS,SRPMS}; \
	cp libnf.spec RPMBUILD/SPECS; \
	mv libnf-@VERSION@.tar.gz RPMBUILD/SOURCES; \
	rpmbuild -ba RPMBUILD/SPECS/libnf.spec -D "_topdir `pwd`/RPMBUILD"

