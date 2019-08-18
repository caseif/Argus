// module pi
#include "argus/threading.hpp"

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
    Thread *thread_create(void *const (routine)(void*), void *arg) {
        pthread_t *thread = static_cast<pthread_t*>(malloc(sizeof(pthread_t)));
        //pthread_create(thread, NULL, routine, arg);

        return static_cast<Thread*>(thread);
    }

    void thread_join(Thread &thread) {
        pthread_join(static_cast<pthread_t>(*thread), NULL);
    }

    void thread_detach(Thread &thread) {
        pthread_detach(static_cast<pthread_t>(*thread));
    }

    void thread_destroy(Thread &thread) {
        pthread_cancel(static_cast<pthread_t>(*thread));
    }
    #else
    Thread &thread_create(void *(*const routine)(void*), void *arg) {
        return *static_cast<Thread*>(new std::thread(routine, arg));
    }

    void thread_join(Thread &thread) {
        static_cast<std::thread*>(&thread)->join();
        return;
    }

    void thread_detach(Thread &thread) {
        static_cast<std::thread*>(&thread)->detach();
        return;
    }

    void thread_destroy(Thread &thread) {
        delete &thread;
        return;
    }
    #endif

    #ifdef _WIN32
    void smutex_create(smutex &mutex) {
        IniitalizeSRWLock(mutex);
    }

    void smutex_destroy(smutex &mutex) {
        // do nothing
    }

    void smutex_lock(smutex &mutex) {
        AcquireSRWLockExclusive(mutex);
    }

    void smutex_try_lock(smutex &mutex) {
        return TryAcquireSRWLockExclusive(mutex) != 0;
    }

    void smutex_unlock(smutex &mutex) {
        ReleaseSRWLockExclusive(mutex);
    }
    
    void smutex_lock_shared(smutex &mutex) {
        AcquireSRWLockShared(mutex);
    }

    void smutex_try_lock_shared(smutex &mutex) {
        return TryAcquireSRWLockShared(mutex) != 0;
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
