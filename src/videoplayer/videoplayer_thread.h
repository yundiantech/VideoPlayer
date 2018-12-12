#ifndef VIDEOPLAYER_THREAD_H
#define VIDEOPLAYER_THREAD_H

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

#include "videoplayer/videoplayer_showvideowidget.h"

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 1
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

#define MAX_AUDIO_SIZE (25 * 16 * 1024)
#define MAX_VIDEO_SIZE (25 * 256 * 1024)

class VideoPlayer_Thread; //前置声明

typedef struct VideoState {
    AVFormatContext *ic;
    int videoStream, audioStream;
    AVFrame *audio_frame;// 解码音频过程中的使用缓存
    PacketQueue audioq;
    AVStream *audio_st; //音频流
    unsigned int audio_buf_size;
    unsigned int audio_buf_index;
    AVPacket audio_pkt;
    uint8_t *audio_pkt_data;
    int audio_pkt_size;
    uint8_t *audio_buf;
    DECLARE_ALIGNED(16,uint8_t,audio_buf2) [AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
    enum AVSampleFormat audio_src_fmt;
    enum AVSampleFormat audio_tgt_fmt;
    int audio_src_channels;
    int audio_tgt_channels;
    int64_t audio_src_channel_layout;
    int64_t audio_tgt_channel_layout;
    int audio_src_freq;
    int audio_tgt_freq;
    struct SwrContext *swr_ctx; //用于解码后的音频格式转换
    int audio_hw_buf_size;

    double audio_clock; ///音频时钟
    double video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame

    AVStream *video_st;
    PacketQueue videoq;

    /// 跳转相关的变量
    int             seek_req; //跳转标志
    int64_t         seek_pos; //跳转的位置 -- 微秒
    int             seek_flag_audio;//跳转标志 -- 用于音频线程中
    int             seek_flag_video;//跳转标志 -- 用于视频线程中
    double          seek_time; //跳转的时间(秒)  值和seek_pos是一样的

    ///播放控制相关
    bool isPause;  //暂停标志
    bool quit;  //停止
    bool readFinished; //文件读取完毕
    bool readThreadFinished;
    bool videoThreadFinished;

    SDL_Thread *video_tid;  //视频线程id

    VideoPlayer_Thread *player; //记录下这个类的指针  主要用于在线程里面调用激发信号的函数

    bool isMute; //静音标识
    float mVolume; //0~1 超过1 表示放大倍数

//    VideoState()
//    {
//        isMute = false;

////        Init();
//    }

//    void Init()
//    {
//        player = NULL;
//        readThreadFinished = false;
//        videoThreadFinished = false;
//        quit = false;
//        audio_frame = NULL;
//        video_clock = 0;
//        audio_clock = 0;

//        ic = NULL;
//        videoStream = 0;
//        audioStream = 0;
//        audio_frame = NULL;// 解码音频过程中的使用缓存
//        audio_st = NULL; //音频流
//        audio_buf_size = 0;
//        audio_buf_index = 0;
//        audio_pkt_data = NULL;
//        audio_pkt_size = 0;
//        audio_buf = NULL;
//        audio_src_channels = 0;
//        audio_tgt_channels = 0;
//        audio_src_channel_layout = 0;
//        audio_tgt_channel_layout = 0;
//        audio_src_freq = 0;
//        audio_tgt_freq = 0;
//        swr_ctx = NULL; //用于解码后的音频格式转换
//        audio_hw_buf_size = false;

//        audio_clock = 0; ///音频时钟
//        video_clock = 0; ///<pts of last decoded frame / predicted pts of next decoded frame

//        video_st = NULL;

//        /// 跳转相关的变量
//        seek_req = 0; //跳转标志
//        seek_pos = 0; //跳转的位置 -- 微秒
//        seek_flag_audio = 0;//跳转标志 -- 用于音频线程中
//        seek_flag_video = 0;//跳转标志 -- 用于视频线程中
//        seek_time = 0; //跳转的时间(秒)  值和seek_pos是一样的

//        ///播放控制相关
//        isPause = false;  //暂停标志
//        quit = false;  //停止
//        readFinished = false; //文件读取完毕
//        readThreadFinished = false;
//        videoThreadFinished = false;

//    }

} VideoState;

class VideoPlayer_Thread : public QThread
{
    Q_OBJECT

public:

    enum PlayerState
    {
        Playing,
        Pause,
        Stop
    };

    explicit VideoPlayer_Thread();
    ~VideoPlayer_Thread();

    bool setFileName(QString path);

    bool replay(); //重新播放

    bool play();
    bool pause();
    bool stop(bool isWait = false); //参数表示是否等待所有的线程执行完毕再返回

    void seek(int64_t pos); //单位是微秒

    void setMute(bool isMute){mIsMute = isMute;}
    void setVolume(float value);

    int64_t getTotalTime(); //单位微秒
    double getCurrentTime(); //单位秒

    void disPlayVideo(QImage img);

    void setVideoWidget(VideoPlayer_ShowVideoWidget*widget);
    QWidget *getVideoWidget(){return mVideoWidget;}

signals:
    void sig_GetOneFrame(QImage); //每获取到一帧图像 就发送此信号

    void sig_StateChanged(VideoPlayer_Thread::PlayerState state);
    void sig_TotalTimeChanged(qint64 uSec); //获取到视频时长的时候激发此信号

protected:
    void run();

private:
    QString mFileName;

    VideoState mVideoState;


    PlayerState mPlayerState; //播放状态

    ///用自己的控件替代SLD 是因为SDL会导致QSS样式失效
    VideoPlayer_ShowVideoWidget *mVideoWidget; //显示视频用的控件

    bool mIsMute;
    float mVolume; //0~1 超过1 表示放大倍数

    SDL_AudioDeviceID mAudioID;

    int openSDL();
    void closeSDL();

    void deInit();

};

#endif // VIDEOPLAYER_THREAD_H
