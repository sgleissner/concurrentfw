cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0048 NEW)

# TODO: strip objects
# TODO: test application linking to static libaray
# TODO: make test application optional
# TODO: more robust install

project(concurrentfw
        VERSION 0.0.5
        DESCRIPTION "C++ Concurrent Framework"
        HOMEPAGE_URL "http://concurrentfw.de"
        LANGUAGES CXX
        )

# TODO perhaps get description from a file, used for rpm spec files
set(project_long_description "${CMAKE_PROJECT_DESCRIPTION}.")

set(github_home "https://github.com/sgleissner")
set(github_project "${github_home}/${CMAKE_PROJECT_NAME}")

include(GNUInstallDirs)

find_package(Catch2 3 REQUIRED)

option(COMPILE_COVERAGE "configure gcc for compiling with coverage code" OFF)
option(COMPILE_32BIT "configure 64 bit gcc for compiling 32 bit applications" OFF)

execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE SHORT_GIT_HASH_RESULT
        OUTPUT_VARIABLE SHORT_GIT_HASH_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (SHORT_GIT_HASH_RESULT EQUAL 0)
    set(SHORT_GIT_HASH "${SHORT_GIT_HASH_OUTPUT}")
    set(VERSION_ADDENDUM "-git_${SHORT_GIT_HASH_OUTPUT}")
endif ()

set(CMAKE_CXX_STANDARD 20)

#execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
#    OUTPUT_VARIABLE DUMPMACHINE)

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    add_compile_options(
            -mcx16
            -mmmx
            -msse
            -msse2
            -msse3
            -DX86_64_FAST_DW_LOAD
            -DX86_64_FAST_DW_STORE
    )
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
        -Wno-terminate
        -Werror
        -O3
)

if (COMPILE_COVERAGE)
    add_compile_options(-g -O0 --coverage -fprofile-abs-path)
    add_link_options(--coverage)
endif ()

if (COMPILE_32BIT)
    add_compile_options(-m32)
    add_link_options(-m32)
endif ()

set(public_headers
        src/concurrentfw/atomic_asm_dwcas_llsc.hpp
        src/concurrentfw/atomic_asm_x86.hpp
        src/concurrentfw/atomic_asm_arm.hpp
        src/concurrentfw/aba_wrapper.hpp
        src/concurrentfw/concurrent_ptr.hpp
        src/concurrentfw/stack.hpp
        src/concurrentfw/atomic.hpp
        src/concurrentfw/version.hpp
        src/concurrentfw/helper.hpp
        src/concurrentfw/futex.hpp
        src/concurrentfw/sysconf.hpp
        ${CMAKE_BINARY_DIR}/concurrentfw/generated_config.hpp
        )

set(library_sources
        src/futex.cpp
        src/stack.cpp
        src/sysconf.cpp
        )

set(test_sources
        src/tests/test_version.cpp
        src/tests/test_platform.cpp
        src/tests/test_aba_wrapper.cpp
        src/tests/test_concurrent_ptr.cpp
        src/tests/test_futex.cpp
        src/tests/test_stack.cpp
        src/tests/test_x86_asm.cpp
        src/tests/test_x86_asm_helper.cpp
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

set(file_generated_warning "This is a cmake-generated file, do not edit!")
configure_file(cmake-templates/${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
configure_file(cmake-templates/config.hpp.in concurrentfw/generated_config.hpp @ONLY)
#configure_file(cmake-templates/sonar-project.properties.in ${CMAKE_SOURCE_DIR}/sonar-project.properties @ONLY)

set_target_properties(concurrentfw PROPERTIES
        VERSION ${PROJECT_VERSION}
        NO_SONAME ON
        # SOVERSION ${PROJECT_VERSION}
        PUBLIC_HEADER "${public_headers}"
        OUTPUT_NAME ${PROJECT_NAME}-${PROJECT_VERSION}
        )

#target_include_directories(concurrentfw INTERFACE
#        PRIVATE src
#        OUTPUT_NAME ${PROJECT_NAME}
#        )

include_directories(BEFORE SYSTEM   # higher priority than older installed library
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}    # for generated_config.h
        )

message(NOTICE "Include dirs: ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}")

target_link_libraries(concurrentfw-tests PRIVATE
        Catch2::Catch2WithMain
        concurrentfw
        -lrt
        -pthread
        )

install(TARGETS concurrentfw
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        )

install(TARGETS concurrentfw-static
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        # PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        )

install(FILES ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.pc
        DESTINATION ${CMAKE_INSTALL_DATADIR}/pkgconfig
        )

include(CTest)
include(Catch)
catch_discover_tests(concurrentfw-tests)
