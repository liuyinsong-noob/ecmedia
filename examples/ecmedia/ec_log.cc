#include "ec_log.h"

ECLog::ECLog(const char* log_path) {
  fstream_.open(log_path, std::fstream::out | std::fstream::binary);
  if (fstream_.is_open())
  {
    int i = 0;
    i = 1;
  } else {
    if (log_path) {
      fstream_.open(log_path);
    }else{
    fstream_.open(".\ecmediaAPI.txt");
    }
  }
}

ECLog ::~ECLog() {
  if (fstream_.is_open()) {
    int i = 0;
    i = 1;
  }
  fstream_.close();
}
//日志格式可在此接口中定义
void ECLog::OnLogMessage(const std::string& message) {
  fstream_.write(message.c_str(), message.size());
}
