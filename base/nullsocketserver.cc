/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "../base/checks.h"
#include "../base/nullsocketserver.h"

namespace cloopenwebrtc {

NullSocketServer::NullSocketServer() : event_(false, false) {}
NullSocketServer::~NullSocketServer() {}

bool NullSocketServer::Wait(int cms, bool process_io) {
  event_.Wait(cms);
  return true;
}

void NullSocketServer::WakeUp() {
  event_.Set();
}

cloopenwebrtc::Socket* NullSocketServer::CreateSocket(int /* type */) {
  NOTREACHED();
  return nullptr;
}

cloopenwebrtc::Socket* NullSocketServer::CreateSocket(int /* family */, int /* type */) {
  NOTREACHED();
  return nullptr;
}

cloopenwebrtc::AsyncSocket* NullSocketServer::CreateAsyncSocket(int /* type */) {
  NOTREACHED();
  return nullptr;
}

cloopenwebrtc::AsyncSocket* NullSocketServer::CreateAsyncSocket(int /* family */,
                                                      int /* type */) {
  NOTREACHED();
  return nullptr;
}

}  // namespace cloopenwebrtc
