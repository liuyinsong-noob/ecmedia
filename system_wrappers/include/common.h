/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_BASE_COMMON_H_  // NOLINT
#define WEBRTC_BASE_COMMON_H_

#include "basictypes.h"
#include "../base/constructormagic.h"

#include <map>

#if defined(_MSC_VER)
// warning C4355: 'this' : used in base member initializer list
#pragma warning(disable:4355)
#endif


namespace cloopenwebrtc
{
	// Class Config is designed to ease passing a set of options across webrtc code.
	// Options are identified by typename in order to avoid incorrect casts.
	//
	// Usage:
	// * declaring an option:
	//    struct Algo1_CostFunction {
	//      virtual float cost(int x) const { return x; }
	//      virtual ~Algo1_CostFunction() {}
	//    };
	//
	// * accessing an option:
	//    config.Get<Algo1_CostFunction>().cost(value);
	//
	// * setting an option:
	//    struct SqrCost : Algo1_CostFunction {
	//      virtual float cost(int x) const { return x*x; }
	//    };
	//    config.Set<Algo1_CostFunction>(new SqrCost());
	//
	// Note: This class is thread-compatible (like STL containers).
	class Config {
	public:
		// Returns the option if set or a default constructed one.
		// Callers that access options too often are encouraged to cache the result.
		// Returned references are owned by this.
		//
		// Requires std::is_default_constructible<T>
		template<typename T> const T& Get() const;

		// Set the option, deleting any previous instance of the same.
		// This instance gets ownership of the newly set value.
		template<typename T> void Set(T* value);

		Config() {}
		~Config() {
			// Note: this method is inline so webrtc public API depends only
			// on the headers.
			for (OptionMap::iterator it = options_.begin();
				it != options_.end(); ++it) {
					delete it->second;
			}
		}

	private:
		typedef void* OptionIdentifier;

		struct BaseOption {
			virtual ~BaseOption() {}
		};

		template<typename T>
		struct Option : BaseOption {
			explicit Option(T* v): value(v) {}
			~Option() {
				delete value;
			}
			T* value;
		};
		// Own implementation of rtti-subset to avoid depending on rtti and its costs.
		template<typename T>
		static OptionIdentifier identifier() {
			static char id_placeholder;
			return &id_placeholder;
		}

		// Used to instantiate a default constructed object that doesn't needs to be
		// owned. This allows Get<T> to be implemented without requiring explicitly
		// locks.
		template<typename T>
		static const T& default_value() {
			static const T def;
			return def;
		}
        
//        int a[20];
		typedef std::map<OptionIdentifier, BaseOption*> OptionMap;
		OptionMap options_;
//        int b[6];
		// DISALLOW_COPY_AND_ASSIGN
		Config(const Config&);
		void operator=(const Config&);
	};

	template<typename T>
	const T& Config::Get() const {
		OptionMap::const_iterator it = options_.find(identifier<T>());
		if (it != options_.end()) {
			const T* t = static_cast<Option<T>*>(it->second)->value;
			if (t) {
				return *t;
			}
		}
		return default_value<T>();
	}

	template<typename T>
	void Config::Set(T* value) {
		BaseOption*& it = options_[identifier<T>()];
		delete it;
		it = new Option<T>(value);
	}
}

//////////////////////////////////////////////////////////////////////
// General Utilities
//////////////////////////////////////////////////////////////////////

#ifndef RTC_UNUSED
#define RTC_UNUSED(x) RtcUnused(static_cast<const void*>(&x))
#define RTC_UNUSED2(x, y) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y))
#define RTC_UNUSED3(x, y, z) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y)); \
    RtcUnused(static_cast<const void*>(&z))
#define RTC_UNUSED4(x, y, z, a) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y)); \
    RtcUnused(static_cast<const void*>(&z)); \
    RtcUnused(static_cast<const void*>(&a))
#define RTC_UNUSED5(x, y, z, a, b) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y)); \
    RtcUnused(static_cast<const void*>(&z)); \
    RtcUnused(static_cast<const void*>(&a)); \
    RtcUnused(static_cast<const void*>(&b))
inline void RtcUnused(const void*) {}
#endif  // RTC_UNUSED

#if !defined(WEBRTC_WIN)

#ifndef strnicmp
#define strnicmp(x, y, n) strncasecmp(x, y, n)
#endif

#ifndef stricmp
#define stricmp(x, y) strcasecmp(x, y)
#endif

// TODO(fbarchard): Remove this. std::max should be used everywhere in the code.
// NOMINMAX must be defined where we include <windows.h>.
#define stdmax(x, y) std::max(x, y)
#else
#define stdmax(x, y) cloopenwebrtc::_max(x, y)
#endif

#define ARRAY_SIZE(x) (static_cast<int>(sizeof(x) / sizeof(x[0])))

/////////////////////////////////////////////////////////////////////////////
// Assertions
/////////////////////////////////////////////////////////////////////////////

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG _DEBUG
#endif  // !defined(ENABLE_DEBUG)

