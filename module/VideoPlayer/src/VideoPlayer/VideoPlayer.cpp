/**
 * 叶海辉
 * QQ群321159586
 * http://blog.yundiantech.com/
 */

#include "VideoPlayer.h"

#ifdef USE_PCM_PLAYER
#include "PcmPlayer/PcmPlayer_SDL.h"
#endif

#include <stdio.h>
#include <iostream>

#include <libavutil/error.h>
#include <stdio.h>

void print_ffmpeg_error(int errnum) 
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;
    if (av_strerror(errnum, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(errnum));
    fprintf(stderr, "Error: %s\n", errbuf_ptr);
}

VideoPlayer::VideoPlayer()
{
    m_state = VideoPlayer::Stop;

    mIsMute = false;

    mIsNeedPause = false;

    mVolume = 1;
#ifdef USE_PCM_PLAYER
    m_pcm_player = new PcmPlayer_SDL();
    m_pcm_player->setSpeed(m_speed);
#endif
    this->setSingleMode(true);

    m_thread_video = new Thread();
    m_thread_video->setSingleMode(true);
    m_thread_video->setThreadFunc(std::bind(&VideoPlayer::decodeVideoThread, this));

    m_thread_audio = new Thread();
    m_thread_audio->setSingleMode(true);
    m_thread_audio->setThreadFunc(std::bind(&VideoPlayer::decodeAudioThread, this));
}

VideoPlayer::~VideoPlayer()
{

}

bool VideoPlayer::initPlayer()
{
    static int isInited = false;

    if (!isInited)
    {
//        av_register_all(); //初始化FFMPEG  调用了这个才能正常使用编码器和解码器
        avformat_network_init(); //支持打开网络文件

        isInited = true;
    }

//SDL初始化需要放入子线程中，否则有些电脑会有问题。
//    if (SDL_Init(SDL_INIT_AUDIO))
//    {
//        fprintf(stderr, "Could not initialize SDL - %s. \n", SDL_GetError());
//        return false;
//    }

    return true;
}

bool VideoPlayer::startPlay(const std::string &filePath)
{
    if (m_state != VideoPlayer::Stop)
    {
        return false;
    }

    mIsQuit = false;
    mIsPause = false;

    if (!filePath.empty())
    {
        m_file_path = filePath;
    }

    //启动新的线程实现读取视频文件
    this->start();

    return true;

}

bool VideoPlayer::replay(bool isWait)
{
    stop(isWait);

    startPlay(m_file_path);

    return true;
}

bool VideoPlayer::play()
{
    mIsNeedPause = false;
    mIsPause = false;

    if (m_state != VideoPlayer::Pause)
    {
        return false;
    }

    uint64_t pauseTime = av_gettime() - mPauseStartTime; //暂停了多长时间
    mVideoStartTime += pauseTime; //将暂停的时间加到开始播放的时间上，保证同步不受暂停的影响

    m_state = VideoPlayer::Playing;
    doPlayerStateChanged(VideoPlayer::Playing, mVideoStream != nullptr, mAudioStream != nullptr);

    return true;
}

bool VideoPlayer::pause()
{
    fprintf(stderr, "%s mIsPause=%d \n", __FUNCTION__, mIsPause);

    mIsPause = true;

    if (m_state != VideoPlayer::Playing)
    {
        return false;
    }

    mPauseStartTime = av_gettime();

    m_state = VideoPlayer::Pause;

    doPlayerStateChanged(VideoPlayer::Pause, mVideoStream != nullptr, mAudioStream != nullptr);

    return true;
}

bool VideoPlayer::stop(bool isWait)
{
    if (m_state == VideoPlayer::Stop)
    {
        return false;
    }

    m_state = VideoPlayer::Stop;
    mIsQuit = true;
    mIsPause = false;

    ///唤醒等待中的线程
    m_cond_video.notify_all();
    m_cond_audio.notify_all();

    if (isWait)
    {
        while(!mIsReadThreadFinished)
        {
            fprintf(stderr, "mIsReadThreadFinished=%d \n", mIsReadThreadFinished);
            mSleep(100);
        }
    }

    return true;
}

