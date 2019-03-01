#include "ThreadPool.h"
#include <assert.h>
ThreadPool::ThreadPool(const std::string &name)
    : mutex_(),
      notEmpty_(mutex_),
      notFull_(mutex_),
      name_(name),
      head(0),
      tail(0),
      currentSize_(0),
      maxQueueSize_(0),
      running_(false)
{
}

ThreadPool::~ThreadPool()
{
  if (running_)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads, int maxQueueSize)
{
  assert(threads_.empty());
  assert(taskQueue_.empty());
  running_ = true;
  setMaxQueueSize(maxQueueSize);
  threads_.reserve(numThreads);
  taskQueue_.resize(maxQueueSize);
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(id, sizeof id, "%d", i + 1);
    std::unique_ptr<Thread> t(new Thread(std::bind(&ThreadPool::runInThread, this), name_ + id));
    threads_.push_back(std::move(t));
    threads_[i]->start();
    printf("thread start...\n");
  }
}

void ThreadPool::stop()
{
  {
    MutexLockGuard lock(mutex_);
    //保护修改running_,否则会有线程进行无限制的休眠
    //一定要加锁!!，因为要修改running_
    //take()里面wait等待的条件里有running
    //如果不加锁保护running，take()中先执行while判断running为true
    //接着下面修改了running，也notifyAll了，这时take中才执行wait，那就永远锁住。
    running_ = false;
    notEmpty_.notifyAll();
  }
  for (auto t = threads_.begin(); t != threads_.end(); ++t)
  {
    (*t)->join();
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return currentSize_;
}

void ThreadPool::run(const Task &task)
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    while (isFullWithLock())
    {
      notFull_.wait();
    }
    assert(!isFullWithLock());

    taskQueue_[tail] = std::move(task);
    tail = (tail + 1) % maxQueueSize_;
    currentSize_++;
    notEmpty_.notify();
  }
}

ThreadPool::Task ThreadPool::take()
{
  MutexLockGuard lock(mutex_);
  // always use a while-loop, due to spurious wakeup
  while (isEmptyWithLock() && running_)
  {
    notEmpty_.wait();
  }
  Task task;
  if (!isEmptyWithLock())
  {
    //printf("get a task....");
    task = std::move(taskQueue_[head]);
    head = (head + 1) % maxQueueSize_;
    currentSize_--;
    if (maxQueueSize_ > currentSize_)
    {
      notFull_.notify();
    }
  }
  //printf("1111\n");
  return task;
}

bool ThreadPool::isFullWithLock() const
{
  return maxQueueSize_ > 0 && head == tail && currentSize_ == maxQueueSize_;
}

bool ThreadPool::isEmptyWithLock() const
{
  return maxQueueSize_ > 0 && head == tail && !currentSize_;
}

void ThreadPool::runInThread()
{
  while (running_)
  {
    // printf("waiting for task..\n");

    Task task(take());
    //  printf("task ok ...\n");
    if (task)
    {
      //    printf("run aaaaaaa task ....\n");
      task();
    }
  }
}