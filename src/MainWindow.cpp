/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFileDialog>
#include <QDebug>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QMessageBox>

#include "AppConfig.h"
#include "Base/FunctionTransfer.h"

#include "Widget/SetVideoUrlDialog.h"
#include "Widget/mymessagebox_withTitle.h"

Q_DECLARE_METATYPE(VideoPlayerState)

MainWindow::MainWindow(QWidget *parent) :
    DragAbleWidget(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this->getContainWidget());

    FunctionTransfer::init(QThread::currentThreadId());

    ///初始化播放器
    VideoPlayer::initPlayer();

    setWindowFlags(Qt::FramelessWindowHint);//|Qt::WindowStaysOnTopHint);  //使窗口的标题栏隐藏
//    setAttribute(Qt::WA_TranslucentBackground);

    //因为VideoPlayer::PlayerState是自定义的类型 要跨线程传递需要先注册一下
    qRegisterMetaType<VideoPlayerState>();

    mPopMenu = new QMenu(this);

    mAddVideoAction              = new QAction(QIcon("images/open.png"), QStringLiteral("打开网络流"), this);
    mEditVideoAction             = new QAction(QIcon("images/open.png"), QStringLiteral("修改数据"), this);
    mDeleteVideoAction           = new QAction(QIcon("images/open.png"), QStringLiteral("删除"), this);
    mClearVideoAction            = new QAction(QIcon("images/open.png"), QStringLiteral("清空"), this);

    mPopMenu->addAction(mAddVideoAction);
//    mPopMenu->addAction(mEditVideoAction);
//    mPopMenu->addSeparator();       //添加分离器
    mPopMenu->addAction(mDeleteVideoAction);
    mPopMenu->addAction(mClearVideoAction);

    connect(mAddVideoAction,     &QAction::triggered, this, &MainWindow::slotActionClick);
    connect(mEditVideoAction,    &QAction::triggered, this, &MainWindow::slotActionClick);
    connect(mDeleteVideoAction,  &QAction::triggered, this, &MainWindow::slotActionClick);
    connect(mClearVideoAction,   &QAction::triggered, this, &MainWindow::slotActionClick);

    connect(ui->pushButton_open, &QPushButton::clicked, this, &MainWindow::slotBtnClick);
    connect(ui->toolButton_open, &QPushButton::clicked, this, &MainWindow::slotBtnClick);
    connect(ui->pushButton_clear,&QPushButton::clicked, this, &MainWindow::slotBtnClick);
    connect(ui->pushButton_play, &QPushButton::clicked, this, &MainWindow::slotBtnClick);
    connect(ui->pushButton_pause,&QPushButton::clicked, this, &MainWindow::slotBtnClick);
    connect(ui->pushButton_stop, &QPushButton::clicked, this, &MainWindow::slotBtnClick);
    connect(ui->pushButton_volume, &QPushButton::clicked, this, &MainWindow::slotBtnClick);

    connect(ui->horizontalSlider, SIGNAL(sig_valueChanged(int)), this, SLOT(slotSliderMoved(int)));
    connect(ui->horizontalSlider_volume, SIGNAL(valueChanged(int)), this, SLOT(slotSliderMoved(int)));

    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::slotItemDoubleClicked);
    connect(ui->listWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::slotCustomContextMenuRequested);

    ui->page_video->setMouseTracking(true);
    ui->widget_videoPlayer->setMouseTracking(true);
