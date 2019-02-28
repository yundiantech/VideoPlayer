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

#define MARGINS 2 //窗体边框

Q_DECLARE_METATYPE(VideoPlayer_Thread::PlayerState)

VideoPlayer::VideoPlayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoPlayer)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);//|Qt::WindowStaysOnTopHint);  //使窗口的标题栏隐藏
    setAttribute(Qt::WA_TranslucentBackground);

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

    mPlayer->setVideoWidget(ui->widget_videoPlayer);

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

    connect(ui->horizontalSlider_volume,SIGNAL(valueChanged(int)),this,SLOT(slotSliderMoved(int)));

    resize(1024,768);

    setTitle(QStringLiteral("我的播放器"));

    ui->widget_video->hide();

    ui->pushButton_pause->hide();




///改变窗体大小相关
    isMax = false;

    int w = this->width();
    int h = this->height();

    QRect screenRect = QApplication::desktop()->screenGeometry();//获取设备屏幕大小
    int x = (screenRect.width() - w) / 2;
    int y = (screenRect.height() - h) / 2;

    mLocation = QRect(x, y, w, h);
    this->setGeometry(mLocation);

    isLeftPressDown = false;
    this->dir = NONE;
    this->setMouseTracking(true);// 追踪鼠标
    ui->widget_frame->setMouseTracking(true);
    ui->widget_back->setMouseTracking(true);
    ui->widget_container->setMouseTracking(true);
//    ui->widget_center->setMouseTracking(true);

    this->setFocusPolicy(Qt::ClickFocus);

    ui->widget_frame->setContentsMargins(MARGINS,MARGINS,MARGINS,MARGINS);
    showBorderRadius(true);

//    ui->widget_frame->setContentsMargins(1, 1, 1, 1);

    //安装事件监听器,让标题栏识别鼠标双击
//    ui->widget_beingClass_back->installEventFilter(this);



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

void VideoPlayer::setTitle(QString str)
{
    ui->label_titleName->setText(str);
    this->setWindowTitle(str);
}

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
    else if (QObject::sender() == ui->horizontalSlider_volume)
    {
        mPlayer->setVolume(value / 100.0);
        ui->label_volume->setText(QString("%1").arg(value));
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




////////////改变窗体大小相关

void VideoPlayer::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        isLeftPressDown = false;
        if(dir != NONE) {
            this->releaseMouse();
            this->setCursor(QCursor(Qt::ArrowCursor));
        }
    }
}

void VideoPlayer::mousePressEvent(QMouseEvent *event)
{
//    qDebug()<<__FUNCTION__;
    if (event->type() == QEvent::MouseButtonDblClick)
    {
        if (event->button() == Qt::LeftButton)
        {
//            if(QApplication::keyboardModifiers() == (Qt::ControlModifier|Qt::ShiftModifier|Qt::AltModifier))
            {
                doChangeFullScreen(); //ctrl + 左键
            }
        }
    }

    switch(event->button()) {
    case Qt::LeftButton:
        if (isMax || this->isFullScreen()) break;
        isLeftPressDown = true;
        checkCursorDirect(event->globalPos());

        if(dir != NONE) {
            this->mouseGrabber();
        } else {
            dragPosition = event->globalPos() - this->frameGeometry().topLeft();
        }
        break;
//    case Qt::RightButton:
//        if (!this->isFullScreen())
//            mAction_FullScreen->setText(tr("show fullscreen"));
//        else
//            mAction_FullScreen->setText(tr("quit fullscreen"));
//        mPopMenu->exec(QCursor::pos());
//        break;
    default:
        QWidget::mousePressEvent(event);
    }

}

