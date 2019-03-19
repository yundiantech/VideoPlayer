#ifndef COND_H
#define COND_H

#if defined(WIN32)
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <pthread.h>
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

#if defined(WIN32)
    CRITICAL_SECTION m_mutex;
    RTL_CONDITION_VARIABLE m_cond;
#else
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
#endif

};

#endif // MUTEX_H
