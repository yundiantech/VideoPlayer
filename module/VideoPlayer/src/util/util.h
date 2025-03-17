#pragma once

#include <string>
#include <stdio.h>
#include <stdint.h>

#if defined(WIN32)
    #include <WinSock2.h>
    #include <Windows.h>
    #include <direct.h>
    #include <io.h> //C (Windows)    access
#else
    #include <sys/time.h>
    #include <stdio.h>
    #include <unistd.h>
#endif

#if defined(WIN32)
#else
    void Sleep(long mSeconds);
#endif

void mSleep(int mSecond);
namespace Util
{
    void mSleep(int mSecond);

    uint64_t GetUtcTime(); //获取UTC时间戳（毫秒）
    uint64_t GetLocalTime(); //获取本地时间戳(毫秒)
    uint64_t GetTodayUtcTime(); //获取今天0点的UTC时间戳(毫秒)
    uint64_t GetUtcTimeInCurWeek(); //获取当前时间相对于本周的UTC时间戳(毫秒)

    std::string getLocalTimeInStringFormat();
};
