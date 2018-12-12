/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */


#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QPaintEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mPlayer = new VideoPlayer;
    connect(mPlayer,SIGNAL(sig_GetOneFrame(QImage)),this,SLOT(slotGetOneFrame(QImage)));

    mPlayer->setFileName("E:\\in.mp4");
    mPlayer->startPlay();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, this->width(), this->height()); //先画成黑色

    if (mImage.size().width() <= 0) return;

    ///将图像按比例缩放成和窗口一样大小
    QImage img = mImage.scaled(this->size(),Qt::KeepAspectRatio);

    int x = this->width() - img.width();
    int y = this->height() - img.height();

    x /= 2;
    y /= 2;

    painter.drawImage(QPoint(x,y),img); //画出图像

}

void MainWindow::slotGetOneFrame(QImage img)
{
    mImage = img;
    update(); //调用update将执行 paintEvent函数
}
