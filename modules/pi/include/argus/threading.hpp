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

    #ifdef __WIN32
    typedef PSRWLOCK smutex;
    #else
    typedef pthread_rwlock_t smutex;
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

    void smutex_create(smutex *mutex);

    void smutex_destroy(smutex *mutex);

    void smutex_lock(smutex *mutex);

    void smutex_try_lock(smutex *mutex);

    void smutex_unlock(smutex *mutex);
    
    void smutex_lock_shared(smutex *mutex);

    void smutex_try_lock_shared(smutex *mutex);
   
    void smutex_unlock_shared(smutex *mutex);

}