void VideoPlayer::seek(int64_t pos)
{
    if(!seek_req)
    {
        seek_pos = pos;
        seek_req = 1;
    }
}

void VideoPlayer::setAbility(bool video_decode, bool encoded_video_callback, bool audio_play, bool encoded_audio_callback)
{
    m_enable_video_decode = video_decode;
    m_enable_encoded_video_callback = encoded_video_callback;
    m_enable_audio_play = audio_play;
    m_enable_encoded_audio_callback = encoded_audio_callback;
}

void VideoPlayer::setEnableHardDec(bool value)
{
    m_enable_hard_dec = value;
}

void VideoPlayer::setMute(bool isMute)
{
    mIsMute = isMute;
#ifdef USE_PCM_PLAYER
    m_pcm_player->setMute(isMute);
#endif
}

void VideoPlayer::setVolume(float value)
{
    mVolume = value;
#ifdef USE_PCM_PLAYER
    m_pcm_player->setVolume(value);
#endif
}

uint64_t VideoPlayer::getCurrentTime()
{
    return getAudioClock();
}

uint64_t VideoPlayer::getAudioClock()
{
    // std::lock_guard<std::mutex> lck(m_mutex_audio_clk);
    return audio_clock;
}

int64_t VideoPlayer::getTotalTime()
{
    return pFormatCtx->duration;
}

//int VideoPlayer::openSDL()
//{
//    ///打开SDL，并设置播放的格式为:AUDIO_S16LSB 双声道，44100hz
//    ///后期使用ffmpeg解码完音频后，需要重采样成和这个一样的格式，否则播放会有杂音
//    SDL_AudioSpec wanted_spec, spec;
//    int wanted_nb_channels = 2;
////    int samplerate = 44100;
//    int samplerate = m_out_sample_rate;

//    wanted_spec.channels = wanted_nb_channels;
//    wanted_spec.freq = samplerate;
//    wanted_spec.samples = FFMAX(512, 2 << av_log2(wanted_spec.freq / 30));
//    wanted_spec.format = AUDIO_S16SYS; // 具体含义请查看“SDL宏定义”部分
//    wanted_spec.silence = 0;            // 0指示静音
////    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;  // 自定义SDL缓冲区大小
//    wanted_spec.callback = sdlAudioCallBackFunc;  // 回调函数
//    wanted_spec.userdata = this;                  // 传给上面回调函数的外带数据

//    int num = SDL_GetNumAudioDevices(0);
//    for (int i=0;i<num;i++)
//    {
//        mAudioID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(i,0), false, &wanted_spec, &spec,0);
//        if (mAudioID > 0)
//        {
//            break;
//        }
//    }

//    /* 检查实际使用的配置（保存在spec,由SDL_OpenAudio()填充） */
////    if (spec.format != AUDIO_S16SYS)
//    if (mAudioID <= 0)
//    {
//        mIsAudioThreadFinished = true;
//        return -1;
//    }
//fprintf(stderr, "mAudioID=%d\n\n\n\n\n\n", mAudioID);
//    return 0;
//}

//void VideoPlayer::closeSDL()
//{
//    if (mAudioID > 0)
//    {
//        SDL_LockAudioDevice(mAudioID);
//        SDL_PauseAudioDevice(mAudioID, 1);
//        SDL_UnlockAudioDevice(mAudioID);

//        SDL_CloseAudioDevice(mAudioID);
//    }

//    mAudioID = 0;
//}


