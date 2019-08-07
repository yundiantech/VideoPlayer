#include "FunctionTransfer.h"

#include <QThread>
#include <QDebug>

Qt::HANDLE FunctionTransfer::gMainThreadId = nullptr;
FunctionTransfer *FunctionTransfer::main_thread_forward = nullptr;

FunctionTransfer::FunctionTransfer(QObject *parent) :
    QObject(parent)
{
    connect(this, SIGNAL(comming(std::function<void()>)), this, SLOT(exec(std::function<void()>)), Qt::BlockingQueuedConnection);
}

FunctionTransfer::~FunctionTransfer()
{

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

void FunctionTransfer::runInMainThread(std::function<void()> f)
{
    FunctionTransfer::main_thread_forward->exec(f);
}

void FunctionTransfer::exec(std::function<void()> f)
{
    if(FunctionTransfer::isMainThread())
    {
        f();
    }
    else
    {
        Q_EMIT this->comming(f);
    }

}
