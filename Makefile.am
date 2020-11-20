
# CPU specific compiler options

if CPU_X86_64
# should work on most x86_64 machines, but cmpxchg16b is needed
OPTIONS_CPU= -mcx16 -mmmx -msse -msse2 -msse3
endif

if CPU_X86_32
OPTIONS_CPU=
endif

if CPU_ARM_64
OPTIONS_CPU=
endif

if CPU_ARM_32
OPTIONS_CPU=
endif

if CPU_UNDEFINED
OPTIONS_CPU=
endif

# install headers into /usr/include/<pkgname>/

# concurrentfwincludedir = $(includedir)/concurrentfw
# concurrentfwinclude_HEADERS =
pkginclude_HEADERS = \
	src/concurrentfw/atomic_asm_dwcas_llsc.h		\
	src/concurrentfw/atomic_asm_x86.h				\
	src/concurrentfw/atomic_asm_arm.h				\
	src/concurrentfw/aba_wrapper.h					\
	src/concurrentfw/concurrent_ptr.h				\
	src/concurrentfw/atomic.h						\
	src/concurrentfw/generated_config.h				\
	src/concurrentfw/version.h						\
	src/concurrentfw/helper.h						\
	src/concurrentfw/futex.h


# shared library

lib_LTLIBRARIES = libconcurrentfw.la

# -isystem src -isystem $(CURDIR)/src

# libconcurrentfw_la_LIBADD   = $(LIB_KPARTS)
libconcurrentfw_la_CPPFLAGS = $(all_includes) -I src -I$(top_srcdir)/src
libconcurrentfw_la_CXXFLAGS = -Wall -Wextra -Wpedantic -O3 $(OPTIONS_CPU)
libconcurrentfw_la_LDFLAGS  = $(all_libraries)
libconcurrentfw_la_SOURCES  = \
	src/futex.cpp

#SUBDIRS = src
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libconcurrentfw.pc


# test application

bin_PROGRAMS = concurrentfw_test

concurrentfw_test_CPPFLAGS = $(all_includes) -I src -I$(top_srcdir)/src
concurrentfw_test_CXXFLAGS = -Wall -Wextra -Wpedantic -O3 $(OPTIONS_CPU)
concurrentfw_test_LDFLAGS  = -L$(libdir) -Wl,-rpath=$(libdir) -lrt -lpthread -lconcurrentfw
concurrentfw_test_SOURCES  =			\
	src/test/test_main.cpp				\
	src/test/test_benchmark_futex.cpp	\
	src/test/test_aba.cpp
	
# cppcheck target (static code analysis)

CPPCHECK=cppcheck

# --enable=warning,performance,portability,information

cppcheck:
	$(CPPCHECK) --check-config --language=c++ --std=c++20 --platform=unix64 \
		-D__x86_64__ -D__LP64__ -U__ILP32__ \
		--inline-suppr \
		--enable=warning,performance,portability \
		--force -q \
		-I $(top_srcdir)/src \
		-I /usr/include \
		-I /usr/include/c++/10 \
		-I /usr/include/linux \
		$(patsubst %,$(top_srcdir)/%,$(libconcurrentfw_la_SOURCES)) \
		tee $(top_builddir)/cppcheck.result

