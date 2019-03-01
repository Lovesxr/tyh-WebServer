#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"

#include <assert.h>

template <typename To, typename From>
inline To implicit_cast(From const &f)
{
    return f;
}

EventLoopThreadPool::EventLoopThreadPool(EventLoop *loop, int idleSeconds)
    : started_(false),
      numThreads_(0),
      next_(0),
      idleSeconds_(idleSeconds),
      baseLoop_(loop)
{
}

// stack variable thus no need to delete loop_
EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;
    for (int i = 0; i < numThreads_; i++)
    {
        std::shared_ptr<EventLoopThread> t(new EventLoopThread(idleSeconds_));
        threads_.push_back(t);
        loops_.push_back(t->startLoop());
    }
    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

//使用round-robin算法获取下一个ioLoop
//TODO:增加优先级以及一些状态变量，使得分配负载更加均衡
EventLoop *EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop *loop = baseLoop_;
    if (!loops_.empty())
    {
        loop = loops_[next_];
        next_ = (next_ + 1) % numThreads_;
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}