// Even for release builds, allow for the override of LogAssert. Though no
// macro is provided, this can still be used for explicit runtime asserts
// and allow applications to override the assert behavior.

namespace cloopenwebrtc {


// If a debugger is attached, triggers a debugger breakpoint. If a debugger is
// not attached, forces program termination.
void Break();

// LogAssert writes information about an assertion to the log. It's called by
// Assert (and from the ASSERT macro in debug mode) before any other action
// is taken (e.g. breaking the debugger, abort()ing, etc.).
void LogAssert(const char* function, const char* file, int line,
               const char* expression);

typedef void (*AssertLogger)(const char* function,
                             const char* file,
                             int line,
                             const char* expression);

// Sets a custom assert logger to be used instead of the default LogAssert
// behavior. To clear the custom assert logger, pass NULL for |logger| and the
// default behavior will be restored. Only one custom assert logger can be set
// at a time, so this should generally be set during application startup and
// only by one component.
void SetCustomAssertLogger(AssertLogger logger);

bool IsOdd(int n);

bool IsEven(int n);

}  // namespace rtc

#if ENABLE_DEBUG

namespace cloopenwebrtc {

inline bool Assert(bool result, const char* function, const char* file,
                   int line, const char* expression) {
  if (!result) {
    LogAssert(function, file, line, expression);
    Break();
  }
  return result;
}

// Same as Assert above, but does not call Break().  Used in assert macros
// that implement their own breaking.
inline bool AssertNoBreak(bool result, const char* function, const char* file,
                          int line, const char* expression) {
  if (!result)
    LogAssert(function, file, line, expression);
  return result;
}

}  // namespace rtc

#if defined(_MSC_VER) && _MSC_VER < 1300
#define __FUNCTION__ ""
#endif

#ifndef ASSERT
#if defined(WIN32)
// Using debugbreak() inline on Windows directly in the ASSERT macro, has the
// benefit of breaking exactly where the failing expression is and not two
// calls up the stack.
#define ASSERT(x) \
    (cloopenwebrtc::AssertNoBreak((x), __FUNCTION__, __FILE__, __LINE__, #x) ? \
     (void)(1) : __debugbreak())
#else
#define ASSERT(x) \
    (void)cloopenwebrtc::Assert((x), __FUNCTION__, __FILE__, __LINE__, #x)
#endif
#endif

#ifndef VERIFY
#if defined(WIN32)
#define VERIFY(x) \
    (cloopenwebrtc::AssertNoBreak((x), __FUNCTION__, __FILE__, __LINE__, #x) ? \
     true : (__debugbreak(), false))
#else
#define VERIFY(x) cloopenwebrtc::Assert((x), __FUNCTION__, __FILE__, __LINE__, #x)
#endif
#endif

#else  // !ENABLE_DEBUG

namespace cloopenwebrtc {

inline bool ImplicitCastToBool(bool result) { return result; }

}  // namespace rtc

#ifndef ASSERT
#define ASSERT(x) (void)0
#endif

#ifndef VERIFY
#define VERIFY(x) cloopenwebrtc::ImplicitCastToBool(x)
#endif

#endif  // !ENABLE_DEBUG

#define COMPILE_TIME_ASSERT(expr)       char CTA_UNIQUE_NAME[expr]
#define CTA_UNIQUE_NAME                 CTA_MAKE_NAME(__LINE__)
#define CTA_MAKE_NAME(line)             CTA_MAKE_NAME2(line)
#define CTA_MAKE_NAME2(line)            constraint_ ## line

// Forces compiler to inline, even against its better judgement. Use wisely.
#if defined(__GNUC__)
#define FORCE_INLINE __attribute__((always_inline))
#elif defined(WEBRTC_WIN)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE
#endif

// Borrowed from Chromium's base/compiler_specific.h.
// Annotate a virtual method indicating it must be overriding a virtual
// method in the parent class.
// Use like:
//   virtual void foo() OVERRIDE;
#if defined(WEBRTC_WIN)
#define OVERRIDE override
#elif defined(__clang__)
// Clang defaults to C++03 and warns about using override. Squelch that.
// Intentionally no push/pop here so all users of OVERRIDE ignore the warning
// too. This is like passing -Wno-c++11-extensions, except that GCC won't die
// (because it won't see this pragma).
#pragma clang diagnostic ignored "-Wc++11-extensions"
#define OVERRIDE override
#elif defined(__GNUC__) && __cplusplus >= 201103 && \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40700
// GCC 4.7 supports explicit virtual overrides when C++11 support is enabled.
#define OVERRIDE override
#else
#define OVERRIDE
#endif

// Annotate a function indicating the caller must examine the return value.
// Use like:
//   int foo() WARN_UNUSED_RESULT;
// To explicitly ignore a result, see |ignore_result()| in <base/basictypes.h>.
// TODO(ajm): Hack to avoid multiple definitions until the base/ of webrtc and
// libjingle are merged.
#if !defined(WARN_UNUSED_RESULT)
#if defined(__GNUC__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif
#endif  // WARN_UNUSED_RESULT

#endif  // WEBRTC_BASE_COMMON_H_    // NOLINT
