/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "videoplayer.h"
#include "ui_videoplayer.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFileDialog>
#include <QDebug>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QMouseEvent>

Q_DECLARE_METATYPE(VideoPlayer_Thread::PlayerState);

VideoPlayer::VideoPlayer(QWidget *parent) :
    CustomTitle(parent),
    ui(new Ui::VideoPlayer)
{
    ui->setupUi(getContentDialog());

    av_register_all(); //初始化FFMPEG  调用了这个才能正常使用编码器和解码器
    avformat_network_init(); //支持打开网络文件

    if (SDL_Init(SDL_INIT_AUDIO)) {
        fprintf(stderr,"Could not initialize SDL - %s. \n", SDL_GetError());
        exit(1);
    }

    //因为VideoPlayer::PlayerState是自定义的类型 要跨线程传递需要先注册一下
    qRegisterMetaType<VideoPlayer_Thread::PlayerState>();


    mPlayer = new VideoPlayer_Thread;
    connect(mPlayer,SIGNAL(sig_TotalTimeChanged(qint64)),this,SLOT(slotTotalTimeChanged(qint64)));
    connect(mPlayer,SIGNAL(sig_StateChanged(VideoPlayer_Thread::PlayerState)),this,SLOT(slotStateChanged(VideoPlayer_Thread::PlayerState)));

    ui->verticalLayout_show_video->addWidget(mPlayer->getVideoWidget()); //将显示视频的控件加入
    mPlayer->getVideoWidget()->show();

    mTimer = new QTimer; //定时器-获取当前视频时间
    connect(mTimer,SIGNAL(timeout()),this,SLOT(slotTimerTimeOut()));
    mTimer->setInterval(500);

    connect(ui->pushButton_open,SIGNAL(clicked()),this,SLOT(slotBtnClick()));
    connect(ui->toolButton_open,SIGNAL(clicked()),this,SLOT(slotBtnClick()));
    connect(ui->pushButton_play,SIGNAL(clicked()),this,SLOT(slotBtnClick()));
    connect(ui->pushButton_pause,SIGNAL(clicked()),this,SLOT(slotBtnClick()));
    connect(ui->pushButton_stop,SIGNAL(clicked()),this,SLOT(slotBtnClick()));

//    connect(ui->horizontalSlider,SIGNAL(sliderMoved(int)),this,SLOT(slotSliderMoved(int)));
    connect(ui->horizontalSlider,SIGNAL(sig_valueChanged(int)),this,SLOT(slotSliderMoved(int)));

    resize(1024,768);
    setTitle("我的播放器");

    ui->widget_right->hide();
    ui->widget_video->hide();

    ui->pushButton_pause->hide();

}

VideoPlayer::~VideoPlayer()
{
    delete ui;
}

void VideoPlayer::doClose()
{
    mPlayer->stop(true);
    close();
}

//void VideoPlayer::mousePressEvent(QMouseEvent *e)
//{qDebug()<<"ccc";
//    if (e->button() == Qt::LeftButton)
//    {

//    }
//}

//void VideoPlayer::mouseDoubleClickEvent(QMouseEvent *e)
//{
//    qDebug()<<"ddd";
////    if (e->type() == Qt::mou)
//}

void VideoPlayer::slotTotalTimeChanged(qint64 uSec)
{
    qint64 Sec = uSec/1000000;

    ui->horizontalSlider->setRange(0,Sec);

//    QString hStr = QString("00%1").arg(Sec/3600);
    QString mStr = QString("00%1").arg(Sec/60);
    QString sStr = QString("00%1").arg(Sec%60);

    QString str = QString("%1:%2").arg(mStr.right(2)).arg(sStr.right(2));
    ui->label_totaltime->setText(str);

}

void VideoPlayer::slotSliderMoved(int value)
{
    if (QObject::sender() == ui->horizontalSlider)
    {
        mPlayer->seek((qint64)value * 1000000);
    }
}

void VideoPlayer::slotTimerTimeOut()
{
    if (QObject::sender() == mTimer)
    {

        qint64 Sec = mPlayer->getCurrentTime();

        ui->horizontalSlider->setValue(Sec);

    //    QString hStr = QString("00%1").arg(Sec/3600);
        QString mStr = QString("00%1").arg(Sec/60);
        QString sStr = QString("00%1").arg(Sec%60);

        QString str = QString("%1:%2").arg(mStr.right(2)).arg(sStr.right(2));
        ui->label_currenttime->setText(str);
    }
}

void VideoPlayer::slotBtnClick()
{
    if (QObject::sender() == ui->pushButton_play)
    {
        mPlayer->play();
    }
    else if (QObject::sender() == ui->pushButton_pause)
    {
        mPlayer->pause();
    }
    else if (QObject::sender() == ui->pushButton_stop)
    {
        mPlayer->stop(true);
    }
    else if (QObject::sender() == ui->pushButton_open || QObject::sender() == ui->toolButton_open)
    {
        QString s = QFileDialog::getOpenFileName(
                   this, "选择要播放的文件",
                    "/",//初始目录
                    "视频文件 (*.flv *.rmvb *.avi *.MP4);; 所有文件 (*.*);; ");
        if (!s.isEmpty())
        {
            s.replace("/","\\");

            mPlayer->stop(true); //如果在播放则先停止

            mPlayer->setFileName(s);

            mTimer->start();

        }
    }

}

void VideoPlayer::slotStateChanged(VideoPlayer_Thread::PlayerState state)
{

    if (state == VideoPlayer_Thread::Stop)
    {
        ui->widget_video->hide();
        ui->widget_showopen->show();
        ui->pushButton_pause->hide();
    }
    else if (state == VideoPlayer_Thread::Playing)
    {
        ui->widget_showopen->hide();
        ui->widget_video->show();

        ui->pushButton_play->hide();
        ui->pushButton_pause->show();
    }
    else if (state == VideoPlayer_Thread::Pause)
    {
        ui->pushButton_pause->hide();
        ui->pushButton_play->show();
    }
}

