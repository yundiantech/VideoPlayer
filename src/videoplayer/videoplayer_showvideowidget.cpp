/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "videoplayer_showvideowidget.h"
#include "ui_videoplayer_showvideowidget.h"

#include <QPainter>

VideoPlayer_ShowVideoWidget::VideoPlayer_ShowVideoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoPlayer_ShowVideoWidget)
{
    ui->setupUi(this);
}

VideoPlayer_ShowVideoWidget::~VideoPlayer_ShowVideoWidget()
{
    delete ui;
}

void VideoPlayer_ShowVideoWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0,0,this->width(),this->height()); //先画成黑色


    if (mImage.size().width() <= 0) return;

    ///将图像按比例缩放成和窗口一样大小
    QImage img = mImage.scaled(this->size(),Qt::KeepAspectRatio);

    int x = this->width() - img.width();
    int y = this->height() - img.height();

    x /= 2;
    y /= 2;

    painter.drawImage(QPoint(x,y),img); //画出图像

}

void VideoPlayer_ShowVideoWidget::slotGetOneFrame(QImage img)
{
    mImage = img;
    update(); //调用update将执行 paintEvent函数
}
