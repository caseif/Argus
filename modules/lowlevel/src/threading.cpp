#include "argus/lowlevel.hpp"

#ifdef USE_PTHREADS
#include <pthread.h>
#include <stdlib.h>
#else
#include <thread>
#endif

namespace argus {

    #ifdef USE_PTHREADS
    Thread *thread_create(void *(routine)(void*), void *arg) {
        pthread_t *thread = static_cast<pthread_t*>(malloc(sizeof(pthread_t)));
        pthread_create(thread, NULL, routine, arg);
        
        return static_cast<Thread*>(thread);
    }

    void thread_join(Thread *thread) {
        pthread_join(static_cast<pthread_t>(*thread), NULL);
    }

    void thread_detach(Thread *thread) {
        pthread_detach(static_cast<pthread_t>(*thread));
    }

    void thread_destroy(Thread *thread) {
        pthread_cancel(static_cast<pthread_t>(*thread));
    }
    #else
    Thread *thread_create(void *(routine)(void*), void *arg) {
        return static_cast<Thread*>(new std::thread(routine, arg));
    }

    void thread_join(Thread *thread) {
        static_cast<std::thread*>(thread)->join();
        return;
    }

    void thread_detach(Thread *thread) {
        static_cast<std::thread*>(thread)->detach();
        return;
    }

    void thread_destroy(Thread *thread) {
        delete(thread);
        return;
    }
    #endif

}
