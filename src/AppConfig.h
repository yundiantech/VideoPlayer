#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QFile>
#include <QString>
#include <QTranslator>
#include <QDateTime>

#define CURRENT_TIME QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]")

#ifdef QT_NO_KEYWORDS
#define foreach Q_FOREACH
#endif

class MainWindow;

class AppConfig
{
public:
    AppConfig();

    static QString APPID;
    static int VERSION;
    static QString VERSION_NAME;

    /// 本地全局变量
    static QString AppDataPath_Main; //程序数据主目录
    static QString AppDataPath_Data; //程序数据的data目录
    static QString AppDataPath_Tmp; //临时目录(程序退出时会清空此目录)
    static QString AppDataPath_TmpFile; //程序运行时 创建次文件，退出时删除此文件，用来判断程序是否正常退出
    static QString AppFilePath_Log; //日志目录
    static QString AppFilePath_LogFile; //日志文件
    static QString AppFilePath_EtcFile; //配置信息

    static MainWindow *gMainWindow;
    static QRect gMainWindowRect; //主窗口的位置 - 用于标记在非全屏模式下的弹窗大小
    static QRect gScreenRect;

    static bool gVideoKeepAspectRatio; //视频按比例播放
    static bool gVideoHardDecoder; //硬解解码
    static QString gVideoFilePath; //打开视频文件的默认位置


    static void MakeDir(QString dirName);
    static void InitAllDataPath(); //初始化所有数据保存的路径

    static QString bufferToString(QByteArray sendbuf);
    static QByteArray StringToBuffer(QString);
    static QString getFileMd5(QString filePath,qint64 size=-1);

    ///配置文件
    static void loadConfigInfoFromFile();
    static void saveConfigInfoToFile();

    ///写日志
    static void WriteLog(QString str);
    static void InitLogFile();
    static QString getSizeInfo(qint64 size);

    static QImage ImagetoGray( QImage image); //生成灰度图

    ///拷贝文件夹
    static bool copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist);

    ///删除目录
    static bool removeDirectory(QString dirName);

    ///重启软件
    static bool restartSelf();

    ///休眠函数(毫秒)
    static void mSleep(int mSecond);
};

#endif // APPCONFIG_H
