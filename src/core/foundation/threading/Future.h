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

class FutureBase {
    template <typename T>
    friend class Future;

private:
    virtual void addDependent(const std::shared_ptr<FutureBase>& other) = 0;

    virtual void dependecyFinished() = 0;

public:
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

        onCompleted(s);
    }

    void completeFailure(const std::shared_ptr<TaskState>& s, const std::exception_ptr& e) {
        FutureStatus expected = FutureStatus::Running;
        if (!s->status.compare_exchange_strong(expected, FutureStatus::Failed)) {
            return;  // already completed by other means
        }

        std::lock_guard<std::mutex> lock(s->mtx);
        s->exception = e;

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
        if (getState()->unresolvedDeps.fetch_sub(1) == 1) {
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

        std::unique_ptr<Future<T>> selfRef = std::make_unique<Future<T>>(*this);
        FutureBase::scheduleCallback(std::move(selfRef), s->executor);
    }

    inline std::shared_ptr<TaskState> getState() const {
        if (stateRef) {
            std::lock_guard<std::mutex> lock(stateRef->mtx);
            return stateRef->state;
        }
        return nullptr;
    }

    inline void incrementHandleCount() {
        auto s = getState();
        if (s) s->handleCount.fetch_add(1, std::memory_order_relaxed);
    }

    inline void decrementHandleCount() {
        auto s = getState();
        if (s) s->handleCount.fetch_sub(1, std::memory_order_relaxed);
    }

    static std::shared_ptr<StateRef> createState() {
        auto ref = std::make_shared<StateRef>();
        ref->state = std::make_shared<TaskState>();
        return ref;
    }

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
    inline Future<T>& dependsOn(Future<U> other) {
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

        return *this;
    }

    inline void resolve(Future<T> other) {
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
    }

    inline Future<T>& start() {
        if (isEmpty()) throw std::runtime_error("Cannot start empty future");

        // Advance status to Finalized if neeeded
        FutureStatus expected = FutureStatus::Building;
        getState()->status.compare_exchange_strong(expected, FutureStatus::Finalized);

        trySchedule();
        return *this;
    }

    virtual inline void execute() override {
        if (isEmpty()) throw std::runtime_error("Cannot execute empty future");

        auto s = getState();

        FutureStatus expected = FutureStatus::Building;
        s->status.compare_exchange_strong(expected, FutureStatus::Finalized);
        expected = FutureStatus::Pending;
        if (!s->status.compare_exchange_strong(expected, FutureStatus::Running)) {
            return;  // already executed by someone else
        }

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

    virtual inline void cancel() override {
        if (isEmpty() || isReady()) return;

        // Force advance status to Running so that completeFailure can run
        auto s = getState();
        FutureStatus expected = FutureStatus::Building;
        s->status.compare_exchange_strong(expected, FutureStatus::Running);
        expected = FutureStatus::Finalized;
        s->status.compare_exchange_strong(expected, FutureStatus::Running);
        expected = FutureStatus::Pending;
        s->status.compare_exchange_strong(expected, FutureStatus::Running);

        completeFailure(s, std::make_exception_ptr(std::runtime_error("Task canceled")));
    }

    inline void reset() {
        decrementHandleCount();
        stateRef.reset();
    }

    // Caller suspends until future has been executed / finished with error
    inline void await() {
        if (isEmpty()) return;
        auto s = getState();
        std::unique_lock<std::mutex> lock(s->mtx);
        s->cv.wait(lock, [this] { return isReady(); });
    }

    virtual inline bool isEmpty() const override { return !stateRef || !stateRef->state; }

    virtual inline bool isReady() const {
        auto s = getState();
        if (!s) return false;
        FutureStatus status = s->status.load();
        return status == FutureStatus::Completed || status == FutureStatus::Failed;
    }

    inline bool hasError() const {
        auto s = getState();
        return s && s->status.load() == FutureStatus::Failed;
    }

    inline size_t useCount() const {
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

    inline std::exception_ptr getException() const {
        if (!hasError()) return nullptr;
        return getState()->exception;
    }

    inline Future<T>& operator=(const Future<T>& other) {
        if (this == &other) return *this;

        decrementHandleCount();

        stateRef = other.stateRef;
        incrementHandleCount();

        return *this;
    }

    inline Future<T>& operator=(Future<T>&& other) noexcept {
        if (this == &other) return *this;

        decrementHandleCount();

        stateRef = std::move(other.stateRef);

        return *this;
    }
};

#endif
