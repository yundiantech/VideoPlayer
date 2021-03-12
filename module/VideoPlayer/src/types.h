#ifndef TYPES_H
#define TYPES_H

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

enum VideoPlayerState
{
    VideoPlayer_Playing,
    VideoPlayer_Pause,
    VideoPlayer_Stop
};

#if defined(WIN32)
#else
    void Sleep(long mSeconds);
#endif

void mSleep(int mSecond);

int64_t getTimeStamp_MilliSecond(); //获取时间戳（毫秒）

#endif // TYPES_H
