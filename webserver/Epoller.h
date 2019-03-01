#ifndef _EPOLLER_H_
#define _EPOLLER_H_
#include "EventLoop.h"
#include "base/noncopyable.h"
#include <map>
#include <vector>

class Channel;
struct epoll_event;
class Epoller : public noncopyable
{
  public:
    typedef std::vector<Channel *> ChannelList;
    Epoller(EventLoop *loop);
    ~Epoller();

    void poll(int timeoutMs, ChannelList *activeChannels);
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    bool hasChannel(Channel *channel) const;

    void assertInLoopThread() const
    {
        ownerLoop_->assertInLoopThread();
    }

  private:
    typedef std::map<int, Channel *> ChannelMap;
    typedef std::vector<struct epoll_event> EpollList;
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    void update(int operation, Channel *channel);

    static const int kInitEventListSize = 16;
    static const char *operationToString(int op);

    int epollfd_;

    EventLoop *ownerLoop_;

    EpollList epollEvents_;

    ChannelMap channels_;
};

#endif
