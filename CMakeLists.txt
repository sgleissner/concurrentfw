cmake_minimum_required(VERSION 3.21)

# TODO: strip objects
# TODO: test application linking to static libaray
# TODO: make test application and cpptest optional
# TODO: more robust install

project(concurrentfw
        VERSION 0.0.4
        DESCRIPTION "Concurrent Framework"
        HOMEPAGE_URL "http://concurrentfw.de"
        LANGUAGES CXX
        )

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_CPPCHECK "cppcheck")

#execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
#    OUTPUT_VARIABLE DUMPMACHINE)

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    add_compile_options(-mcx16 -mmmx -msse -msse2 -msse3)
elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
    message(FATAL_ERROR "${CMAKE_SYSTEM_PROCESSOR}: not yet supported")
elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
    add_compile_options(-march=armv8-a -mtune=cortex-a72) # Minimum architecure should be a RasPi 4
elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm")
    message(FATAL_ERROR "${CMAKE_SYSTEM_PROCESSOR}: not yet supported")
else ()
    message(FATAL_ERROR "${CMAKE_SYSTEM_PROCESSOR}: unsupported architecture")
endif ()

add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -O3
        -Wno-terminate
)

set(public_headers
        src/concurrentfw/atomic_asm_dwcas_llsc.hpp
        src/concurrentfw/atomic_asm_x86.hpp
        src/concurrentfw/atomic_asm_arm.hpp
        src/concurrentfw/aba_wrapper.hpp
        src/concurrentfw/concurrent_ptr.hpp
        src/concurrentfw/atomic.hpp
        src/concurrentfw/version.hpp
        src/concurrentfw/helper.hpp
        src/concurrentfw/futex.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/concurrentfw/generated_config.hpp
        )

set(library_sources
        src/futex.cpp
)

set(test_sources
        src/test/test_main.cpp
        src/test/test_benchmark_futex.cpp
        src/test/test_aba.cpp
)

add_library(concurrentfw SHARED
        ${library_sources}
)

add_library(concurrentfw-static STATIC
        ${library_sources}
)

add_executable(concurrentfw-test
        ${test_sources}
)

add_dependencies(concurrentfw-static concurrentfw-test)

configure_file(libconcurrentfw.pc.in libconcurrentfw.pc @ONLY)
configure_file(config.hpp.in concurrentfw/generated_config.hpp @ONLY)

set_target_properties(concurrentfw PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${public_headers}"
)

target_include_directories(concurrentfw INTERFACE
        PRIVATE src
)

include_directories(src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})    # for generated_config.h

target_link_libraries(concurrentfw-test -lrt)
target_link_libraries(concurrentfw-test -pthread)
target_link_libraries(concurrentfw-test concurrentfw)

install(TARGETS concurrentfw concurrentfw-static concurrentfw-test
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/concurrentfw
)

# message(WARNING "${CMAKE_INSTALL_DATAROOTDIR}")

install(FILES ${CMAKE_BINARY_DIR}/libconcurrentfw.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

## cppcheck target (static code analysis)
#CPPCHECK=cppcheck
#
## --enable=warning,performance,portability,information
#
#cppcheck:
#        $(CPPCHECK) --check-config --language=c++ --std=c++20 --platform=unix64 \
#                -D__x86_64__ -D__LP64__ -U__ILP32__ \
#                --inline-suppr \
#                --enable=warning,performance,portability \
#                --force -q \
#                -I $(top_srcdir)/src \
#                -I /usr/include \
#                -I /usr/include/c++/10 \
#                -I /usr/include/linux \
#                $(patsubst %,$(top_srcdir)/%,$(libconcurrentfw_la_SOURCES)) \
#                tee $(top_builddir)/cppcheck.result
