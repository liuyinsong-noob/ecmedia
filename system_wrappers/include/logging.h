/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This is a highly stripped-down version of libjingle's talk/base/logging.h.
// It is a thin wrapper around WEBRTC_TRACE, maintaining the libjingle log
// semantics to ease a transition to that format.

// NOTE: LS_INFO maps to a new trace level which should be reserved for
// infrequent, non-verbose logs. The other levels below kTraceWarning have been
// rendered essentially useless due to their verbosity. Carefully consider the
// impact of adding a new LS_INFO log. If it will be logged at anything
// approaching a frame or packet frequency, use LS_VERBOSE if necessary, or
// preferably, do not log at all.

//   LOG(...) an ostream target that can be used to send formatted
// output to a variety of logging targets, such as debugger console, stderr,
// file, or any StreamInterface.
//   The severity level passed as the first argument to the LOGging
// functions is used as a filter, to limit the verbosity of the logging.
//   Static members of LogMessage documented below are used to control the
// verbosity and target of the output.
//   There are several variations on the LOG macro which facilitate logging
// of common error conditions, detailed below.

// LOG(sev) logs the given stream at severity "sev", which must be a
//     compile-time constant of the LoggingSeverity type, without the namespace
//     prefix.
// LOG_V(sev) Like LOG(), but sev is a run-time variable of the LoggingSeverity
//     type (basically, it just doesn't prepend the namespace).
// LOG_F(sev) Like LOG(), but includes the name of the current function.

#ifndef WEBRTC_SYSTEM_WRAPPERS_INCLUDE_LOGGING_H_
#define WEBRTC_SYSTEM_WRAPPERS_INCLUDE_LOGGING_H_

#include <sstream>
#include <errno.h>
#include "clock.h"

namespace cloopenwebrtc {

//////////////////////////////////////////////////////////////////////

// Note that the non-standard LoggingSeverity aliases exist because they are
// still in broad use.  The meanings of the levels are:
//  LS_SENSITIVE: Information which should only be logged with the consent
//   of the user, due to privacy concerns.
//  LS_VERBOSE: This level is for data which we do not want to appear in the
//   normal debug log, but should appear in diagnostic logs.
//  LS_INFO: Chatty level used in debugging for all sorts of things, the default
//   in debug builds.
//  LS_WARNING: Something that may warrant investigation.
//  LS_ERROR: Something that should not have occurred.
enum LoggingSeverity {
  LS_SENSITIVE, LS_VERBOSE, LS_INFO, LS_WARNING, LS_ERROR
};

class LogMessage {
 public:
  LogMessage(const char* file, int line, LoggingSeverity sev);
  ~LogMessage();

  static bool Loggable(LoggingSeverity sev);
  std::ostream& stream() { return print_stream_; }

 private:
  // The ostream that buffers the formatted message before output
  std::ostringstream print_stream_;

  // The severity level of this message
  LoggingSeverity severity_;
};

class LogMessageEx {
public:
	LogMessageEx(const char* file, int line, LoggingSeverity sev, std::string& strLast, bool bForeWriteLog = false);
	~LogMessageEx();

	std::ostream& stream() { return print_stream_; }

private:
	// The ostream that buffers the formatted message before output
	std::ostringstream print_stream_;

	// The severity level of this message
	LoggingSeverity severity_;

	std::string& strLast_;
	bool bForeWriteLog_;
};

//////////////////////////////////////////////////////////////////////
// Macros which automatically disable logging when WEBRTC_LOGGING == 0
//////////////////////////////////////////////////////////////////////

#ifndef LOG
// The following non-obvious technique for implementation of a
// conditional log stream was stolen from google3/base/logging.h.

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".

class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream&) { }
};

#if defined(WEBRTC_RESTRICT_LOGGING)
// This should compile away logs matching the following condition.
#define RESTRICT_LOGGING_PRECONDITION(sev)  \
  sev < cloopenwebrtc::LS_INFO ? (void) 0 :
#else
#define RESTRICT_LOGGING_PRECONDITION(sev)
#endif

#define LOG_SEVERITY_PRECONDITION(sev) \
  RESTRICT_LOGGING_PRECONDITION(sev) !(cloopenwebrtc::LogMessage::Loggable(sev)) \
    ? (void) 0 \
    : cloopenwebrtc::LogMessageVoidify() &

#define LOG(sev) \
  LOG_SEVERITY_PRECONDITION(cloopenwebrtc::sev) \
    cloopenwebrtc::LogMessage(__FILE__, __LINE__, cloopenwebrtc::sev).stream()

// The _V version is for when a variable is passed in.  It doesn't do the
// namespace concatination.
#define LOG_V(sev) \
  LOG_SEVERITY_PRECONDITION(sev) \
    cloopenwebrtc::LogMessage(__FILE__, __LINE__, sev).stream()

