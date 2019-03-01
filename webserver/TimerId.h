#ifndef _TIMERID_H
#define _TIMERID_H

#include <stdint.h>
#include <stddef.h>
class Timer;

///
/// An opaque identifier, for canceling Timer.
/// 此类是为了cancelTimer而使用的
class TimerId
{
  public:
    TimerId()
        : timer_(NULL),
          sequence_(0)
    {
    }

    TimerId(Timer *timer, int64_t seq)
        : timer_(timer),
          sequence_(seq)
    {
    }

    // default copy-ctor, dtor and assignment are okay
    friend class TimerQueue;

  private:
    Timer *timer_;
    int64_t sequence_;
};

#endif
