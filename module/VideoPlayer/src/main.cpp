
/**
 * 叶海辉
 * QQ群321159586
 * http://blog.yundiantech.com/
 */

#include <QApplication>
#include <QTextCodec>

#include "VideoPlayer/VideoPlayer.h"

#undef main
int main(int argc, char *argv[])
{
    VideoPlayer *mPlayer = new VideoPlayer();
//    mPlayer->setVideoPlayerCallBack(this);

    mPlayer->startPlay("F:/test.rmvb");

    return 0;
}

