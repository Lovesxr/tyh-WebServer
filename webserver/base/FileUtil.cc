#include "FileUtil.h"

#include "assert.h"

AppendFile::AppendFile(StringArg filename)
    : fp_(fopen(filename.c_str(), "ae")), // e open O_CLOEXEC flag 执行exec调用的 会关闭打开这个标志的描述符
      writtenBytes_(0)
{
    assert(fp_);
    //set for fp_'s buffer
    setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
    fclose(fp_);
    printf("write bytes== %d\n", writtenBytes_);
}

void AppendFile::append(const char *logline, size_t len)
{
    size_t n = this->write(logline, len);
    size_t remain = len - n;
    //printf("apend file....\n");
    while (remain > 0)
    {
        size_t x = this->write(logline + n, remain);
        if (x == 0)
        {
            int err = ferror(fp_);
            if (err)
            {
                fprintf(stderr, "AppendFile::append() failed.\n");
            }
            break;
        }
        remain -= x;
    }
    writtenBytes_ += len;
}

void AppendFile::flush()
{
    fflush(fp_);
}

size_t AppendFile::write(const char *logline, size_t len)
{
    //@para2: 写入数据项的字节大小
    //@para3: 写入数据的数目
    return fwrite_unlocked(logline, 1, len, fp_);
}