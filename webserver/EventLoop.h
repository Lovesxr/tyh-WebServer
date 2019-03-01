#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_

#include <functional>
#include <memory>
#include <pthread.h>
#include <vector>

#include "Callback.h"
#include "base/CurrentThread.h"
#include "base/Mutex.h"
#include "base/noncopyable.h"
#include "TcpConnection.h"
#include "TimerQueue.h"
#include "base/Timestamp.h"
#include "TimerId.h"
#include <algorithm>
#include <list>
class Channel;
class Epoller;

typedef std::list<std::weak_ptr<TcpConnection>> TcpConnectionTimeList;

struct TimeNode
{
    Timestamp lastReceiveTime;
    TcpConnectionTimeList::iterator position;
};

class EventLoop : public noncopyable
{
  public:
    typedef std::function<void()> Functor;
    EventLoop();
    EventLoop(int idleSeconds_);
    ~EventLoop();

    void initConnectionPool();

    void loop();

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const
    {
        return threadId_ == CurrentThread::tid();
    }

    void quit();

    TimerId runAt(const Timestamp &time, TimerCallback &&cb);
    TimerId runAfter(double delay, TimerCallback &&cb);
    TimerId runEvery(double interval, TimerCallback &&cb);

    void cancel(TimerId timerId);
    // schedules the task during different threads.
    // run the callback fuction when in the current IO thread, otherwise add into the task queue of the thread.
    void runInLoop(Functor &&cb);
    // void runInLoop(InitFunctor&& cb);
    // add the callback function into the task queue of the current thread.
    void queueInLoop(Functor &&cb);

    void wakeup(); // internal usage

    void updateChannel(Channel *channel);

    void removeChannel(Channel *channel);

    //从TcpConn空闲链表中获得一个TcpConn对象,如果没有空闲的对象再申请一个
    //TO DO:设置一个上限，避免TcpConn对象太多,内存溢出
    std::shared_ptr<TcpConnection> getNextFreeConnection(int connFd);

    void returnBackConnection(std::shared_ptr<TcpConnection> TcpConn);

    //以下四个函数和空闲连接的管理有关
    //这里实现采用std::list作为空闲链表的底层实现，每个TcpConn持有一个TimeNode
    //TimeNode有2个成员，一个expireTime，一个指示在list中位置的迭代器
    //这样每次来了新的Tcp连接，将它插入到list的尾端
    //Tcp老的连接发送来了数据，就将它从现有位置移动到链表尾端
    //连接的删除是设定一个每秒循环的定时器删除，从链表头进行遍历，每次在定时器中读取时间，与超期时间比较，到了就删除
    //插入 更新操作时间复杂度都是O(1)，删除从链表头开始遍历，平均时间复杂度O(n)
    void insert(std::shared_ptr<TcpConnection> conn);

    void updateIdleTimeList(std::shared_ptr<TimeNode> node);

    void removeExpiredConnection();

    void printConnectionList() const;

  private:
    typedef std::vector<Channel *> ChannelList; // Channel in interest
    typedef std::vector<std::shared_ptr<TcpConnection>> TcpConnectionList;

    void abortNotInLoopThread();
    void handleRead();        // wakeupFd_:eventfd, wakeup()
    void doPendingFunctors(); 
    bool looping_;
    bool eventHandling_;
    bool quit_;
    bool callingPendingFunctors_;

    int wakeupFd_; // Fuctors in doPendingFunctors() may call queueInLoop(functor) again, so queueInLoop() should wake up the IO thread.
    const pid_t threadId_;

    int idleSeconds_;
    Channel *wakeupChannel_;
    Channel *currentActiveChannel_;

    const std::unique_ptr<Epoller> poller_; 
    const std::unique_ptr<TimerQueue> timerQueue_;

    std::shared_ptr<TcpConnection> freeTcpConnection_; // 指向第一个空闲可用的TcpConnectionPtr 减少了对象构造的成本

    std::vector<Functor> pendingFunctors_;

    ChannelList activeChannels_;
    TcpConnectionList activeConnections_;
    TcpConnectionTimeList connectionTimeList_;
    MutexLock mutex_;
};

#endif
