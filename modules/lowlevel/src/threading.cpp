/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/lowlevel/threading.hpp"

#include <exception>
#include <functional>
#include <memory>

#ifdef USE_PTHREADS
    #include <pthread.h>

    #include <cstdlib>
#else
    #include <thread>
#endif

#ifdef _WIN32
    #include <Windows.h>
#endif

#include <cstddef>

namespace argus {

#ifdef USE_PTHREADS
    struct FunctionDelegate {
        static void *invoke_static(void *self) {
            return static_cast<FunctionDelegate *>(self)->invoke();
        }

        std::function<void *(void *)> &func;
        void *arg;

        FunctionDelegate(std::function<void *(void *)> &func, void *arg): func(func), arg(arg) {
        }

        void *invoke() {
            return func(arg);
        }
    };

    Thread &Thread::create(std::function<void *(void *)> routine, void *arg) {
        pthread_t pthread;

        FunctionDelegate delegate(routine, arg);
        pthread_create(&pthread, NULL, delegate.invoke_static, &delegate);

        return *new Thread(pthread);
    }

    Thread::Thread(pthread_t handle): handle(handle) {
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
    Thread &Thread::create(std::function<void *(void *)> routine, void *arg) {
        return *new Thread(new std::thread(routine, arg));
    }

    Thread::Thread(std::thread *handle): handle(handle) {
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
    SharedMutex::SharedMutex(void) {
        InitializeSRWLock(&handle);
    }

    SharedMutex::~SharedMutex(void) {
        // do nothing
    }

    void SharedMutex::lock(void) {
        AcquireSRWLockExclusive(&handle);
    }

    bool SharedMutex::try_lock(void) {
        return TryAcquireSRWLockExclusive(&handle);
    }

    void SharedMutex::unlock(void) {
        ReleaseSRWLockExclusive(&handle);
    }

    void SharedMutex::lock_shared(void) {
        AcquireSRWLockShared(&handle);
    }

    bool SharedMutex::try_lock_shared(void) {
        return TryAcquireSRWLockShared(&handle);
    }

    void SharedMutex::unlock_shared(void) {
        ReleaseSRWLockShared(&handle);
    }
#else
    SharedMutex::SharedMutex(void) {
        pthread_rwlock_init(&handle, NULL);
    }

    SharedMutex::~SharedMutex(void) {
        pthread_rwlock_destroy(&handle);
    }

    void SharedMutex::lock(void) {
        pthread_rwlock_wrlock(&handle);
    }

    bool SharedMutex::try_lock(void) {
        return pthread_rwlock_tryrdlock(&handle) == 0;
    }

    void SharedMutex::unlock(void) {
        pthread_rwlock_unlock(&handle);
    }

    void SharedMutex::lock_shared(void) {
        pthread_rwlock_rdlock(&handle);
    }

    bool SharedMutex::try_lock_shared(void) {
        return pthread_rwlock_rdlock(&handle) == 0;
    }

    void SharedMutex::unlock_shared(void) {
        pthread_rwlock_unlock(&handle);
    }
#endif

    std::future<void> make_future(const std::function<void(void)> function, const std::function<void(void)> callback) {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        std::future<void> future = promise_ptr->get_future();
        Thread thread = Thread::create(
            [function, callback, promise_ptr](void *) mutable -> void * {
                try {
                    function();
                    promise_ptr->set_value_at_thread_exit();

                    if (callback != nullptr) {
                        callback();
                    }
                } catch (...) {
                    promise_ptr->set_exception_at_thread_exit(std::current_exception());
                }

                return nullptr;
            },
            nullptr);

        return future;
    }

}
