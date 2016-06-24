/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "rw_lock_posix.h"

namespace cloopenwebrtc {
#ifdef ANDROID_UNDER_8
    RWLockPosix::RWLockPosix()
    {

    }
    
    RWLockPosix::~RWLockPosix()
    {
        pthread_mutex_destroy(&_lock);
    }
    
    int RWLockPosix::Init()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        return pthread_mutex_init(&_lock, &attr);
    }
    
    void RWLockPosix::AcquireLockExclusive()
    {
        pthread_mutex_lock(&_lock);
    }
    
    void RWLockPosix::ReleaseLockExclusive()
    {
        pthread_mutex_unlock(&_lock);
    }
    
    void RWLockPosix::AcquireLockShared()
    {
        pthread_mutex_lock(&_lock);
    }
    
    void RWLockPosix::ReleaseLockShared()
    {
        pthread_mutex_unlock(&_lock);
    }
    
#else
RWLockPosix::RWLockPosix() : _lock()
{
}

RWLockPosix::~RWLockPosix()
{
    pthread_rwlock_destroy(&_lock);
}

int RWLockPosix::Init()
{
    return pthread_rwlock_init(&_lock, 0);
}

void RWLockPosix::AcquireLockExclusive()
{
    pthread_rwlock_wrlock(&_lock);
}

void RWLockPosix::ReleaseLockExclusive()
{
    pthread_rwlock_unlock(&_lock);
}

void RWLockPosix::AcquireLockShared()
{
    pthread_rwlock_rdlock(&_lock);
}

void RWLockPosix::ReleaseLockShared()
{
    pthread_rwlock_unlock(&_lock);
}
#endif
} // namespace cloopenwebrtc
