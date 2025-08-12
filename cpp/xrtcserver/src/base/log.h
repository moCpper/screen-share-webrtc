#ifndef __BASE_LOG_H__
#define __BASE_LOG_H__

#include <fstream>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>

#include "rtc_base/logging.h"

/*
enum LoggingSeverity {
  LS_VERBOSE,
  LS_INFO,
  LS_WARNING,
  LS_ERROR,
  LS_NONE,
  INFO = LS_INFO,
  WARNING = LS_WARNING,
  LERROR = LS_ERROR
};
*/

namespace xrtc{

class XrtcLog : public rtc::LogSink {
public:
    XrtcLog(std::string log_dir,std::string log_name,std::string log_level);
    virtual ~XrtcLog();

    int init();

    bool start();
    void end();
    void join();

    void set_log_to_stderr(bool);

    void OnLogMessage(const std::string& message,rtc::LoggingSeverity serverity) override;
    void OnLogMessage(const std::string& message) override;

    static rtc::LoggingSeverity get_log_serverity(std::string level);
private:
    std::string log_dir_;
    std::string log_name_;
    std::string log_level_;

    std::string log_file_;
    std::string log_file_wf_;

    std::ofstream out_file_;
    std::ofstream out_file_wf_;

    std::queue<std::string> log_queue_;
    std::mutex log_mtx_;
    std::queue<std::string> log_queue_wf_;
    std::mutex log_wf_mtx_;

    std::thread t_;
    std::atomic_bool running_;
};

}
#endif 