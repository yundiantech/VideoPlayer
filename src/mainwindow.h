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

private slots:
    void slotGetOneFrame(QImage img);

};

#endif // MAINWINDOW_H
