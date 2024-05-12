#include <thread>

#include "gtest/gtest.h"

#include "async/async.h"
#include "async/then.h"

namespace async::tests {

class ThenTest : public ::testing::Test {
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

TEST_F(ThenTest, TestSimpleThen) {
    Promise<int> p;
    auto composed = p.MakeFuture() | Then([](int value) { return value * 2.0; });

    ASSERT_FALSE(composed.TryGet().has_value());

    std::move(p).SetValue(1);

    auto result = composed.TryGet();
    ASSERT_TRUE(result.has_value());
    ASSERT_DOUBLE_EQ(result.value(), 2.0);
}

TEST_F(ThenTest, TestAsyncThen) {
    auto f = Async(TestInLoop, ITERATIONS, true, false);
    auto composed = (f & Then([](int value) { return value * 4.0; })) |
        Then([](double value) { return value / 2.0; });

    ASSERT_FALSE(composed.TryGet().has_value());

    ASSERT_DOUBLE_EQ(composed.Get(), ITERATIONS * 2.0);
}

TEST_F(ThenTest, TestSimpleThenException) {
    Promise<int> p;
    auto f = p.MakeFuture();
    auto composed = (f & Then([](int value) { return value * 2.0; })) |
        Then([](double value) { return value * 2.0; });

    ASSERT_FALSE(composed.TryGet().has_value());

    try {
        throw std::logic_error("");
    } catch(...) {
        std::move(p).SetException(std::current_exception());
    }

    ASSERT_THROW(composed.TryGet(), std::logic_error);
}

TEST_F(ThenTest, TestAsyncThenException) {
    auto f = Async(TestInLoop, ITERATIONS, true, false);
    auto with_exception = f & Then([](int value) { throw std::logic_error(std::to_string(value)); return 1.0; });
    auto composed = with_exception & Then([](double value) { return value * 2.0; });

    ASSERT_FALSE(composed.TryGet().has_value());

    ASSERT_THROW(with_exception.Get(), std::logic_error);

    ASSERT_EQ(f.Get(), ITERATIONS);
}

}   // namespace async::tests
