#ifndef _CALLBACK_H
#define _CALLBACK_H
#include <functional>
#include <memory>
#include "base/Timestamp.h"
class TcpConnection;
class Buffer;
typedef std::function<void()> TimerCallback;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void(const TcpConnectionPtr &)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr &)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr &)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr &, size_t)> HighWaterMarkCallback;

void defaultConnectionCallback(const TcpConnectionPtr &conn);

typedef std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)> MessageCallback;
void defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *, Timestamp);

// get_pointer as it be named
template <class T>
T *get_pointer(T *p)
{
    return p;
}

template <class T>
T *get_pointer(std::unique_ptr<T> const &p)
{
    return p.get();
}

template <class T>
T *get_pointer(std::shared_ptr<T> const &p)
{
    return p.get();
}
#endif