static int CheckInterrupt(void* ctx)
{
    VideoPlayer *pointer = (VideoPlayer*)ctx;

    int ret = 0;

    if (pointer->mIsOpenStream)
    {
        ///打开超时10秒
        if ((av_gettime()-pointer->mCallStartTime) >= 10000000)
        {
            ret = 1;
        }
    }
    else
    {
        ///读取超时10秒
        if ((av_gettime()-pointer->mCallStartTime) >= 10000000)
        {
            ret = 1;
        }
    }
//fprintf(stderr,"%s mIsOpenStream=%d ret=%d \n", __FUNCTION__, pointer->mIsOpenStream, ret);

    return ret;
}

void VideoPlayer::run()
{
//    ///SDL初始化需要放入子线程中，否则有些电脑会有问题。
//    if (SDL_Init(SDL_INIT_AUDIO))
//    {
//        doOpenSdlFailed(-100);
//        fprintf(stderr, "Could not initialize SDL - %s. \n", SDL_GetError());
//        return;
//    }

    mIsReadThreadFinished = false;
    mIsReadFinished = false;
    mIsReadError = false;

    const char * file_path = m_file_path.c_str();

    pFormatCtx = nullptr;
    pCodecCtx = nullptr;
    pCodec = nullptr;

    aCodecCtx = nullptr;
    aCodec = nullptr;
    aFrame = nullptr;

    swrCtx = nullptr;

    AVBSFContext *bsf_ctx = nullptr;
    mAudioStream = nullptr;
    mVideoStream = nullptr;

    audio_clock = 0;
    video_clock = 0;

    int audioStream ,videoStream;

    //Allocate an AVFormatContext.
    pFormatCtx = avformat_alloc_context();

    pFormatCtx->interrupt_callback.callback = CheckInterrupt;//超时回调
    pFormatCtx->interrupt_callback.opaque = this;

    AVDictionary* opts = NULL;
    av_dict_set(&opts, "rtsp_transport", "udp", 0); //设置tcp or udp，默认一般优先tcp再尝试udp
//    av_dict_set(&opts, "rtsp_transport", "tcp", 0); //设置tcp or udp，默认一般优先tcp再尝试udp
    av_dict_set(&opts, "stimeout", "5000000", 0);//设置超时5秒

    mCallStartTime = av_gettime();
    mIsOpenStream  = true;

    int ret = avformat_open_input(&pFormatCtx, file_path, nullptr, &opts);
    if (ret != 0)
    {
        fprintf(stderr, "can't open the file. ret=%d \n", ret);
        print_ffmpeg_error(ret);
        mIsReadError = true;
        doOpenVideoFileFailed();
        goto end;
    }

    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0)
    {
        fprintf(stderr, "Could't find stream infomation.\n");
        mIsReadError = true;
        doOpenVideoFileFailed();
        goto end;
    }

    videoStream = -1;
    audioStream = -1;
