#include <assert.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdio.h>

#include "Channel.h"
#include "Epoller.h"
#include "EventLoop.h"
#include "base/Logging.h"
using namespace std;

__thread EventLoop *t_loopInThisThread = 0;

const int kPollTimeMs = 10000;
const int MaxConnectionPoolSize = 512;

int createEventfd();

// 为保证线程安全，跨线程对象在构造函数期间不可把this指针泄露出去
// EventLoop不为跨线程对象，它与IO线程是一一对应关系
EventLoop::EventLoop()
    : looping_(false),
      eventHandling_(false),
      quit_(false),
      callingPendingFunctors_(false),
      wakeupFd_(createEventfd()),
      threadId_(CurrentThread::tid()),
      idleSeconds_(-1),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(NULL),
      poller_(new Epoller(this)),
      timerQueue_(new TimerQueue(this)),
      freeTcpConnection_(NULL)
{
    // LOG_DEBUG << "init EventLoop";
    if (t_loopInThisThread)
    {
        // LOG_FATAL << "another thread can access this eventloop";
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this)); // wake up the possibly blocked IO thread
    wakeupChannel_->enableReading();
}

EventLoop::EventLoop(int idleSeconds)
    : looping_(false),
      eventHandling_(false),
      quit_(false),
      callingPendingFunctors_(false),
      wakeupFd_(createEventfd()),
      threadId_(CurrentThread::tid()),
      idleSeconds_(idleSeconds),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(NULL),
      poller_(new Epoller(this)),
      timerQueue_(new TimerQueue(this)),
      freeTcpConnection_(NULL)
{
    // LOG_DEBUG << "init EventLoop";
    if (t_loopInThisThread)
    {
        // LOG_FATAL << "another thread can access this eventloop";
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this)); // wake up the possibly blocked IO thread
    wakeupChannel_->enableReading();
    if (idleSeconds_ > 0)
    {
        runEvery(1.0, bind(&EventLoop::removeExpiredConnection, this));
    }
}