//    ui->page_video->installEventFilter(this);
    ui->widget_videoPlayer->installEventFilter(this);
    ui->widget_container->installEventFilter(this);

    mCurrentIndex = 0;
    mCurrentItem = nullptr;
    mIsNeedPlayNext = false;

    mPlayer = new VideoPlayer();
    mPlayer->setVideoPlayerCallBack(this);

    mTimer = new QTimer; //定时器-获取当前视频时间
    connect(mTimer, &QTimer::timeout, this, &MainWindow::slotTimerTimeOut);
    mTimer->setInterval(500);

    mTimer_CheckControlWidget = new QTimer; //用于控制控制界面的出现和隐藏
    connect(mTimer_CheckControlWidget, &QTimer::timeout, this, &MainWindow::slotTimerTimeOut);
    mTimer_CheckControlWidget->setInterval(2500);

    mAnimation_ControlWidget  = new QPropertyAnimation(ui->widget_controller, "geometry");

    ui->stackedWidget->setCurrentWidget(ui->page_open);
    ui->pushButton_pause->hide();

    resize(1024,768);
    setTitle(QStringLiteral("我的播放器-V%1").arg(AppConfig::VERSION_NAME));

    mVolume = mPlayer->getVolume();

    std::thread([=]
    {
        while (1)
        {
            AppConfig::mSleep(500);

            static QPoint lastPoint = QPoint(0, 0);

            FunctionTransfer::runInMainThread([=]()
            {
                QPoint point = QCursor::pos();

                if (this->geometry().contains(point))
                {
                    if (lastPoint != point)
                    {
                        if (!mTimer_CheckControlWidget->isActive())
                        {
                            showOutControlWidget();
                        }

                        mTimer_CheckControlWidget->stop();
                        mTimer_CheckControlWidget->start();

                        lastPoint = point;
                    }
                }
            });
        }

    }).detach();
}

MainWindow::~MainWindow()
{
qDebug()<<__FUNCTION__;

    AppConfig::saveConfigInfoToFile();
    AppConfig::removeDirectory(AppConfig::AppDataPath_Tmp);

    delete ui;
}

void MainWindow::showOutControlWidget()
{

    mAnimation_ControlWidget->setDuration(800);

    int w = ui->widget_controller->width();
    int h = ui->widget_controller->height();
    int x = 0;
    int y = ui->widget_container->height() - ui->widget_controller->height();

    if (ui->widget_controller->isHidden())
    {
        ui->widget_controller->show();
        mAnimation_ControlWidget->setStartValue(ui->widget_controller->geometry());
    }
    else
    {
        mAnimation_ControlWidget->setStartValue(ui->widget_controller->geometry());
    }

//    mAnimation_ControlWidget->setKeyValueAt(0, QRect(0, 0, 00, 00));
//    mAnimation_ControlWidget->setKeyValueAt(0.4, QRect(20, 250, 20, 30));
//    mAnimation_ControlWidget->setKeyValueAt(0.8, QRect(100, 250, 20, 30));
//    mAnimation_ControlWidget->setKeyValueAt(1, QRect(250, 250, 100, 30));
    mAnimation_ControlWidget->setEndValue(QRect(x, y, w, h));
    mAnimation_ControlWidget->setEasingCurve(QEasingCurve::Linear); //设置动画效果

    mAnimation_ControlWidget->start();

}

void MainWindow::hideControlWidget()
{
    mAnimation_ControlWidget->setTargetObject(ui->widget_controller);

    mAnimation_ControlWidget->setDuration(300);

    int w = ui->widget_controller->width();
    int h = ui->widget_controller->height();
    int x = 0;
    int y = ui->widget_container->height() + h;

    mAnimation_ControlWidget->setStartValue(ui->widget_controller->geometry());
    mAnimation_ControlWidget->setEndValue(QRect(x, y, w, h));
    mAnimation_ControlWidget->setEasingCurve(QEasingCurve::Linear); //设置动画效果

    mAnimation_ControlWidget->start();
}


void MainWindow::setVideoNums(const int &nums)
{
    ui->label_num->setText(QStringLiteral("%1个文件").arg(nums));
}

void MainWindow::addVideoFiles(const QStringList &videoFileList)
{
    if (!videoFileList.isEmpty())
    {
        QString filePath = videoFileList.first();
        AppConfig::gVideoFilePath = QFileInfo(filePath).absoluteDir().path();
        AppConfig::saveConfigInfoToFile();
    }

    for (QString filePath : videoFileList)
    {
        addVideoFile(filePath);
    }
}

void MainWindow::addVideoFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);

    QListWidgetItem *item = new QListWidgetItem();
    item->setText(fileInfo.fileName());
    ui->listWidget->addItem(item);

    mVideoFileList.append(filePath);

    setVideoNums(mVideoFileList.size());
}

void MainWindow::clear()
{
    stopPlay();
    ui->listWidget->clear();
    mVideoFileList.clear();

    setVideoNums(mVideoFileList.size());
}

void MainWindow::startPlay()
{
    playVideo(0);
}

