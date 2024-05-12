#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "exec/executor.h"
#include "exec/queue.h"

namespace exec {

// Thread pool for independent CPU-bound tasks
// Fixed pool of worker threads + shared unbounded blocking queue

class ThreadPool : public IExecutor {
public:
    using WorkerThread = std::thread;

public:
    explicit ThreadPool(size_t threads);

    // Non-copyable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Non-movable
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool();

    void Start();

    // IExecutor
    void Submit(Task);

    static ThreadPool* Current();

    bool HasFreeWorkers() const;

    void Stop();

protected:
    void StopGracefully();

    void AddWorker();

    static void WorkerEntry(ThreadPool *pool);

private:
    bool started_ {false};

    const size_t threads_count_;
    std::vector<WorkerThread> workers_;

    UnboundedBlockingQueue<Task> tasks_queue_;

    std::atomic<size_t> free_workers_count_ {0};
    std::atomic<bool> stopped_ {false};
};

}  // namespace exec
