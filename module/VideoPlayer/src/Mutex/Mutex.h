#ifndef MUTEX_H
#define MUTEX_H


#if defined(WIN32)
    #include <WinSock2.h>
    #include <Windows.h>
//#elif defined(Q_OS_LINUX)
#else
    #include <pthread.h>
#endif

class Mutex
{
public:
    Mutex();
    ~Mutex();

    //确保拥有互斥对象的线程对被保护资源的独自访问
    int Lock() const;

    //释放当前线程拥有的互斥对象，以使其它线程可以拥有互斥对象，对被保护资源进行访问
    int Unlock() const;

private:

#if defined(WIN32)
     HANDLE m_mutex;
#else
    pthread_mutex_t mutex;
#endif

};

#endif // MUTEX_H
