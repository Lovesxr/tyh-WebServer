#ifndef _LOGFILE_H
#define _LOGFILE_H

#include "noncopyable.h"
#include "Mutex.h"
#include "FileUtil.h"
#include <memory>
class LogFile : public noncopyable
{
  public:
    LogFile(const std::string &basename,
            off_t rollSize,
            int flushInterval_,
            int flushEveryN = 1024);
    ~LogFile();
    void append(const char *logline, int len);
    void flush();
    bool rollFile();

  private:
    void append_unlocked(const char *logline, int len);

    static string getLogFileName(const string &basename, time_t *now);
    const std::string basename_;
    const off_t rollSize_;
    const int flushInterval_;
    const int flushEveryN_;
    int count_;

    std::unique_ptr<MutexLock> mutex_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;
    std::unique_ptr<AppendFile> file_;

    const static int kRollPerSeconds_ = 60 * 60 * 24;
};

#endif