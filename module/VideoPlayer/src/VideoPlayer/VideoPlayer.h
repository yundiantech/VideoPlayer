/**
 * 叶海辉
 * QQ群321159586
 * http://blog.yundiantech.com/
 */

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <thread>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/time.h>
    #include <libavutil/pixfmt.h>
    #include <libavutil/display.h>
    #include <libavutil/avstring.h>
    #include <libavutil/opt.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/imgutils.h>
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavcodec/bsf.h>
}

///启用滤镜，用于旋转带角度的视频
#define CONFIG_AVFILTER 1

#include "util/util.h"
#include "util/thread.h"
#include "PcmPlayer/PcmPlayer.h"
#include "frame/AudioFrame/AACFrame.h"
#include "frame/VideoFrame/VideoFrame.h"

#define SDL_AUDIO_BUFFER_SIZE 1024
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

#define MAX_AUDIO_SIZE (50 * 20)
#define MAX_VIDEO_SIZE (25 * 20)

#define FLUSH_DATA "FLUSH"

/**
 * @brief The VideoPlayer class
 * 用到了c++11的语法，需要编译器开启c++11支持
 * 播放器类，纯c++实现，方便移植，与界面的交互通过回调函数的方式实现
 */

class VideoPlayer : public Util::Thread
{
public:
    enum State
    {
        Playing = 0,
        Pause,
        Stop,
        ReadError,
    };

    class EventHandle
    {
    public:
        ///打开文件失败
        virtual void onOpenVideoFileFailed(const int &code = 0) = 0;

        ///打开sdl失败的时候回调此函数
        virtual void onOpenSdlFailed(const int &code) = 0;

        ///获取到视频时长的时候调用此函数
        virtual void onTotalTimeChanged(const int64_t &uSec) = 0;

        ///播放器状态改变的时候回调此函数
        virtual void onPlayerStateChanged(const VideoPlayer::State &state, const bool &hasVideo, const bool &hasAudio) = 0;

        ///播放视频，此函数不宜做耗时操作，否则会影响播放的流畅性。
        virtual void onDisplayVideo(VideoRawFramePtr videoFrame) = 0;

        virtual void onVideoBuffer(VideoEncodedFramePtr video_frame){};
        virtual void onAudioBuffer(AACFramePtr audio_frame){};
        virtual void onAudioBuffer(PCMFramePtr audio_frame){};
    };

public:
    VideoPlayer();
    ~VideoPlayer();

    ///初始化播放器（必需要调用一次）
    static bool initPlayer();

    /**
     * @brief setVideoPlayerCallBack 设置播放器回调函数
     * @param pointer
     */
    void setEventHandle(VideoPlayer::EventHandle *handle){m_event_handle=handle;}

    bool startPlay(const std::string &filePath);

    bool replay(bool isWait = false); //重新播放

    bool play(); //播放（用于暂停后，重新开始播放）
    bool pause(); //暂停播放
    bool stop(bool isWait = true); //停止播放-参数表示是否等待所有的线程执行完毕再返回

    void seek(int64_t pos); //单位是微秒

    void setEnableHardDec(bool value); //设置是否启用硬件解码

    /**
     * 设置能力函数
     * 
     * @param video_decode 是否支持视频解码
     * @param encoded_video_callback 是否支持解码前的视频回调
     * 
     * 此函数用于配置对象的视频处理能力，包括是否支持视频解码和是否支持解码前视频的回调
     * 通过设置这些参数，可以控制对象在视频处理过程中的行为和功能
     */
    void setAbility(bool video_decode, bool encoded_video_callback, bool audio_play, bool encoded_audio_callback);

    void setMute(bool isMute);
    void setVolume(float value);
    float getVolume(){return mVolume;}

    int64_t getTotalTime(); //单位微秒
    uint64_t getCurrentTime(); //单位秒

    ///用于判断是否打开超时或读取超时
    bool mIsOpenStream; //是否正在打开流（用于回调函数中判断是打开流还是读取流）
    int64_t mCallStartTime = 0;

protected:
    void run(); //读取视频文件
    void decodeVideoThread(); //解码视频的线程
    void decodeAudioThread(); //解码音频的线程

//    static void sdlAudioCallBackFunc(void *userdata, Uint8 *stream, int len);
//    void sdlAudioCallBack(Uint8 *stream, int len);
    // int decodeAudioFrame(bool isBlock = false);

private:
    std::string m_file_path; //视频文件路径
    bool m_is_live_mode = false; //是否为直播流
    float m_speed = 1; //倍速播放

    State m_state; //播放状态

    ///音量相关变量
    bool  mIsMute;
    float mVolume; //音量 0~1 超过1 表示放大倍数

    /// 跳转相关的变量
    int           seek_req = 0; //跳转标志
    int64_t       seek_pos; //跳转的位置 -- 微秒
    int           seek_flag_audio;//跳转标志 -- 用于音频线程中
    int           seek_flag_video;//跳转标志 -- 用于视频线程中
    int64_t       seek_time; //跳转的时间(毫秒秒)  值和seek_pos是一样的