void VideoPlayer::mouseMoveEvent(QMouseEvent *event)
{
//    qDebug()<<__FUNCTION__;

    if (isMax || this->isFullScreen()) return;

    QPoint gloPoint = event->globalPos();
    QRect rect = this->rect();
    QPoint tl = mapToGlobal(rect.topLeft());
    QPoint rb = mapToGlobal(rect.bottomRight());

    if(!isLeftPressDown) {
        checkCursorDirect(gloPoint);
    } else {

        if(dir != NONE) {
            QRect rMove(tl, rb);

            switch(dir) {
            case LEFT:
                if(rb.x() - gloPoint.x() <= this->minimumWidth())
                    rMove.setX(tl.x());
                else
                    rMove.setX(gloPoint.x());
                break;
            case RIGHT:
                rMove.setWidth(gloPoint.x() - tl.x());
                break;
            case UP:
                if(rb.y() - gloPoint.y() <= this->minimumHeight())
                    rMove.setY(tl.y());
                else
                    rMove.setY(gloPoint.y());
                break;
            case DOWN:
                rMove.setHeight(gloPoint.y() - tl.y());
                break;
            case LEFTTOP:
                if(rb.x() - gloPoint.x() <= this->minimumWidth())
                    rMove.setX(tl.x());
                else
                    rMove.setX(gloPoint.x());
                if(rb.y() - gloPoint.y() <= this->minimumHeight())
                    rMove.setY(tl.y());
                else
                    rMove.setY(gloPoint.y());
                break;
            case RIGHTTOP:
                rMove.setWidth(gloPoint.x() - tl.x());
                rMove.setY(gloPoint.y());
                break;
            case LEFTBOTTOM:
                rMove.setX(gloPoint.x());
                rMove.setHeight(gloPoint.y() - tl.y());
                break;
            case RIGHTBOTTOM:
                rMove.setWidth(gloPoint.x() - tl.x());
                rMove.setHeight(gloPoint.y() - tl.y());
                break;
            default:
                break;
            }
            this->setGeometry(rMove);
//            emit sig_WindowMoved(rMove);
        } else {
            checkCursorDirect(event->globalPos());

            if (dir == NONE && !isMax)
            {
                QPoint point = event->globalPos() - dragPosition;

                QRect mLimitRect = QApplication::desktop()->availableGeometry();

                if (point.x() < mLimitRect.x())
                    point.setX(mLimitRect.x());

                if (point.x() > (mLimitRect.x()+mLimitRect.width()-this->width()))
                    point.setX(mLimitRect.x()+mLimitRect.width()-this->width());


                if (point.y() < mLimitRect.y())
                    point.setY(mLimitRect.y());

                if (point.y() > (mLimitRect.y()+mLimitRect.height()-this->height()))
                    point.setY(mLimitRect.y()+mLimitRect.height()-this->height());

                move(point);
            }

            event->accept();
        }
    }
//    QWidget::mouseMoveEvent(event);、
    event->accept();
}

void VideoPlayer::checkCursorDirect(const QPoint &cursorGlobalPoint)
{
    // 获取窗体在屏幕上的位置区域，tl为topleft点，rb为rightbottom点
    QRect rect = this->rect();
    QPoint tl = mapToGlobal(rect.topLeft());
    QPoint rb = mapToGlobal(rect.bottomRight());

    int x = cursorGlobalPoint.x();
    int y = cursorGlobalPoint.y();

    if(tl.x() + PADDING >= x && tl.x() <= x && tl.y() + PADDING >= y && tl.y() <= y) {
        // 左上角
        dir = LEFTTOP;
        this->setCursor(QCursor(Qt::SizeFDiagCursor));  // 设置鼠标形状
    } else if(x >= rb.x() - PADDING && x <= rb.x() && y >= rb.y() - PADDING && y <= rb.y()) {
        // 右下角
        dir = RIGHTBOTTOM;
        this->setCursor(QCursor(Qt::SizeFDiagCursor));
    } else if(x <= tl.x() + PADDING && x >= tl.x() && y >= rb.y() - PADDING && y <= rb.y()) {
        //左下角
        dir = LEFTBOTTOM;
        this->setCursor(QCursor(Qt::SizeBDiagCursor));
    } else if(x <= rb.x() && x >= rb.x() - PADDING && y >= tl.y() && y <= tl.y() + PADDING) {
        // 右上角
        dir = RIGHTTOP;
        this->setCursor(QCursor(Qt::SizeBDiagCursor));
    } else if(x <= tl.x() + PADDING && x >= tl.x()) {
        // 左边
        dir = LEFT;
        this->setCursor(QCursor(Qt::SizeHorCursor));
    } else if( x <= rb.x() && x >= rb.x() - PADDING) {
        // 右边
        dir = RIGHT;
        this->setCursor(QCursor(Qt::SizeHorCursor));
    }else if(y >= tl.y() && y <= tl.y() + PADDING){
        // 上边
        dir = UP;
        this->setCursor(QCursor(Qt::SizeVerCursor));
    } else if(y <= rb.y() && y >= rb.y() - PADDING) {
        // 下边
        dir = DOWN;
        this->setCursor(QCursor(Qt::SizeVerCursor));
    }else {
        // 默认
        dir = NONE;
        this->setCursor(QCursor(Qt::ArrowCursor));
    }
}


void VideoPlayer::on_btnMenu_Close_clicked()
{
    qDebug()<<__FUNCTION__;

    close();

//    QTimer::singleShot(500,this,[&]()
//    {
//        QPoint pt(0, 0);
//        QMouseEvent evt(QEvent::Leave, pt, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
//        qApp->sendEvent(ui->btnMenu_Close, &evt);
//    });

}

