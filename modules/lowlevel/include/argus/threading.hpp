/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

/**
 * \file argus/threading.hpp
 *
 * Platform-agnostic system threading interface.
 */

#pragma once

#include <atomic>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h> // need it for rwlock
#endif

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
     * \brief An abstract handle to a system shared mutex.
     */
    #ifdef _WIN32
    typedef SRWLOCK smutex_handle_t;
    #else
    typedef pthread_rwlock_t smutex_handle_t;
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

    /**
     * \brief A read/write mutex.
     *
     * A shared mutex allows both shared and exclusive locking, allowing
     * multiple threads to read at once through shared acquisition. However, a
     * thread performing a write operation will acquire an exclusive lock, in
     * which case the shared mutex behaves like a standard mutex and allows only
     * one concurrent accessor.
     */
    class SharedMutex {
        private:
            /**
             * \brief The handle to the system mutex.
             */
            smutex_handle_t handle;

        public:
            /**
             * \brief Constructs a new SharedMutex.
             */
            SharedMutex(void);

            ~SharedMutex(void);

            /**
             * \brief Acquires an exclusive lock on the mutex, blocking the
             *        calling thread if necessary.
             *
             * \attention Only one thread may hold an exclusive lock at a time,
             *            and no shared locks may be held as long as an
             *            exclusive lock is held.
             *
             * \sa SharedMutex::try_lock(void)
             */
            void lock(void);

            /**
             * \brief Attempts to acquire an exclusive lock on the mutex, but
             *        fails fast and does not block.
             *
             * \return Whether a lock was acquired.
             *
             * \sa SharedMutex::lock(void)
             */
            bool try_lock(void);

            /**
             * \brief Releases the current exclusive lock on the mutex.
             *
             * \warning This function should only be invoked if an exclusive
             *          lock is guaranteed to be held by the current thread.
             */
            void unlock(void);

            /**
             * \brief Acquires a shared lock, blocking the thread if
             *        necessary.
             *
             * \remark Multiple threads may hold a shared lock at once, so long
             *         as no thread holds an exclusive lock.
             *
             * \sa SharedMutex::try_lock_shared(void)
             */
            void lock_shared(void);

            /**
             * \brief Attempts to acquire a shared lock on the given mutex, but
             *        fails quickly and does not block.
             *
             * \return Whether a lock was acquired.
             *
             * \sa SharedMutex::lock_shared(void)
             */
            bool try_lock_shared(void);

            /**
             * \brief Releases the current shared lock on the given mutex.
             *
             * \warning This function should be invoked only if a shared lock is
             *          guaranteed to be held by the current thread.
             */
            void unlock_shared(void);
    };

    /**
     * \brief A drop-in replacement for std::atomic for non-trvially copyable
     *        types.
     *
     * Because std::atomic generally operates on primitive types only, it cannot
     * be used with complex types such as std::string. A ComplexAtomic object
     * wraps an object not eligible for use with std::atomic and provides
     * transparent atomicity support in a similar fashion.
     *
     * \tparam ValueType The type of value to be wrapped for atomic use.
     */
    template <typename ValueType>
    class ComplexAtomic {
        private:
            ValueType value;
            std::mutex mutex;

        public:
            /**
             * \brief The default constructor; creates a `ComplexAtomic` with an
             *        empty value.
             */
            ComplexAtomic(void):
                    value(),
                    mutex() {
            }

            /**
             * \brief The copy constructor.
             *
             * \param val The value to copy into this `ComplexAtomic`'s state.
             */
            ComplexAtomic(ValueType &val):
                    value(std::move(val)),
                    mutex() {
            }

            /**
             * \brief The move constructor.
             *
             * \param val The value to move into this `ComplexAtomic`'s state.
             */
            ComplexAtomic(ValueType &&val):
                    value(std::move(val)),
                    mutex() {
            }

            /**
             * \brief Converts the ComplexAtomic to its base type, effectively
             * "unwrapping" it.
             *
             * \return The base value.
             */
            inline operator ValueType(void) {
                return value;
            }

            /**
             * \brief Performs an atomic assignment to an lvalue.
             *
             * \param rhs The value to assign.
             *
             * \return This ComplexAtomic.
             */
            inline ComplexAtomic &operator =(const ValueType &rhs) {
                mutex.lock();
                value = rhs;
                mutex.unlock();
                return *this;
            }

            /**
             * \brief Performs an atomic assignment to an rvalue.
             *
             * \param rhs The value to assign.
             *
             * \return This ComplexAtomic.
             */
            inline ComplexAtomic &operator =(const ValueType &&rhs) {
                mutex.lock();
                value = std::move(rhs);
                mutex.unlock();
                return *this;
            }
    };

    /**
     * \brief Represents a value which is to be read and written atomically,
     *        and contains a "dirtiness" attribute.
     *
     * An `AtomicDirtiable` is essentially equivalent to a ComplexAtomic, but
     * contains an additional std::atomic_bool attribute to track its dirtiness.
     *
     * \tparam ValueType The type of value to be wrapped for use.
     */
    template <typename ValueType>
    class AtomicDirtiable {
        private:
            ComplexAtomic<ValueType> value;

        public:
            /**
             * \brief The current dirtiness of the value.
             */
            std::atomic_bool dirty;

            /**
             * \brief Converts the AtomicDirtiable to its base type, effectively
             *        "unwrapping" it.
             *
             * \return The base value.
             */
            inline operator ValueType() {
                return value;
            };

            /**
             * \brief Performs an atomic assignment to an lvalue, setting the
             *        dirty flag.
             *
             * \param rhs The value to assign.
             *
             * \return This AtomicDirtiable.
             */
            inline AtomicDirtiable &operator =(const ValueType &rhs) {
                value = rhs;
                dirty = true;
                return *this;
            };

            /**
             * \brief Performs an atomic assignment to an rvalue, setting the
             *        dirty flag.
             *
             * \param rhs The value to assign.
             *
             * \return This AtomicDirtiable.
             */
            inline AtomicDirtiable &operator =(const ValueType &&rhs) {
                value = std::move(rhs);
                dirty = true;
                return *this;
            };

            inline void clean(void) {
                this->dirty = false;
            }
    };

    /**
     * \brief Constructs a std::future with the given function as a supplier,
     *        and optionally invoking the given callback upon completion.
     *
     * \param function A function containing a task which will supply the
     *                 returned std::future.
     * \param callback The function to invoke after completion of the task.
     *                 This callback must accept the supplied value and may be
     *                 left absent if unneeded.
     *
     * \tparam Out The type of value provided by the returned std::future.
     *
     * \return The created std::future.
     *
     * \attention The provided functions \em must be thread-safe, as they will
     *            be performed on a new thread.
     */
    template <typename Out>
    std::future<Out> make_future(const std::function<Out(void)> function, const std::function<void(Out)> callback) {
        if (!function) {
            throw std::invalid_argument("make_future: Function must be present");
        }

        auto promise_ptr = std::make_shared<std::promise<Out>>();
        std::future<Out> future = promise_ptr->get_future();
        Thread thread = Thread::create([function, callback, promise_ptr](void*) mutable -> void* {
            try {
                Out res = function();
                promise_ptr->set_value_at_thread_exit(res);

                if (callback != nullptr) {
                    callback(res);
                }
            } catch (...) {
                promise_ptr->set_exception(std::make_exception_ptr(std::current_exception()));
            }

            return nullptr;
        }, nullptr);

        return future;
    }

    /**
     * \brief Template specialization for make_future for the `void` type.
     *
     * This is useful when an asynchronous task does not return anything
     * meaningful, but notification of completion is still desired.
     *
     * \param function The function containing the task to be executed.
     * \param callback The callback to be invoked upon completion of the task.
     *
     * \remark This specialization is necessary for technical reasons, as the
     *         `void` type has unique language semantics which require special
     *         handling.
     */
    std::future<void> make_future(const std::function<void(void)> function, const std::function<void(void)> callback);
}
