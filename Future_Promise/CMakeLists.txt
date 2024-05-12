project(async)

cmake_minimum_required(VERSION 3.22)

function(add_git_submodule dir)
    # add a Git submodule directory to CMake, assuming the
    # Git submodule directory is a CMake project.
    #
    # Usage: in CMakeLists.txt
    #
    # include(AddGitSubmodule.cmake)
    # add_git_submodule(mysubmod_dir)

    find_package(Git REQUIRED)

    if(NOT EXISTS ${dir}/CMakeLists.txt)
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${dir}
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    endif()

    add_subdirectory(${dir})

endfunction(add_git_submodule)

add_link_options("-fuse-ld=lld")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O1")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(third_party/function2)

set(SOURCES
    async/async.cpp
    exec/thread_pool.cpp)

add_library(async STATIC ${SOURCES})

target_sources(async PUBLIC
    async/async.h
    async/future.h
    async/promise.h
    async/shared_state.h
    async/then.h
    exec/executor.h
    exec/queue.h
    exec/thread_pool.h)

target_include_directories(async PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(async PUBLIC cxx_std_20)

target_link_libraries(async PUBLIC function2)

add_git_submodule(third_party/googletest)
add_subdirectory(tests)
