#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "base/noncopyable.h"

#include <memory>

class Channel;
class EventLoop;
class EventLoopThreadPool;

class TcpServer : public noncopyable
{
  public:
    TcpServer(EventLoop *loop, int threadNum, int port, int idleSeconds_);

    ~TcpServer();

    void start();

    void handleNewConn();

  private:
    int threadNum_;
    int port_;
    int listenFd_;
    int idleSeconds_;
    EventLoop *loop_;
    int maxFds_;
    Channel *acceptChannel_;
    const std::unique_ptr<EventLoopThreadPool> threadPool_;
};

#endif
