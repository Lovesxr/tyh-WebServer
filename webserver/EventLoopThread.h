#ifndef _EVENTLOOP_THREAD_H
#define _EVENTLOOP_THREAD_H
#include "base/Condition.h"
#include "base/Mutex.h"
#include "base/noncopyable.h"
#include "base/Thread.h"

class EventLoop;
class EventLoopThread : public noncopyable
{
  public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    EventLoopThread(int idleSeconds, const ThreadInitCallback &cb = ThreadInitCallback());
    ~EventLoopThread();
    EventLoop *startLoop();

  private:
    void threadFunc();
    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    int idleSeconds_;
    ThreadInitCallback callback_;
};

#endif