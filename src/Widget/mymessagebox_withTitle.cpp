#include "mymessagebox_withTitle.h"
#include "ui_mymessagebox_withTitle.h"

MyMessageBox_WithTitle::MyMessageBox_WithTitle(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyMessageBox_WithTitle)
{
    ui->setupUi(this);

    setWindowFlags(Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);  //使窗口的标题栏隐藏
    setAttribute(Qt::WA_TranslucentBackground, true);

}

MyMessageBox_WithTitle::~MyMessageBox_WithTitle()
{
    delete ui;
}

void MyMessageBox_WithTitle::setText(QString title,QString text,QString acceptBtnText,QString rejectBtnText)
{
    ui->lab_Title->setText(title);
    ui->lab_info->setText(text);

    if (acceptBtnText != NULL)
    {
        ui->pushButton_accept->setText(acceptBtnText);
    }
    else
    {
        ui->pushButton_accept->hide();
    }

    if (rejectBtnText == NULL)
    {
        ui->pushButto_reject->hide();
    }
    else
    {
        ui->pushButto_reject->setText(rejectBtnText);
    }
}

int MyMessageBox_WithTitle::showText(QString title,QString text,QString acceptBtnText,QString rejectBtnText)
{
    MyMessageBox_WithTitle dialog;
    dialog.setText(title,text,acceptBtnText,rejectBtnText);
    dialog.setMode(2);
    return dialog.exec();
}

int MyMessageBox_WithTitle::showSucceesText(QString title,QString text,QString acceptBtnText,QString rejectBtnText)
{
    MyMessageBox_WithTitle dialog;
    dialog.setText(title,text,acceptBtnText,rejectBtnText);
    dialog.setMode(1);
    return dialog.exec();
}

int MyMessageBox_WithTitle::showErroText(QString title,QString text,QString acceptBtnText,QString rejectBtnText)
{
    MyMessageBox_WithTitle dialog;
    dialog.setText(title,text,acceptBtnText,rejectBtnText);
    dialog.setMode(2);
    return dialog.exec();
}

int MyMessageBox_WithTitle::showWarningText(QString title,QString text,QString acceptBtnText,QString rejectBtnText)
{
    MyMessageBox_WithTitle dialog;
    dialog.setText(title,text,acceptBtnText,rejectBtnText);
    dialog.setMode(3);
    return dialog.exec();
}

void MyMessageBox_WithTitle::setMode(int mode)
{
    if (mode == 1)
    {
        ui->lab_ico_succees->show();
        ui->lab_ico_erro->hide();
        ui->lab_ico_warnning->hide();
    }
    else if (mode == 2)
    {
        ui->lab_ico_succees->hide();
        ui->lab_ico_erro->show();
        ui->lab_ico_warnning->hide();
    }
    else
    {
        ui->lab_ico_succees->hide();
        ui->lab_ico_erro->hide();
        ui->lab_ico_warnning->show();
    }
}
