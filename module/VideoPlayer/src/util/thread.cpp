#include "thread.h"
#include <iostream>
#include <stdio.h>

using namespace std;
namespace Util {
Thread::Thread()
    : m_thread(nullptr),
      m_is_pause(false),
      m_is_stop(false),
      m_state(Stoped)
{
    setSingleMode(false);
}

Thread::Thread(bool single_mode)
    : m_thread(nullptr),
      m_is_pause(false),
      m_is_stop(false),
      m_state(Stoped)
{
    setSingleMode(single_mode);
}

Thread::~Thread()
{
    stop();
}

Thread::State Thread::state() const
{
    return m_state;
}

void Thread::start()
{
    m_mutex_thread.lock();

    //线程已经退出了，需要销毁再重新new一个
    if (m_state == Stoped && m_thread != nullptr)
    {
        m_thread->join(); // wait for thread finished
        delete m_thread;
        m_thread = nullptr;
    }

    if (m_thread == nullptr)
    {
        m_is_pause = false;
        m_is_stop = false;
        m_state = Running;
        m_thread = new thread(&Thread::threadFunc, this);
    }
    else
    {
        resume();
    }
    m_mutex_thread.unlock();
}

void Thread::stop()
{
    m_mutex_thread.lock();
    if (m_thread != nullptr)
    {
        m_is_pause = false;
        m_is_stop = true;
        m_condition.notify_all();  // Notify one waiting thread, if there is one.
        m_thread->join(); // wait for thread finished
        delete m_thread;
        m_thread = nullptr;
        m_state = Stoped;
    }
    m_mutex_thread.unlock();
}

void Thread::pause()
{
    m_mutex_thread.lock();
    if (m_thread != nullptr)
    {
        m_is_pause = true;
        m_state = Paused;
    }
    m_mutex_thread.unlock();
}

void Thread::resume()
{
    m_mutex_thread.lock();
    if (m_thread != nullptr)
    {
        m_is_pause = false;
        m_condition.notify_all();
        m_state = Running;
    }
    m_mutex_thread.unlock();
}

void Thread::setSingleMode(const bool &single_mode)
{
    if (single_mode)
    {
        m_sleep_time = 0;
    }
    else if (m_sleep_time == 0)
    {
        m_sleep_time = -1;
    }

}

void Thread::run()
{
    if (m_run_func)
    {
        m_run_func();
    }
}

void Thread::threadFunc()
{
    // cout << "enter thread:" << this_thread::get_id() << endl;

    while (!m_is_stop)
    {
        m_state = Running;
        run();
        if (m_is_stop)
        {
            break;
        }
        else if (m_sleep_time > 0)
        {
            unique_lock<mutex> locker(m_mutex);
            m_state = Paused;
            if (m_condition.wait_for(locker, std::chrono::milliseconds(m_sleep_time)) == std::cv_status::timeout)
            {

            }
        }
        else if (m_is_pause)// || m_sleep_time == 0)
        {
            unique_lock<mutex> locker(m_mutex);
            m_state = Paused;
            do
            {
                m_condition.wait(locker); // Unlock _mutex and wait to be notified
            }while (m_is_pause);

            locker.unlock();
        }
        else if (m_sleep_time == 0)
        {
            break;
        }
    }
    m_is_pause = false;
    m_is_stop = false;
    m_state = Stoped;
    
    // cout << "exit thread:" << this_thread::get_id() << endl;
}
}