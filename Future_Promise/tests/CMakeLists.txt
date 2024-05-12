set(BINARY tests)

set(SOURCES
    async_test.cpp
    future_promise_test.cpp
    main.cpp
    then_test.cpp
    )

add_executable(${BINARY} ${SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC async gtest)
