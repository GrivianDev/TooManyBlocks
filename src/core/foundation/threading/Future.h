#ifndef TOOMANYBLOCKS_FUTURE_H
#define TOOMANYBLOCKS_FUTURE_H

#include <stddef.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <vector>

constexpr uint64_t DEFAULT_TASKCONTEXT = 0;

enum class Executor {
    Worker,
    Main
};

enum class FutureStatus {
    // State allows increasing the dependency count
    Building,
    Finalized,
    Pending,
    Running,
    Completed,
    Failed
};

#ifdef ENABLE_FUTURE_DEBUGGING
#include <sstream>
#include <string>
#include <thread>
#include <typeinfo>

#include "Logger.h"

constexpr const char* _toString(Executor executor) {
    switch (executor) {
        case Executor::Worker: return "Worker";
        case Executor::Main: return "Main";
    }
    return "Unknown";
}

constexpr const char* _toString(FutureStatus status) {
    switch (status) {
        case FutureStatus::Building: return "Building";
        case FutureStatus::Finalized: return "Finalized";
        case FutureStatus::Pending: return "Pending";
        case FutureStatus::Running: return "Running";
        case FutureStatus::Completed: return "Completed";
        case FutureStatus::Failed: return "Failed";
    }
    return "Unknown";
}

#endif

class FutureBase {
    template <typename T>
    friend class Future;

private:
    virtual void addDependent(const std::shared_ptr<FutureBase>& other) = 0;

    virtual void dependecyFinished() = 0;

public:
#ifdef ENABLE_FUTURE_DEBUGGING
    inline static std::atomic<uint64_t> nextDebugId{1};
#endif

    inline static void (*scheduleCallback)(std::unique_ptr<FutureBase>, Executor) = nullptr;

    virtual ~FutureBase() = default;

    virtual uint64_t getContext() const = 0;

    virtual bool isEmpty() const = 0;

    virtual bool isReady() const = 0;

    virtual void execute() = 0;

    virtual void cancel() = 0;
};

/**
 * Advanced future class for async tasks. Once the task finished, the future is no longer responsible
 * for the result. It will just cleanly provide the value to all consumers that hold an future instance
 * pointing to the result value. All futures can only be executed once.
 *
 * Futures can also be chained. This way it can be guranteed that once one future runs,all dependency futures
 * are ready so await never needs to be called.
 */
template <typename T>
class Future : public FutureBase {
private:
    template <typename U>
    friend class Future;

#ifdef ENABLE_FUTURE_DEBUGGING
    struct DebugInfo {
        uint64_t id;
        std::string name;
        const char* typeName;
    };
#endif

    struct TaskState {
        std::atomic<FutureStatus> status{FutureStatus::Building};
        Executor executor;
        uint64_t taskContext;
        std::atomic<size_t> handleCount{1};

        std::mutex mtx;
        std::condition_variable cv;

        std::atomic<int> unresolvedDeps{0};
        std::vector<std::shared_ptr<FutureBase>> dependents;

        std::function<T()> task;
        std::conditional_t<!std::is_void_v<T>, std::optional<T>, char> value;
        std::exception_ptr exception;

#ifdef ENABLE_FUTURE_DEBUGGING
        DebugInfo debugInfo;

        TaskState() { debugInfo.typeName = typeid(T).name(); }

        ~TaskState() { logDebug(this, "destroyed"); }
#endif
    };

    struct StateRef {
        std::mutex mtx;
        std::shared_ptr<TaskState> state;
    };

    std::shared_ptr<StateRef> stateRef;

    void completeSuccess(const std::shared_ptr<TaskState>& s, std::conditional_t<std::is_void_v<T>, char, T>&& v = 0) {
        FutureStatus expected = FutureStatus::Running;
        if (!s->status.compare_exchange_strong(expected, FutureStatus::Completed)) {
            return;  // already completed by other means
        }

        std::lock_guard<std::mutex> lock(s->mtx);
        if constexpr (!std::is_void_v<T>) {
            s->value.emplace(std::move(v));
        }

#ifdef ENABLE_FUTURE_DEBUGGING
        logDebug(s, "completed");
#endif

        onCompleted(s);
    }