printf("%s:%d pFormatCtx->nb_streams=%d \n", __FILE__, __LINE__, pFormatCtx->nb_streams);
    ///循环查找视频中包含的流信息，
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStream < 0)
        {
            videoStream = i;
        }
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO  && audioStream < 0)
        {
            audioStream = i;
        }
    }

    doTotalTimeChanged(getTotalTime());

    ///打开视频解码器，并启动视频线程
    if (videoStream >= 0)
    {
        AVStream *video_stream = pFormatCtx->streams[videoStream];
        mVideoStream = video_stream;

        // if (m_enable_video_decode)
        // {
        //     ///查找视频解码器
        //     pCodec = (AVCodec*)avcodec_find_decoder(video_stream->codecpar->codec_id);
        //     pCodecCtx = avcodec_alloc_context3(pCodec);
        //     pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

        //     avcodec_parameters_to_context(pCodecCtx, video_stream->codecpar);

        //     if (pCodec == nullptr)
        //     {
        //         fprintf(stderr, "PCodec not found.\n");
        //         doOpenVideoFileFailed();
        //         goto end;
        //     }

        //     ///打开视频解码器
        //     if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        //     {
        //         fprintf(stderr, "Could not open video codec.\n");
        //         doOpenVideoFileFailed();
        //         goto end;
        //     }
        // }

        if (m_enable_video_decode)
        {
            bool ret = openVideoDecoder(video_stream->codecpar->codec_id);
            if (!ret)
            {
                fprintf(stderr, "Could not open video codec.\n");
                doOpenVideoFileFailed();
                goto end;
            }
        }

        if (!m_is_live_mode && m_enable_encoded_video_callback)
        {
            ///如果需要回调解码前的数据，则需要设置AVBitStreamFilter
            ///用于添加起始码和sps/pps
            do{
                //设置AVBitStreamFilter
                int ret;
                const AVBitStreamFilter *filter = nullptr;

                if (mVideoStream->codecpar->codec_id == AV_CODEC_ID_H264) 
                {
                    filter = av_bsf_get_by_name("h264_mp4toannexb");
                } 
                else if (mVideoStream->codecpar->codec_id == AV_CODEC_ID_HEVC) 
                {
                    filter = av_bsf_get_by_name("hevc_mp4toannexb");
                }

                if (!filter) 
                {
                    printf("Unkonw bitstream filter");
                    break;
                }

                ret = av_bsf_alloc(filter, &bsf_ctx);
                if (ret < 0) 
                {
                    printf("alloc bsf error");
                    break;
                }

                ret = avcodec_parameters_copy(bsf_ctx->par_in, mVideoStream->codecpar);
                if (ret < 0)
                {
                    printf("copy bsf error");

                    av_bsf_free(&bsf_ctx);
                    bsf_ctx = nullptr;

                    break;
                }

                av_bsf_init(bsf_ctx);

            }while(0);
        }

        ///启动视频解码线程
        m_thread_video->start();
    }

    if (audioStream >= 0)
    {
        AVStream *audio_stream = pFormatCtx->streams[audioStream];

        ///查找音频解码器
        aCodec = (AVCodec*)avcodec_find_decoder(audio_stream->codecpar->codec_id);

        if (aCodec == NULL)
        {
            fprintf(stderr, "ACodec not found.\n");
            audioStream = -1;
        }
        else
        {
            /* Allocate a codec context for the decoder */
            aCodecCtx = avcodec_alloc_context3(aCodec);
            avcodec_parameters_to_context(aCodecCtx, audio_stream->codecpar);

            ///打开音频解码器
            if (avcodec_open2(aCodecCtx, aCodec, nullptr) < 0)
            {
                fprintf(stderr, "Could not open audio codec.\n");
                doOpenVideoFileFailed();
                goto end;
            }

            ///解码音频相关
            aFrame = av_frame_alloc();


            //重采样设置选项-----------------------------------------------------------start
            aFrame_ReSample = nullptr;

            //frame->16bit 44100 PCM 统一音频采样格式与采样率
            swrCtx = nullptr;

            //输入的声道布局
            int in_ch_layout;

            //输出的声道布局
            out_ch_layout = av_get_default_channel_layout(audio_tgt_channels); ///AV_CH_LAYOUT_STEREO

            out_ch_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;

            /// 这里音频播放使用了固定的参数
            /// 强制将音频重采样成44100 双声道  AV_SAMPLE_FMT_S16
            /// SDL播放中也是用了同样的播放参数
            //重采样设置选项----------------
            //输入的采样格式
            in_sample_fmt = aCodecCtx->sample_fmt;
            //输出的采样格式 16bit PCM
            out_sample_fmt = AV_SAMPLE_FMT_S16;
            //输入的采样率
            m_in_sample_rate = aCodecCtx->sample_rate;
            //输入的声道布局
            in_ch_layout = aCodecCtx->channel_layout;

            //输出的采样率
//            m_out_sample_rate = 44100;
            m_out_sample_rate = aCodecCtx->sample_rate;

            //输出的声道布局

            audio_tgt_channels = 2; ///av_get_channel_layout_nb_channels(out_ch_layout);
            out_ch_layout = av_get_default_channel_layout(audio_tgt_channels); ///AV_CH_LAYOUT_STEREO

            out_ch_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;

            /// 2019-5-13添加
            /// wav/wmv 文件获取到的aCodecCtx->channel_layout为0会导致后面的初始化失败，因此这里需要加个判断。
            if (in_ch_layout <= 0)
            {
                in_ch_layout = av_get_default_channel_layout(aCodecCtx->channels);
            }

            swrCtx = swr_alloc_set_opts(nullptr, out_ch_layout, out_sample_fmt, m_out_sample_rate,
                                                 in_ch_layout, in_sample_fmt, m_in_sample_rate, 0, nullptr);

            /** Open the resampler with the specified parameters. */
            int ret = swr_init(swrCtx);
            if (ret < 0)
            {
                char buff[128]={0};
                av_strerror(ret, buff, 128);

                fprintf(stderr, "Could not open resample context %s\n", buff);
                swr_free(&swrCtx);
                swrCtx = nullptr;
                doOpenVideoFileFailed();
                goto end;
            }

            //存储pcm数据
            int out_linesize = m_out_sample_rate * audio_tgt_channels;

    //        out_linesize = av_samples_get_buffer_size(NULL, audio_tgt_channels, av_get_bytes_per_sample(out_sample_fmt), out_sample_fmt, 1);
            out_linesize = AVCODEC_MAX_AUDIO_FRAME_SIZE;


            mAudioStream = pFormatCtx->streams[audioStream];

//            ///打开SDL播放声音
//            int code = openSDL();

//            if (code == 0)
//            {
//                SDL_LockAudioDevice(mAudioID);
//                SDL_PauseAudioDevice(mAudioID,0);
//                SDL_UnlockAudioDevice(mAudioID);

                mIsAudioThreadFinished = false;
//            }
//            else
//            {
//                doOpenSdlFailed(code);
//            }
        }

        ///启动音频解码线程
        m_thread_audio->start();
    }

    av_dump_format(pFormatCtx, 0, file_path, 0); //输出视频信息

    m_state = VideoPlayer::Playing;
    doPlayerStateChanged(VideoPlayer::Playing, mVideoStream != nullptr, mAudioStream != nullptr);

    mVideoStartTime = av_gettime();
