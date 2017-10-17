/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Borrowed from Chromium's src/base/threading/thread_checker_impl.cc.

#include "thread_checker_impl.h"

#include "platform_thread.h"

namespace cloopenwebrtc {

ThreadCheckerImpl::ThreadCheckerImpl() : valid_thread_(CurrentThreadRef()) {
    printf("gezhaoyou 111 this %p, valid_thread %p\n", this, valid_thread_);
}

ThreadCheckerImpl::~ThreadCheckerImpl() {
}

bool ThreadCheckerImpl::CalledOnValidThread() const {
  const PlatformThreadRef current_thread = CurrentThreadRef();
  CritScope scoped_lock(&lock_);
    if (!valid_thread_) { // Set if previously detached.
    valid_thread_ = current_thread;
        printf("gezhaoyou 112 this %p, valid_thread %p, should not be called\n", this, valid_thread_);
    }
    
    printf("gezhaoyou 113 this %p, valid_thread %p\n", this, valid_thread_);
  return IsThreadRefEqual(valid_thread_, current_thread);
}

void ThreadCheckerImpl::DetachFromThread() {
  CritScope scoped_lock(&lock_);
    printf("gezhaoyou 114 this %p, valid_thread %p\n", this, valid_thread_);
  valid_thread_ = 0;
}

}  // namespace rtc
