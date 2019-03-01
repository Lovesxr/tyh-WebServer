#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <functional>
#include <memory>
#include "EventLoop.h"
#include "base/noncopyable.h"
#include "base/Timestamp.h"
#include "TcpConnection.h"
//class EventLoop;

class Channel : public noncopyable
{
  public:
    typedef std::function<void()> EventCallback;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void handleEvent();

    void setReadCallback(EventCallback &&cb)
    {
        readCallback_ = cb;
    }
    void setWriteCallback(EventCallback &&cb)
    {
        writeCallback_ = cb;
    }
    void setErrorCallback(EventCallback &&cb)
    {
        errorCallback_ = cb;
    }
    void setCloseCallback(EventCallback &&cb)
    {
        closeCallback_ = cb;
    }
    const int fd() const
    {
        return fd_;
    }

    void setFd(int fd)
    {
        fd_ = fd;
    }

    const int events() const
    {
        return events_;
    }
    void set_revents(int revt)
    {
        revents_ = revt;
    }
    bool isNoneEvent() const
    {
        return events_ == kNoneEvent;
    }
    void enableReading()
    {
        events_ |= kReadEvent; // 相应位变１
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent; // 相应位变０
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    int index()
    { // for Poller
        return index_;
    }

    void setIndex(int idx)
    {
        index_ = idx;
    }
    EventLoop *ownerLoop()
    {
        return loop_;
    }

    void remove(); // TimerQueue.h

    bool isReading() const
    {
        return events_ & kReadEvent;
    }
    bool isWriting() const
    {
        return events_ & kWriteEvent;
    }

    void setHolder(std::shared_ptr<TcpConnection> holder)
    {
        holder_ = holder;
    }
    std::shared_ptr<TcpConnection> getHolder()
    {
        std::shared_ptr<TcpConnection> holder(holder_.lock());
        return holder;
    }

  private:
    void update();
    void handleEventWithGuard();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    bool eventHandling_;
    bool addedToLoop_;
    bool tied_;

    EventLoop *loop_;
    int fd_;
    int events_;
    int revents_;
    int index_; // used by Poller

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
    std::weak_ptr<TcpConnection> holder_; // 方便找到上层持有该Channel的对象
};

#endif