#ifndef EC_LOG_H
#define EC_LOG_H
#include <fstream>
#include "rtc_base/logging.h"
class ECLog : public rtc::LogSink {
 public:
  ECLog(char* log_path);
  ~ECLog();
  virtual void OnLogMessage(const std::string& message) override;

 private:
  std::fstream  fstream_;
};
#endif