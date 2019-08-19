#include "argus/threading.hpp"

#include <functional>

#ifdef USE_PTHREADS
#include <pthread.h>
#include <stdlib.h>
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
            printf("func: %p\n", func);
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
        InitializeSRWLock(mutex);
    }

    void smutex_destroy(smutex &mutex) {
        // do nothing
    }

    void smutex_lock(smutex &mutex) {
        AcquireSRWLockExclusive(mutex);
    }

    bool smutex_try_lock(smutex &mutex) {
        return TryAcquireSRWLockExclusive(mutex);
    }

    void smutex_unlock(smutex &mutex) {
        ReleaseSRWLockExclusive(mutex);
    }

    void smutex_lock_shared(smutex &mutex) {
        AcquireSRWLockShared(mutex);
    }

    bool smutex_try_lock_shared(smutex &mutex) {
        return TryAcquireSRWLockShared(mutex);
    }

    void smutex_unlock_shared(smutex &mutex) {
        ReleaseSRWLockShared(mutex);
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

}