fprintf(stderr, "%s mIsQuit=%d mIsPause=%d file_path=%s \n", __FUNCTION__, mIsQuit, mIsPause, file_path);

    if (m_file_path.find("rtmp://") != std::string::npos
        || m_file_path.find("rtsp://") != std::string::npos) 
    {
        m_is_live_mode = true;
    }
    else
    {
        m_is_live_mode = false;
    }

    video_clock = 0;
    audio_clock = 0;
    seek_req = 0;
    seek_time = 0;
    seek_flag_audio = 0;
    seek_flag_video = 0;

    while (1)
    {
        if (mIsQuit)
        {
            //停止播放了
            break;
        }

        if (m_is_live_mode && mIsReadError)
        {
            break; //网络流读取失败，需要重新打开流
        }

        if (seek_req)
        {
            int stream_index = -1;
            int64_t seek_target = seek_pos;

            if (videoStream >= 0)
                stream_index = videoStream;
            else if (audioStream >= 0)
                stream_index = audioStream;

            AVRational aVRational = {1, AV_TIME_BASE};
            if (stream_index >= 0)
            {
                seek_target = av_rescale_q(seek_target, aVRational, pFormatCtx->streams[stream_index]->time_base);
            }
std::cout<<" video:"<<pFormatCtx->streams[videoStream]->duration<<" "<<pFormatCtx->streams[videoStream]->time_base.den
         <<" audio:"<<pFormatCtx->streams[audioStream]->duration<<" "<<pFormatCtx->streams[audioStream]->time_base.den<<std::endl;

            bool seek_succeed = false;
            if (av_seek_frame(pFormatCtx, stream_index, seek_target, AVSEEK_FLAG_BACKWARD) < 0)
            {
                fprintf(stderr, "%s: error while seeking. stream_index=%d \n", file_path, stream_index);
                if (audioStream >= 0 && stream_index != audioStream)
                {
                    stream_index = audioStream;
                    seek_target = av_rescale_q(seek_target, aVRational, pFormatCtx->streams[stream_index]->time_base);
                    if (av_seek_frame(pFormatCtx, stream_index, seek_target, AVSEEK_FLAG_ANY) < 0)
                    {
                        fprintf(stderr, "%s: error while seeking. stream_index=%d \n", file_path, stream_index);
                    }
                    else
                    {
                        seek_succeed = true;
                        fprintf(stderr, "%s: seeking success. stream_index=%d \n", file_path, stream_index);
                    }
                }
            }
            else
            {
                seek_succeed = true;
                fprintf(stderr, "%s: seeking success. stream_index=%d \n", file_path, stream_index);
            }

            if (seek_succeed)
            {
                if (audioStream >= 0)
                {
                    AVPacket packet;
                    av_new_packet(&packet, 10);
                    strcpy((char*)packet.data,FLUSH_DATA);
                    clearAudioQuene(); //清除队列
                    inputAudioQuene(packet); //往队列中存入用来清除的包
                }

                if (videoStream >= 0)
                {
                    AVPacket packet;
                    av_new_packet(&packet, 10);
                    strcpy((char*)packet.data,FLUSH_DATA);
                    clearVideoQuene(); //清除队列
                    inputVideoQuene(packet); //往队列中存入用来清除的包
                    video_clock = 0;
                }

                mVideoStartTime = av_gettime() - seek_pos;
                mPauseStartTime = av_gettime();
            }
            seek_req = 0;
            seek_time = seek_pos / 1000;
            seek_flag_audio = 1;
            seek_flag_video = 1;

            if (mIsPause)
            {
                mIsNeedPause = true;
                mIsPause = false;
            }

        }

        //这里做了个限制  当队列里面的数据超过某个大小的时候 就暂停读取  防止一下子就把视频读完了，导致的空间分配不足
        //这个值可以稍微写大一些
// fprintf(stderr, "%s %d m_audio_pkt_list.size()=%d m_video_pkt_list.size()=%d \n", __FILE__, __LINE__, m_audio_pkt_list.size(), m_video_pkt_list.size());
        if (m_audio_pkt_list.size() >= MAX_AUDIO_SIZE || m_video_pkt_list.size() >= MAX_VIDEO_SIZE)
        {
            mSleep(10);
            continue;
        }

        if (mIsPause == true)
        {
            mSleep(10);
            continue;
        }

        AVPacket packet;
//qDebug("%s av_read_frame .... \n", __FUNCTION__);

        mCallStartTime = av_gettime();
        mIsOpenStream  = false;
// qDebug()<<__FUNCTION__<<video_clock<<audio_clock;
        if (av_read_frame(pFormatCtx, &packet) < 0)
        {
            mIsReadFinished = true;
            mIsReadError = true;

            ///唤醒等待中的线程
            m_cond_video.notify_all();
            m_cond_audio.notify_all();

            // printf("%s av_read_frame failed %s mIsVideoThreadFinished=%d mIsAudioThreadFinished=%d mIsQuit=%d m_video_pkt_list.size()=%d m_audio_pkt_list.size()=%d \n", 
            // __FUNCTION__, file_path, mIsVideoThreadFinished, mIsAudioThreadFinished, mIsQuit, m_video_pkt_list.size(), m_audio_pkt_list.size());
//            if (mIsQuit)
//            {
//                break; //解码线程也执行完了 可以退出了
//            }

            mSleep(10);
            continue;
        }
        else
        {
            mIsReadFinished = false;
        }
// qDebug("%s mIsQuit=%d mIsPause=%d packet.stream_index=%d \n", __FUNCTION__, mIsQuit, mIsPause, packet.stream_index);
// fprintf(stderr, "%s mIsQuit=%d mIsPause=%d packet.stream_index=%d videoStream=%d audioStream=%d \n", __FUNCTION__, mIsQuit, mIsPause, packet.stream_index, videoStream, audioStream);
        if (packet.stream_index == videoStream)
        {
            if (bsf_ctx)
            {
                if (av_bsf_send_packet(bsf_ctx, &packet) < 0) 
                {
                    // JLOGE("send_packet error");
                    av_packet_unref(&packet);
                    continue;
                }

                while (av_bsf_receive_packet(bsf_ctx, &packet) == 0) 
                {
                    inputVideoQuene(packet);
                    //这里我们将数据存入队列 因此不调用 av_free_packet 释放
                    //av_packet_unref(&packet);
                }
            }
            else
            {
                inputVideoQuene(packet);
            }
        }
        else if(packet.stream_index == audioStream)
        {
            if (mIsAudioThreadFinished)
            { ///SDL没有打开，则音频数据直接释放
                av_packet_unref(&packet);
            }
            else
            {
                inputAudioQuene(packet);
                //这里我们将数据存入队列 因此不调用 av_free_packet 释放
            }

        }
        else
        {
            // Free the packet that was allocated by av_read_frame
            av_packet_unref(&packet);
        }
// fprintf(stderr, "%s:%d \n", __FILE__, __LINE__);
    }

    if (bsf_ctx)
    {
        //flush
        if (av_bsf_send_packet(bsf_ctx, NULL) < 0)
        {
            // fprintf(stderr, "send_packet error");
            // break;
        }

        AVPacket pkt;
        while (av_bsf_receive_packet(bsf_ctx, &pkt) == 0)
        {
            av_packet_unref(&pkt);
        }

        // av_bsf_free(&bsf_ctx);

    //    printf("bsf_ctx fflush finish\n");
    }

    ///文件读取结束 跳出循环的情况
    ///等待播放完毕
    while (!mIsQuit)
    {
        mSleep(100);
    }

