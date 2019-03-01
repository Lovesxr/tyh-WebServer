#include "Timer.h"
#include "unistd.h"
AtomicInt64 Timer::sqe_numCreated_;

void Timer::restart(Timestamp now){
    //printf("restart timer...\n");
    if(repeat_){
        //printf("restart success....\n");
        expiration_=addTime(now,interval_);
    }
    else{
    expiration_ = Timestamp::invalid();
    }
}