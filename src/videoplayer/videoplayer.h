/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QThread>
#include <QImage>

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include <libavutil/time.h>
    #include "libavutil/pixfmt.h"
    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"


    #include <SDL.h>
    #include <SDL_audio.h>
    #include <SDL_types.h>
    #include <SDL_name.h>
    #include <SDL_main.h>
    #include <SDL_config.h>
}

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 1
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

typedef struct VideoState {
    AVCodecContext *aCodecCtx; //音频解码器
    AVFrame *audioFrame;// 解码音频过程中的使用缓存
    PacketQueue *audioq;

    double video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame

    AVStream *video_st;

} VideoState;

class VideoPlayer : public QThread
{
    Q_OBJECT

public:
    explicit VideoPlayer();
    ~VideoPlayer();

    void setFileName(QString path){mFileName = path;}

    void startPlay();

signals:
    void sig_GetOneFrame(QImage); //每获取到一帧图像 就发送此信号

protected:
    void run();

private:
    QString mFileName;

    VideoState mVideoState; //用来 传递给 SDL音频回调函数的数据

};

#endif // VIDEOPLAYER_H
