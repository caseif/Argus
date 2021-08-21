/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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
