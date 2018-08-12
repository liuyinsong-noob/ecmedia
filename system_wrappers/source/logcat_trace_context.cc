/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "../system_wrappers/include/logcat_trace_context.h"

#include <android/log.h>
#include <assert.h>

#include "../system_wrappers/include/logging.h"

namespace yuntongxunwebrtc {

static android_LogPriority AndroidLogPriorityFromWebRtcLogLevel(
    TraceLevel webrtc_level) {
  // NOTE: this mapping is somewhat arbitrary.  StateInfo and Info are mapped
  // to DEBUG because they are highly verbose in webrtc code (which is
  // unfortunate).
  switch (webrtc_level) {
    case yuntongxunwebrtc::kTraceStateInfo: return ANDROID_LOG_DEBUG;
    case yuntongxunwebrtc::kTraceWarning: return ANDROID_LOG_WARN;
    case yuntongxunwebrtc::kTraceError: return ANDROID_LOG_ERROR;
    case yuntongxunwebrtc::kTraceCritical: return ANDROID_LOG_FATAL;
    case yuntongxunwebrtc::kTraceApiCall: return ANDROID_LOG_VERBOSE;
    case yuntongxunwebrtc::kTraceModuleCall: return ANDROID_LOG_VERBOSE;
    case yuntongxunwebrtc::kTraceMemory: return ANDROID_LOG_VERBOSE;
    case yuntongxunwebrtc::kTraceTimer: return ANDROID_LOG_VERBOSE;
    case yuntongxunwebrtc::kTraceStream: return ANDROID_LOG_VERBOSE;
    case yuntongxunwebrtc::kTraceDebug: return ANDROID_LOG_DEBUG;
    case yuntongxunwebrtc::kTraceInfo: return ANDROID_LOG_DEBUG;
    case yuntongxunwebrtc::kTraceTerseInfo: return ANDROID_LOG_INFO;
    default:
      LOG(LS_ERROR) << "Unexpected log level" << webrtc_level;
      return ANDROID_LOG_FATAL;
  }
}

LogcatTraceContext::LogcatTraceContext() {
  yuntongxunwebrtc::Trace::CreateTrace();
  if (yuntongxunwebrtc::Trace::SetTraceCallback(this) != 0)
    assert(false);
}

LogcatTraceContext::~LogcatTraceContext() {
  if (yuntongxunwebrtc::Trace::SetTraceCallback(NULL) != 0)
    assert(false);
  yuntongxunwebrtc::Trace::ReturnTrace();
}

void LogcatTraceContext::Print(TraceLevel level,
                               const char* message,
                               int length) {
  __android_log_print(AndroidLogPriorityFromWebRtcLogLevel(level),
                      "WEBRTC", "%.*s", length, message);
}

}  // namespace yuntongxunwebrtc
