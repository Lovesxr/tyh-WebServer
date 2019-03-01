#ifndef _CONDITION_H
#define _CONDITION_H
#include "noncopyable.h"
#include "Mutex.h"
#include <errno.h>
class Condition : public noncopyable
{
  public:
    Condition(MutexLock &mutex) : mutex_(mutex)
    {
        pthread_cond_init(&pcond_, NULL);
    }

    ~Condition()
    {
        pthread_cond_destroy(&pcond_);
    }

    void wait()
    {
        pthread_cond_wait(&pcond_, mutex_.getMutex());
    }

    bool waitForSeconds(int seconds)
    {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += static_cast<time_t>(seconds);
        return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getMutex(), &abstime);
    }

    void notify()
    {
        pthread_cond_signal(&pcond_);
    }

    void notifyAll()
    {
        pthread_cond_broadcast(&pcond_);
    }

  private:
    MutexLock &mutex_;
    pthread_cond_t pcond_;
};

#endif
