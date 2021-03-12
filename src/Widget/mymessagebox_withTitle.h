#ifndef MYMESSAGEBOX_WITHTITLE_H
#define MYMESSAGEBOX_WITHTITLE_H

#include <QDialog>

namespace Ui {
class MyMessageBox_WithTitle;
}

class MyMessageBox_WithTitle : public QDialog
{
    Q_OBJECT

public:
    explicit MyMessageBox_WithTitle(QWidget *parent = 0);
    ~MyMessageBox_WithTitle();

    static int showSucceesText(QString title,QString text,QString acceptBtnText = NULL,QString rejectBtnText = NULL);
    static int showErroText(QString title,QString text,QString acceptBtnText = NULL,QString rejectBtnText = NULL);
    static int showWarningText(QString title,QString text,QString acceptBtnText = NULL,QString rejectBtnText = NULL);

    static int showText(QString title,QString text,QString acceptBtnText = NULL,QString rejectBtnText = NULL);

    void setMode(int mode); //1-succees 2-erro 3-danger

private:
    Ui::MyMessageBox_WithTitle *ui;

    void setText(QString title,QString text,QString acceptBtnText = NULL,QString rejectBtnText = NULL);

};

#endif // MYMESSAGEBOX_WITHTITLE_H
