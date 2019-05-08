#ifndef VIDEOPLAYEREVENTHANDLE_H
#define VIDEOPLAYEREVENTHANDLE_H

#include "types.h"

class VideoPlayerCallBack
{
public:
    ///打开文件失败
    virtual void onOpenVideoFileFailed(const int &code = 0) = 0;

    ///打开sdl失败的时候回调此函数
    virtual void onOpenSdlFailed(const int &code) = 0;

    ///获取到视频时长的时候调用此函数
    virtual void onTotalTimeChanged(const int64_t &uSec) = 0;

    ///播放器状态改变的时候回调此函数
    virtual void onPlayerStateChanged(const VideoPlayerState &state, const bool &hasVideo, const bool &hasAudio) = 0;

    ///显示rgb数据，此函数不宜做耗时操作，否则会影响播放的流畅性，传入的brgb32Buffer，在函数返回后既失效。
    virtual void onDisplayVideo(const uint8_t *brgb32Buffer, const int &width, const int &height) = 0;

};

#endif // VIDEOPLAERYEVENTHANDLE_H
