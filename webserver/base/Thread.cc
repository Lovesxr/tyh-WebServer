#include "Thread.h"
#include "CurrentThread.h"
#include <sys/prctl.h>
#include <assert.h>
//为了将tid,name这些传给线程创建函数pthread_create()
struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    std::string name_;
    pid_t *tid_;
    CountDownLatch *latch_;

    ThreadData(const ThreadFunc &func, const std::string &name, pid_t *tid, CountDownLatch *latch)
        : func_(func),
          name_(name),
          tid_(tid),
          latch_(latch)
    {
    }

    void runInThread()
    {
        *tid_ = CurrentThread::tid();
        tid_ = NULL; //tid指针指向的内容已经设置为该线程的tid了，调用runInThread函数的线程可以放心的使用tid,这个就是countDowLatch的作用
        latch_->countDown();
        latch_ = NULL;

        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);
        func_();
        CurrentThread::t_threadName = "finished";
    }
};

void *startThread(void *obj)
{
    ThreadData *data = static_cast<ThreadData *>(obj);
    data->runInThread();
    delete data;
    return NULL;
}
Thread::Thread(const ThreadFunc &func, const std::string &name)
    : started_(false),
      joined_(false),
      tid_(0),
      pthreadId_(0),
      func_((func)),
      name_((name)),
      latch_(1)
{
}
Thread::Thread(ThreadFunc &&func, std::string &&name)
    : started_(false),
      joined_(false),
      tid_(0),
      pthreadId_(0),
      func_(std::move(func)),
      name_(std::move(name)),
      latch_(1)
{
}
Thread::~Thread()
{
    if (started_ && !joined_)
    {
        //如果主线程没有join,避免资源泄露
        pthread_detach(pthreadId_);
    }
}
void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData *data = new ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &startThread, data))
    {
        started_ = false;
        delete data;
    }
    else
    {
        latch_.wait();
        assert(tid_ > 0);
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}
