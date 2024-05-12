#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace exec {

// Unbounded blocking multi-producers/multi-consumers (MPMC) queue

template <typename T>
class UnboundedBlockingQueue {
public:
    bool Put(T elem) {
        std::lock_guard lg{take_mutex_};
        if (closed_) {
            return false;
        }
        buffer_.push_front(std::move(elem));
        // notifying due to possibly existing waiters
        if (buffer_.size() == 1) {
            take_cv_.notify_one();
        }
        return true;
    }

    std::optional<T> Take() {
        std::unique_lock lg{take_mutex_};
        while (buffer_.empty() && !closed_) {
            take_cv_.wait(lg);
        }
        return TakeUnderLock();
    }

    void Close() {
        std::lock_guard lg{take_mutex_};
        closed_ = true;
        take_cv_.notify_all();
    }

private:
    std::optional<T> TakeUnderLock() {
        if (buffer_.empty()) {
            return std::nullopt;
        }
        auto ret = std::optional<T>{std::move(buffer_.back())};
        buffer_.pop_back();
        return ret;
    }

private:
    // push front, pop back
    std::deque<T> buffer_;

    bool closed_ {false};

    std::condition_variable take_cv_;
    std::mutex take_mutex_;
};

}  // namespace exec
