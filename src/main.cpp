
/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include <QApplication>
#include <QTextCodec>

#include <QDebug>

#include "AppConfig.h"
#include "MainWindow.h"

#undef main
int main(int argc, char *argv[])
{
//    bool userSoftWareOpenGL = false; //使用软件模拟openGL

//    for (int i=0;i<argc;i++)
//    {
//        QString str = QString(argv[i]);

//        if (str == "usesoftopengl")
//        {
//            userSoftWareOpenGL = true;
//        }
//        else if (str == "harddecoder")
//        {
//            AppConfig::gVideoHardDecoder = true;
//        }
//        qDebug()<<__FUNCTION__<<argv[i]<<str;
//    }

//    if (userSoftWareOpenGL)
//    {
//        qDebug()<<__FUNCTION__<<"\n\n !!! userSoftWareOpenGL !!! \n\n";
//        ///没有安装显卡驱动的系统需要使用软件模拟的openGL，一般是linux系统下会存在这种情况
//        QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
//    }
//    else
//    {
//#if defined(WIN32)
//        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES); //解决windows下使用独立显卡的时候 切换全屏 闪烁问题
//#else

//#endif
//    }

    QApplication a(argc, argv);

    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QTextCodec::setCodecForLocale(codec);

    AppConfig::InitAllDataPath(); //初始化一些变量和路径信息
    AppConfig::loadConfigInfoFromFile();

    AppConfig::WriteLog(QString( "\n #############\n Version = %1 \n ##############").arg(AppConfig::VERSION_NAME));

    MainWindow w;
    w.show();

    return a.exec();
}