void MainWindow::stopPlay()
{
    if (mCurrentItem != nullptr)
    {
        mCurrentItem->setBackgroundColor(QColor(0, 0, 0, 0));
    }

    mCurrentItem = nullptr;

    mIsNeedPlayNext = false;
    mPlayer->stop(true);
}

void MainWindow::playVideo(const int &index)
{
    int playIndex = index;

//    ///播放到最后一个后，从头开始播放
//    {
//        if ((playIndex < 0) || (mVideoFileList.size() <= playIndex))
//        {
//            playIndex = 0;
//        }
//    }

    if (index >= 0 && mVideoFileList.size() > playIndex)
    {
        mCurrentIndex = playIndex;

        QString filePath = mVideoFileList.at(playIndex);

qDebug()<<__FUNCTION__<<filePath<<playIndex;

        playVideoFile(filePath);

//        ui->listWidget->setCurrentRow(playIndex);
        QListWidgetItem *item = ui->listWidget->item(playIndex);

        if (mCurrentItem != nullptr)
        {
            mCurrentItem->setBackgroundColor(QColor(0, 0, 0, 0));
        }

        mCurrentItem = item;

        mCurrentItem->setBackgroundColor(QColor(75, 92, 196));
    }
}

void MainWindow::playVideoFile(const QString &filePath)
{
    mIsNeedPlayNext = false;
    mPlayer->stop(true);
    mPlayer->startPlay(filePath.toStdString());
}

void MainWindow::slotSliderMoved(int value)
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

void MainWindow::slotTimerTimeOut()
{
    if (QObject::sender() == mTimer)
    {
        qint64 Sec = mPlayer->getCurrentTime();

        ui->horizontalSlider->setValue(Sec);

		QString curTime;
		QString hStr = QString("0%1").arg(Sec / 3600);
		QString mStr = QString("0%1").arg(Sec / 60 % 60);
		QString sStr = QString("0%1").arg(Sec % 60);
		if (hStr == "00")
		{
			curTime = QString("%1:%2").arg(mStr.right(2)).arg(sStr.right(2));
		}
		else
		{
			curTime = QString("%1:%2:%3").arg(hStr).arg(mStr.right(2)).arg(sStr.right(2));
		}

        ui->label_currenttime->setText(curTime);
    }
    else if (QObject::sender() == mTimer_CheckControlWidget)
    {
        mTimer_CheckControlWidget->stop();
        hideControlWidget();
    }
}


void MainWindow::doAdd()
{
    QStringList fileList = QFileDialog::getOpenFileNames(
               this, QStringLiteral("选择要播放的文件"),
                AppConfig::gVideoFilePath,//初始目录
                QStringLiteral("视频文件 (*.flv *.rmvb *.avi *.MP4 *.mkv);;")
                +QStringLiteral("音频文件 (*.mp3 *.wma *.wav);;")
                +QStringLiteral("所有文件 (*.*)"));

    if (!fileList.isEmpty())
    {
        addVideoFiles(fileList);
    }

    ///第一次添加，则直接播放
    if (mVideoFileList.size() == fileList.size())
    {
        startPlay();
    }
}

void MainWindow::doAddStream()
{
    SetVideoUrlDialog dialog;

//        dialog.setVideoUrl(AppConfig::gVideoFilePath);

    if (dialog.exec() == QDialog::Accepted)
    {
        QString s = dialog.getVideoUrl();

        if (!s.isEmpty())
        {
            mIsNeedPlayNext = false;
            mPlayer->stop(true); //如果在播放则先停止
            mPlayer->startPlay(s.toStdString());

            AppConfig::gVideoFilePath = s;
            AppConfig::saveConfigInfoToFile();
        }
    }
}

