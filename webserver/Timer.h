#ifndef _TIMER_H
#define _TIMER_H
#include "base/noncopyable.h"
#include "base/Timestamp.h"
#include "base/Atomic.h"
#include "Callback.h"
class Timer : public noncopyable
{
  public:
    Timer(TimerCallback &&cb, Timestamp when, double interval)
        : sequence_(sqe_numCreated_.incrementAndGet()),
          repeat_(interval > 0.0),
          interval_(interval),
          expiration_(when),
          callback_(std::move(cb))
    {
    }

    void run() const
    {
        callback_();
    }

    Timestamp expiration() const
    {
        return expiration_;
    }

    bool repeat() const
    {
        return repeat_;
    }
    int64_t sequence() const
    {
        return sequence_;
    }

    void restart(Timestamp now);
    static int64_t sqe_numCreated()
    {
        return sqe_numCreated_.get();
    }

  private:
    static AtomicInt64 sqe_numCreated_;
    const int64_t sequence_;
    const bool repeat_;
    const int interval_;
    Timestamp expiration_;
    const TimerCallback callback_;
};
#endif