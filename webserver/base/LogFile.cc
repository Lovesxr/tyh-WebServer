#include "LogFile.h"
#include <assert.h>
#include <unistd.h>
LogFile::LogFile(const std::string &basename,
                 off_t rollSize,
                 int flushInterval,
                 int flushEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      flushEveryN_(flushEveryN),
      count_(0),
      mutex_(new MutexLock),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0)
{
  assert(basename.find('/') == string::npos);
  rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::append(const char *logline, int len)
{
  MutexLockGuard lock(*mutex_);
  append_unlocked(logline, len);
}

void LogFile::append_unlocked(const char *logline, int len)
{
  file_->append(logline, len);
  ++count_;
  if (file_->writtenBytes() > rollSize_)
  {
    rollFile();
  }
  else
  {
    ++count_;
    if (count_ > flushEveryN_)
    {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod != startOfPeriod_)
      {
        rollFile();
      }
      else if (now - lastFlush_ > flushInterval_)
      {
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

void LogFile::flush()
{
  MutexLockGuard lock(*mutex_);
  file_->flush();
}

bool LogFile::rollFile()
{
  time_t now = 0;
  string filename = getLogFileName(basename_, &now);
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
  if (now > lastRoll_)
  {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    file_.reset(new AppendFile(filename));
    return true;
  }
  return false;
}
string hostname()
{
  // HOST_NAME_MAX 64
  // _POSIX_HOST_NAME_MAX 255
  char buf[256];
  if (::gethostname(buf, sizeof buf) == 0)
  {
    buf[sizeof(buf) - 1] = '\0';
    return buf;
  }
  else
  {
    return "unknownhost";
  }
}
string LogFile::getLogFileName(const string &basename, time_t *now)
{
  //归档的日志名格式为  basename_+time+hostname+pid+.log
  string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_r(now, &tm); // FIXME: localtime_r ?
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;

  filename += hostname();

  char pidbuf[32];
  snprintf(pidbuf, sizeof pidbuf, ".%d", ::getpid());
  filename += pidbuf;

  filename += ".log";

  return filename;
}