#ifndef _TIMERQUEUE_H_
#define _TIMERQUEUE_H_

#include <memory>
#include <set>
#include <sys/timerfd.h>
#include <vector>

#include "Callback.h"
#include "Channel.h"
#include "base/noncopyable.h"
#include "base/Timestamp.h"

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : public noncopyable
{
  public:
    TimerQueue(EventLoop *loop);
    ~TimerQueue();

    TimerId addTimer(TimerCallback &&cb, Timestamp when, double interval); // schedules the callback to be run at given time

    void cancel(TimerId timerId);

  private:
    typedef std::pair<Timestamp, Timer *> Entry; // BST or other options: linear list or priority queue based on binary heap(LinYaCool::Websever)
    typedef std::set<Entry> TimerList;
    typedef std::pair<Timer *, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer *timer);
    void cancelInLoop(TimerId timerId);
    void handleRead(); // handle the readable timefd event

    std::vector<Entry> getExpired(Timestamp now); // get the expired timer
    void reset(const std::vector<Entry> &expired, Timestamp now);
    bool insert(Timer *timer);

    bool callingExpiredTimers_;

    const int timerfd_;

    EventLoop *loop_;

    std::shared_ptr<Channel> timerfdChannel_;

    TimerList timers_; // timers sorted by timer's expiration

    ActiveTimerSet activeTimers_;    // for calcel() and timers sorted by timer's addresses
    ActiveTimerSet cancelingTimers_; // avoid self-unregister
};

#endif