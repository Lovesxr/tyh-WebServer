#include "../EventLoop.h"
#include "../base/Thread.h"

#include <functional>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>



EventLoop* g_loop;

void callback()
{
  printf("callback(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  EventLoop anotherLoop;
}

void threadFunc()
{
  printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
  loop.runAfter(1.0, callback);
  loop.loop();
}

int main()
{
  printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

  Thread thread(threadFunc,"EventLoop");
  thread.start();

  loop.loop();
  return 0;
}