void VideoPlayer::on_btnMenu_Max_clicked()
{
    doChangeFullScreen();
}

void VideoPlayer::on_btnMenu_Min_clicked()
{
    if (this->isFullScreen())
    {
//        mAnimation->stop();
//        mTimer_CheckTitle->stop();
        ui->widget_title->move(ui->widget_title->x(), 0 - ui->widget_title->height());
        ui->widget_title->hide();
    }
    this->showMinimized();

    QTimer::singleShot(500,this,[&]()
    {
        QPoint pt(0, 0);
        QMouseEvent evt(QEvent::Leave, pt, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        qApp->sendEvent(ui->btnMenu_Min, &evt);
    });
}

void VideoPlayer::doShowFullScreen()
{
    this->show();
    this->showFullScreen();
    this->raise();
    ui->widget_frame->setContentsMargins(0,0,0,0); //隐藏边框

    showBorderRadius(false);

    ui->btnMenu_Max->setIcon(QIcon(":/image/shownormalbtn.png"));

    ui->widget_title->hide(); //隐藏标题栏
//    ui->verticalLayout_titleWidget_Back->removeWidget(ui->widget_title);
//    ui->widget_title->setParent(NULL);
//    ui->widget_title->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::Tool|Qt::X11BypassWindowManagerHint);
//    ui->widget_title->resize(QApplication::desktop()->screen()->width(), ui->widget_title->height());
//    ui->widget_title->move(0,0);
//    ui->widget_title->show();

}

void VideoPlayer::doShowNormal()
{
    qDebug()<<__FUNCTION__;

    this->show();
    this->showNormal();
    this->raise();

    if (!isMax)
    {
        ui->widget_frame->setContentsMargins(MARGINS,MARGINS,MARGINS,MARGINS);
        showBorderRadius(true);
    } else {
        ui->widget_frame->setContentsMargins(0,0,0,0);
        showBorderRadius(false);
    }


    ui->btnMenu_Max->setIcon(QIcon(":/image/showmaxsizebtn.png"));

    QTimer::singleShot(20,this,[&]()
    {
        ui->verticalLayout_titleWidget_Back->addWidget(ui->widget_title);
        ui->widget_title->show();
    });

}


void VideoPlayer::showBorderRadius(bool isShow)
{
    QString str;

    if (isShow)
    {
        str = QString("QWidget#widget_frame\
                        {\
                            border:3px solid  rgb(46, 165, 255);\
                            background-color: rgba(255, 255, 255, 0);\
                            border-radius:5px;\
                        }\
                        QWidget#widget_back\
                        {\
                        border-radius:3px;\
                        }\
                        QWidget#widget_title\
                        {\
                            border-top-right-radius:5px;\
                            border-top-left-radius:5px;\
                        }\
                        QWidget#widget_container\
                        {\
                            border-bottom-right-radius:5px;\
                            border-bottom-left-radius:5px;\
                        }\
                        QStackedWidget\
                        {\
                            border-bottom-right-radius:5px;\
                            border-bottom-left-radius:5px;\
                        }\
                        QWidget#page_courseList\
                        {\
                            border-bottom-right-radius:5px;\
                            border-bottom-left-radius:5px;\
                        }");
    }
    else
    {
        str = QString("QWidget#widget_frame\
                        {\
                            border:3px solid  rgb(46, 165, 255);\
                            background-color: rgba(255, 255, 255, 0);\
                            border-radius:0px;\
                        }\
                        QWidget#widget_back\
                        {\
                        border-radius:0px;\
                        }\
                        QWidget#widget_title\
                        {\
                            border-top-right-radius:0px;\
                            border-top-left-radius:0px;\
                        }\
                        QWidget#widget_container\
                        {\
                            border-bottom-right-radius:0px;\
                            border-bottom-left-radius:0px;\
                        }\
                        QStackedWidget\
                        {\
                            border-bottom-right-radius:0px;\
                            border-bottom-left-radius:0px;\
                        }\
                        QWidget#page_courseList\
                        {\
                            border-bottom-right-radius:0px;\
                            border-bottom-left-radius:0px;\
                        }");
    }

    ui->widget_frame->setStyleSheet(str);

}

void VideoPlayer::doChangeFullScreen()
{
    if (this->isFullScreen())
    {
        this->doShowNormal();
//        mAction_FullScreen->setText(tr("show fullscreen"));
    }
    else
    {
        this->doShowFullScreen();
//        mAction_FullScreen->setText(tr("quit fullscreen"));
    }
}
