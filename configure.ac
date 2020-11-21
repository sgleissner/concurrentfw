# configure autoconf & automake
AC_INIT([concurrentfw], [0.0.2], [bugs@concurrentfw.de], [concurrentfw], [http://concurrentfw.de])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_FILES([Makefile libconcurrentfw.pc])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# use libtool
AM_PROG_AR
LT_PREREQ([2.4])
LT_INIT

# custom config header, usable in library headers (package autoconf-archive needed)
AC_CONFIG_HEADERS(config.h)
AX_PREFIX_CONFIG_H(src/concurrentfw/generated_config.h)

# get CPU for target platform
AC_CANONICAL_HOST
AS_CASE([$host_cpu],
    [x86_64],  [PLATFORM="x86_64"],
    [i686],    [PLATFORM="x86_32"],
    [aarch64], [PLATFORM="arm_64"],
    [arm*],    [PLATFORM="arm_32"],
               [PLATFORM="undefined"])
AM_CONDITIONAL([CPU_X86_64], [test "$PLATFORM" = "x86_64"])
AM_CONDITIONAL([CPU_X86_32], [test "$PLATFORM" = "x86_32"])
AM_CONDITIONAL([CPU_ARM_64], [test "$PLATFORM" = "arm_64"])
AM_CONDITIONAL([CPU_ARM_32], [test "$PLATFORM" = "arm_32"])
AM_CONDITIONAL([CPU_UNDEFINED], [test "$PLATFORM" = "undefined"])

# used tools
AC_PROG_CXX
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

# use C++20 (start)
AC_LANG_PUSH([C++])
AX_CHECK_COMPILE_FLAG([-std=c++20],
	[CXXFLAGS="$CXXFLAGS -std=c++20"],
	[AX_CHECK_COMPILE_FLAG([-std=c++2a],
		[CXXFLAGS="$CXXFLAGS -std=c++2a"],
		[AC_MSG_ERROR([C++20 support needed, either -std=c++2a or -std=c++20 must be supported])])])
AC_LANG_POP([C++])
# in the future: use AX_CXX_COMPILE_STDCXX([20], [noext], [mandatory])
# use C++20 (end)

# Checks for libraries
AC_CHECK_LIB([rt], [aio_suspend])
AC_CHECK_LIB([pthread], [pthread_create])

# install to /usr
AC_PREFIX_DEFAULT (/usr)

# generate files
AC_OUTPUT