// The _F version prefixes the message with the current function name.
#if (defined(__GNUC__) && !defined(NDEBUG)) || defined(WANT_PRETTY_LOG_F)
#define LOG_F(sev) LOG(sev) << __PRETTY_FUNCTION__ << ": "
#else
#define LOG_F(sev) LOG(sev) << __FUNCTION__ << ": "
#endif

#define LOG_CHECK_LEVEL(sev) \
cloopenwebrtc::LogCheckLevel(cloopenwebrtc::sev)
#define LOG_CHECK_LEVEL_V(sev) \
cloopenwebrtc::LogCheckLevel(sev)

    
#define LOG_E(sev, ctx, err, ...) \
LOG_SEVERITY_PRECONDITION(cloopenwebrtc::sev) \
cloopenwebrtc::LogMessage(__FILE__, __LINE__, cloopenwebrtc::sev, \
cloopenwebrtc::ERRCTX_ ## ctx, err , ##__VA_ARGS__) \
.stream()
    
#define LOG_T(sev) LOG(sev) << this << ": "
    
#define LOG_ERRNO_EX(sev, err) \
LOG_E(sev, ERRNO, err)
#define LOG_ERRNO(sev) \
LOG_ERRNO_EX(sev, errno)
    
#if defined(WEBRTC_WIN)
#define LOG_GLE_EX(sev, err) \
LOG_E(sev, HRESULT, err)
#define LOG_GLE(sev) \
LOG_GLE_EX(sev, GetLastError())
#define LOG_GLEM(sev, mod) \
LOG_E(sev, HRESULT, GetLastError(), mod)
#define LOG_ERR_EX(sev, err) \
LOG_GLE_EX(sev, err)
#define LOG_ERR(sev) \
LOG_GLE(sev)
#define LAST_SYSTEM_ERROR \
(::GetLastError())
#elif defined(__native_client__) && __native_client__
#define LOG_ERR_EX(sev, err) \
LOG(sev)
#define LOG_ERR(sev) \
LOG(sev)
#define LAST_SYSTEM_ERROR \
(0)
#elif defined(WEBRTC_POSIX)
#define LOG_ERR_EX(sev, err) \
LOG_ERRNO_EX(sev, err)
#define LOG_ERR(sev) \
LOG_ERRNO(sev)
#define LAST_SYSTEM_ERROR \
(errno)
#endif  // WEBRTC_WIN
    
#define LOG_TAG(sev, tag)        \
LOG_SEVERITY_PRECONDITION(sev) \
cloopenwebrtc::LogMessage(nullptr, 0, sev, tag).stream()
    
#define PLOG(sev, err) \
LOG_ERR_EX(sev, err)
    
// TODO(?): Add an "assert" wrapper that logs in the same manner.


//optimize printing log infomation
#define LOG_VALNAME_DECL(type,name,suffix) \
	type name##suffix

#define LOG_VALNAME_DECL_DEFAULT(type,name,suffix,init) \
	type name##suffix = init

#define LOG_VALNAME_INCREASEMENT(name,suffix) \
	name##suffix ++

#define LOG_VALNAME_SET(name,suffix,val) \
	name##suffix = val

#define LOG_VALNAME_SET_VALNAME(name,suffix,valName) \
	name##suffix = valName##suffix

#define LOG_VALNAME_OPT_VALNAME(name,suffix,opt,valName) \
	name##suffix = name##suffix opt valName##suffix

#define IF_VALNAME_CHECK(name,suffix,opt,cv) \
	if (name##suffix opt cv)

#define LOG_BY_COUNT(sev,suffix,times)\
	LOG_VALNAME_DECL_DEFAULT(static int64_t,count,suffix,times);\
	LOG_VALNAME_DECL_DEFAULT(bool,writelog,suffix,false);\
	LOG_VALNAME_INCREASEMENT(count,suffix);\
	IF_VALNAME_CHECK(count,suffix,>=,times) \
	{\
		LOG_VALNAME_SET(count,suffix,0);\
		LOG_VALNAME_SET(writelog,suffix,true);\
	}\
	IF_VALNAME_CHECK(writelog,suffix,==,true)\
		LOG(sev)


#define LOG_SUB_COMMON_DIFF_MSG(sev,last,forewrite) \
	LOG_SEVERITY_PRECONDITION(sev) \
	LogMessageEx(__FILE__, __LINE__, sev, last, forewrite).stream()

#define LOG_SUB_COMMON_DIFF(sev,name,suffix,writelog) \
	LOG_SUB_COMMON_DIFF_MSG(sev,name##suffix,writelog##suffix)

#define LOG_BY_TIME(sev,suffix,timelong)\
	LOG_VALNAME_DECL(static int64_t,lastTime,suffix);\
	LOG_VALNAME_DECL_DEFAULT(bool,writelog,suffix,false);\
	LOG_VALNAME_DECL(int64_t,diff,suffix);\
	LOG_VALNAME_SET(diff,suffix,Clock::GetRealTimeClock()->TimeInMicroseconds());\
	LOG_VALNAME_OPT_VALNAME(diff,suffix,-,lastTime);\
	IF_VALNAME_CHECK(diff,suffix,>=,timelong*1000)\
	{\
		LOG_VALNAME_SET(lastTime,suffix,Clock::GetRealTimeClock()->TimeInMicroseconds());\
		LOG_VALNAME_SET(writelog,suffix,true);\
	}\
	IF_VALNAME_CHECK(writelog,suffix,==,true)\
		LOG(sev)

#define LOG_BY_COUNT_CONTENT_DIFF(sev,suffix,times)\
	LOG_VALNAME_DECL(static std::string, lastStr, suffix); \
	LOG_VALNAME_DECL(static int64_t,count,suffix);\
	LOG_VALNAME_DECL_DEFAULT(bool,writelog,suffix,false);\
	LOG_VALNAME_INCREASEMENT(count,suffix);\
	IF_VALNAME_CHECK(count,suffix,>=,times) \
	{\
		LOG_VALNAME_SET(count,suffix,0);\
		LOG_VALNAME_SET(writelog,suffix,true);\
	}\
	LOG_SUB_COMMON_DIFF(sev, lastStr, suffix, writelog)


#define LOG_BY_TIME_CONTENT_DIFF(sev,suffix,timelong)\
	LOG_VALNAME_DECL(static std::string, lastStr, suffix); \
	LOG_VALNAME_DECL(static int64_t,lastTime,suffix);\
	LOG_VALNAME_DECL_DEFAULT(bool,writelog,suffix,false);\
	LOG_VALNAME_DECL(int64_t,diff,suffix);\
	LOG_VALNAME_SET(diff,suffix,Clock::GetRealTimeClock()->TimeInMicroseconds());\
	LOG_VALNAME_OPT_VALNAME(diff,suffix,-,lastTime);\
	IF_VALNAME_CHECK(diff,suffix,>=,timelong*1000)\
	{\
		LOG_VALNAME_SET(lastTime,suffix,Clock::GetRealTimeClock()->TimeInMicroseconds());\
		LOG_VALNAME_SET(writelog,suffix,true);\
	}\
	LOG_SUB_COMMON_DIFF(sev, lastStr, suffix, writelog)


#define LOG_SUB_DIFF_MSG(sev,last) \
	LOG_SEVERITY_PRECONDITION(sev) \
	LogMessageEx(__FILE__, __LINE__, sev, last).stream()

#define LOG_SUB_DIFF(sev,name,suffix) \
	LOG_SUB_DIFF_MSG(sev,name##suffix)

#define LOG_BY_DIFF(sev,suffix) \
	LOG_VALNAME_DECL(static std::string, lastStr, suffix); \
	LOG_SUB_DIFF(sev,lastStr,suffix)



//timelong is how long (unit is in milliseconds)
#define LOG_TIME(sev,timelong)\
	LOG_BY_TIME(sev,__LINE__,timelong)

#define LOG_TIME_F(sev,timelong)\
	LOG_TIME(sev,timelong) << __FUNCTION__ << ": "

//times is the times of looping
#define LOG_COUNT(sev,times)\
	LOG_BY_COUNT(sev,__LINE__,times)

#define LOG_COUNT_F(sev,times)\
	LOG_COUNT(sev,times) << __FUNCTION__ << ": "

//the string that with same previous, not write logging
#define LOG_DIFF(sev)\
	LOG_BY_DIFF(sev,__LINE__)

#define LOG_DIFF_F(sev)\
	LOG_DIFF(sev) << __FUNCTION__ << ": "

//the content of string is same with the previous and little the times, not write logging
#define LOG_COUNT_CONTENT(sev,times)\
	LOG_BY_COUNT_CONTENT_DIFF(sev,__LINE__,times)

#define LOG_COUNT_CONTENT_F(sev,times)\
	LOG_COUNT_CONTENT(sev,times) << __FUNCTION__ << ": "

//timelong is how long (unit is in milliseconds). the content is same and short the time long, not  write logging.
#define LOG_TIME_CONTENT(sev,timelong)\
	LOG_BY_TIME_CONTENT_DIFF(sev,__LINE__,timelong)

#define LOG_TIME_CONTENT_F(sev,timelong)\
	LOG_TIME_CONTENT(sev,timelong) << __FUNCTION__ << ": "

    
#endif  // LOG

}  // namespace cloopenwebrtc

#endif  // WEBRTC_SYSTEM_WRAPPERS_INCLUDE_LOGGING_H_
