#include "Timestamp.h"

#include <sys/time.h>
#include <stdio.h>
Timestamp Timestamp::now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

std::string Timestamp::toFormattedString() const
{
  char buf[64] = {0};
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  struct tm tm_time;
  gmtime_r(&seconds, &tm_time);
  int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
  snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d", tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
  return buf;
}

std::string Timestamp::toString() const
{
  char buf[32] = {0};
  int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
  int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
  snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 " ", seconds, microseconds); //PRId64保证跨平台INT64的正确显示 一个宏 类似 "XXXX"
  return buf;
}