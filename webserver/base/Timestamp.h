#ifndef _TIMESTAMP_H
#define _TIMESTAMP_H

#include <inttypes.h>
#include <string>

class Timestamp
{
  public:
    Timestamp() : microSecondsSinceEpoch_(0)
    {
    }

    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
    {
    }

    ~Timestamp()
    {
    }

    bool operator<(const Timestamp &rhs) const
    {
        return this->microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    bool operator==(const Timestamp &rhs) const
    {
        return this->microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }

    void swap(Timestamp &that)
    {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    std::string toFormattedString() const;

    bool valid() const
    {
        return microSecondsSinceEpoch_ > 0;
    }

    int64_t microSecondsSinceEpoch() const
    {
        return microSecondsSinceEpoch_;
    }
    std::string toString() const;
    static Timestamp now();
    static Timestamp invalid()
    {
        return Timestamp();
    }
    inline double timeDifference(Timestamp high, Timestamp low)
    {
        int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
    }
    static const int kMicroSecondsPerSecond = 1000 * 1000;

  private:
    int64_t microSecondsSinceEpoch_;
};

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}
#endif