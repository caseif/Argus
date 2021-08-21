/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/lowlevel/threading/thread.hpp"

#include <functional>
#include <thread>

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
}
