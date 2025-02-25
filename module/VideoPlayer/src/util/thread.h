#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace Util {

class Thread
{
public:
    Thread();
    Thread(bool single_mode);
    virtual ~Thread();

    enum State
    {
        Stoped,     ///<停止状态，包括从未启动过和启动后被停止
        Running,    ///<运行状态
        Paused      ///<暂停状态
    };

    State state() const;

    void start();
    void stop();
    void pause();
    void resume();

    void setSingleMode(const bool &single_mode); //设置线程只执行一次

    //每执行完一次run函数，休眠多久(毫秒)，
    //<0表示run函数会一直触发，=0表示run函数触发一次就会进入休眠
    void setSleepTime(const int time){m_sleep_time = time;}

    //设置线程执行函数
    //可以重载run也可以传入这个run_func，二者选一
    void setThreadFunc(std::function<void()> run_func){m_run_func = run_func;}

    bool stopFlag(){return m_is_stop;}
protected:
    virtual void run();

private:
    void threadFunc();

protected:
    std::recursive_mutex m_mutex_thread;
    std::thread* m_thread = nullptr;

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic_bool m_is_pause;   ///<暂停标识
    std::atomic_bool m_is_stop;   ///<停止标识
    State m_state;

    //每执行完一次run函数，休眠多久(毫秒)，
    //<0表示run函数会一直触发，=0表示run函数触发一次就会进入休眠
    int m_sleep_time = 0; 

    std::function<void()> m_run_func = nullptr; //可以重载上面的run也可以传入这个run_func，二者选一

};

}

#endif // THREAD_H
