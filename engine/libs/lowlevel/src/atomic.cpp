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

#include "argus/lowlevel/atomic.hpp"

namespace argus {
    #ifdef _WIN32
    SharedMutex::SharedMutex(void) {
        InitializeSRWLock(&handle);
    }

    SharedMutex::~SharedMutex(void) {
        // do nothing
    }

    void SharedMutex::lock(void) {
        AcquireSRWLockExclusive(&handle);
    }

    bool SharedMutex::try_lock(void) {
        return TryAcquireSRWLockExclusive(&handle);
    }

    void SharedMutex::unlock(void) {
        ReleaseSRWLockExclusive(&handle);
    }

    void SharedMutex::lock_shared(void) {
        AcquireSRWLockShared(&handle);
    }

    bool SharedMutex::try_lock_shared(void) {
        return TryAcquireSRWLockShared(&handle);
    }

    void SharedMutex::unlock_shared(void) {
        ReleaseSRWLockShared(&handle);
    }
    #else
    SharedMutex::SharedMutex(void) {
        pthread_rwlock_init(&handle, nullptr);
    }

    SharedMutex::~SharedMutex(void) {
        pthread_rwlock_destroy(&handle);
    }

    void SharedMutex::lock(void) {
        pthread_rwlock_wrlock(&handle);
    }

    bool SharedMutex::try_lock(void) {
        return pthread_rwlock_tryrdlock(&handle) == 0;
    }

    void SharedMutex::unlock(void) {
        pthread_rwlock_unlock(&handle);
    }

    void SharedMutex::lock_shared(void) {
        pthread_rwlock_rdlock(&handle);
    }

    bool SharedMutex::try_lock_shared(void) {
        return pthread_rwlock_rdlock(&handle) == 0;
    }

    void SharedMutex::unlock_shared(void) {
        pthread_rwlock_unlock(&handle);
    }
    #endif
}
