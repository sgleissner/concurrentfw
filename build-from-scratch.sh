#!/bin/bash

function separator {
	echo ""
	echo "#####"
	echo "##### run $1"
	echo "#####"
	echo ""
}

function failure {
	echo ""
	echo "ERROR: $1"
	exit 1
}

WORK="build_$(uname -m)"

rm -rf 				\
	aclocal.m4		\
	autom4te.cache	\
	ar-lib			\
	compile			\
	config.guess	\
	config.h.in		\
	config.h.in~	\
	config.sub		\
	configure		\
	depcomp			\
	install-sh		\
	ltmain.sh		\
	Makefile.in		\
	missing			\
	$WORK


#sudo rm -rf \
#	/usr/include/concurrentfw		\
#	/usr/bin/concurrentfw_test		\
#	/usr/lib64/libconcurrentfw.*	\
#	/usr/lib64/pkgconfig/libconcurrentfw.pc

# exit

separator "autoreconf"
autoreconf -vfi || failure "autoreconf"

separator "autoconf"
autoconf || failure "autoconf"

mkdir -p $WORK
pushd $WORK

separator "configure"
../configure --prefix /usr/local || failure "configure"

separator "make cppcheck"
make cppcheck || failure "make cppcheck"

separator "make"
JOBS=$(nproc)
make -j $JOBS || failure "make"

# separator "install"
# sudo make install-strip || failure "install"

popd

exit






-rwxr-xr-x 1 nobody nobody   273  2. Jun 11:30 build-from-scratch.sh
-rw-r--r-- 1 nobody nobody  1256  1. Jun 11:34 configure.ac
-rw-r--r-- 1 nobody nobody   263  2. Jun 11:25 libconcurrentfw.pc.in
-rw-r--r-- 1 nobody nobody 26526 30. Mai 13:46 LICENSE
-rw-r--r-- 1 nobody nobody  1465  2. Jun 11:27 Makefile.am
-rw-r--r-- 1 nobody nobody   249  2. Jun 11:28 README.md
drwxr-xr-x 3 nobody nobody  4096 30. Mai 19:30 src
