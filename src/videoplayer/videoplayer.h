/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>

#include <QImage>
#include <QPaintEvent>
#include <QTimer>
#include <QPushButton>

#include "videoplayer/videoplayer_thread.h"

//鼠标实现改变窗口大小
#define PADDING 6
enum Direction { UP=0, DOWN, LEFT, RIGHT, LEFTTOP, LEFTBOTTOM, RIGHTBOTTOM, RIGHTTOP, NONE };


namespace Ui {
class VideoPlayer;
}

///这个是播放器的主界面 包括那些按钮和进度条之类的

class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayer(QWidget *parent = 0);
    ~VideoPlayer();

    void setTitle(QString str);

protected:
    void doClose();

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


///以下是改变窗体大小相关
    ////////
protected:
//    bool eventFilter(QObject *obj, QEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    bool isMax; //是否最大化
    QRect mLocation;

    bool isLeftPressDown;  // 判断左键是否按下
    QPoint dragPosition;   // 窗口移动拖动时需要记住的点
    int dir;        // 窗口大小改变时，记录改变方向

    void checkCursorDirect(const QPoint &cursorGlobalPoint);

    void doShowFullScreen();
    void doShowNormal();

    void showBorderRadius(bool isShow);
    void doChangeFullScreen();

private slots:
    void on_btnMenu_Close_clicked();
    void on_btnMenu_Max_clicked();
    void on_btnMenu_Min_clicked();

};

#endif // VIDEOPLAYER_H
