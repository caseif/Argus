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

    /**
     * \brief Creates a new thread.
     *
     * Note that this object returns a handle defined by Argus in order to
     * enable compatibility with multiple threading backends.
     *
     * \param routine The callback to invoke in the new thread.
     * \param arg The argument to pass to the callback.
     * \return A handle to the new thread.
     */
    Thread *thread_create(void *(routine)(void*), void *arg);

    /**
     * \brief Pauses execution of the current thread until the target thread has
     *        exited.
     *
     * \param thread The thread to join with.
     */
    void thread_join(Thread *thread);

    /**
     * \brief Detaches the target thread from its parent.
     *
     * \param thread The thread to detach.
     */
    void thread_detach(Thread *thread);

    /**
     * \brief Destroys the target thread.
     * 
     * This will send an interrupt signal to the target thread.
     *
     * \param thread The thread to destroy.
     */
    void thread_destroy(Thread *thread);

    /**
     * \brief Sleeps for the specified amount of time in nanoseconds.
     *
     * \param ns The number of nanoseconds to sleep for.
     */
    void sleep_nanos(unsigned long long ns);

    /**
     * \brief Returns the number of microseconds since the Unix epoch.
     *
     * \return The number of microseconds since the Unix epoch.
     */
    unsigned long long microtime(void);

}
