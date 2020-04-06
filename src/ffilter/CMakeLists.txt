cmake_minimum_required(VERSION 2.8.8)

project(nffilter)

set(NF_FILTER_DESCRIPTION, "Interpreter of filtering language with support for multiple types")

set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -Wall  -std=gnu++11")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wextra -pedantic")

set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} -Wall -std=gnu11")
set(CMAKE_C_FLAGS_RELEASE   "-O3")
set(CMAKE_C_FLAGS_DEBUG     "-g -O0 -Wextra -pedantic")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O2")

set(ENABLE_TESTING ON)

set(nf_filter_VERSION_MAJOR 1)
set(nf_filter_VERSION_MINOR 0)

set(SOURCES ${SOURCES} "ffilter.c")
set(SOURCES ${SOURCES} "fcore.c")
set(SOURCES ${SOURCES} "ffilter_internal.h")
set(SOURCES ${SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/ffilter_gram.c")
set(SOURCES ${SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/ffilter_lex.c")

#set(SOURCES ${SOURCES} "tests/ipx_core_perf.cpp")
#set(SOURCES ${SOURCES} "tests/ipx_filter_convertors.cpp")
#set(SOURCES ${SOURCES} "tests/ipx_filter_data_types.cpp")

#pass cmake settings
configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_BINARY_DIR}/config.h"
)

find_package(FLEX)
find_package(BISON)

if(NOT BISON_FOUND)
    message(WARNING "Missing Bison.\nTarget bison does not exist.")
elseif (NOT FLEX_FOUND)
    message(WARNING "Missing Flex.\nTarget flex does not exist.")
else()
    add_custom_command(
            #SOURCE ffilter_lex.l
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ffilter_lex.c
                    ${CMAKE_CURRENT_BINARY_DIR}/ffilter_lex.h
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND ${FLEX_EXECUTABLE}
            ARGS -L -o ${CMAKE_CURRENT_BINARY_DIR}/ffilter_lex.c
                    --header=${CMAKE_CURRENT_BINARY_DIR}/ffilter_lex.h ffilter_lex.l
            COMMAND test -f lex.yy.c && mv lex.yy.c ${CMAKE_CURRENT_BINARY_DIR}/ffilter_lex.c
    )
    add_custom_command(
            #SOURCE ffilter_gram.y
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ffilter_gram.c
                   ${CMAKE_CURRENT_BINARY_DIR}/ffilter_gram.h
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND ${BISON_EXECUTABLE}
            ARGS -Wno-other -l -o ${CMAKE_CURRENT_BINARY_DIR}/ffilter_gram.c
                    --defines=${CMAKE_CURRENT_BINARY_DIR}/ffilter_gram.h ffilter_gram.y
    )
endif()


###Google test
# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
        ${CMAKE_BINARY_DIR}/googletest-build)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
#link_directories(${CMAKE_CURRENT_BINARY_DIR})

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(str_convertors tests/ipx_filter_convertors.cpp)
target_link_libraries(str_convertors
        nf_filter
        gtest_main)
add_test(NAME str_convertors_test COMMAND str_convertors)

add_test(NAME data_types_prof_test COMMAND valgrind --tool=callgrind data_types)

add_executable(data_types tests/ipx_filter_data_types.cpp)
target_link_libraries(data_types
        nf_filter
        gtest_main)
add_test(NAME data_types_test COMMAND data_types)

add_executable(performance tests/ipx_core_perf.cpp)
target_link_libraries(performance
        nf_filter
        gtest_main)
add_test(NAME performance_test COMMAND perf_test)
##Google test

# Make library target
add_library(nf_filter SHARED ${SOURCES})
