
/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include <QApplication>
#include <QTextCodec>

#include "videoplayer/videoplayer.h"

#undef main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTextCodec *codec = QTextCodec::codecForName("GBK");
    QTextCodec::setCodecForLocale(codec);

    VideoPlayer w;
    w.show();

    return a.exec();
}