    ///播放控制相关
    bool mIsNeedPause; //暂停后跳转先标记此变量
    bool mIsPause;  //暂停标志
    bool mIsQuit;   //停止
    bool mIsReadFinished; //文件读取完毕
    bool mIsReadThreadFinished;
    bool mIsVideoThreadFinished; //视频解码线程
    bool mIsAudioThreadFinished; //音频播放线程
    bool mIsReadError = false; //是否读取失败

    ///音视频同步相关
    uint64_t mVideoStartTime; //开始播放视频的时间
    uint64_t mPauseStartTime; //暂停开始的时间
    int64_t audio_clock; ///音频时钟毫秒
    int64_t video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
    AVStream *mVideoStream = nullptr; //视频流
    AVStream *mAudioStream = nullptr; //音频流
    // std::mutex m_mutex_audio_clk;
    uint64_t getAudioClock();

    ///视频相关
    AVFormatContext *pFormatCtx = nullptr;
    AVCodecContext *pCodecCtx = nullptr;
    AVCodec *pCodec = nullptr;
    bool m_enable_hard_dec = true; //是否启用硬件解码
    bool openVideoDecoder(const AVCodecID &codec_id); //打开视频解码器
    bool openHardDecoder_Cuvid(const AVCodecID &codec_id); //打开硬件解码器（英伟达）
    bool openHardDecoder_Qsv(const AVCodecID &codec_id);   //打开硬件解码器（intel）
    bool openSoftDecoder(const AVCodecID &codec_id); //打开软解码器

    ///音频相关
    AVCodecContext *aCodecCtx = nullptr;
    AVCodec *aCodec = nullptr;
    AVFrame *aFrame = nullptr;

    ///以下变量用于音频重采样
    /// 由于ffmpeg解码出来后的pcm数据有可能是带平面的pcm，因此这里统一做重采样处理，
    /// 重采样成44100的16 bits 双声道数据(AV_SAMPLE_FMT_S16)
    AVFrame *aFrame_ReSample = nullptr;
    SwrContext *swrCtx = nullptr;

    enum AVSampleFormat in_sample_fmt; //输入的采样格式
    enum AVSampleFormat out_sample_fmt;//输出的采样格式 16bit PCM
    int m_in_sample_rate;//输入的采样率
    int m_out_sample_rate;//输出的采样率
    int audio_tgt_channels; ///av_get_channel_layout_nb_channels(out_ch_layout);
    int out_ch_layout;
    unsigned int audio_buf_size;
    unsigned int audio_buf_index;
//    DECLARE_ALIGNED(16,uint8_t,audio_buf) [AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
    uint8_t audio_buf[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];

    int autorotate = 1;
    int find_stream_info = 1;
    int filter_nbthreads = 0;

#if CONFIG_AVFILTER
    const char **vfilters_list = NULL;
    int nb_vfilters = 0;
    char *afilters = NULL;

    int vfilter_idx;
    AVFilterContext *in_video_filter;   // the first filter in the video chain
    AVFilterContext *out_video_filter;  // the last filter in the video chain
//    AVFilterContext *in_audio_filter;   // the first filter in the audio chain
//    AVFilterContext *out_audio_filter;  // the last filter in the audio chain
//    AVFilterGraph *agraph;              // audio filter graph
#endif

    ///视频帧队列
    Thread *m_thread_video = nullptr;
    std::mutex m_mutex_video;
    std::condition_variable m_cond_video;
    std::list<AVPacket> m_video_pkt_list;
    bool inputVideoQuene(const AVPacket &pkt);
    void clearVideoQuene();
    bool m_enable_video_decode = true;
    bool m_enable_encoded_video_callback = false; //是否回调解码之前的视频数据

    ///音频帧队列
    Thread *m_thread_audio = nullptr;
    std::mutex m_mutex_audio;
    std::condition_variable m_cond_audio;
    std::list<AVPacket> m_audio_pkt_list;
    bool inputAudioQuene(const AVPacket &pkt);
    void clearAudioQuene();
    bool m_enable_audio_play = true; //是否播放音频
    bool m_enable_encoded_audio_callback = false; //是否回调解码之前的音频数据

#ifdef USE_PCM_PLAYER
    PcmPlayer *m_pcm_player = nullptr;
#endif

    int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph, AVFilterContext *source_ctx, AVFilterContext *sink_ctx);
    int configure_video_filters(AVFilterGraph *graph, const char *vfilters, AVFrame *frame);

    ///回调函数相关，主要用于输出信息给界面
private:
    ///回调函数
    EventHandle *m_event_handle = nullptr;

    ///打开文件失败
    void doOpenVideoFileFailed(const int &code = 0);

    ///打开sdl失败的时候回调此函数
    void doOpenSdlFailed(const int &code);

    ///获取到视频时长的时候调用此函数
    void doTotalTimeChanged(const int64_t &uSec);

    ///播放器状态改变的时候回调此函数
    void doPlayerStateChanged(const VideoPlayer::State &state, const bool &hasVideo, const bool &hasAudio);

    ///显示视频数据，此函数不宜做耗时操作，否则会影响播放的流畅性。
    void doDisplayVideo(const uint8_t *yuv420Buffer, const int &width, const int &height);

};


#endif // VIDEOPLAYER_H
