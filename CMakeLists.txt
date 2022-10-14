cmake_minimum_required(VERSION 3.21)

# TODO: strip objects
# TODO: test application linking to static libaray
# TODO: make test application optional
# TODO: more robust install

project(concurrentfw
        VERSION 0.0.4
        DESCRIPTION "Concurrent Framework"
        HOMEPAGE_URL "http://concurrentfw.de"
        LANGUAGES CXX
        )

include(GNUInstallDirs)

find_package(Catch2 3 REQUIRED)

set(CMAKE_CXX_STANDARD 20)

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
        src/test/test_version.cpp
        src/test/test_platform.cpp
        src/test/test_aba_wrapper.cpp
        src/test/test_concurrent_ptr.cpp
        # src/test/test_benchmark_futex.cpp
        )

add_library(concurrentfw SHARED
        ${library_sources}
)

add_library(concurrentfw-static STATIC
        ${library_sources}
)

add_executable(concurrentfw-tests
        ${test_sources}
)

add_dependencies(concurrentfw-static concurrentfw-tests)

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

target_link_libraries(concurrentfw-tests PRIVATE
        Catch2::Catch2WithMain
        concurrentfw
        )

include(CTest)
include(Catch)
catch_discover_tests(concurrentfw-tests)

#target_link_libraries(concurrentfw-tests -lrt)
#target_link_libraries(concurrentfw-tests -pthread)
#target_link_libraries(concurrentfw-tests concurrentfw)

install(TARGETS concurrentfw concurrentfw-static
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/concurrentfw
        )

# message(WARNING "${CMAKE_INSTALL_DATAROOTDIR}")

install(FILES ${CMAKE_BINARY_DIR}/libconcurrentfw.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        )
