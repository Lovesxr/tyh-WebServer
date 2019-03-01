#include "../EventLoopThread.h"
#include "../EventLoop.h"
#include "../base/Thread.h"
#include "../base/CountDownLatch.h"

#include <functional>

#include <stdio.h>
#include <unistd.h>

void print(EventLoop *p = NULL)
{
  printf("print: pid = %d, tid = %d, loop = %p\n",
         getpid(), CurrentThread::tid(), p);
}

void quit(EventLoop *p)
{
  print(p);
  p->quit();
}

int main()
{
  print();

  {
    EventLoopThread thr1(-1); // never start
  }

  {
    // dtor calls quit()
    EventLoopThread thr2(-1);
    EventLoop *loop = thr2.startLoop();
    loop->runInLoop(std::bind(print, loop));
    sleep(5);
  }

  {
    // quit() before dtor
    EventLoopThread thr3(-1);
    EventLoop *loop = thr3.startLoop();
    loop->runInLoop(std::bind(quit, loop));
    sleep(5);
  }
}
