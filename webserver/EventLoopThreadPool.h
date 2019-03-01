#ifndef _EVENTLOOPTHREADPOOL_H_
#define _EVENTLOOPTHREADPOOL_H_

#include <functional>
#include <vector>

#include "base/noncopyable.h"
#include <memory>
#include <vector>
class EventLoop;
class EventLoopThread;

// stack variable
class EventLoopThreadPool : public noncopyable
{
  public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop *loop, int idleSeconds);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads)
    {
        numThreads_ = numThreads;
    }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();

    bool started() const
    {
        return started_;
    }

  private:
    bool started_;

    int numThreads_;
    int next_;
    int idleSeconds_;
    EventLoop *baseLoop_;

    std::vector<std::shared_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_;
};

#endif
