#ifndef COND_H
#define COND_H

/// 注意Mingw的话使用的是linux下的api pthread
/// 没有_MSC_VER这个宏 我们就认为他用的是mingw编译器

#ifndef _MSC_VER
#define MINGW
#endif

#if defined(WIN32) && !defined(MINGW)
    #include <WinSock2.h>
    #include <Windows.h>
#else
    #include <pthread.h>
    #include <time.h>
#endif

class Cond
{
public:
    Cond();
    ~Cond();

    //上锁
    int Lock();

    //解锁
    int Unlock();

    //
    int Wait();

    //固定时间等待
    int TimedWait(int second);

    //
    int Signal();

    //唤醒所有睡眠线程
    int Broadcast();

private:

#if defined(WIN32) && !defined(MINGW)
    CRITICAL_SECTION m_mutex;
    RTL_CONDITION_VARIABLE m_cond;
#else
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
#endif

};

#endif // MUTEX_H
