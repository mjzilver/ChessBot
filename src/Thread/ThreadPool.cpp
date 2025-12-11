#include "ThreadPool.h"

#include <thread>

ThreadPool::ThreadPool() {
    for (size_t i = 0; i < nThreads; i++) {
        workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        tasks.push(std::move(task));
    }
    cv.notify_one();
}

void ThreadPool::join() {
    std::unique_lock<std::mutex> lock(queueMutex);
    cv.wait(lock, [this]() { return tasks.empty(); });
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] { return !tasks.empty() || stop; });

            if (stop && tasks.empty()) return;

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();

        cv.notify_all();
    }
}

ThreadPool::~ThreadPool() {
    stop = true;
    cv.notify_all();
    for (auto& thread : workers) {
        if (thread.joinable()) thread.join();
    }
}