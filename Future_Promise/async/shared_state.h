#pragma once

#include <atomic>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>

#include <function2/function2.hpp>

#include "exec/executor.h"

namespace async::_detail {

template <typename T>
struct SharedState {
public:
    using Callback = fu2::unique_function<void(SharedState<T> &)>;

public:
    // These two static methods are used for creating Future with a ready result.
    static std::shared_ptr<SharedState<T>> MakeResult(T &&value) {
        auto res = std::make_shared<SharedState<T>>();
        res->state.store(READY);
        res->futures_counter.store(1, std::memory_order_relaxed);

        res->result = std::move(value);
        return res;
    }

    static std::shared_ptr<SharedState<T>> MakeException(std::exception_ptr ptr) {
        auto res = std::make_shared<SharedState<T>>();
        res->state.store(READY);
        res->futures_counter.store(1, std::memory_order_relaxed);

        res->exception = std::move(ptr);
        return res;
    }

    SharedState() = default;

    // Non-copyable.
    SharedState(const SharedState&) = delete;
    SharedState& operator=(const SharedState&) = delete;

    // Movable.
    SharedState(SharedState&&) = default;
    SharedState& operator=(SharedState&&) = default;

    ~SharedState() noexcept = default;

    template <typename F>
    void SetContinuation(F &&f) {
        assert(!continuation.has_value());

        auto prev = futures_counter.fetch_add(1);
        if (prev == 1) {
            // If value is not ready, just set it and signal its readiness.
            continuation.emplace(std::move(f));
            futures_counter.fetch_add(1);   // Will set counter to either 3 or 4.
        } else {
            // Otherwise future must be already created and value is ready - can call right away.
            assert(prev == 2);
            f(*this);
        }
    }

public:
    // States.
    static constexpr uint32_t INIT = 0;
    static constexpr uint32_t READY = 1;
    static constexpr uint32_t DONE_WAITING = 2;

    std::atomic<uint32_t> state {INIT};
    std::mutex state_mutex;
    std::condition_variable wait_result;

    std::optional<T> result;
    std::exception_ptr exception;

    std::atomic<uint32_t> futures_counter {0};

    exec::IExecutor *executor {nullptr};
    std::optional<Callback> continuation;
};

}  // namespace async::_detail
