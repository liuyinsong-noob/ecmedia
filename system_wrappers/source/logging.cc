/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "../system_wrappers/include/logging.h"

#include <string.h>

#include <sstream>

#include "../module/common_types.h"
#include "../system_wrappers/include/trace.h"

namespace yuntongxunwebrtc {
namespace {

	static const std::string SPLIT_TOKEN = "\t";

TraceLevel WebRtcSeverity(LoggingSeverity sev) {
  switch (sev) {
    // TODO(ajm): SENSITIVE doesn't have a corresponding webrtc level.
    case LS_SENSITIVE:  return kTraceInfo;
    case LS_VERBOSE:    return kTraceInfo;
    case LS_INFO:       return kTraceTerseInfo;
    case LS_WARNING:    return kTraceWarning;
    case LS_ERROR:      return kTraceError;
    default:            return kTraceNone;
  }
}

// Return the filename portion of the string (that following the last slash).
const char* FilenameFromPath(const char* file) {
  const char* end1 = ::strrchr(file, '/');
  const char* end2 = ::strrchr(file, '\\');
  if (!end1 && !end2)
    return file;
  else
    return (end1 > end2) ? end1 + 1 : end2 + 1;
}

}  // namespace

LogMessage::LogMessage(const char* file, int line, LoggingSeverity sev)
    : severity_(sev) {
  print_stream_ << "(" << FilenameFromPath(file) << ":" << line << "): ";
}

bool LogMessage::Loggable(LoggingSeverity sev) {
  // |level_filter| is a bitmask, unlike libjingle's minimum severity value.
  return WebRtcSeverity(sev) & Trace::level_filter() ? true : false;
}

LogMessage::~LogMessage() {
  const std::string& str = print_stream_.str();
  Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, "%s", str.c_str());
}

LogMessageEx::LogMessageEx(const char* file, int line, LoggingSeverity sev, std::string& strLast, bool bForeWriteLog)
	: severity_(sev)
	, bForeWriteLog_(bForeWriteLog)
	, strLast_(strLast) {
	print_stream_ << "(" << FilenameFromPath(file) << ":" << line << "): " << SPLIT_TOKEN.c_str();
}

LogMessageEx::~LogMessageEx() {
	const std::string& str = print_stream_.str();
	std::string::size_type pos = str.find_first_of(SPLIT_TOKEN);
	std::string strsub = str.substr(pos + SPLIT_TOKEN.length(), str.length() - pos - SPLIT_TOKEN.length());
	if (bForeWriteLog_ || strsub.compare(strLast_))
	{
		Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, "%s", str.c_str());
	}
	strLast_ = strsub;
}


LogMessageCounter::LogMessageCounter(const char* file, int line, LoggingSeverity sev, std::string& strPrev, int64_t prevValue, int64_t currValue, int countinues, bool bWriteCurrLog, bool bWritePrevLog)
	: severity_(sev)
	, prevValue_(prevValue)
	, currValue_(currValue)
	, countinues_(countinues)
	, bWriteCurrLog_(bWriteCurrLog)
	, bWritePrevLog_(bWritePrevLog)
	, strPrev_(strPrev) {
	print_stream_ << "(" << FilenameFromPath(file) << ":" << line << "): ";
}

LogMessageCounter::~LogMessageCounter() {
	const std::string& str = print_stream_.str();

	//out of order
	if (currValue_ < prevValue_ && !strPrev_.empty())
	{
		if (countinues_ > 1)
		{
			Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, "(continuations %d: %lld--%lld) %s", countinues_, prevValue_ - countinues_ + 1, prevValue_, strPrev_.c_str());
		}
		Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, " (out-of-order: %lld-->%lld) %s", prevValue_, currValue_, str.c_str());
	}
	else
	{
		bool bMissing = false;
		if (bWritePrevLog_ && !strPrev_.empty())
		{
			if (countinues_ > 1)
			{
				Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, "(continuations %d: %lld--%lld) %s", countinues_, prevValue_ - countinues_ + 1, prevValue_, strPrev_.c_str());
			}
			bMissing = true;
		}

		if (bWriteCurrLog_)
		{
			if (bMissing)
			{
				int64_t prev = prevValue_ + 1;
				int64_t curr = currValue_ - 1;
				int64_t missing = currValue_ - prevValue_ - 1;

				missing = missing > 0 ? missing : 0;
				curr = curr > 0 ? curr : 0;

				if (missing == 1)
				{
					Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, " (lost %lld: %lld) %s", missing, prev, str.c_str());
				}
				else
				{
					Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, " (lost %lld: %lld--%lld) %s", missing, prev, curr, str.c_str());
				}
			}
			else
			{
				Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, " (don't lost)%s", str.c_str());
			}
		}
	}

	strPrev_ = str;
}

}  // namespace yuntongxunwebrtc
