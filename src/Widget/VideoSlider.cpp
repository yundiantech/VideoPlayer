/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "VideoSlider.h"

#include <QDebug>
#include <QTimer>
#include <QResizeEvent>
#include <QStyle>

VideoSlider::VideoSlider(QWidget *parent) :
    QSlider(parent)
{
    setMouseTracking(true);

    this->setOrientation(Qt::Horizontal);

    isSliderMoving = false;

    m_timer = new QTimer;
    m_timer->setInterval(100);
    connect(m_timer,SIGNAL(timeout()),this,SLOT(slotTimerTimeOut()));

    m_timer_mousemove = new QTimer;
    m_timer_mousemove->setInterval(100);
    connect(m_timer_mousemove,SIGNAL(timeout()),this,SLOT(slotMousemoveTimerTimeOut()));
}

VideoSlider::~VideoSlider()
{

}

void VideoSlider::resizeEvent(QResizeEvent *event)
{
//    qDebug()<<"void MySlider::resizeEvent(QResizeEvent *event)";
}

void VideoSlider::mousePressEvent(QMouseEvent *event)
{
//    isSliderMoving = true;

    int posX = event->pos().x();

    int w = this->width();

//    qint64 value = posX*1.0/w * this->maximum();

    int value = QStyle::sliderValueFromPosition(minimum(), maximum(), event->pos().x(), width());

    setValue(value);

//    qDebug()<<value<<maximum();

    emit sig_valueChanged(value);
//    emit valueChanged(value);
//    QSlider::mousePressEvent(event);
}

void VideoSlider::mouseMoveEvent(QMouseEvent *event)
{
//    qDebug()<<"void MySlider::mouseMoveEvent(QMouseEvent *event)"<<event->pos();

//    if (isSliderMoving)
//    {
//        m_timer->stop();
//        videoThread->hideIn();
//    }
//    else
//    {
//        m_posX = event->pos().x();

//        m_timer_mousemove->stop();
//        m_timer_mousemove->start();
//    }

    m_posX = event->pos().x();

    m_timer_mousemove->stop();
    m_timer_mousemove->start();

    QSlider::mouseMoveEvent(event);

}

void VideoSlider::mouseReleaseEvent(QMouseEvent *event)
{
//    emit sig_valueChanged(this->value());
//    isSliderMoving = false;
//    QSlider::mouseReleaseEvent(event);
}

void VideoSlider::enterEvent(QEvent *)
{
    m_timer->stop();
}

void VideoSlider::leaveEvent(QEvent *)
{
    m_timer_mousemove->stop();
    m_timer->start();
}

bool VideoSlider::seek()
{
    return true;
}

void VideoSlider::setValue(int value)
{
    if (!isSliderMoving)
    QSlider::setValue(value);
}

//void VideoSlider::setCutMode()
//{
//    videoThread->setCutMode();
//    connect(videoThread,SIGNAL(sig_setStart(qint64)),this,SIGNAL(sig_setStart(qint64)));
//    connect(videoThread,SIGNAL(sig_setEnd(qint64)),this,SIGNAL(sig_setEnd(qint64)));
//}

bool VideoSlider::openFile(char*fileName)
{
//    videoThread->openFile(fileName);

    return true;
}

bool VideoSlider::closeFile()
{


    return 0;
}

//void VideoSliderView::slotTotalTimeChanged(qint64 uSec)
//{
//    setRange(0,uSec);
//}

void VideoSlider::slotTimerTimeOut()
{
//    qDebug()<<videoThread->getWidget()->hasFocus();
//    qDebug()<<videoThread->getWidget()->geometry()<<QCursor::pos()<<videoThread->getWidget()->geometry().contains(QCursor::pos());
//    if (!videoThread->getWidget()->geometry().contains(QCursor::pos()))
//    {
//        m_timer->stop();
////        videoThread->hideIn();
//    }
}

void VideoSlider::slotMousemoveTimerTimeOut()
{

    m_timer_mousemove->stop();

    int w = this->width();

//    qint64 value = m_posX*1.0/w*videoThread->getTotalTime();

//    this->setValue(value);

    QPoint point = this->mapToGlobal(QPoint(0,0));
    int width = this->width();

    if (parent() != NULL)
    {
        QWidget *widget = (QWidget *)this->parent();
        point = widget->mapToGlobal(QPoint(0,0));

        width = widget->width();
    }

//    videoThread->showOut(point,width);

////    qDebug()<<value<<videoThread->getTotalTime();

//    videoThread->seek(value);
}
