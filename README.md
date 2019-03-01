# tyh-WebServer
A High-performance C++ WebServer


#特性概述
* 基于对象编程，在具体的类中注册回调函数(function+bind)，避免以继承方式承担虚函数指针以及虚函数表的开销，以及多重继承复杂性高，可拓展新不足的问题
* 采用Reactor模型(one eventloop one thread)+ThreadPool处理计算任务，使用epoll LT来进行IO多路复用，采用单进程多线程的设计，充分发挥多核性能
* 使用shared_ptr,weak_ptr,unique_ptr以及用RAIL手法管理资源
* 为避免TcpConnection对象的频繁创建与销毁，在每个ioLoop启动阶段，预先生成多个TcpConnection对象以供使用
* 实现了双缓冲异步日志
* 使用std::list管理不活跃的Tcp连接
* 使用timerfd来管理定时器，使用eventfd进行唤醒
* 学习了muduo的runInLoop以及queueInLoop方法，可以安全的进行跨线程调用
* 对于临界区的资源，如等待跨线程调用的任务，采用COW手法缩小临界区
* 支持优雅的关闭连接

# 并发模型
程序采用Reactor模型，采用一个mainReactor+多个SubReactor+ThreadPool的结构，mainReactor负责client用户的连接请求，负责建立连接，然后使用Round Robin算法分配给SubReactor线程进行IO请求处理，ThreadPool采用Round Robin算法分配计算线程对任务进行处理

## epoll工作模式
epoll的触发方式有LT以及ET两种，这里采用了LT，主要有以下的考虑:
* ET的读写必须读写到EAGAIN，否则会漏掉事件，而LT没有读取完毕，下一次epoll返回时还会继续提醒
* ET的实际的效率应该是高于LT，具体表现在对于EPOLLOUT事件，epoll_ctl一次之后就不用修改了，而LT关注EPOLLOUT事件，当写入完毕后，必须在epoll_ctl移除EPOLLOUT,否则epoll_wait会发生busy loop.
* ET和LT对于EPOLLIN，我个人倾向于LT，第一采用了一个64KB的栈上空间和Buffer使用readv从内核读取数据，一次读取往往就能读取完毕，同时照顾了连接的公平性每个连接一次loop循环只读取一次，
就算消息过大没有读完，下一次epoll_wait还会返回，不会丢失消息，然而对于ET来说，必须读取到EAGAIN,至少read2次，否则事件会丢失
* 因此，就我个人而言，我觉得理想的触发方式应该是读可以选择LT,写可以选择ET,这样可以兼具效率以及公平

## 使用list管理超时连接
超时连接的管理主要可以使用时间轮，堆，以及链表，这里采用list对超时连接进行管理，每个ioLoop持有一个ConnectionList，每个TcpConnection内含有一个TimeNode

每个TcpConnection内含有一个TimeNode，包含2个成员，一个exipreTime,一个list的迭代器指示其在list中的位置，方便高效的删除以及更新

* 对于一个新的连接,直接将它插入到链表的尾部，时间复杂度O(1)
* 老的连接在超时时间内发送了新的数据，直接利用TimeNode中的pos信息，利用list的splice方法移动到链表尾部，时间复杂度O(1)
* 定期删除过期的连接，ioLoop中注册了一个每秒运行的removeConnection方法，每次从表头开始遍历，利用gettimeofday()调用得到当前时间，与expireTime比较，若小于,直接终止循环，若大于，则关闭这个Tcp连接，继续遍历下一个，时间复杂度O(n)

## 使用timefd管理定时器
timefd是Linux新增的系统调用，将时间的到期变得像文件描述符读取一样，可以方便的纳入epoll进行管理，eventLoop内含一个timerQueue，采用std::set对定时器进行管理

## ThreadPool
使用function作为任务的接口，可以方便的利用bind传入想要的函数，任务放进vector中，用vector来模拟循环队列，利用封装好的Mutex,Condition保证对任务的有序提取

## Log
Log的实现分为前端和后端，前端往后端写，后端往磁盘写，因为磁盘IO有时是非常慢的，如果不做这样的处理，直接往磁盘写入，发生了阻塞，这样就浪费了大量的CPU资源，使用一个后端线程专门向磁盘写入，前端只需往后端的缓冲里塞入向写入的记录，多个前端写入只需要利用Mutex进行保护，由于是在内存中操作，前端向后端的缓冲区写入是十分快的，最后由后端负责将记录写入到磁盘中。

后端主要是由2组缓冲区构成的，实际实现采用了2组，每组2个缓冲区，当前端写满了一组缓冲区，就交换缓冲区，后端线程负责将满的缓冲区写入到磁盘，前端继续向空的缓冲区进行写入。

# 测试
各个模块进行了测试，利用WebBench也对WebServer进行了并发的测试，使用ping-ping测试分别测试了服务器在单线程和多线程下的吞吐量

测试结果见[测试及分析.md](https://github.com/StellaYuhao/tyh-WebServer/blob/master/测试及分析.md))
