#include "ThreadPool.h"

#include "Logger.h"

#define MAX_MAINTHREAD_TASKS_PER_CALL 1024

void ThreadPool::loop(unsigned int workerIndex) {
    while (true) {
        std::unique_ptr<FutureBase> job;
        {
            std::unique_lock<std::mutex> lock(m_workerJobsMtx);
            // Wait for a job arriving or terminate flag beeing set
            m_workerTaskAvailableCVar.wait(lock, [this] { return !m_workerJobs.empty() || m_terminateFlag.load(); });
            if (m_terminateFlag.load()) return;

            // Get first job in queue
            job = std::move(m_workerJobs.front());
            m_workerJobs.pop();
        }

        {
            std::lock_guard<std::mutex> lock(m_schedulingMtx);
            if (!isContextActive(job->getContext())) {
                job->cancel();
            } else {
                // Adjust tracking
                m_executionPointStatuses[workerIndex].isRunning = true;
            }
        }

        // Execute job + Release lock
        job->execute();  // This will never throw since errors are captured by the internal future object

        {
            // Push finished jobs to cleanup container
            std::lock_guard<std::mutex> lock(m_finishedJobsMtx);
            m_finishedJobs.emplace_back(std::move(job));
        }

        {
            std::lock_guard<std::mutex> lock(m_schedulingMtx);
            // Adjust tracking
            m_executionPointStatuses[workerIndex].completionCount++;
            m_executionPointStatuses[workerIndex].isRunning = false;
            m_watingForActiveTaskCVar.notify_all();
        }
    }
}

bool ThreadPool::isContextActive(uint64_t taskContext) const {
    return m_activeContextSet.find(taskContext) != m_activeContextSet.end();
}

bool ThreadPool::hasExecutionPassed(
    const std::vector<ExecutionSlotStatus>& a,
    const std::vector<ExecutionSlotStatus>& b
) const {
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i].isRunning && a[i].completionCount == b[i].completionCount) {
            return false;
        }
    }
    return true;
}

ThreadPool::ThreadPool(size_t numThreads)
    : m_terminateFlag(false),
      m_taskContextGen(0),
      m_activeContextSet(DEFAULT_TASKCONTEXT),
      m_executionPointStatuses(numThreads + 1) {
    m_threads.reserve(numThreads);
    for (size_t i = 0; i < numThreads; i++) {
        m_threads.emplace_back(&ThreadPool::loop, this, i);
    }
    m_activeContextSet.insert(DEFAULT_TASKCONTEXT);
}

ThreadPool::~ThreadPool() {
    if (!m_terminateFlag.load()) {
        shutdown();
    }
}

uint64_t ThreadPool::getNewTaskContext() {
    std::lock_guard<std::mutex> lock(m_schedulingMtx);
    uint64_t newContext = ++m_taskContextGen;

    if (!m_terminateFlag.load()) {
        m_activeContextSet.insert(newContext);
    }

    return newContext;
}

void ThreadPool::destroyTaskContext(uint64_t taskContext) {
    std::lock_guard<std::mutex> lock(m_schedulingMtx);
    m_activeContextSet.erase(taskContext);
}

void ThreadPool::waitForCurrentActiveTasks() {
    std::unique_lock<std::mutex> lock(m_schedulingMtx);

    std::vector<ExecutionSlotStatus> statusSnapshot = m_executionPointStatuses;
    m_watingForActiveTaskCVar.wait(lock, [&statusSnapshot, this] {
        return hasExecutionPassed(statusSnapshot, m_executionPointStatuses);
    });
}

void ThreadPool::pushJob(std::unique_ptr<FutureBase> future, Executor executor) {
    if (future->isEmpty() || future->isReady()) return;

    switch (executor) {
        case Executor::Main: {
            std::lock_guard<std::mutex> lock(m_mainThreadJobsMtx);
            m_mainThreadJobs.push(std::move(future));
        } break;

        case Executor::Worker: {
            std::lock_guard<std::mutex> lock(m_workerJobsMtx);
            m_workerJobs.push(std::move(future));
            m_workerTaskAvailableCVar.notify_one();
        } break;

        default: return;
    }
}

void ThreadPool::processMainThreadJobs() {
    std::unique_ptr<FutureBase> job;
    size_t processedJobCount = 0;
    while (true) {
        if (processedJobCount >= MAX_MAINTHREAD_TASKS_PER_CALL) {
            lgr::lout.warn("Exceeded maximum main thread tasks per call threshold");
            break;
        }
        {
            std::lock_guard<std::mutex> lock(m_mainThreadJobsMtx);
            if (m_terminateFlag.load()) return;
            if (m_mainThreadJobs.empty()) break;

            job = std::move(m_mainThreadJobs.front());
            m_mainThreadJobs.pop();
        }

        {
            std::lock_guard<std::mutex> lock(m_schedulingMtx);
            if (!isContextActive(job->getContext())) {
                job->cancel();
            } else {
                // Adjust tracking
                m_executionPointStatuses.back().isRunning = true;
            }
        }

        // Execute job + Release lock
        job->execute();  // This will never throw since errors are captured by the internal future object

        {
            // Push finished jobs to cleanup container
            std::lock_guard<std::mutex> lock(m_finishedJobsMtx);
            m_finishedJobs.emplace_back(std::move(job));
        }

        {
            std::lock_guard<std::mutex> lock(m_schedulingMtx);
            // Adjust tracking
            m_executionPointStatuses.back().completionCount++;
            m_executionPointStatuses.back().isRunning = false;
            m_watingForActiveTaskCVar.notify_all();
        }
        processedJobCount++;
    }
}

void ThreadPool::cleanupFinishedJobs() {
    std::lock_guard<std::mutex> lock(m_finishedJobsMtx);
    m_finishedJobs.clear();
}

void ThreadPool::shutdown() {
    m_terminateFlag.store(true);
    // Wake up all threads if after setting terminate flag
    m_workerTaskAvailableCVar.notify_all();
    // Wait for all threads to finish and exit
    for (std::thread& activeThread : m_threads) {
        activeThread.join();
    }
}