void MainWindow::doDelete()
{
    QList<int> RowList;

    QList<QListWidgetItem*> selectedItemList = ui->listWidget->selectedItems();

    for (QListWidgetItem* item : selectedItemList)
    {
        int rowValue = ui->listWidget->row(item);

        int index = RowList.size();
        for (int i=0;i<RowList.size();i++)
        {
            int value = RowList.at(i);

            if (rowValue > value)
            {
                index = i;
                break;
            }
        }

        RowList.insert(index, rowValue);
    }
qDebug()<<__FUNCTION__<<RowList;
    if (RowList.isEmpty())
    {
        int ret = MyMessageBox_WithTitle::showWarningText(QStringLiteral("警告"),
                                                           QStringLiteral("请先选择需要删除的数据！"),
                                                           NULL,
                                                           QStringLiteral("关闭"));
        return;
    }

    int ret = MyMessageBox_WithTitle::showWarningText(QStringLiteral("警告"),
                                                       QStringLiteral("确定删除这%1条数据么？").arg(RowList.size()),
                                                       QStringLiteral("确定"),
                                                       QStringLiteral("取消"));

    if (ret == QDialog::Accepted)
    {
        for(int i=0; i<RowList.size(); i++)
        {
            int row = RowList.at(i);

            mVideoFileList.removeAt(i);

            QListWidgetItem *item = ui->listWidget->takeItem(row);

            if (mCurrentItem == item)
            {
                mCurrentItem = nullptr;
            }

            delete item;

            if (row <= mCurrentIndex)
            {
                mCurrentIndex --;
            }
        }
    }

    setVideoNums(mVideoFileList.size());
}

void MainWindow::doClear()
{
    int ret = MyMessageBox_WithTitle::showWarningText(QStringLiteral("警告"),
                                                       QStringLiteral("确定要清空所有数据么？"),
                                                       QStringLiteral("确定"),
                                                       QStringLiteral("取消"));

    if (ret == QDialog::Accepted)
    {
        clear();
    }
}

void MainWindow::slotBtnClick(bool isChecked)
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
        mIsNeedPlayNext = false;
        mPlayer->stop(true);
    }
    else if (QObject::sender() == ui->pushButton_open)
    {
        doAdd();
    }
    else if (QObject::sender() == ui->pushButton_clear)
    {
        doClear();
    }
    else if (QObject::sender() == ui->toolButton_open)
    {
        doAddStream();
    }
    else if (QObject::sender() == ui->pushButton_volume)
    {
       qDebug()<<isChecked;

        bool isMute = isChecked;
        mPlayer->setMute(isMute);

        if (isMute)
        {
            mVolume = mPlayer->getVolume();

            ui->horizontalSlider_volume->setValue(0);
            ui->horizontalSlider_volume->setEnabled(false);
            ui->label_volume->setText(QString("%1").arg(0));
        }
        else
        {
            int volume = mVolume * 100.0;
            ui->horizontalSlider_volume->setValue(volume);
            ui->horizontalSlider_volume->setEnabled(true);
            ui->label_volume->setText(QString("%1").arg(volume));
        }

    }

}


void MainWindow::slotItemDoubleClicked(QListWidgetItem *item)
{
    if (QObject::sender() == ui->listWidget)
    {
        int index = ui->listWidget->row(item);
        playVideo(index);
    }
}

void MainWindow::slotCustomContextMenuRequested()
{
//    QPoint point(ui->listWidget->mapFromGlobal(QCursor::pos()));//获取控件的全局坐标

//    int h = ui->tableWidget->horizontalHeader()->height();

//    point.setY(point.y() - h);

//    QTableWidgetItem *item = ui->tableWidget->itemAt(point);

//    if (item == NULL || !item->isSelected())
//    {
//        ui->tableWidget->clearSelection();

//        mAddUserAction->setEnabled(true);
//        mEditUserAction->setEnabled(false);
//        mDeleteUserAction->setEnabled(false);
//        mExportAction->setEnabled(false);
//        mUploadAction_Selected->setEnabled(false);
//    }
//    else
//    {
//        mAddUserAction->setEnabled(false);
//        mEditUserAction->setEnabled(true);
//        mDeleteUserAction->setEnabled(true);
//        mExportAction->setEnabled(true);
//        mUploadAction_Selected->setEnabled(true);
//    }

    mPopMenu->exec(QCursor::pos());
}

void MainWindow::slotActionClick()
{
    if (QObject::sender() == mAddVideoAction)
    {
        doAddStream();
    }
    else if (QObject::sender() == mEditVideoAction)
    {

    }
    else if (QObject::sender() == mDeleteVideoAction)
    {
        doDelete();
    }
    else if (QObject::sender() == mClearVideoAction)
    {
        doClear();
    }
}

///打开文件失败
void MainWindow::onOpenVideoFileFailed(const int &code)
{
    FunctionTransfer::runInMainThread([=]()
    {
        QMessageBox::critical(NULL, "tips", QString("open file failed %1").arg(code));
    });
}

