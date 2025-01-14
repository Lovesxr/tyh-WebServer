#include <assert.h>
#include <functional>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "TimerId.h"
#include "EventLoop.h"
#include "Timer.h"
#include "TimerQueue.h"
#include <functional>

int createTimerfd();

struct timespec howMuchTimeFromNow(Timestamp when);

void readTimerfd(int timerfd, Timestamp now);

void resetTimerfd(int timerfd, Timestamp expiration);

TimerQueue::TimerQueue(EventLoop *loop)
    : callingExpiredTimers_(false),
      timerfd_(createTimerfd()),
      loop_(loop),
      timers_(),
      timerfdChannel_(new Channel(loop, timerfd_))
{

    timerfdChannel_->setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_->enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_->disableAll();
    timerfdChannel_->remove();
    close(timerfd_);
    for (TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it)
    {
        delete it->second;
    }
}

TimerId TimerQueue::addTimer(TimerCallback &&cb, Timestamp when, double interval)
{
    // TODO: use TimerPool to avoid cost of new operation
    Timer *timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(1 == n);
        // TODO: TimerPool
        delete it->first;
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire;
    for (std::vector<Entry>::const_iterator it = expired.begin();
         it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        if (it->second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it->second->restart(now);
            insert(it->second);
        }
        else
        {
            delete it->second;
        }
    }
    //printf("Timers.size()===%d \n",timers_.size());
    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }
    if (nextExpire.valid())
    {
        //printf("reset timer fd .....\n");
        resetTimerfd(timerfd_, nextExpire);
    }
}

void TimerQueue::handleRead()
{
    //printf("time handle read....\n");
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);
    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it)
    {
        it->second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
    //printf("time handle read over....\n");
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);
    // active timer erase.
    for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(1 == n);
        (void)n;
    }
    assert(timers_.size() == activeTimers_.size());
    return expired;
}

bool TimerQueue::insert(Timer *timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
        (void)result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
        (void)result;
    }
    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}

int createTimerfd()
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        // LOG << "failed to create timer fd";
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = read(timerfd, &howmany, sizeof howmany);
    //LOG << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof howmany)
    {
        // LOG << "failed to read timer fd";
    }
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        //LOG << "failed to reset timer fd";
        //printf("falied to set timerFd....\n");
    }
}
