#include "FunctionTransfer.h"

#include <QThread>
#include <QDebug>

Qt::HANDLE FunctionTransfer::gMainThreadId = nullptr;
FunctionTransfer *FunctionTransfer::main_thread_forward = nullptr;

Q_DECLARE_METATYPE(std::function<void()>)

FunctionTransfer::FunctionTransfer(QObject *parent) :
    QObject(parent)
{
    //因为std::function<void()>是自定义的类型 要跨线程传递需要先注册一下
    qRegisterMetaType<std::function<void()>>();

    mOnlyRunOneceFunc = nullptr;

    mTimer = new QTimer;
    connect(mTimer, &QTimer::timeout, this, &FunctionTransfer::slotTimerTimeOut);

    connect(this, SIGNAL(comming(std::function<void()>)), this, SLOT(slotExec(std::function<void()>)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(comming_noBlock(std::function<void()>)), this, SLOT(slotExec(std::function<void()>)), Qt::QueuedConnection);
}

FunctionTransfer::~FunctionTransfer()
{

}

void FunctionTransfer::init()
{
    init(QThread::currentThreadId());
}

void FunctionTransfer::init(Qt::HANDLE id)
{
    gMainThreadId = id;
    FunctionTransfer::main_thread_forward = new FunctionTransfer();
}

bool FunctionTransfer::isMainThread()
{
    if (gMainThreadId == nullptr)
    {
        qDebug()<<__FILE__<<__LINE__<<__FUNCTION__<<"the main thread id is not set!";
        return false;
    }

    if (QThread::currentThreadId() == gMainThreadId)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void FunctionTransfer::runInMainThread(std::function<void()> f, bool isBlock)
{
    runInMainThread(FunctionTransfer::main_thread_forward, f, isBlock);
}

void FunctionTransfer::runInMainThread(FunctionTransfer *pointer, std::function<void()> f, bool isBlock)
{
//    FunctionTransfer::main_thread_forward->exec(f, isBlock);
    if(FunctionTransfer::isMainThread())
    {
        f();
    }
    else
    {
        FunctionTransfer *p = pointer;

        if (p == nullptr)
        {
            p = FunctionTransfer::main_thread_forward;
        }

        if (isBlock)
        {
            Q_EMIT pointer->comming(f);
        }
        else
        {
            Q_EMIT pointer->comming_noBlock(f);
        }
    }
}

void FunctionTransfer::runOnece(std::function<void()> f, const int &time)
{
    runOnece(FunctionTransfer::main_thread_forward, f, time);
}

void FunctionTransfer::runOnece(FunctionTransfer *pointer, std::function<void()> f, const int &time)
{
    if (time < 0) return;

    FunctionTransfer::runInMainThread(pointer, [=]
    {
        ///检测升级
        FunctionTransfer *p = pointer;
        if (p == nullptr)
        {
            p = FunctionTransfer::main_thread_forward;
        }

        p->mOnlyRunOneceFunc = f;
        p->mTimer->stop();
        p->mTimer->start(time);
    });
}

void FunctionTransfer::slotExec(std::function<void()> f)
{
    f();
}

void FunctionTransfer::slotTimerTimeOut()
{
    if (QObject::sender() == mTimer)
    {
        mTimer->stop();
        mOnlyRunOneceFunc();
    }
}