end:

    clearAudioQuene();
    clearVideoQuene();

    if (m_state != VideoPlayer::Stop) //不是外部调用的stop 是正常播放结束
    {
        stop(false);
    }

    ///唤醒等待中的线程
    m_cond_video.notify_all();
    m_cond_audio.notify_all();
    while((mVideoStream != nullptr && !mIsVideoThreadFinished) || (mAudioStream != nullptr && !mIsAudioThreadFinished))
    {
fprintf(stderr, "%s:%d mIsVideoThreadFinished=%d mIsAudioThreadFinished=%d \n", __FILE__, __LINE__, mIsVideoThreadFinished, mIsAudioThreadFinished);
        mSleep(10);
    } //确保视频线程结束后 再销毁队列
#ifdef USE_PCM_PLAYER
    m_pcm_player->stopPlay();
    m_pcm_player->clearFrame();
#endif
    if (bsf_ctx)
    {
        av_bsf_free(&bsf_ctx);
        bsf_ctx = nullptr;
    }

    if (swrCtx != nullptr)
    {
        swr_free(&swrCtx);
        swrCtx = nullptr;
    }

    if (aFrame != nullptr)
    {
        av_frame_free(&aFrame);
        aFrame = nullptr;
    }

    if (aFrame_ReSample != nullptr)
    {
        av_frame_free(&aFrame_ReSample);
        aFrame_ReSample = nullptr;
    }

    if (aCodecCtx != nullptr)
    {
        avcodec_close(aCodecCtx);
        aCodecCtx = nullptr;
    }

    if (pCodecCtx != nullptr)
    {
        avcodec_close(pCodecCtx);
        pCodecCtx = nullptr;
    }

    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);

    // SDL_Quit();

    if (mIsReadError && !mIsQuit)
    {
        doPlayerStateChanged(VideoPlayer::ReadError, mVideoStream != nullptr, mAudioStream != nullptr);
    }
    else
    {
        doPlayerStateChanged(VideoPlayer::Stop, mVideoStream != nullptr, mAudioStream != nullptr);
    }

    mIsReadThreadFinished = true;

