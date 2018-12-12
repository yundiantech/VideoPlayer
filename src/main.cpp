
/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include <QApplication>
#include <QTextCodec>

#include "videoplayer/videoplayer.h"
#include "CustomTitleWidget/myhelper.h"

#undef main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
//    QTextCodec::setCodecForLocale(codec);
//    QTextCodec::setCodecForCStrings(codec);
//    QTextCodec::setCodecForTr(codec);

    myHelper::SetUTF8Code();
//    myHelper::SetStyle("black");//黑色风格
//    myHelper::SetStyle("blue");//蓝色风格
    myHelper::SetStyle("gray");//灰色风格
//    myHelper::SetStyle("navy");//天蓝色风格
    myHelper::SetChinese();


    VideoPlayer w;
    w.show();

    return a.exec();
}

