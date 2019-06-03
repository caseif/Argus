#pragma once

#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <thread>
#endif

namespace argus {

    #ifdef USE_PTHREADS
    typedef pthread_t Thread;
    #else
    typedef std::thread Thread;
    #endif

    Thread *thread_create(void *(routine)(void*), void *arg);

    void thread_join(Thread *thread);

    void thread_detach(Thread *thread);

    void thread_destroy(Thread *thread);

    unsigned long int microtime();

}
