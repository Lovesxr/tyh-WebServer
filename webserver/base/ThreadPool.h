#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include "noncopyable.h"
#include <functional>
#include <string>
#include "Thread.h"
#include <vector>
class ThreadPool : public noncopyable
{
public:
  typedef std::function<void()> Task;
  explicit ThreadPool(const std::string &nameArg = std::string("ThreadPool"));
  ~ThreadPool();

  void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
  void start(int numThreads, int maxQueueSize);
  void stop();

  const std::string &name() const
  {
    return name_;
  }

  size_t queueSize() const;

  // Could block if maxQueueSize > 0
  void run(const Task &f);

private:
  bool isFullWithLock() const;
  bool isEmptyWithLock() const;
  void runInThread();
  Task take();
  mutable MutexLock mutex_;
  Condition notEmpty_;
  Condition notFull_;
  std::string name_;
  int head;
  int tail;
  int currentSize_;
  std::vector<std::unique_ptr<Thread>> threads_;
  std::vector<Task> taskQueue_;
  size_t maxQueueSize_;
  bool running_;
};

#endif