fprintf(stderr, "%s finished \n", __FUNCTION__);
}

bool VideoPlayer::inputVideoQuene(const AVPacket &pkt)
{
//    if (av_dup_packet((AVPacket*)&pkt) < 0)
//    {
//        return false;
//    }

    std::lock_guard<std::mutex> lck(m_mutex_video);
    m_video_pkt_list.push_back(pkt);
    m_cond_video.notify_all();

    return true;
}

void VideoPlayer::clearVideoQuene()
{
    std::lock_guard<std::mutex> lck(m_mutex_video);
    for (AVPacket pkt : m_video_pkt_list)
    {
//        av_free_packet(&pkt);
        av_packet_unref(&pkt);
    }
    m_video_pkt_list.clear();
}

bool VideoPlayer::inputAudioQuene(const AVPacket &pkt)
{
//    if (av_dup_packet((AVPacket*)&pkt) < 0)
//    {
//        return false;
//    }
    std::lock_guard<std::mutex> lck(m_mutex_audio);
    m_audio_pkt_list.push_back(pkt);
    m_cond_audio.notify_all();

    return true;
}

void VideoPlayer::clearAudioQuene()
{
    std::lock_guard<std::mutex> lck(m_mutex_audio);
    for (AVPacket pkt : m_audio_pkt_list)
    {
//        av_free_packet(&pkt);
        av_packet_unref(&pkt);
    }
    m_audio_pkt_list.clear();
#ifdef USE_PCM_PLAYER
    m_pcm_player->clearFrame();
#endif
}

