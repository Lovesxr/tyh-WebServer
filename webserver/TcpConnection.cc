#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "HttpData.h"
#include "base/SocketsOps.h"
#include <unistd.h>

TcpConnection::TcpConnection(EventLoop *loop)
    : state_(KConnecting),
      index_(-1),
      loop_(loop),
      nextTcpConn_(NULL),
      httpData_(new HttpData()),
      TimeNode_(new TimeNode),
      channel_(new Channel(loop, -1))
{

    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection()
{
}

void TcpConnection::send(Buffer *buf)
{
    if (state_ == KConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf->peek(), buf->readableBytes()));
            buf->retrieveAll();
        }
    }
}

void TcpConnection::sendInLoop(const char *data, size_t len)
{
    //printf("send in loop...\n");
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    ssize_t remaining = 0;
    bool faultError = false;
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), static_cast<const void *>(data), len);
        //printf("send in send in loop==%d \n",nwrote);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining > 0)
            {
                // LOG << "I am going to write more data.";
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                if (errno == EPIPE || errno == ECONNRESET || errno == EFAULT || errno == EINTR)
                {
                    faultError = true;
                }
            }
        }
    }
    assert(nwrote >= 0);
    if (!faultError && remaining > 0)
    {
        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
    else if (!faultError && remaining == 0)
    {
        // loop_->givebackContext(shared_from_this());
        // handleClose();
    }
}
void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == KConnecting);
    setState(KConnected);
    channel_->setHolder(shared_from_this());
    channel_->enableReading();
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == KConnected)
    {
        setState(KDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}
void TcpConnection::configEvent()
{
    loop_->updateChannel(channel_.get());
    channel_->enableReading();
    httpData_->setHolder(shared_from_this());
}
void TcpConnection::echoMessageCallback(Buffer *buf)
{
    this->send(buf);
}
void TcpConnection::handleRead()
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    //printf("received:%d\n",n);
    loop_->updateIdleTimeList(this->TimeNode_);
    //
    if (n > 0)
    {
        //use for http-server test
        //requestCallback_(&inputBuffer_);

        //use for ping-pong test
        echoMessageCallback(&inputBuffer_);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        // LOG << "there is a error"
        handleError();
    }
}

void TcpConnection::shutdown()
{
    if (state_ == KConnected)
    {
        setState(KDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        sockets::shutdownWrite(channel_->fd());
    }
}

void TcpConnection::forceClose()
{
    if (state_ == KConnected || state_ == KDisconnecting)
    {
        setState(KDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}
void TcpConnection::forceCloseWithDelay(double seconds)
{
    if (state_ == KConnected || state_ == KDisconnecting)
    {
        setState(KDisconnecting);
        loop_->runAfter(
            seconds,
            std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}
void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == KConnected || state_ == KDisconnecting)
    {
        // as if we received 0 byte in handleRead()
        handleClose();
    }
}
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        //printf("send:%d\n",n);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (state_ == KDisconnecting)
                {
                    shutdownInLoop();
                }
            }
            else
            {
                // LOG << "I am going to write more data";
            }
        }
        else
        {
            // LOG << "failed to write fd";
        }
    }
    else
    {
        // The Connection is down
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    //printf("close a conn....\n");
    assert(state_ == KConnected || state_ == KDisconnecting);
    setState(KDisconnected);
    channel_->disableAll();
    ::close(channel_->fd());
    channel_->setFd(-1);
    reset();
    //connectionCallback_(shared_from_this());
    //这里似乎不需要runInLoop了，已经在ioLoop里面了
    loop_->runInLoop(std::bind(&EventLoop::removeChannel, loop_, channel_.get()));
    loop_->runInLoop(std::bind(&EventLoop::returnBackConnection, loop_, shared_from_this()));
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    // LOG << "there is an error, check the errno";
}

void TcpConnection::setChannel(int connfd)
{
    channel_->setFd(connfd);
    channel_->setHolder(shared_from_this());
}
