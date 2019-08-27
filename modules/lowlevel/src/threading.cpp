#include "argus/threading.hpp"

#include <functional>
#include <type_traits>

#ifdef USE_PTHREADS
#include <pthread.h>
#include <cstdlib>
#else
#include <thread>
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace argus {

    #ifdef USE_PTHREADS
    struct FunctionDelegate {
        static void *invoke_static(void *self) {
            return static_cast<FunctionDelegate*>(self)->invoke();
        }

        std::function<void*(void*)> &func;
        void *arg;

        FunctionDelegate(std::function<void*(void*)> &func, void *arg):
                func(func),
                arg(arg) {
        }

        void *invoke() {
            return func(arg);
        }
    };

    Thread &Thread::create(std::function<void*(void*)> routine, void *arg) {
        pthread_t pthread;

        FunctionDelegate delegate(routine, arg);
        pthread_create(&pthread, NULL, delegate.invoke_static, &delegate);

        return *new Thread(pthread);
    }

    Thread::Thread(pthread_t handle):
            handle(handle) {
    }

    void Thread::join() {
        pthread_join(handle, NULL);
    }

    void Thread::detach() {
        pthread_detach(handle);
    }

    void Thread::destroy() {
        pthread_cancel(handle);
        delete this;
    }
    #else
    Thread &Thread::create(std::function<void*(void*)> routine, void *arg) {
        return *new Thread(new std::thread(routine, arg));
    }

    Thread::Thread(std::thread *handle):
            handle(handle) {
    }

    void Thread::join() {
        handle->join();
        return;
    }

    void Thread::detach() {
        handle->detach();
        return;
    }

    void Thread::destroy() {
        delete handle;
        delete this;
        return;
    }
    #endif

    #ifdef _WIN32
    void smutex_create(smutex &mutex) {
        InitializeSRWLock(&mutex);
    }

    void smutex_destroy(smutex &mutex) {
        // do nothing
    }

    void smutex_lock(smutex &mutex) {
        AcquireSRWLockExclusive(&mutex);
    }

    bool smutex_try_lock(smutex &mutex) {
        return TryAcquireSRWLockExclusive(&mutex);
    }

    void smutex_unlock(smutex &mutex) {
        ReleaseSRWLockExclusive(&mutex);
    }

    void smutex_lock_shared(smutex &mutex) {
        AcquireSRWLockShared(&mutex);
    }

    bool smutex_try_lock_shared(smutex &mutex) {
        return TryAcquireSRWLockShared(&mutex);
    }

    void smutex_unlock_shared(smutex &mutex) {
        ReleaseSRWLockShared(&mutex);
    }
    #else
    void smutex_create(smutex &mutex) {
        pthread_rwlock_init(&mutex, NULL);
    }

    void smutex_destroy(smutex &mutex) {
        pthread_rwlock_destroy(&mutex);
    }

    void smutex_lock(smutex &mutex) {
        pthread_rwlock_wrlock(&mutex);
    }

    bool smutex_try_lock(smutex &mutex) {
        return pthread_rwlock_tryrdlock(&mutex) == 0;
    }

    void smutex_unlock(smutex &mutex) {
        pthread_rwlock_unlock(&mutex);
    }

    void smutex_lock_shared(smutex &mutex) {
        pthread_rwlock_rdlock(&mutex);
    }

    bool smutex_try_lock_shared(smutex &mutex) {
        return pthread_rwlock_rdlock(&mutex) == 0;
    }

    void smutex_unlock_shared(smutex &mutex) {
        pthread_rwlock_unlock(&mutex);
    }
    #endif

    template <typename T, typename DataType>
    AsyncRequestHandle<T, DataType> AsyncRequestHandle<T, DataType>::operator =(AsyncRequestHandle<T, DataType> const &rhs) {
        return AsyncFileRequestHandle(rhs);
    }

    template <typename T, typename DataType>
    AsyncRequestHandle<T, DataType>::AsyncRequestHandle(AsyncRequestHandle<T, DataType> const &rhs):
            item(rhs.item),
            data(rhs.data),
            result_valid(rhs.result_valid.load()),
            success(rhs.success),
            thread(rhs.thread) {
    }

    template <typename T, typename DataType>
    AsyncRequestHandle<T, DataType>::AsyncRequestHandle(AsyncRequestHandle<T, DataType> const &&rhs) :
            item(std::move(rhs.item)),
            data(std::move(rhs.data)),
            result_valid(rhs.result_valid.load()),
            success(std::move(rhs.success)),
            thread(std::move(rhs.thread)) {
    }

    template <typename T, typename DataType>
    AsyncRequestHandle<T, DataType>::AsyncRequestHandle(T &item, const DataType &&data,
            AsyncRequestCallback<T, DataType> callback):
            data(data),
            callback(callback),
            result_valid(false),
            thread(nullptr) {
    }

    template <typename T, typename DataType>
    AsyncRequestHandle<T, DataType>::AsyncRequestHandle(T item, const DataType &&data,
            AsyncRequestCallback<T, DataType> callback):
            data(data),
            callback(callback),
            result_valid(false),
            thread(nullptr) {
        static_assert(std::is_pointer<T>::value, "item parameter must be a reference for non-pointer template type");
    }

    template <typename T, typename DataType>
    void AsyncRequestHandle<T, DataType>::execute(std::function<void*(void*)> routine) {
        thread = Thread::create(routine, static_cast<void*>(this));
    }

    template <typename T, typename DataType>
    void AsyncRequestHandle<T, DataType>::join(void) {
        thread->join();
    }

    template <typename T, typename DataType>
    void AsyncRequestHandle<T, DataType>::cancel(void) {
        thread->detach();
        thread->destroy();
    }

    template <typename T, typename DataType>
    int AsyncRequestHandle<T, DataType>::get_data(DataType &target) {
        if (!result_valid) {
            return -1;
        }
        target = data;
    }

    template <typename T, typename DataType>
    bool AsyncRequestHandle<T, DataType>::was_successful(void) {
        if (!result_valid) {
            return false;
        }
        return success;
    }

    template <typename T, typename DataType>
    bool AsyncRequestHandle<T, DataType>::is_result_valid(void) {
        return result_valid;
    }

}
