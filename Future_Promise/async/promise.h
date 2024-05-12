#pragma once

#include <memory>
#include <mutex>
#include <type_traits>

#include "async/shared_state.h"
#include "async/future.h"

namespace async {

template <typename T>
class Promise {
public:
    Promise() : state_(new async::_detail::SharedState<T>) {}

    // Non-copyable
    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    // Movable
    Promise(Promise&&) = default;
    Promise& operator=(Promise&&) = default;

    ~Promise() noexcept = default;

    // One-shot
    [[nodiscard]] Future<T> MakeFuture() {
        // check that this call is the first one
        if (state_->futures_counter.fetch_add(1) == 0) {
            return Future<T>(state_);
        }
        throw std::runtime_error("double call to Promise::MakeFuture");
    }

    // One-shot
    // Fulfill promise with value
    void SetValue(T &&value) && {
        Set<T, false>(std::forward<T>(value));
    }

    template <typename U>
    void SetValue(U value) && {
        Set<T, false>(std::forward<U>(value));
    }

    // One-shot
    // Fulfill promise with exception
    void SetException(std::exception_ptr ptr) && {
        Set<std::exception_ptr, true>(std::move(ptr));
    }

    void SetExecutor(exec::IExecutor *executor) {
        state_->executor = executor;
    }

private:
    template <typename U, bool IS_EXCEPTION>
    void Set(U &&opt) {
        std::lock_guard lg{state_->state_mutex};

        // Relaxed due to being under lock.
        // If result has not been previously set.
        if (state_->state.load(std::memory_order_relaxed) == async::_detail::SharedState<U>::INIT) {
            // Set value or exception.
            if constexpr (IS_EXCEPTION) {
                state_->exception = std::forward<U>(opt);
            } else {
                state_->result = std::forward<U>(opt);
            }
            // State transition.
            // Relaxed due to being under lock.
            state_->state.store(async::_detail::SharedState<U>::READY, std::memory_order_relaxed);
            // `notify_all` due to mutliple threads possibly waiting in `Future::Get`
            // - after waking up all of them, only one will eventually get the result,
            // while others will catch exception due to interface misuse
            state_->wait_result.notify_all();

            HandleContinuation();
        } else {
            throw std::runtime_error("double call to Promise::set");
        }
    }

    void HandleContinuation() {
        auto prev = state_->futures_counter.fetch_add(1);
        if (prev == 2) {
            // Wait until continuation is set.
            while (state_->futures_counter.load() != 4) {
                asm volatile("pause");
            }
            std::invoke(state_->continuation.value(), *state_);
        } else if (prev == 3) {
            std::invoke(state_->continuation.value(), *state_);
        } else {
            // Future must be already created.
            assert(prev == 1);
            // Continuation will be called after it's set.
        }
    }

private:
    std::shared_ptr<async::_detail::SharedState<T>> state_ {nullptr};
};

static_assert(std::is_move_assignable<Promise<int>>::value);

}  // namespace async
