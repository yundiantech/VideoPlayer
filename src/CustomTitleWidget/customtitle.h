#ifndef CUSTOMTITLE_H
#define CUSTOMTITLE_H

#include <QDialog>

namespace Ui {
class CustomTitle;
}

class CustomTitle : public QDialog
{
    Q_OBJECT

public:
    explicit CustomTitle(QWidget *parent = 0);
    ~CustomTitle();

    QWidget *getContentWidget();
    QDialog *getContentDialog();

    void setTitle(QString str);

    void showMaximized();

    void changeMax();

    virtual void doClose();

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *e);

private slots:
    void on_btnMenu_Close_clicked();

    void on_btnMenu_Max_clicked();

    void on_btnMenu_Min_clicked();

private:
    Ui::CustomTitle *ui;

    QPoint mousePoint;
    bool mousePressed;
    bool max;
    QRect location;

    QDialog *dialog;

    void InitStyle();

};

#endif // CUSTOMTITLE_H
