#ifndef _ASYNC_LOGGING_H
#define _ASYNC_LOGGING_H
#include "noncopyable.h"
#include <memory>
#include <vector>
#include "CountDownLatch.h"
#include "Mutex.h"
#include <string>
#include "Thread.h"
#include "LogStream.h"
class AsyncLogging : public noncopyable
{
  public:
    AsyncLogging(const std::string &basename, off_t rollSize_, int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_)
            stop();
    }
    void append(const char *logline, int len);

    void start()
    {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop()
    {
        running_ = false;
        cond_.notify();
        thread_.join();
    }

  private:
    void threadFunc();
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
    typedef std::unique_ptr<Buffer> BufferPtr;
    const int flushInterval_;
    bool running_;
    std::string basename_;
    off_t rollSize_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
    CountDownLatch latch_;
};
#endif