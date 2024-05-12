project(inheritance)

cmake_minimum_required(VERSION 3.22)

add_link_options("-fuse-ld=lld")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O1")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(SOURCES
    main.cpp)

add_executable(inheritance_test ${SOURCES})

target_sources(inheritance_test PUBLIC
    macro_inheritance.h)

target_include_directories(inheritance_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(inheritance_test PUBLIC cxx_std_20)
