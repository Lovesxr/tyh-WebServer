#include "../base/Logging.h"
#include "../base/LogFile.h"
#include "../base/Timestamp.h"
#include <stdio.h>
#include <string>

void bench()
{
    Timestamp start(Timestamp::now());
    long g_total=0;
    const int batch = 1000 * 1000;
    const bool kLongLog = false;
    string empty = " ";
    string longStr(3000, 'X');
    longStr += " ";
    string s1="Hello 0123456789";
    string s2=" abcdefghijklmnopqrstuvwxyz ";
    for (int i = 0; i < batch; ++i)
    {
        LOG << "Hello 0123456789"
                 << " abcdefghijklmnopqrstuvwxyz "
                 << (kLongLog ? longStr : empty)
                 << i;
       g_total=g_total+s1.size()+s2.size()+2;
    }
    Timestamp end(Timestamp::now());
    double seconds = end.timeDifference(end, start);
    printf("%f seconds, %ld bytes, %.2f msg/s, %.2f MiB/s\n",
           seconds, g_total, batch / seconds, g_total / seconds / 1024 / 1024);
}

int main()
{
    LOG<<"Hello world!";
    LOG<< sizeof(Logger);
    LOG<< sizeof(LogStream);
    LOG<< sizeof(LogStream::Buffer);

    bench();
    return 0;
}