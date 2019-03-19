/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef SHOWVIDEOWIDGET_H
#define SHOWVIDEOWIDGET_H

#include <QWidget>
#include <QPaintEvent>

namespace Ui {
class ShowVideoWidget;
}

///显示视频用的widget
///这个仅仅是显示视频画面的控件

class ShowVideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ShowVideoWidget(QWidget *parent = 0);
    ~ShowVideoWidget();

    void inputOneFrame(const QImage &img);
    void clear();

protected:
    void paintEvent(QPaintEvent *event);

    QImage mImage; //记录当前的图像

private:
    Ui::ShowVideoWidget *ui;

};

#endif // SHOWVIDEOWIDGET_H
