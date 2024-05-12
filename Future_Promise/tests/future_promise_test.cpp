#include <thread>

#include "gtest/gtest.h"

#include "async/future.h"
#include "async/promise.h"

namespace async::tests {

class FuturePromiseTest : public ::testing::Test {
public:
    static constexpr int ITERATIONS = 1000000;
};

TEST_F(FuturePromiseTest, TestGet) {
    async::Promise<int> p;
    auto f = p.MakeFuture();

    std::jthread t([p = std::move(p)]() mutable {
        int i = 0;
        for (; i < ITERATIONS; ++i) {
            asm volatile("pause");
        }
        std::move(p).SetValue(i);
    });

    ASSERT_EQ(f.Get(), ITERATIONS);
}

TEST_F(FuturePromiseTest, TestTryGet) {
    async::Promise<int> p;
    auto f = p.MakeFuture();

    std::jthread t([p = std::move(p)]() mutable {
        int i = 0;
        for (; i < ITERATIONS; ++i) {
        }
        std::move(p).SetValue(i);
    });

    int result = 0;
    int wait_iterations = 0;
    for (; wait_iterations < ITERATIONS && result == 0; ++wait_iterations) {
        auto res = f.TryGet();
        if (res) {
            result = res.value();
        }
    }
    ASSERT_EQ(result, ITERATIONS);
    ASSERT_GT(wait_iterations, 0);
}

TEST_F(FuturePromiseTest, TestException) {
    async::Promise<int> p;
    auto f = p.MakeFuture();

    std::jthread t([p = std::move(p)]() mutable {
        int i = 0;
        for (; i < ITERATIONS; ++i) {
            asm volatile("pause");
        }

        try {
            throw std::logic_error("");
        } catch(...) {
            std::move(p).SetException(std::current_exception());
        }
    });

    ASSERT_THROW(f.Get(), std::logic_error);
}

static void TryGetLoop(Future<int> &f, int &wait_iterations) {
    for (; wait_iterations < FuturePromiseTest::ITERATIONS; ++wait_iterations) {
        auto res = f.TryGet();
        ASSERT_FALSE(res.has_value());
    }
}

TEST_F(FuturePromiseTest, TestTryException) {
    async::Promise<int> p;
    auto f = p.MakeFuture();

    std::jthread t([p = std::move(p)]() mutable {
        int i = 0;
        for (; i < ITERATIONS; ++i) {
        }

        try {
            throw std::logic_error("");
        } catch(...) {
            std::move(p).SetException(std::current_exception());
        }
    });

    int wait_iterations = 0;
    ASSERT_THROW(TryGetLoop(f, wait_iterations), std::logic_error);
    ASSERT_GT(wait_iterations, 0);
}

}   // namespace async::tests