    void completeFailure(const std::shared_ptr<TaskState>& s, const std::exception_ptr& e) {
        FutureStatus expected = FutureStatus::Running;
        if (!s->status.compare_exchange_strong(expected, FutureStatus::Failed)) {
            return;  // already completed by other means
        }

        std::lock_guard<std::mutex> lock(s->mtx);
        s->exception = e;

#ifdef ENABLE_FUTURE_DEBUGGING
        logDebug(s, "failed");
#endif

        onCompleted(s);
    }

    void onCompleted(const std::shared_ptr<TaskState>& s) {
        for (const std::shared_ptr<FutureBase>& dep : s->dependents) {
            // Decrement remaining dependency count
            dep->dependecyFinished();
        }

        std::vector<std::shared_ptr<FutureBase>>().swap(s->dependents);  // Force release vector allocation
        s->cv.notify_all();
    }

    virtual void addDependent(const std::shared_ptr<FutureBase>& other) override {
        getState()->dependents.emplace_back(other);
    }

    virtual void dependecyFinished() override {
        auto s = getState();
        if (s->unresolvedDeps.fetch_sub(1) == 1) {
#ifdef ENABLE_FUTURE_DEBUGGING
            logDebug(s, "dependencies-completed");
#endif
            trySchedule();
        }
    }

    virtual uint64_t getContext() const override { return getState()->taskContext; }

    void trySchedule() {
        if (isEmpty()) throw std::runtime_error("Cannot schedule empty future");

        auto s = getState();
        std::lock_guard<std::mutex> lock(s->mtx);
        if (s->unresolvedDeps.load() > 0) {
            return;
        }

        FutureStatus expected = FutureStatus::Finalized;
        if (!s->status.compare_exchange_strong(expected, FutureStatus::Pending)) {
            return;  // already scheduled by someone else or not finalized
        }

#ifdef ENABLE_FUTURE_DEBUGGING
        logDebug(s, "scheduled");
#endif

        std::unique_ptr<Future<T>> selfRef = std::make_unique<Future<T>>(*this);
        FutureBase::scheduleCallback(std::move(selfRef), s->executor);
    }

    std::shared_ptr<TaskState> getState() const {
        if (stateRef) {
            std::lock_guard<std::mutex> lock(stateRef->mtx);
            return stateRef->state;
        }
        return nullptr;
    }

    void incrementHandleCount() {
        auto s = getState();
        if (s) s->handleCount.fetch_add(1, std::memory_order_relaxed);
    }

    void decrementHandleCount() {
        auto s = getState();
        if (s) s->handleCount.fetch_sub(1, std::memory_order_relaxed);
    }

    static std::shared_ptr<StateRef> createState() {
        auto ref = std::make_shared<StateRef>();
        ref->state = std::make_shared<TaskState>();
        return ref;
    }

#ifdef ENABLE_FUTURE_DEBUGGING

    static void logDebug(const std::shared_ptr<TaskState>& s, const char* event) { logDebug(s.get(), event); }

    static void logDebug(const TaskState* s, const char* event) {
        if (!s || s->debugInfo.name.empty()) return;
        std::stringstream ss;
        ss << event << " [Future#" << s->debugInfo.id << ": \"" << s->debugInfo.name
           << "\", type=" << s->debugInfo.typeName << ", status=" << _toString(s->status.load())
           << ", deps=" << s->unresolvedDeps.load() << ", handles=" << s->handleCount.load(std::memory_order_relaxed)
           << ", executor=" << _toString(s->executor) << ", context=" << s->taskContext;

        if constexpr (!std::is_void_v<T>) {
            if (s->value.has_value()) {
                ss << ", value=" << static_cast<const void*>(std::addressof(*s->value));
            } else {
                ss << ", value=null";
            }
        }

        ss << ", thread=" << std::this_thread::get_id() << "]";
        lgr::lout.debug(ss.str());
    }

#endif

public:
    template <typename U = T>
    static std::enable_if_t<!std::is_void_v<U>, Future<T>> completed(U value) {
        Future<T> future;
        future.stateRef = createState();
        future.stateRef->state->status = FutureStatus::Completed;
        future.stateRef->state->value.emplace(std::move(value));

        return future;
    }

