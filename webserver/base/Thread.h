#ifndef _THREAD_H
#define _THREAD_H
#include "noncopyable.h"
#include <functional>
#include <memory>
#include "CountDownLatch.h"
class Thread : public noncopyable
{
  public:
    typedef std::function<void()> ThreadFunc;
    explicit Thread(const ThreadFunc &, const std::string &name);
    explicit Thread(ThreadFunc &&func, std::string &&name_);
    ~Thread();
    void start();
    int join();
    bool started() const
    {
        return started_;
    }
    pid_t tid() const
    {
        return tid_;
    }
    const std::string &name() const
    {
        return name_;
    }

  private:
    void setDefaultName();
    bool started_;
    bool joined_;

    pid_t tid_;
    pthread_t pthreadId_;

    ThreadFunc func_;
    CountDownLatch latch_;
    std::string name_;
};

#endif