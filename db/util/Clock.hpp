#pragma once

#include <chrono>
#include <ctime>
#include <iostream>
#include <ratio>

namespace Util {

class Clock {
public:
    enum class Format{
        SHORT_TIME,
        AMERICAN,
        MID_TIME,
        LONG_TIME,
        NO_CLOCK_SHORT,
        NO_CLOCK_AMERICAN,
        NO_CLOCK_MID,
        NO_CLOCK_LONG
    };

    using rep = time_t;
    using period = std::ratio<1, 1>; // 1 second
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<Clock>;
};

inline Clock::Format time_format = Util::Clock::Format::NO_CLOCK_AMERICAN;

namespace Time {


// This expects data in human-readable units
// E.g if you want to create 1970-02-15 you call create(1970, 2, 15, 0, 0, 0).
// Maybe it is obvious but C library uses another format.
inline Clock::time_point create(int year, int month, int day, int hour = 0, int minute = 0, int second = 0){
    tm time {
        .tm_sec = second,
        .tm_min = minute,
        .tm_hour = hour,
        .tm_mday = day,
        .tm_mon = month - 1,
        .tm_year = year - 1900,
        .tm_wday = 0,
        .tm_yday = 0,
        .tm_isdst = 0,
#ifdef __USE_MISC
        .tm_gmtoff = {},
        .tm_zone = {},
#endif
    };
    auto time_epoch = mktime(&time);
    return Util::Clock::time_point(Util::Clock::duration(time_epoch));
}

}

// We need at least 8 bytes to represent our simulation times!
static_assert(sizeof(time_t) == 8, "");

}

inline std::ostream& operator<<(std::ostream& out, Util::Clock::time_point const& time){
    auto time_as_time_t = time.time_since_epoch().count();

    tm* time_as_tm = gmtime(&time_as_time_t);

    char buffer[256];

    switch (Util::time_format) {
    case Util::Clock::Format::SHORT_TIME:
        strftime(buffer, 256, "%D, %T", time_as_tm);
        break;
    case Util::Clock::Format::AMERICAN:
        strftime(buffer, 256, "%F, %r", time_as_tm);
        break;
    case Util::Clock::Format::MID_TIME:
        strftime(buffer, 256, "%a, %d %b %G, %T", time_as_tm);
        break;
    case Util::Clock::Format::LONG_TIME:
        strftime(buffer, 256, "%A, %d %B %G, %T", time_as_tm);
        break;
    case Util::Clock::Format::NO_CLOCK_SHORT:
        strftime(buffer, 256, "%D", time_as_tm);
        break;
    case Util::Clock::Format::NO_CLOCK_AMERICAN:
        strftime(buffer, 256, "%F", time_as_tm);
        break;
    case Util::Clock::Format::NO_CLOCK_MID:
        strftime(buffer, 256, "%a, %d %b %G", time_as_tm);
        break;
    case Util::Clock::Format::NO_CLOCK_LONG:
        strftime(buffer, 256, "%A, %d %B %G", time_as_tm);
        break;
    }

    out << buffer;
    return out;
}
