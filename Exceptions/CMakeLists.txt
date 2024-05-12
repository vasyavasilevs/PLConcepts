project(exceptions)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    macro_exceptions.cpp
    main.cpp)

add_executable(exceptions_test ${SOURCES})