EventLoop::~EventLoop()
{
    assert(!looping_);
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::initConnectionPool()
{
    freeTcpConnection_ = std::shared_ptr<TcpConnection>(new TcpConnection(this));
    std::shared_ptr<TcpConnection> tc(freeTcpConnection_);
    for (int i = 0; i < MaxConnectionPoolSize; i++)
    {
        std::shared_ptr<TcpConnection> tct(new TcpConnection(this));
        tct->setIndex(i);
        tc->setNextTcpConn(tct);
        tc = tc->getNextTcpConn();
    }
}

std::shared_ptr<TcpConnection> EventLoop::getNextFreeConnection(int connfd)
{
    std::shared_ptr<TcpConnection> tc;
    if (freeTcpConnection_->getNextTcpConn() != NULL)
    {
        tc = freeTcpConnection_->getNextTcpConn();
        freeTcpConnection_->setNextTcpConn(tc->getNextTcpConn());
        tc->resetNextTcpConn();
    }
    else
    {
        tc = std::shared_ptr<TcpConnection>(new TcpConnection(this));
    }
    activeConnections_.push_back(tc);
    tc->setStateKConnected();
    tc->setChannel(connfd);
    insert(tc);
    tc->configEvent();
    return tc;
}

void EventLoop::returnBackConnection(std::shared_ptr<TcpConnection> stc)
{
    stc->setNextTcpConn(freeTcpConnection_->getNextTcpConn());
    freeTcpConnection_->setNextTcpConn(stc);
    TcpConnectionList::iterator i = std::find(activeConnections_.begin(), activeConnections_.end(), stc);
    assert(i != activeConnections_.end());
    activeConnections_.erase(i);
    //将TcpConn从TimeList移除
    if (idleSeconds_ <= 0)
        return;
    std::shared_ptr<TimeNode> node = stc->getTimeNode();
    connectionTimeList_.erase(node->position);
}

void EventLoop::insert(std::shared_ptr<TcpConnection> conn)
{
    if (idleSeconds_ <= 0)
        return;
    std::shared_ptr<TimeNode> node = conn->getTimeNode();
    node->lastReceiveTime = Timestamp::now();
    connectionTimeList_.push_back(conn);
    node->position = --connectionTimeList_.end();
}

void EventLoop::updateIdleTimeList(std::shared_ptr<TimeNode> node)
{
    if (idleSeconds_ <= 0)
        return;
    node->lastReceiveTime = Timestamp::now();
    connectionTimeList_.splice(connectionTimeList_.end(), connectionTimeList_, node->position);
    assert(node->position == --connectionTimeList_.end());
}
void EventLoop::removeExpiredConnection()
{
    if (idleSeconds_ <= 0)
        return;
    //printConnectionList();
    int deleten = 0;
    //printf("begin remove conn... \n");
    Timestamp now = Timestamp::now();
    for (TcpConnectionTimeList::iterator it = connectionTimeList_.begin();
         it != connectionTimeList_.end();)
    {
        TcpConnectionPtr conn = it->lock();
        if (conn->getNextTcpConn() == NULL)
        {
            std::shared_ptr<TimeNode> n = conn->getTimeNode();
            double age = now.timeDifference(now, n->lastReceiveTime);
            if (age > idleSeconds_)
            {
                if (conn->connected())
                {
                    conn->shutdown();
                    //LOG << "shutting down " << conn->name();
                    conn->forceCloseWithDelay(3.5); // > round trip of the whole Internet.
                    ++deleten;
                }
            }
            else if (age < 0)
            {
                //LOG << "Time jump";
                n->lastReceiveTime = now;
            }
            else
            {
                break;
            }
            ++it;
        }
        else
        {
            //LOG << "Expired";
            it = connectionTimeList_.erase(it);
            ++deleten;
        }
    }
    //printf("delete con=%d,end remove conn...\n",deleten);
}

void EventLoop::printConnectionList() const
{
    //LOG << "size = " << connectionTimeList_.size();

    for (TcpConnectionTimeList::const_iterator it = connectionTimeList_.begin();
         it != connectionTimeList_.end(); ++it)
    {
        TcpConnectionPtr conn = it->lock();
        if (conn->getNextTcpConn() == NULL)
        {
            //printf("conn %p\n", get_pointer(conn));
            std::shared_ptr<TimeNode> n = conn->getTimeNode();
            //printf("    time %s\n", n->lastReceiveTime.toString().c_str());
        }
        else
        {
            //printf("expired\n");
        }
    }
}
void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    while (!quit_)
    {
        //printf("begin one time in loop \n");
        activeChannels_.clear();
        //LOG <<" epoll wait...";
        poller_->poll(kPollTimeMs, &activeChannels_);
        //LOG <<"handle read of...";

        eventHandling_ = true;
        for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); ++it)
        {
            //LOG<<"handle events...";
            //printf("event handling \n");
            currentActiveChannel_ = (*it);
            currentActiveChannel_->handleEvent();
            //printf("eventhanle over \n");
        }

        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        doPendingFunctors();
        //printf("end one time in loop \n");
    }
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Timestamp &time, TimerCallback &&cb)
{
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback &&cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback &&cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId){
    return timerQueue_->cancel(timerId);
}
void EventLoop::runInLoop(Functor &&cb)
{
    //printf("run in loop.... \n");
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor &&cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    assert(n == one);
    if (n != sizeof one)
    {
        // LOG_ERROR << "failed to wake up eventloop";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        // LOG_ERROR << "failed to handle read event of eventloop";
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors; // narrow the critical area
    callingPendingFunctors_ = true;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (size_t i = 0; i < functors.size(); i++)
    {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    // TODO: take this channel being active channel or not into consideration
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel || std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    // LOG_FATAL << "access this eventloop in wrong thread";
}

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        // LOG_SYSERR << "cannot get right event fd";
        abort();
    }
    return evtfd;
}
