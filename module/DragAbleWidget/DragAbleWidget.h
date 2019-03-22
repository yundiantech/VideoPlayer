/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef DRAGABLEWIDGET_H
#define DRAGABLEWIDGET_H

#include <QWidget>
#include <QTimer>

namespace Ui {
class DragAbleWidget;
}

//鼠标实现改变窗口大小
#define PADDING 6
enum Direction { UP=0, DOWN, LEFT, RIGHT, LEFTTOP, LEFTBOTTOM, RIGHTBOTTOM, RIGHTTOP, NONE };


class DragAbleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DragAbleWidget(QWidget *parent = 0);
    ~DragAbleWidget();

    QWidget *getContainWidget();

    void setTitle(QString str);

private:
    Ui::DragAbleWidget *ui;

    QTimer *mTimer;

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
    void slotTimerTimeOut();

    void on_btnMenu_Close_clicked();
    void on_btnMenu_Max_clicked();
    void on_btnMenu_Min_clicked();

};

#endif // DRAGABLEWIDGET_H
