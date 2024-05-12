#include <cassert>

#include "exec/thread_pool.h"

namespace exec {

thread_local ThreadPool *SELF_ {nullptr};

ThreadPool::ThreadPool(size_t threads) : threads_count_(threads), free_workers_count_(threads) {
    workers_.reserve(threads_count_);
}

void ThreadPool::Start() {
    if (started_) {
        return;
    }

    for (size_t i = 0; i < threads_count_; ++i) {
        AddWorker();
    }
}

ThreadPool::~ThreadPool() {
    StopGracefully();
}

void ThreadPool::Submit(Task task) {
    // register new task to be able to wait it done
    // wait_group_.Add(1);
    tasks_queue_.Put(std::move(task));
}

/* static */
ThreadPool* ThreadPool::Current() {
    return SELF_;
}

void ThreadPool::Stop() {
    StopGracefully();
    // tasks_queue_.Close();
    stopped_.store(true);
}

bool ThreadPool::HasFreeWorkers() const {
    return free_workers_count_.load() > 0;
}

void ThreadPool::StopGracefully() {
    tasks_queue_.Close();
    for (auto &w : workers_) {
        w.join();
    }
    workers_.clear();
}

void ThreadPool::AddWorker() {
    ThreadPool::WorkerThread worker([self = this](){
        WorkerEntry(self);
    });
    workers_.emplace_back(std::move(worker));
}

/* static */
void ThreadPool::WorkerEntry(ThreadPool *pool) {
    assert(pool != nullptr);
    SELF_ = pool;

    while (!pool->stopped_.load()) {
        pool->free_workers_count_.fetch_add(1);
        auto task = pool->tasks_queue_.Take();
        if (!task) {
            return;
        }
        pool->free_workers_count_.fetch_sub(1);

        // execute function
        task.value()();
    }
}

}  // namespace exec
