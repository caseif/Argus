/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#pragma once

#include <functional>
#include <thread>

namespace argus {
    /**
     * \brief Simple abstraction for system threads.
     */
    class Thread {
        private:
            std::thread* handle;

            explicit Thread(std::thread *handle);

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
