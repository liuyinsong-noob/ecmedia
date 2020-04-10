#ifndef EC_MEDIA_MANAGER_H
#define EC_MEDIA_MANAGER_H
#include <memory>

#include "media/base/media_engine.h"


class ECMediaManager {
 public:
  ECMediaManager(rtc::Thread* worker_thread, rtc::Thread* network_thread, rtc::Thread* signal_thread);
  ~ECMediaManager();


 private:
  std::unique_ptr<cricket::MediaEngineInterface> media_engine_;
  rtc::Thread* worker_thread_;
  rtc::Thread* network_thread_;
  rtc::Thread* signaling_thread_;
};

#endif