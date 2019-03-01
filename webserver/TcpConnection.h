#ifndef _TcpConnection_H_
#define _TcpConnection_H_

#include <functional>
#include <memory>

#include "base/Buffer.h"
#include "base/noncopyable.h"
#include "Callback.h"
class Channel;
class EventLoop;
class HttpData;
class TcpConnection;
class TimeNode;
class TcpConnection : public noncopyable, public std::enable_shared_from_this<TcpConnection>
{
  public:
    typedef std::function<void(Buffer *)> Callback;
    TcpConnection(EventLoop *loop);
    ~TcpConnection();

    void setChannel(int connfd);
    std::shared_ptr<Channel> getChannel()
    {
        return channel_;
    }

    std::shared_ptr<TcpConnection> getNextTcpConn()
    {
        return nextTcpConn_;
    }
    void setNextTcpConn(std::shared_ptr<TcpConnection> nTcpConn)
    {
        nextTcpConn_ = nTcpConn;
    }
    void resetNextTcpConn()
    {
        nextTcpConn_.reset();
    }
    bool connected() const { return state_ == KConnected; }
    bool disconnected() const { return state_ == KDisconnected; }
    void setRequestCallback(const Callback &cb)
    {
        requestCallback_ = std::move(cb);
    }

    std::shared_ptr<TimeNode> getTimeNode()
    {
        return TimeNode_;
    }
    void shutdown();
    void shutdownInLoop();

    void forceClose();
    void forceCloseWithDelay(double seconds);
    void setStateKConnected()
    {
        state_ = KConnected;
    }

    Buffer &getInputBuffer()
    {
        return inputBuffer_;
    }
    Buffer &getOutputBuffer()
    {
        return outputBuffer_;
    }

    void send(Buffer *buf);

    void configEvent();

    void setIndex(int index)
    {
        index_ = index;
    }

    int getIndex()
    {
        return index_;
    }

    void reset()
    {
        state_ = KConnecting;
        inputBuffer_.retrieveAll();
        outputBuffer_.retrieveAll();
    }
    void connectEstablished();
    void connectDestroyed();
    void echoMessageCallback(Buffer *buf);

  private:
    void sendInLoop(const char *message, size_t len);
    void forceCloseInLoop();
    enum StateE
    {
        KConnecting,
        KConnected,
        KDisconnecting,
        KDisconnected
    };
    void setState(StateE s)
    {
        state_ = s;
    }

    StateE state_;
    int index_;

    EventLoop *loop_;

    Callback requestCallback_;
    ConnectionCallback connectionCallback_;

    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    std::shared_ptr<TcpConnection> nextTcpConn_; // 当在连接链表中时，作为指针指引下一个可用的空闲连接
    std::shared_ptr<HttpData> httpData_;         // 从连接链表中取出时，作为指针指向数据

    std::shared_ptr<Channel> channel_;   // socket fd...
    std::shared_ptr<TimeNode> TimeNode_; //用于管理TcpConnection的超时连接
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

#endif
