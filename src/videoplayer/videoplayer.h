/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <customtitle.h>

#include <QImage>
#include <QPaintEvent>
#include <QTimer>
#include <QPushButton>

#include "videoplayer/videoplayer_thread.h"

namespace Ui {
class VideoPlayer;
}

///这个是播放器的主界面 包括那些按钮和进度条之类的

class VideoPlayer : public CustomTitle
{
    Q_OBJECT

public:
    explicit VideoPlayer(QWidget *parent = 0);
    ~VideoPlayer();

protected:
    void doClose();
//    void mousePressEvent(QMouseEvent *e);
//    void mouseDoubleClickEvent(QMouseEvent *);

private:
    Ui::VideoPlayer *ui;

    VideoPlayer_Thread *mPlayer; //播放线程
    QTimer *mTimer; //定时器-获取当前视频时间

private slots:
    ///播放器相关的槽函数
    void slotTotalTimeChanged(qint64 uSec);
    void slotSliderMoved(int value);
    void slotTimerTimeOut();
    void slotBtnClick();

    void slotStateChanged(VideoPlayer_Thread::PlayerState state);

};

#endif // VIDEOPLAYER_H
