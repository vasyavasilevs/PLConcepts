#include <thread>

#include "gtest/gtest.h"

#include "async/async.h"

namespace async::tests {

class AsyncTest : public ::testing::Test {
public:
    void TearDown() override {
        for (auto &f : futures) {
            f.Get();
        }
        futures.clear();
    }

public:
    static constexpr int ITERATIONS = 1000000;

    std::vector<Future<int>> futures;
};

static int TestInLoop(int iterations, bool must_pause, bool must_throw) {
    int i = 0;
    for (; i < iterations; ++i) {
        if (must_pause) {
            asm volatile("pause");
        }
    }
    if (must_throw) {
        throw std::logic_error("");
    }
    return i;
}

TEST_F(AsyncTest, TestSync) {
    auto f = Async(Launch::sync, TestInLoop, ITERATIONS, true, false);
    // Must be synchronous.
    auto res = f.TryGet();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value(), ITERATIONS);
}

TEST_F(AsyncTest, TestAsync) {
    auto f = Async(Launch::async, TestInLoop, ITERATIONS, false, false);

    int result = 0;
    int wait_iterations = 0;
    for (; wait_iterations < ITERATIONS && result == 0; ++wait_iterations) {
        auto res = f.TryGet();
        if (res) {
            result = res.value();
        }
        asm volatile ("pause");
    }

    ASSERT_EQ(result, ITERATIONS);
    ASSERT_GT(wait_iterations, 0);
}

TEST_F(AsyncTest, TestAsyncFullLoad) {
    for (size_t i = 0; i < std::thread::hardware_concurrency() * 2; ++i) {
        futures.emplace_back(Async(Launch::async, TestInLoop, ITERATIONS * 10, true, false));
    }

    auto f = Async(Launch::sync, TestInLoop, ITERATIONS, true, false);
    // Must be synchronous.
    auto res = f.TryGet();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value(), ITERATIONS);
}

TEST_F(AsyncTest, TestSyncException) {
    auto f = Async(Launch::sync, TestInLoop, ITERATIONS, true, true);
    // Must be synchronous.
    ASSERT_THROW(f.TryGet(), std::logic_error);
}

static void TryGetLoop(Future<int> &f, int &wait_iterations) {
    for (; wait_iterations < AsyncTest::ITERATIONS; ++wait_iterations) {
        auto res = f.TryGet();
        ASSERT_FALSE(res.has_value());
        asm volatile ("pause");
    }
}

TEST_F(AsyncTest, TestAsyncException) {
    auto f = Async(Launch::async, TestInLoop, ITERATIONS, false, true);

    int wait_iterations = 0;
    ASSERT_THROW(TryGetLoop(f, wait_iterations), std::logic_error);
    ASSERT_GT(wait_iterations, 0);
}

TEST_F(AsyncTest, TestAsyncFullLoadException) {
    for (size_t i = 0; i < std::thread::hardware_concurrency() * 2; ++i) {
        futures.emplace_back(Async(Launch::async, TestInLoop, ITERATIONS * 10, true, false));
    }

    auto f = Async(Launch::sync, TestInLoop, ITERATIONS, true, true);
    // Must be synchronous.
    ASSERT_THROW(f.TryGet(), std::logic_error);
}

}   // namespace async::tests
