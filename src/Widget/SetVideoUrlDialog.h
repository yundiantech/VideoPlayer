#ifndef SetVideoUrlDialog_H
#define SetVideoUrlDialog_H

#include <QDialog>

namespace Ui {
class SetVideoUrlDialog;
}

class SetVideoUrlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetVideoUrlDialog(QWidget *parent = 0);
    ~SetVideoUrlDialog();

    void setVideoUrl(const QString &url);
    QString getVideoUrl();

private:
    Ui::SetVideoUrlDialog *ui;

private slots:
    void slotBtnClick(bool isChecked);

};

#endif // SetVideoUrlDialog_H
