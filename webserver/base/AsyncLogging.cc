#include "AsyncLogging.h"
#include "LogFile.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <functional>

AsyncLogging::AsyncLogging(const std::string &logFileName_, off_t rollSize, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(logFileName_),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      latch_(1)
{
    assert(logFileName_.size() > 1);
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
    printf("rollsize==%d \n", rollSize_);
}

void AsyncLogging::append(const char *logline, int len)
{
    MutexLockGuard lock(mutex_);
    if (currentBuffer_->avail() > len)
        currentBuffer_->append(logline, len);
    else
    {
        buffers_.push_back(std::move(currentBuffer_));
        currentBuffer_.reset();
        if (nextBuffer_)
            currentBuffer_ = std::move(nextBuffer_);
        else
            currentBuffer_.reset(new Buffer);
        currentBuffer_->append(logline, len);
        cond_.notify();
    }
}
void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_, rollSize_, flushInterval_);
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            MutexLockGuard lock(mutex_);
            if (buffers_.empty()) // unusual usage!
            {
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());

        if (buffersToWrite.size() > 25)
        {
            //对于前端线程输出的过多日志，选择丢掉多余的日志，防止日志崩溃
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        for (size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }

        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}