/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QPaintEvent>
#include <QTimer>

#include "videoplayer/videoplayer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event);

private:
    Ui::MainWindow *ui;

    VideoPlayer *mPlayer; //播放线程

    QImage mImage; //记录当前的图像

    QTimer *mTimer; //定时器-获取当前视频时间

private slots:
    void slotGetOneFrame(QImage img);

    void slotTotalTimeChanged(qint64 uSec);

    void slotSliderMoved(int value);

    void slotTimerTimeOut();

    void slotBtnClick();
};

#endif // MAINWINDOW_H
