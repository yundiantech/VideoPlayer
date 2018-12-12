/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QThread>
#include <QImage>

class VideoPlayer : public QThread
{
    Q_OBJECT

public:
    explicit VideoPlayer();
    ~VideoPlayer();

    void setFileName(QString path){mFileName = path;}

    void startPlay();

signals:
    void sig_GetOneFrame(QImage); //没获取到一帧图像 就发送此信号

protected:
    void run();

private:
    QString mFileName;
};

#endif // VIDEOPLAYER_H
