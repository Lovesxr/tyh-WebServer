#ifndef _MUTEX_H
#define _MUTEX_H
#include "noncopyable.h"
#include <pthread.h>
// use RAII to manage mutex
class MutexLock : public noncopyable
{
  public:
    MutexLock()
    {
        pthread_mutex_init(&mutex_, NULL);
    }

    ~MutexLock()
    {
        pthread_mutex_destroy(&mutex_);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex_);
    }

    void unlock()
    {
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t *getMutex()
    {
        return &mutex_;
    }

  private:
    //friend class Condition;
    pthread_mutex_t mutex_;
};

class MutexLockGuard : public noncopyable
{
  public:
    explicit MutexLockGuard(MutexLock &mutex) : mutex_(mutex)
    {
        mutex_.lock();
    }
    ~MutexLockGuard()
    {
        mutex_.unlock();
    }

  private:
    MutexLock &mutex_;
};
#endif