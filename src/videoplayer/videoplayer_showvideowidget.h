/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef VIDEOPLAYER_SHOWVIDEOWIDGET_H
#define VIDEOPLAYER_SHOWVIDEOWIDGET_H

#include <QWidget>
#include <QPaintEvent>

namespace Ui {
class VideoPlayer_ShowVideoWidget;
}


///显示视频用的widget
///这个仅仅是显示视频画面的控件

class VideoPlayer_ShowVideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoPlayer_ShowVideoWidget(QWidget *parent = 0);
    ~VideoPlayer_ShowVideoWidget();

protected:
    void paintEvent(QPaintEvent *event);

    QImage mImage; //记录当前的图像

private:
    Ui::VideoPlayer_ShowVideoWidget *ui;

public slots:
    ///播放器相关的槽函数
    void slotGetOneFrame(QImage img);

};

#endif // VIDEOPLAYER_SHOWVIDEOWIDGET_H
