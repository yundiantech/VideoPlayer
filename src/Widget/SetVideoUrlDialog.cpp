#include "SetVideoUrlDialog.h"
#include "ui_SetVideoUrlDialog.h"

#include <QFileDialog>

#include "AppConfig.h"

SetVideoUrlDialog::SetVideoUrlDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetVideoUrlDialog)
{
    ui->setupUi(this);

    connect(ui->pushButton_selectFile,   &QPushButton::clicked, this, &SetVideoUrlDialog::slotBtnClick);

}

SetVideoUrlDialog::~SetVideoUrlDialog()
{
    delete ui;
}

void SetVideoUrlDialog::setVideoUrl(const QString &url)
{
    ui->lineEdit_fileUrl->setText(url);
}

QString SetVideoUrlDialog::getVideoUrl()
{
    QString url = ui->lineEdit_fileUrl->text();
    return url;
}

void SetVideoUrlDialog::slotBtnClick(bool isChecked)
{
    if (QObject::sender() == ui->pushButton_selectFile)
    {
        QString s = QFileDialog::getOpenFileName(
                   this, QStringLiteral("选择要播放的文件"),
                    AppConfig::gVideoFilePath,//初始目录
                    QStringLiteral("视频文件 (*.flv *.rmvb *.avi *.MP4 *.mkv);;")
                    +QStringLiteral("音频文件 (*.mp3 *.wma *.wav);;")
                    +QStringLiteral("所有文件 (*.*)"));
        if (!s.isEmpty())
        {
            ui->lineEdit_fileUrl->setText(s);
        }
    }
}