    static Future<void> completed() {
        Future<void> future;
        future.stateRef = createState();
        future.stateRef->state->status = FutureStatus::Completed;

        return future;
    }

    static Future<T> deferred() {
        Future<T> future;
        future.stateRef = createState();
        return future;
    }

    constexpr Future() noexcept = default;

    Future(const Future& other) : stateRef(other.stateRef) { incrementHandleCount(); }

    Future(Future&& other) noexcept : stateRef(std::move(other.stateRef)) {}

    template <
        typename F,
        typename =
            std::enable_if_t<std::is_invocable<F&>::value && std::is_convertible<std::invoke_result_t<F&>, T>::value>>
    Future(F&& fn, uint64_t taskContext = DEFAULT_TASKCONTEXT, Executor executor = Executor::Worker)
        : stateRef(createState()) {
        auto& s = stateRef->state;
        s->task = std::move(fn);
        s->taskContext = taskContext;
        s->executor = executor;
    }

    virtual ~Future() { decrementHandleCount(); }

    template <typename U>
    Future<T>& dependsOn(Future<U> other) {
        if (isEmpty() || other.isEmpty()) throw std::runtime_error("Cannot depend on empty future");

        auto s = getState();
        if (s->status.load() != FutureStatus::Building)
            throw std::runtime_error("Trying to add dependency to a finalized future");

        // Hold both locks to avoid concurrent state changes
        std::lock_guard<std::mutex> lock(s->mtx);
        std::lock_guard<std::mutex> otherLock(other.getState()->mtx);

        if (other.isReady()) return *this;

        s->unresolvedDeps.fetch_add(1);

        other.addDependent(std::make_shared<Future<T>>(*this));

#ifdef ENABLE_FUTURE_DEBUGGING
        logDependency<U>(s.get(), other.getState().get());
#endif

        return *this;
    }

    void resolve(Future<T> other) {
        if (isEmpty() || other.isEmpty()) throw std::runtime_error("Cannot resolve empty future");

        auto from = getState();
        auto to = other.getState();

        std::vector<std::shared_ptr<FutureBase>> fromDependents;

        {
            std::scoped_lock lock(from->mtx, to->mtx);
            fromDependents.swap(from->dependents);
            {
                std::lock_guard lock(stateRef->mtx);
                stateRef->state = to;
            }
            to->handleCount.fetch_add(from->handleCount.load());

            if (to->status.load() == FutureStatus::Completed || to->status.load() == FutureStatus::Failed) {
                for (auto& dependent : fromDependents) dependent->dependecyFinished();
            } else {
                to->dependents.insert(to->dependents.end(), fromDependents.begin(), fromDependents.end());
            }
        }
#ifdef ENABLE_FUTURE_DEBUGGING
        logDebug(to, "resolved-to");
#endif
    }

    Future<T>& start() {
        if (isEmpty()) throw std::runtime_error("Cannot start empty future");

        // Advance status to Finalized if neeeded
        FutureStatus expected = FutureStatus::Building;
        getState()->status.compare_exchange_strong(expected, FutureStatus::Finalized);

        trySchedule();
        return *this;
    }

    void execute() override {
        if (isEmpty()) throw std::runtime_error("Cannot execute empty future");

        auto s = getState();

        FutureStatus expected = FutureStatus::Building;
        s->status.compare_exchange_strong(expected, FutureStatus::Finalized);
        expected = FutureStatus::Pending;
        if (!s->status.compare_exchange_strong(expected, FutureStatus::Running)) {
            return;  // already executed by someone else
        }

#ifdef ENABLE_FUTURE_DEBUGGING
        logDebug(s, "execute-begin");
#endif

        std::function<T()> tmpTask = std::move(s->task);
        s->task = {};

        try {
            if (!tmpTask) throw std::runtime_error("Cannot execute future without a task");

            if constexpr (std::is_void_v<T>) {
                tmpTask();
                completeSuccess(s);
            } else {
                T result = tmpTask();
                completeSuccess(s, std::move(result));
            }
        } catch (...) {
            completeFailure(s, std::current_exception());
        }
    }

