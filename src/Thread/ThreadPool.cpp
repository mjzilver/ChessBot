#include "ThreadPool.h"

#include <stdexcept>
#include <thread>

ThreadPool::ThreadPool() {
    for (size_t i = 0; i < nThreads; i++) {
        workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (stop) throw std::runtime_error("submit on stopped ThreadPool");
        tasks.push(std::move(task));
    }
    cvWorkers.notify_one();
}

void ThreadPool::join() {
    std::unique_lock<std::mutex> lock(queueMutex);
    cvJoin.wait(lock, [this] { return tasks.empty() && activeTasks == 0; });
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cvWorkers.wait(lock, [this] { return stop || !tasks.empty(); });

            if (stop && tasks.empty()) return;

            task = std::move(tasks.front());
            tasks.pop();
            ++activeTasks;
        }

        task();

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            --activeTasks;
            if (tasks.empty() && activeTasks == 0) {
                cvJoin.notify_one();
            }
        }
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    cvWorkers.notify_all();

    for (auto& thread : workers) {
        if (thread.joinable()) thread.join();
    }
}