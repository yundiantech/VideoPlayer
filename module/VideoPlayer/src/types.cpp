#include "types.h"

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


int64_t getTimeStamp_MilliSecond()
{

    int mSecond = 0; //当前毫秒数

#if defined(WIN32)

    SYSTEMTIME sys;
    GetLocalTime( &sys );

    mSecond = sys.wMilliseconds;

#else

    struct timeval    tv;
    struct timezone tz;

    struct tm         *p;

    gettimeofday(&tv, &tz);
    p = localtime(&tv.tv_sec);

    mSecond = tv.tv_usec / 1000;


#endif

    int64_t timeStamp = ((int64_t)time(NULL)) * 1000 + mSecond;

    return timeStamp;
}
