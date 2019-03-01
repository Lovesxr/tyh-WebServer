#include <netinet/in.h>

#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "HttpData.h"
#include "base/SocketsOps.h"
#include "TcpConnection.h"
#include "TcpServer.h"
#include "base/Logging.h"
#include "unistd.h"
TcpServer::TcpServer(EventLoop *loop, int threadNum, int port, int idleSeconds)
    : threadNum_(threadNum),
      port_(port),
      listenFd_(sockets::socketBindListen(port)), //SocketsOps.h
      idleSeconds_(idleSeconds),
      loop_(loop),
      maxFds_(100000),
      acceptChannel_(new Channel(loop, listenFd_)),
      threadPool_(new EventLoopThreadPool(loop, idleSeconds))
{
    if (listenFd_ < 0)
    {
        printf("error listen!");
    }
    //printf("construct server over!\n");
}

// threadPool_ 中的连接池需要及时清理掉，交给std::list的默认dtor
TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
    threadPool_->setThreadNum(threadNum_);
    threadPool_->start();
    acceptChannel_->enableReading(); 
    acceptChannel_->setReadCallback(std::bind(&TcpServer::handleNewConn, this));
    loop_->updateChannel(acceptChannel_);
    //LOG<<"TcpServer started over...";
}

void TcpServer::handleNewConn()
{
    struct sockaddr_in6 addr;
    bzero(&addr, sizeof addr);
    int connfd = 0;
    //LOG <<"enter tcpserver handle conn...";
    // printf("enter tcpserver handle conn...\n");
    //这里采用一直accept到不能accept为止，优化了对短连接的处理
    //否则每次只处理一个，如果来了大量短连接,每一个都会触发一次epoll，系统调用耗时不划算
    while ((connfd = sockets::accept(listenFd_, &addr)) > 0)
    {
        //LOG <<"accept a new conn...";
        //printf("accept a new conn...\n");
        if (connfd >= maxFds_)
        {
            close(connfd);
            continue;
        }
        EventLoop *ioLoop = threadPool_->getNextLoop();

        if (sockets::setSocketNonBlocking(connfd) < 0)
        {
            return;
        }
        sockets::setTcpNoDelay(connfd, true);
        sockets::setReuseAddr(connfd, true);
        sockets::setReusePort(connfd, true);
        ioLoop->runInLoop(std::bind(&EventLoop::getNextFreeConnection, ioLoop, connfd));
    }
}
