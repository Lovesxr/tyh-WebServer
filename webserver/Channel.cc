#include <assert.h>
#include <sys/epoll.h>

#include "Channel.h"
#include "EventLoop.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : eventHandling_(false),
      addedToLoop_(false),
      tied_(false),
      index_(-1),
      loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
}

void Channel::update()
{
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void Channel::remove()
{
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::handleEvent()
{
    std::shared_ptr<TcpConnection> guard;
    if (tied_)
    {
        guard = holder_.lock(); //如果持有channel的是TCPCON对象，如果因为一些原因注销了，那就没有必要调用了嘛
        if (guard)
        {
            handleEventWithGuard();
        }
    }
    else
        handleEventWithGuard();
}

void Channel::handleEventWithGuard()
{
    eventHandling_ = true;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    if (revents_ & (EPOLLERR))
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if (readCallback_)
        {
            readCallback_();
        }
    }
    if (revents_ & (EPOLLOUT))
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
    eventHandling_ = false;
}