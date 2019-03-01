#include "EventLoopThread.h"
#include "EventLoop.h"
#include <functional>
#include <assert.h>
#include <stdio.h>

EventLoopThread::EventLoopThread(int idleSeconds, const ThreadInitCallback &cb)
    : loop_(NULL),
      exiting_(false),
      thread_(std::move(std::bind(&EventLoopThread::threadFunc, this)), "EventLoopThread"),
      callback_(cb),
      mutex_(),
      cond_(mutex_),
      idleSeconds_(idleSeconds)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL)
    { // not 100% race safe
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL)
        {
            cond_.wait();
        }
    }
    return loop_;
}

void EventLoopThread::threadFunc()
{
    if (idleSeconds_ <= 0)
    {
        EventLoop loop;
        if (callback_)
        {
            callback_(&loop);
        }
        {
            MutexLockGuard lock(mutex_);
            loop_ = &loop;
            cond_.notify();
        }
        loop.initConnectionPool();
        loop.loop();
        loop_ = NULL;
    }
    else
    {
        EventLoop loop(idleSeconds_);
        if (callback_)
        {
            callback_(&loop);
        }
        {
            MutexLockGuard lock(mutex_);
            loop_ = &loop;
            cond_.notify();
        }
        loop.initConnectionPool();
        loop.loop();
        loop_ = NULL;
    }
}