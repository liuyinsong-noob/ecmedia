/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "../base/thread.h"

#if defined(WEBRTC_WIN)
#include <comdef.h>
#elif defined(WEBRTC_POSIX)
#include <time.h>
#endif

#include "../base/checks.h"
#include "../system_wrappers/include/logging.h"
#include "../base/nullsocketserver.h"
#include "../base/platform_thread.h"
#include "../base/stringutils.h"
#include "../base/timeutils.h"
#include "../base/trace_event.h"

#if defined(WEBRTC_MAC)
#include "../base/maccocoathreadhelper.h"
#include "../base/scoped_autorelease_pool.h"
#endif

namespace cloopenwebrtc {

ThreadManager* ThreadManager::Instance() {
  RTC_DEFINE_STATIC_LOCAL(ThreadManager, thread_manager, ());
  return &thread_manager;
}

// static
Thread* Thread::Current() {
  return ThreadManager::Instance()->CurrentThread();
}

#if defined(WEBRTC_POSIX)
ThreadManager::ThreadManager() {
  pthread_key_create(&key_, nullptr);
#ifndef NO_MAIN_THREAD_WRAPPING
  WrapCurrentThread();
#endif
#if defined(WEBRTC_MAC)
  // This is necessary to alert the cocoa runtime of the fact that
  // we are running in a multithreaded environment.
  InitCocoaMultiThreading();
#endif
}

ThreadManager::~ThreadManager() {
#if defined(WEBRTC_MAC)
  // This is called during exit, at which point apparently no NSAutoreleasePools
  // are available; but we might still need them to do cleanup (or we get the
  // "no autoreleasepool in place, just leaking" warning when exiting).
  ScopedAutoreleasePool pool;
#endif
  UnwrapCurrentThread();
  pthread_key_delete(key_);
}

Thread *ThreadManager::CurrentThread() {
  return static_cast<Thread *>(pthread_getspecific(key_));
}

void ThreadManager::SetCurrentThread(Thread *thread) {
  pthread_setspecific(key_, thread);
}
#endif

#if defined(WEBRTC_WIN)
ThreadManager::ThreadManager() {
  key_ = TlsAlloc();
#ifndef NO_MAIN_THREAD_WRAPPING
  WrapCurrentThread();
#endif
}

ThreadManager::~ThreadManager() {
  UnwrapCurrentThread();
  TlsFree(key_);
}

Thread *ThreadManager::CurrentThread() {
  return static_cast<Thread *>(TlsGetValue(key_));
}

void ThreadManager::SetCurrentThread(Thread *thread) {
  TlsSetValue(key_, thread);
}
#endif

Thread *ThreadManager::WrapCurrentThread() {
  Thread* result = CurrentThread();
  if (nullptr == result) {
    result = new Thread();
    result->WrapCurrentWithThreadManager(this, true);
  }
  return result;
}

void ThreadManager::UnwrapCurrentThread() {
  Thread* t = CurrentThread();
  if (t && !(t->IsOwned())) {
    t->UnwrapCurrent();
    delete t;
  }
}

struct ThreadInit {
  Thread* thread;
  Runnable* runnable;
};

Thread::ScopedDisallowBlockingCalls::ScopedDisallowBlockingCalls()
  : thread_(Thread::Current()),
    previous_state_(thread_->SetAllowBlockingCalls(false)) {
}

Thread::ScopedDisallowBlockingCalls::~ScopedDisallowBlockingCalls() {
  DCHECK(thread_->IsCurrent());
  thread_->SetAllowBlockingCalls(previous_state_);
}

Thread::Thread() : Thread(SocketServer::CreateDefault()) {}

Thread::Thread(SocketServer* ss)
    : MessageQueue(ss, false),
      running_(true, false),
#if defined(WEBRTC_WIN)
      thread_(nullptr),
      thread_id_(0),
#endif
      owned_(true),
      blocking_calls_allowed_(true) {
  SetName("Thread", this);  // default name
  DoInit();
}

Thread::Thread(std::unique_ptr<SocketServer> ss)
    : MessageQueue(std::move(ss), false),
      running_(true, false),
#if defined(WEBRTC_WIN)
      thread_(nullptr),
      thread_id_(0),
#endif
      owned_(true),
      blocking_calls_allowed_(true) {
  SetName("Thread", this);  // default name
  DoInit();
}

Thread::~Thread() {
  Stop();
  DoDestroy();
}

std::unique_ptr<Thread> Thread::CreateWithSocketServer() {
  return std::unique_ptr<Thread>(new Thread(SocketServer::CreateDefault()));
}

std::unique_ptr<Thread> Thread::Create() {
  return std::unique_ptr<Thread>(
      new Thread(std::unique_ptr<SocketServer>(new NullSocketServer())));
}

bool Thread::SleepMs(int milliseconds) {
  AssertBlockingIsAllowedOnCurrentThread();

#if defined(WEBRTC_WIN)
  ::Sleep(milliseconds);
  return true;
#else
  // POSIX has both a usleep() and a nanosleep(), but the former is deprecated,
  // so we use nanosleep() even though it has greater precision than necessary.
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  int ret = nanosleep(&ts, nullptr);
  if (ret != 0) {
    //LOG_ERR(LS_WARNING) << "nanosleep() returning early";
    return false;
  }
  return true;
#endif
}

bool Thread::SetName(const std::string& name, const void* obj) {
  if (running()) return false;
  name_ = name;
  if (obj) {
    char buf[16];
    sprintfn(buf, sizeof(buf), " 0x%p", obj);
    name_ += buf;
  }
  return true;
}

bool Thread::Start(Runnable* runnable) {
  DCHECK(owned_);
  if (!owned_) return false;
  DCHECK(!running());
  if (running()) return false;

  Restart();  // reset IsQuitting() if the thread is being restarted

  // Make sure that ThreadManager is created on the main thread before
  // we start a new thread.
  ThreadManager::Instance();

  ThreadInit* init = new ThreadInit;
  init->thread = this;
  init->runnable = runnable;
#if defined(WEBRTC_WIN)
  thread_ = CreateThread(nullptr, 0, PreRun, init, 0, &thread_id_);
  if (thread_) {
    running_.Set();
  } else {
    return false;
  }
#elif defined(WEBRTC_POSIX)
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  int error_code = pthread_create(&thread_, &attr, PreRun, init);
  if (0 != error_code) {
    LOG(LS_ERROR) << "Unable to create pthread, error " << error_code;
    return false;
  }
  running_.Set();
#endif
  return true;
}

bool Thread::WrapCurrent() {
  return WrapCurrentWithThreadManager(ThreadManager::Instance(), true);
}

void Thread::UnwrapCurrent() {
  // Clears the platform-specific thread-specific storage.
  ThreadManager::Instance()->SetCurrentThread(nullptr);
#if defined(WEBRTC_WIN)
  if (thread_ != nullptr) {
    if (!CloseHandle(thread_)) {
      //LOG_GLE(LS_ERROR) << "When unwrapping thread, failed to close handle.";
    }
    thread_ = nullptr;
  }
#endif
  running_.Reset();
}

void Thread::SafeWrapCurrent() {
  WrapCurrentWithThreadManager(ThreadManager::Instance(), false);
}

void Thread::Join() {
  if (running()) {
    DCHECK(!IsCurrent());
    if (Current() && !Current()->blocking_calls_allowed_) {
      LOG(LS_WARNING) << "Waiting for the thread to join, "
                      << "but blocking calls have been disallowed";
    }

#if defined(WEBRTC_WIN)
    DCHECK(thread_ != nullptr);
    WaitForSingleObject(thread_, INFINITE);
    CloseHandle(thread_);
    thread_ = nullptr;
    thread_id_ = 0;
#elif defined(WEBRTC_POSIX)
    void *pv;
    pthread_join(thread_, &pv);
#endif
    running_.Reset();
  }
}

bool Thread::SetAllowBlockingCalls(bool allow) {
  DCHECK(IsCurrent());
  bool previous = blocking_calls_allowed_;
  blocking_calls_allowed_ = allow;
  return previous;
}

// static
void Thread::AssertBlockingIsAllowedOnCurrentThread() {
#if !defined(NDEBUG)
  Thread* current = Thread::Current();
  DCHECK(!current || current->blocking_calls_allowed_);
#endif
}

// static
#if defined(WEBRTC_WIN)
DWORD WINAPI Thread::PreRun(LPVOID pv) {
#else
void* Thread::PreRun(void* pv) {
#endif
  ThreadInit* init = static_cast<ThreadInit*>(pv);
  ThreadManager::Instance()->SetCurrentThread(init->thread);
  cloopenwebrtc::SetCurrentThreadName(init->thread->name_.c_str());
#if defined(WEBRTC_MAC)
  // Make sure the new thread has an autoreleasepool
  ScopedAutoreleasePool pool;
#endif
  if (init->runnable) {
    init->runnable->Run(init->thread);
  } else {
    init->thread->Run();
  }
  delete init;
#ifdef WEBRTC_WIN
  return 0;
#else
  return nullptr;
#endif
}

void Thread::Run() {
  ProcessMessages(kForever);
}

bool Thread::IsOwned() {
  return owned_;
}

void Thread::Stop() {
  MessageQueue::Quit();
  Join();
}

void Thread::Send(const Location& posted_from,
                  MessageHandler* phandler,
                  uint32_t id,
                  MessageData* pdata) {
  if (IsQuitting())
    return;

  // Sent messages are sent to the MessageHandler directly, in the context
  // of "thread", like Win32 SendMessage. If in the right context,
  // call the handler directly.
  Message msg;
  msg.posted_from = posted_from;
  msg.phandler = phandler;
  msg.message_id = id;
  msg.pdata = pdata;
  if (IsCurrent()) {
    phandler->OnMessage(&msg);
    return;
  }

  AssertBlockingIsAllowedOnCurrentThread();

  AutoThread thread;
  Thread *current_thread = Thread::Current();
  DCHECK(current_thread != nullptr);  // AutoThread ensures this

  bool ready = false;
  {
    CritScope cs(&crit_);
    _SendMessage smsg;
    smsg.thread = current_thread;
    smsg.msg = msg;
    smsg.ready = &ready;
    sendlist_.push_back(smsg);
  }

  // Wait for a reply
  WakeUpSocketServer();

  bool waited = false;
  crit_.Enter();
  while (!ready) {
    crit_.Leave();
    // We need to limit "ReceiveSends" to |this| thread to avoid an arbitrary
    // thread invoking calls on the current thread.
    current_thread->ReceiveSendsFromThread(this);
    current_thread->socketserver()->Wait(kForever, false);
    waited = true;
    crit_.Enter();
  }
  crit_.Leave();

  // Our Wait loop above may have consumed some WakeUp events for this
  // MessageQueue, that weren't relevant to this Send.  Losing these WakeUps can
  // cause problems for some SocketServers.
  //
  // Concrete example:
  // Win32SocketServer on thread A calls Send on thread B.  While processing the
  // message, thread B Posts a message to A.  We consume the wakeup for that
  // Post while waiting for the Send to complete, which means that when we exit
  // this loop, we need to issue another WakeUp, or else the Posted message
  // won't be processed in a timely manner.

  if (waited) {
    current_thread->socketserver()->WakeUp();
  }
}

void Thread::ReceiveSends() {
  ReceiveSendsFromThread(nullptr);
}

void Thread::ReceiveSendsFromThread(const Thread* source) {
  // Receive a sent message. Cleanup scenarios:
  // - thread sending exits: We don't allow this, since thread can exit
  //   only via Join, so Send must complete.
  // - thread receiving exits: Wakeup/set ready in Thread::Clear()
  // - object target cleared: Wakeup/set ready in Thread::Clear()
  _SendMessage smsg;

  crit_.Enter();
  while (PopSendMessageFromThread(source, &smsg)) {
    crit_.Leave();

    smsg.msg.phandler->OnMessage(&smsg.msg);

    crit_.Enter();
    *smsg.ready = true;
    smsg.thread->socketserver()->WakeUp();
  }
  crit_.Leave();
}

bool Thread::PopSendMessageFromThread(const Thread* source, _SendMessage* msg) {
  for (std::list<_SendMessage>::iterator it = sendlist_.begin();
       it != sendlist_.end(); ++it) {
    if (it->thread == source || source == nullptr) {
      *msg = *it;
      sendlist_.erase(it);
      return true;
    }
  }
  return false;
}

void Thread::InvokeInternal(const Location& posted_from,
                            MessageHandler* handler) {
  TRACE_EVENT2("webrtc", "Thread::Invoke", "src_file_and_line",
               posted_from.file_and_line(), "src_func",
               posted_from.function_name());
  Send(posted_from, handler);
}

void Thread::Clear(MessageHandler* phandler,
                   uint32_t id,
                   MessageList* removed) {
  CritScope cs(&crit_);

  // Remove messages on sendlist_ with phandler
  // Object target cleared: remove from send list, wakeup/set ready
  // if sender not null.

  std::list<_SendMessage>::iterator iter = sendlist_.begin();
  while (iter != sendlist_.end()) {
    _SendMessage smsg = *iter;
    if (smsg.msg.Match(phandler, id)) {
      if (removed) {
        removed->push_back(smsg.msg);
      } else {
        delete smsg.msg.pdata;
      }
      iter = sendlist_.erase(iter);
      *smsg.ready = true;
      smsg.thread->socketserver()->WakeUp();
      continue;
    }
    ++iter;
  }

  MessageQueue::Clear(phandler, id, removed);
}

bool Thread::ProcessMessages(int cmsLoop) {
  int64_t msEnd = (kForever == cmsLoop) ? 0 : TimeAfter(cmsLoop);
  int cmsNext = cmsLoop;

  while (true) {
#if defined(WEBRTC_MAC)
    // see: http://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSAutoreleasePool_Class/Reference/Reference.html
    // Each thread is supposed to have an autorelease pool. Also for event loops
    // like this, autorelease pool needs to be created and drained/released
    // for each cycle.
    ScopedAutoreleasePool pool;
#endif
    Message msg;
    if (!Get(&msg, cmsNext))
      return !IsQuitting();
    Dispatch(&msg);

    if (cmsLoop != kForever) {
      cmsNext = static_cast<int>(TimeUntil(msEnd));
      if (cmsNext < 0)
        return true;
    }
  }
}

bool Thread::WrapCurrentWithThreadManager(ThreadManager* thread_manager,
                                          bool need_synchronize_access) {
  if (running())
    return false;

#if defined(WEBRTC_WIN)
  if (need_synchronize_access) {
    // We explicitly ask for no rights other than synchronization.
    // This gives us the best chance of succeeding.
    thread_ = OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());
    if (!thread_) {
      //LOG_GLE(LS_ERROR) << "Unable to get handle to thread.";
      return false;
    }
    thread_id_ = GetCurrentThreadId();
  }
#elif defined(WEBRTC_POSIX)
  thread_ = pthread_self();
#endif

  owned_ = false;
  running_.Set();
  thread_manager->SetCurrentThread(this);
  return true;
}

AutoThread::AutoThread() {
  if (!ThreadManager::Instance()->CurrentThread()) {
    ThreadManager::Instance()->SetCurrentThread(this);
  }
}

AutoThread::~AutoThread() {
  Stop();
  if (ThreadManager::Instance()->CurrentThread() == this) {
    ThreadManager::Instance()->SetCurrentThread(nullptr);
  }
}

#if defined(WEBRTC_WIN)
ComThread::~ComThread() {
  Stop();
}

void ComThread::Run() {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  DCHECK(SUCCEEDED(hr));
  if (SUCCEEDED(hr)) {
    Thread::Run();
    CoUninitialize();
  } else {
    LOG(LS_ERROR) << "CoInitialize failed, hr=" << hr;
  }
}
#endif

}  // namespace cloopenwebrtc