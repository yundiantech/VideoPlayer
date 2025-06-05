#include "util.h"

#include <time.h>

#if defined(WIN32)
    #include <WinSock2.h>
    #include <Windows.h>
    #include <direct.h>
    #include <io.h> //C (Windows)    access
#else
    #include <sys/time.h>
    #include <stdio.h>
    #include <unistd.h>

void Sleep(long mSeconds)
{
    usleep(mSeconds * 1000);
}

#endif

void mSleep(int mSecond)
{
#if defined(WIN32)
    Sleep(mSecond);
#else
    usleep(mSecond * 1000);
#endif
}

namespace Util
{

#ifdef WIN32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
int gettimeofday(struct timeval* tv, int* /*tz*/) 
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;

    if( tv )
    {
        GetSystemTimeAsFileTime( &ft );

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        tmpres /= 10;

        tmpres -= DELTA_EPOCH_IN_MICROSECS;

        tv->tv_sec = (time_t)(tmpres / 1000000UL);
        tv->tv_usec = (int)(tmpres % 1000000UL);

        return 0;
    }

    return -1;
}
#endif

uint64_t GetUtcTime()
{
    /* 获取本地utc时间 */
    struct timeval tv;
    gettimeofday(&tv, NULL);

//    /* utc时间 单位 秒*/
//    long timestamp = tv.tv_sec;

    /* utc时间 单位 毫秒 */
    uint64_t timestamp = (uint64_t)tv.tv_sec*1000 + tv.tv_usec/1000;

//    /* utc时间 单位 微秒 */
//    long long timestamp = tv.tv_sec*1000*1000 + tv.tv_usec;

    return timestamp;
}

uint64_t GetLocalTime() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t current_time_ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    // printf("Current timestamp in milliseconds: %llu\n", current_time_ms);
    return current_time_ms;
}

uint64_t GetTodayUtcTime()
{
//    auto getTimeStr = [=](struct tm* t)
//    {
//        char tStr[50] = {0};
//        snprintf(tStr, sizeof(tStr), "%04d-%02d-%02d %02d:%02d:%02d", (1900 + t->tm_year), ( 1 + t->tm_mon), t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
//        return std::string(tStr);
//    };

    time_t ts = time(nullptr);
    struct tm* tUTC = gmtime(&ts);
//    std::cout<<"UTC time:"<<getTimeStr(tUTC)<<std::endl;
//    struct tm* tLocal = localtime(&ts);
//    std::cout<<"Local time:"<<getTimeStr(tLocal)<<std::endl;

#if 1
    tUTC->tm_hour = 0;
    tUTC->tm_min = 0;
    tUTC->tm_sec = 0;
    uint64_t timestamp = (uint64_t)mktime(tUTC) * 1000;
#else
    uint64_t timestamp = (GetUtcTime() - tUTC->tm_hour * 3600000 - tUTC->tm_min * 60000 - tUTC->tm_sec * 1000);
#endif

    return timestamp;
}

uint64_t GetUtcTimeInCurWeek()
{
    /* 获取本地utc时间 */
    struct timeval tv;
    gettimeofday(&tv, NULL);

    /* utc时间 单位 秒*/
    uint64_t timestamp = tv.tv_usec / 1000; //获取毫秒数

    time_t ts = time(nullptr);
    struct tm* tUTC = gmtime(&ts);
    timestamp += (tUTC->tm_hour * 3600000 + tUTC->tm_min * 60000 + tUTC->tm_sec * 1000); //今天的时间戳
    timestamp += tUTC->tm_wday * 86400000; //本周的时间戳

    return timestamp;
}

std::string getLocalTimeInStringFormat()
{
    struct tm nowtime;
    struct timeval tv;
    char time_now[128] = {0};
    gettimeofday(&tv, NULL);

#ifdef WIN32
    time_t t = static_cast<time_t>(tv.tv_sec);
    localtime_s(&nowtime, &t);
#else
    localtime_r(&tv.tv_sec, &nowtime);
#endif

    sprintf(time_now, "%4d%02d%02d%02d%02d%02d%03d",
            nowtime.tm_year + 1900,
            nowtime.tm_mon + 1,
            nowtime.tm_mday,
            nowtime.tm_hour,
            nowtime.tm_min,
            nowtime.tm_sec,
            (int) (tv.tv_usec / 1000));

    return std::string(time_now);
}

};