///当使用界面类继承了本类之后，以下函数不会执行

///打开文件失败
void VideoPlayer::doOpenVideoFileFailed(const int &code)
{
    fprintf(stderr, "%s \n", __FUNCTION__);

    if (m_event_handle != nullptr)
    {
        m_event_handle->onOpenVideoFileFailed(code);
    }

}

///打开sdl失败的时候回调此函数
void VideoPlayer::doOpenSdlFailed(const int &code)
{
    fprintf(stderr, "%s \n", __FUNCTION__);

    if (m_event_handle != nullptr)
    {
        m_event_handle->onOpenSdlFailed(code);
    }
}

///获取到视频时长的时候调用此函数
void VideoPlayer::doTotalTimeChanged(const int64_t &uSec)
{
    fprintf(stderr, "%s \n", __FUNCTION__);

    if (m_event_handle != nullptr)
    {
        m_event_handle->onTotalTimeChanged(uSec);
    }
}

///播放器状态改变的时候回调此函数
void VideoPlayer::doPlayerStateChanged(const VideoPlayer::State &state, const bool &hasVideo, const bool &hasAudio)
{
    fprintf(stderr, "%s state=%d\n", __FUNCTION__, state);

    if (m_event_handle != nullptr)
    {
        m_event_handle->onPlayerStateChanged(state, hasVideo, hasAudio);
    }

}

///显示视频数据，此函数不宜做耗时操作，否则会影响播放的流畅性。
void VideoPlayer::doDisplayVideo(const uint8_t *yuv420Buffer, const int &width, const int &height)
{
//    fprintf(stderr, "%s width=%d height=%d \n", __FUNCTION__, width, height);
    if (m_event_handle != nullptr)
    {
        VideoRawFramePtr videoFrame = std::make_shared<VideoRawFrame>();

        videoFrame->initBuffer(width, height, VideoRawFrame::FRAME_TYPE_YUV420P);
        videoFrame->setFramebuf(yuv420Buffer);

        m_event_handle->onDisplayVideo(videoFrame);
    }
}