///打开SDL失败的时候回调此函数
void MainWindow::onOpenSdlFailed(const int &code)
{
    FunctionTransfer::runInMainThread([=]()
    {
        QMessageBox::critical(NULL, "tips", QString("open Sdl failed %1").arg(code));
    });
}

///获取到视频时长的时候调用此函数
void MainWindow::onTotalTimeChanged(const int64_t &uSec)
{
    FunctionTransfer::runInMainThread([=]()
    {
        qint64 Sec = uSec/1000000;

        ui->horizontalSlider->setRange(0,Sec);

		QString totalTime;
		QString hStr = QString("0%1").arg(Sec/3600);
		QString mStr = QString("0%1").arg(Sec / 60 % 60);
		QString sStr = QString("0%1").arg(Sec % 60);
		if (hStr == "00")
		{
			totalTime = QString("%1:%2").arg(mStr.right(2)).arg(sStr.right(2));
		}
		else
		{
			totalTime = QString("%1:%2:%3").arg(hStr).arg(mStr.right(2)).arg(sStr.right(2));
		}

        ui->label_totaltime->setText(totalTime);
    });
}

///播放器状态改变的时候回调此函数
void MainWindow::onPlayerStateChanged(const VideoPlayerState &state, const bool &hasVideo, const bool &hasAudio)
{
    FunctionTransfer::runInMainThread([=]()
    {
qDebug()<<__FUNCTION__<<state<<mIsNeedPlayNext;
        if (state == VideoPlayer_Stop)
        {
            ui->stackedWidget->setCurrentWidget(ui->page_open);

            ui->pushButton_pause->hide();
            ui->widget_videoPlayer->clear();

            ui->horizontalSlider->setValue(0);
            ui->label_currenttime->setText("00:00");
            ui->label_totaltime->setText("00:00");

            mTimer->stop();

            if (mIsNeedPlayNext)
            {
                mCurrentIndex++;
                playVideo(mCurrentIndex);
            }

            mIsNeedPlayNext = true;
        }
        else if (state == VideoPlayer_Playing)
        {
            if (hasVideo)
            {
                ui->stackedWidget->setCurrentWidget(ui->page_video);
            }
            else
            {
                ui->stackedWidget->setCurrentWidget(ui->page_audio);
            }

            ui->pushButton_play->hide();
            ui->pushButton_pause->show();

            mTimer->start();

            mIsNeedPlayNext = true;
        }
        else if (state == VideoPlayer_Pause)
        {
            ui->pushButton_pause->hide();
            ui->pushButton_play->show();
        }
    });
}

///显示视频数据，此函数不宜做耗时操作，否则会影响播放的流畅性。
void MainWindow::onDisplayVideo(std::shared_ptr<VideoFrame> videoFrame)
{
    ui->widget_videoPlayer->inputOneFrame(videoFrame);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    mPlayer->stop(true);
}

//图片显示部件时间过滤器处理
bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
    if(target == ui->widget_container)
    {
        if(event->type() == QEvent::Resize)
        {
            ///停止动画，防止此时刚好开始动画，导致位置出错
            mAnimation_ControlWidget->stop();

            QResizeEvent * e = (QResizeEvent*)event;
            int w = e->size().width();
            int h = e->size().height();
            ui->widget_video_back->move(0, 0);
            ui->widget_video_back->resize(w, h);

            int x = 0;
            int y = h - ui->widget_controller->height();
            ui->widget_controller->move(x, y);
            ui->widget_controller->resize(w, ui->widget_controller->height());
        }
    }
    else if(target == ui->page_video || target == ui->widget_videoPlayer)
    {
        if(event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress)
        {
            if (!mTimer_CheckControlWidget->isActive())
            {
                showOutControlWidget();
            }

            mTimer_CheckControlWidget->stop();
            mTimer_CheckControlWidget->start();
        }
        else if(event->type() == QEvent::Enter)
        {
            showOutControlWidget();
        }
        else if(event->type() == QEvent::Leave)
        {
            mTimer_CheckControlWidget->stop();
            mTimer_CheckControlWidget->start();
        }
    }

    //其它部件产生的事件则交给基类处理
    return DragAbleWidget::eventFilter(target, event);
}
