#include "../base/ThreadPool.h"
#include <functional>
#include <iostream>
#include <unistd.h>
using namespace std;
void f(int i){
    printf("i==%d \n",i);
}
int main(int argc, char* argv[ ]){
    int numThread=1;
    int maxQueueSize=6;
    int maxTest=50;
    if(argc >2){
        numThread=atoi(argv[1]);
        maxQueueSize=atoi(argv[2]);
        maxTest=atoi(argv[3]);
    }
    ThreadPool tp;
    tp.start(numThread,maxQueueSize);

    for (int i = 0; i <= maxTest; ++i)
    {
        /
        //printf("q.size==%d current i==%d\n", tp.queueSize(), i);
        tp.run(bind(f, i));
    }
    
    sleep(5);

    return 0;
}
