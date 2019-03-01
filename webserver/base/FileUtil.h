#ifndef _FILEUTIL_H
#define _FILEUTIL_H
#include "noncopyable.h"
#include "StringPiece.h"
class AppendFile : public noncopyable
{
  public:
    explicit AppendFile(StringArg filename);
    ~AppendFile();

    void append(const char *logline, const size_t len);
    void flush();
    int writtenBytes() const
    {
        return writtenBytes_;
    }

  private:
    size_t write(const char *logline, size_t len);
    FILE *fp_;
    char buffer_[64 * 1024];
    int writtenBytes_;
};

#endif