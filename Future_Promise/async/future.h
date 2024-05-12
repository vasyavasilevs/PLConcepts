#pragma once

#include <cassert>
#include <concepts>
#include <memory>
#include <mutex>
#include <optional>

#include "async/shared_state.h"

namespace async {

namespace _detail {

template <typename F, typename T>
concept VoidReturnContinuation = requires (F &&f, _detail::SharedState<T> &s) {
    { f(s) } -> std::same_as<void>;
};

}   // namespace _detail

template <typename T>
class Future {
public:
    using Value = T;

public:
    static Future<T> MakeReady(T &&result) {
        return Future<T>(async::_detail::SharedState<T>::MakeResult(std::move(result)));
    }

    static Future<T> MakeException(std::exception_ptr ptr) {
        return Future<T>(async::_detail::SharedState<T>::MakeException(std::move(ptr)));
    }

    // Non-copyable
    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    // Movable
    Future(Future&&) = default;
    Future& operator=(Future&&) = default;

    ~Future() noexcept = default;

    // One-shot
    // Wait for result (value or exception)
    T Get() {
        std::unique_lock lg{state_->state_mutex};
        // Relaxed due to being under lock.
        auto s = state_->state.load(std::memory_order_relaxed);
        while (s == async::_detail::SharedState<T>::INIT) {
            // wait until state is changed
            state_->wait_result.wait(lg);
            // Relaxed due to being under lock.
            s = state_->state.load(std::memory_order_relaxed);
        }

        if (s == async::_detail::SharedState<T>::DONE_WAITING) {
            return GetReadyResult();
        }

        return GetUnderLock();
    }

    // Wait for result (value or exception)
    std::optional<T> TryGet() {
        // Sequential due to no lock held.
        auto s = state_->state.load();
        if (s == async::_detail::SharedState<T>::INIT) {
            return {};
        }
        if (s == async::_detail::SharedState<T>::DONE_WAITING) {
            return GetReadyResult();
        }
        std::unique_lock lg{state_->state_mutex};
        return GetUnderLock();
    }

    template <_detail::VoidReturnContinuation<T> F>
    void Then(F &&continuation) {
        state_->SetContinuation(std::move(continuation));
    }

    exec::IExecutor *GetExecutor() {
        return state_->executor;
    }

    void SetExecutor(exec::IExecutor *executor) {
        state_->executor = executor;
    }

private:
    Future(std::shared_ptr<async::_detail::SharedState<T>> state) : state_{state} {
        assert(state_ != nullptr);
    }

    T GetUnderLock() {
        // Relaxed due to being under lock.
        auto state_number = state_->state.load(std::memory_order_relaxed);
        assert(state_number != async::_detail::SharedState<T>::INIT);

        // If current call to `Get` is the first one.
        if (state_number == async::_detail::SharedState<T>::READY) {
            // Signal other threads that result was already retrieved.
            // Relaxed due to being under lock.
            state_->state.store(async::_detail::SharedState<T>::DONE_WAITING,
                                std::memory_order_relaxed);

            if (state_->result.has_value()) {
                return state_->result.value();
            }
            assert((state_->exception) && "result must hold exception");
            std::rethrow_exception(state_->exception);
        } else {
            // otherwise it violates future-promise contract
            throw std::runtime_error("double call to Future::Get");
        }
    }

    T GetReadyResult() {
        if (state_->exception) {
            std::rethrow_exception(state_->exception);
        }
        assert(state_->result.has_value());
        return state_->result.value();
    }

private:
    template <typename U>
    friend class Promise;

private:
    std::shared_ptr<async::_detail::SharedState<T>> state_;
};

static_assert(std::is_move_assignable<Future<int>>::value);

}  // namespace async
