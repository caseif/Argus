/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <functional>
#include <thread>

#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <thread>
#endif

namespace argus {
    /**
     * \brief An abstract handle to a system thread.
     */
    #ifdef USE_PTHREADS
    typedef pthread_t thread_handle_t;
    #else
    typedef std::thread* thread_handle_t;
    #endif

    /**
     * \brief Simple abstraction for system threads.
     */
    class Thread {
        private:
            #ifdef USE_PTHREADS
            pthread_t handle;

            Thread(pthread_t handle);
            #else
            std::thread* handle;

            explicit Thread(std::thread *handle);
            #endif

            Thread(Thread&) = delete;

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
             */
            void join(void);

            /**
             * \brief Detaches the target thread from its parent.
             */
            void detach(void);

            /**
             * \brief Destroys the target thread.
             *
             * This will send an interrupt signal to the target thread.
             */
            void destroy(void);
    };
}
