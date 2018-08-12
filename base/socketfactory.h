/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_BASE_SOCKETFACTORY_H__
#define WEBRTC_BASE_SOCKETFACTORY_H__

#include "../base/socket.h"
#include "../base/asyncsocket.h"

namespace yuntongxunwebrtc {

class SocketFactory {
public:
  virtual ~SocketFactory() {}

  // Returns a new socket for blocking communication.  The type can be
  // SOCK_DGRAM and SOCK_STREAM.
  // TODO: C++ inheritance rules mean that all users must have both
  // CreateSocket(int) and CreateSocket(int,int). Will remove CreateSocket(int)
  // (and CreateAsyncSocket(int) when all callers are changed.
  virtual Socket* CreateSocket(int type) = 0;
  virtual Socket* CreateSocket(int family, int type) = 0;
  // Returns a new socket for nonblocking communication.  The type can be
  // SOCK_DGRAM and SOCK_STREAM.
  virtual AsyncSocket* CreateAsyncSocket(int type) = 0;
  virtual AsyncSocket* CreateAsyncSocket(int family, int type) = 0;
};

} // namespace yuntongxunwebrtc

#endif // WEBRTC_BASE_SOCKETFACTORY_H__
