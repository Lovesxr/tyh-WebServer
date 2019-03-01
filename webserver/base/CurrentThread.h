#ifndef _CURRENT_THREAD_H
#define _CURRENT_THREAD_H
#include <pthread.h>
namespace CurrentThread
{
//使用gettid获得系统内唯一的ID pid_t一般使用int保存的
extern __thread int t_cachedTid;
//保存tid的字符串形式
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char *t_threadName;

pid_t gettid();
void cachedTid();

inline int tid()
{
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
        cachedTid();
    }
    return t_cachedTid;
}

inline const char *tidString()
{
    return t_tidString;
}

inline int tidStringLength()
{
    return t_tidStringLength;
}

inline const char *name()
{
    return t_threadName;
}

} // namespace CurrentThread

#endif