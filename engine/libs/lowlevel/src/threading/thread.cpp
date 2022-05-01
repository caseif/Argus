/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "argus/lowlevel/threading/thread.hpp"

#include <functional>
#include <thread>

namespace argus {
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
}