    void cancel() override {
        if (isEmpty() || isReady()) return;

        // Force advance status to Running so that completeFailure can run
        auto s = getState();
        FutureStatus expected = FutureStatus::Building;
        s->status.compare_exchange_strong(expected, FutureStatus::Running);
        expected = FutureStatus::Finalized;
        s->status.compare_exchange_strong(expected, FutureStatus::Running);
        expected = FutureStatus::Pending;
        s->status.compare_exchange_strong(expected, FutureStatus::Running);

#ifdef ENABLE_FUTURE_DEBUGGING
        logDebug(s, "cancel-requested");
#endif

        completeFailure(s, std::make_exception_ptr(std::runtime_error("Task canceled")));
    }

    void reset() {
        decrementHandleCount();
        stateRef.reset();
    }

    // Caller suspends until future has been executed / finished with error
    void await() {
        if (isEmpty()) return;
        auto s = getState();
        std::unique_lock<std::mutex> lock(s->mtx);

#ifdef ENABLE_FUTURE_DEBUGGING
        logDebug(s, "await-begin");
#endif
        s->cv.wait(lock, [this] { return isReady(); });
#ifdef ENABLE_FUTURE_DEBUGGING
        logDebug(s, "await-end");
#endif
    }

    bool isEmpty() const override { return !stateRef || !stateRef->state; }

    bool isReady() const override {
        auto s = getState();
        if (!s) return false;
        FutureStatus status = s->status.load();
        return status == FutureStatus::Completed || status == FutureStatus::Failed;
    }

    bool hasError() const {
        auto s = getState();
        return s && s->status.load() == FutureStatus::Failed;
    }

    size_t useCount() const {
        auto s = getState();
        return s ? s->handleCount.load(std::memory_order_relaxed) : 0;
    }

    template <typename U = T>
    std::enable_if_t<!std::is_void_v<U>, const U&> inline value() const {
        if (!isReady()) throw std::runtime_error("Acessing unfinished or empty future");
        auto s = getState();
        if (s->exception) std::rethrow_exception(s->exception);
        return *s->value;
    }

    template <typename U = T>
    std::enable_if_t<!std::is_void_v<U>, U&> inline value() {
        if (!isReady()) throw std::runtime_error("Acessing unfinished or empty future");
        auto s = getState();
        if (s->exception) std::rethrow_exception(s->exception);
        return *s->value;
    }

    std::exception_ptr getException() const {
        if (!hasError()) return nullptr;
        return getState()->exception;
    }

    Future<T>& operator=(const Future<T>& other) {
        if (this == &other) return *this;

        decrementHandleCount();

        stateRef = other.stateRef;
        incrementHandleCount();

        return *this;
    }

    Future<T>& operator=(Future<T>&& other) noexcept {
        if (this == &other) return *this;

        decrementHandleCount();

        stateRef = std::move(other.stateRef);

        return *this;
    }

#ifdef ENABLE_FUTURE_DEBUGGING

    template <typename U>
    void logDependency(const TaskState* future, const typename Future<U>::TaskState* dependency) {
        if (!future || !dependency) return;
        if (future->debugInfo.name.empty() && dependency->debugInfo.name.empty()) return;

        std::stringstream ss;

        ss << "dependency [";

        if (!future->debugInfo.name.empty()) {
            ss << "Future#" << future->debugInfo.id << ": \"" << future->debugInfo.name << "\"";
        } else {
            ss << "<untracked>" << static_cast<const void*>(future);
        }
        ss << ", type=" << future->debugInfo.typeName << "] -> [";

        if (!dependency->debugInfo.name.empty()) {
            ss << "Future#" << dependency->debugInfo.id << ": \"" << dependency->debugInfo.name << "\"";
        } else {
            ss << "<untracked>" << static_cast<const void*>(dependency);
        }
        ss << ", type=" << dependency->debugInfo.typeName << "]";

        lgr::lout.info(ss.str());
    }

    Future<T>& debug(const std::string& name) {
        auto s = getState();

        s->debugInfo.id = FutureBase::nextDebugId.fetch_add(1, std::memory_order_relaxed);
        s->debugInfo.name = name;

        return *this;
    }
#endif
};

#endif
