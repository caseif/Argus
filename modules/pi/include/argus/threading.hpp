#pragma once

#include <functional>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <thread>
#endif

namespace argus {

    #ifdef USE_PTHREADS
    typedef pthread_t thread_handle_t;
    #else
    typedef std::thread* thread_handle_t;
    #endif

    #ifdef _WIN32
    typedef SRWLOCK smutex;
    #else
    typedef pthread_rwlock_t smutex;
    #endif

    class Thread {
        private:
            #ifdef USE_PTHREADS
            pthread_t handle;

            Thread(pthread_t handle);
            #else
            std::thread* handle;

            Thread(std::thread *handle);
            #endif
    

        public:
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
            static Thread &create(std::function<void*(void*)> routine, void *arg);

            /**
             * \brief Pauses execution of the current thread until the target thread has
             *        exited.
             *
             * \param thread The thread to join with.
             */
            void join(void);

            /**
             * \brief Detaches the target thread from its parent.
             *
             * \param thread The thread to detach.
             */
            void detach(void);

            /**
             * \brief Destroys the target thread.
             * 
             * This will send an interrupt signal to the target thread.
             *
             * \param thread The thread to destroy.
             */
            void destroy(void);
    };

    /**
     * \brief Initializes a new smutex at the given pointer.
     *
     * An smutex is a read/write mutex, allowing data to be read by multiple
     * threads at once (but only written by one).
     */
    void smutex_create(smutex &mutex);

    /**
     * \brief Destroys the given smutex.
     *
     * Note that on Windows, this function does nothing since SRWLOCK
     * destruction is not required (or possible).
     */
    void smutex_destroy(smutex &mutex);

    /**
     * \brief Acquires an exclusive lock on the given mutex, blocking the thread
     * if necessary.
     */
    void smutex_lock(smutex &mutex);

    /**
     * \brief Attempts to acquire an exclusive lock on the given mutex, but
     * fails quickly and does not block.
     *
     * \return Whether a lock was acquired.
     */
    bool smutex_try_lock(smutex &mutex);

    /**
     * \brief Releases the current exclusive lock on the given mutex.
     *
     * This function should never be invoked if an exclusive lock is not
     * guaranteed to be held by the current thread.
     */
    void smutex_unlock(smutex &mutex);
    
    /**
     * \brief Acquires a shared lock on the given mutex, blocking the thread if
     * necessary.
     *
     * Multiple threads may hold a shared lock at once, so long as no thread
     * holds an exclusive lock.
     */
    void smutex_lock_shared(smutex &mutex);

    /**
     * \brief Attempts to acquire a shared lock on the given mutex, but fails
     * quickly and does not block.
     *
     * \return Whether a lock was acquired.
     */
    bool smutex_try_lock_shared(smutex &mutex);
   
    /**
     * \brief Releases the current shared lock on the given mutex.
     *
     * This function should never be invoked if an shared lock is not guaranteed
     * to be held by the current thread.
     */
    void smutex_unlock_shared(smutex &mutex);

}
