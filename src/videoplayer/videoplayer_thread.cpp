/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "videoplayer_thread.h"

#include <stdio.h>

#include <QDebug>

#define SDL_AUDIO_BUFFER_SIZE 1024
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

#define FLUSH_DATA "FLUSH"

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
    q->size = 0;
    q->nb_packets = 0;
    q->first_pkt = NULL;
    q->last_pkt = NULL;
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

    AVPacketList *pkt1;
    if (av_dup_packet(pkt) < 0) {
        return -1;
    }
    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for (;;) {

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }

    }

    SDL_UnlockMutex(q->mutex);
    return ret;
}

static void packet_queue_flush(PacketQueue *q)
{
    AVPacketList *pkt, *pkt1;

    SDL_LockMutex(q->mutex);
    for(pkt = q->first_pkt; pkt != NULL; pkt = pkt1)
    {
        pkt1 = pkt->next;

        if(pkt1->pkt.data != (uint8_t *)"FLUSH")
        {

        }
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);

    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    SDL_UnlockMutex(q->mutex);
}

static int audio_decode_frame(VideoState *is, double *pts_ptr)
{
    int len1, len2, decoded_data_size;
    AVPacket *pkt = &is->audio_pkt;
    int got_frame = 0;
    int64_t dec_channel_layout;
    int wanted_nb_samples, resampled_data_size, n;

    double pts;

    for (;;) {

        while (is->audio_pkt_size > 0) {

            if (is->isPause == true) //判断暂停
            {
                SDL_Delay(10);
                continue;
            }

            if (!is->audio_frame) {
                if (!(is->audio_frame = avcodec_alloc_frame())) {
                    return AVERROR(ENOMEM);
                }
            } else
                avcodec_get_frame_defaults(is->audio_frame);

            len1 = avcodec_decode_audio4(is->audio_st->codec, is->audio_frame,
                    &got_frame, pkt);
            if (len1 < 0) {
                // error, skip the frame
                is->audio_pkt_size = 0;
                break;
            }

            is->audio_pkt_data += len1;
            is->audio_pkt_size -= len1;

            if (!got_frame)
                continue;

            /* 计算解码出来的桢需要的缓冲大小 */
            decoded_data_size = av_samples_get_buffer_size(NULL,
                    is->audio_frame->channels, is->audio_frame->nb_samples,
                    (AVSampleFormat)is->audio_frame->format, 1);

            dec_channel_layout =
                    (is->audio_frame->channel_layout
                            && is->audio_frame->channels
                                    == av_get_channel_layout_nb_channels(
                                            is->audio_frame->channel_layout)) ?
                            is->audio_frame->channel_layout :
                            av_get_default_channel_layout(
                                    is->audio_frame->channels);

            wanted_nb_samples = is->audio_frame->nb_samples;

            if (is->audio_frame->format != is->audio_src_fmt
                    || dec_channel_layout != is->audio_src_channel_layout
                    || is->audio_frame->sample_rate != is->audio_src_freq
                    || (wanted_nb_samples != is->audio_frame->nb_samples
                            && !is->swr_ctx)) {
                if (is->swr_ctx)
                    swr_free(&is->swr_ctx);
                is->swr_ctx = swr_alloc_set_opts(NULL,
                        is->audio_tgt_channel_layout, (AVSampleFormat)is->audio_tgt_fmt,
                        is->audio_tgt_freq, dec_channel_layout,
                        (AVSampleFormat)is->audio_frame->format, is->audio_frame->sample_rate,
                        0, NULL);
                if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
                    //fprintf(stderr,"swr_init() failed\n");
                    break;
                }
                is->audio_src_channel_layout = dec_channel_layout;
                is->audio_src_channels = is->audio_st->codec->channels;
                is->audio_src_freq = is->audio_st->codec->sample_rate;
                is->audio_src_fmt = is->audio_st->codec->sample_fmt;
            }

            /* 这里我们可以对采样数进行调整，增加或者减少，一般可以用来做声画同步 */
            if (is->swr_ctx) {
                const uint8_t **in =
                        (const uint8_t **) is->audio_frame->extended_data;
                uint8_t *out[] = { is->audio_buf2 };
                if (wanted_nb_samples != is->audio_frame->nb_samples) {
                    if (swr_set_compensation(is->swr_ctx,
                            (wanted_nb_samples - is->audio_frame->nb_samples)
                                    * is->audio_tgt_freq
                                    / is->audio_frame->sample_rate,
                            wanted_nb_samples * is->audio_tgt_freq
                                    / is->audio_frame->sample_rate) < 0) {
                        //fprintf(stderr,"swr_set_compensation() failed\n");
                        break;
                    }
                }

                len2 = swr_convert(is->swr_ctx, out,
                        sizeof(is->audio_buf2) / is->audio_tgt_channels
                                / av_get_bytes_per_sample(is->audio_tgt_fmt),
                        in, is->audio_frame->nb_samples);
                if (len2 < 0) {
                    //fprintf(stderr,"swr_convert() failed\n");
                    break;
                }
                if (len2
                        == sizeof(is->audio_buf2) / is->audio_tgt_channels
                                / av_get_bytes_per_sample(is->audio_tgt_fmt)) {
                    //fprintf(stderr,"warning: audio buffer is probably too small\n");
                    swr_init(is->swr_ctx);
                }
                is->audio_buf = is->audio_buf2;
                resampled_data_size = len2 * is->audio_tgt_channels
                        * av_get_bytes_per_sample(is->audio_tgt_fmt);
            } else {
                resampled_data_size = decoded_data_size;
                is->audio_buf = is->audio_frame->data[0];
            }

            pts = is->audio_clock;
            *pts_ptr = pts;
            n = 2 * is->audio_st->codec->channels;
            is->audio_clock += (double) resampled_data_size
                    / (double) (n * is->audio_st->codec->sample_rate);


            if (is->seek_flag_audio)
            {
                //发生了跳转 则跳过关键帧到目的时间的这几帧
               if (is->audio_clock < is->seek_time)
               {
                   break;
               }
               else
               {
                   is->seek_flag_audio = 0;
               }
            }


            // We have data, return it and come back for more later
            return resampled_data_size;
        }

        if (pkt->data)
            av_free_packet(pkt);
        memset(pkt, 0, sizeof(*pkt));

        if (is->quit)
            return -1;

        if (is->isPause == true) //判断暂停
        {
            SDL_Delay(10);
            continue;
        }

        if (packet_queue_get(&is->audioq, pkt, 0) <= 0)
        {
            return -1;
        }

        //收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
        if(strcmp((char*)pkt->data,FLUSH_DATA) == 0)
        {
            avcodec_flush_buffers(is->audio_st->codec);
            av_free_packet(pkt);
            continue;
        }

        is->audio_pkt_data = pkt->data;
        is->audio_pkt_size = pkt->size;

        /* if update, update the audio clock w/pts */
        if (pkt->pts != AV_NOPTS_VALUE) {
            is->audio_clock = av_q2d(is->audio_st->time_base) * pkt->pts;
        }
    }

    return 0;
}


static void audio_callback(void *userdata, Uint8 *stream, int len) {
    VideoState *is = (VideoState *) userdata;

    int len1, audio_data_size;

    double pts;

    /*   len是由SDL传入的SDL缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据 */
    while (len > 0) {
        /*  audio_buf_index 和 audio_buf_size 标示我们自己用来放置解码出来的数据的缓冲区，*/
        /*   这些数据待copy到SDL缓冲区， 当audio_buf_index >= audio_buf_size的时候意味着我*/
        /*   们的缓冲为空，没有数据可供copy，这时候需要调用audio_decode_frame来解码出更
         /*   多的桢数据 */
        if (is->audio_buf_index >= is->audio_buf_size) {

            audio_data_size = audio_decode_frame(is, &pts);

            /* audio_data_size < 0 标示没能解码出数据，我们默认播放静音 */
            if (audio_data_size < 0) {
                /* silence */
                is->audio_buf_size = 1024;
                /* 清零，静音 */
                if (is->audio_buf == NULL) return;
                memset(is->audio_buf, 0, is->audio_buf_size);
            } else {
                is->audio_buf_size = audio_data_size;
            }
            is->audio_buf_index = 0;
        }
        /*  查看stream可用空间，决定一次copy多少数据，剩下的下次继续copy */
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }

        if (is->audio_buf == NULL) return;

        memcpy(stream, (uint8_t *) is->audio_buf + is->audio_buf_index, len1);
//        SDL_MixAudio(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, len1, 50);

//        SDL_MixAudioFormat(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, 50);

        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }

}

static double get_audio_clock(VideoState *is)
{
    double pts;
    int hw_buf_size, bytes_per_sec, n;

    pts = is->audio_clock; /* maintained in the audio thread */
    hw_buf_size = is->audio_buf_size - is->audio_buf_index;
    bytes_per_sec = 0;
    n = is->audio_st->codec->channels * 2;
    if(is->audio_st)
    {
        bytes_per_sec = is->audio_st->codec->sample_rate * n;
    }
    if(bytes_per_sec)
    {
        pts -= (double)hw_buf_size / bytes_per_sec;
    }
    return pts;
}

static double synchronize_video(VideoState *is, AVFrame *src_frame, double pts) {

    double frame_delay;

    if (pts != 0) {
        /* if we have pts, set video clock to it */
        is->video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = is->video_clock;
    }
    /* update the video clock */
    frame_delay = av_q2d(is->video_st->codec->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    is->video_clock += frame_delay;
    return pts;
}

int audio_stream_component_open(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecContext *codecCtx;
    AVCodec *codec;
    SDL_AudioSpec wanted_spec, spec;
    int64_t wanted_channel_layout = 0;
    int wanted_nb_channels;
    /*  SDL支持的声道数为 1, 2, 4, 6 */
    /*  后面我们会使用这个数组来纠正不支持的声道数目 */
    const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };

    if (stream_index < 0 || stream_index >= ic->nb_streams) {
        return -1;
    }

    codecCtx = ic->streams[stream_index]->codec;
    wanted_nb_channels = codecCtx->channels;
    if (!wanted_channel_layout
            || wanted_nb_channels
                    != av_get_channel_layout_nb_channels(
                            wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(
                wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }

    wanted_spec.channels = av_get_channel_layout_nb_channels(
            wanted_channel_layout);
    wanted_spec.freq = codecCtx->sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        //fprintf(stderr,"Invalid sample rate or channel count!\n");
        return -1;
    }
    wanted_spec.format = AUDIO_S16SYS; // 具体含义请查看“SDL宏定义”部分
    wanted_spec.silence = 0;            // 0指示静音
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;  // 自定义SDL缓冲区大小
    wanted_spec.callback = audio_callback;        // 音频解码的关键回调函数
    wanted_spec.userdata = is;                    // 传给上面回调函数的外带数据

//    SDL_AudioDeviceID audioID = 1;// = SDL_OpenAudioDevice("",0,&wanted_spec, &spec,1);
//    int num = SDL_GetNumAudioDevices(0);
//    for (int i=0;i<num;i++)
//    {
//        qDebug()<<SDL_GetAudioDeviceName(i,0);
//    }

//    ///  打开SDL播放设备 - 开始
//    SDL_LockAudio();
//    SDL_AudioSpec spec;
//    SDL_AudioSpec wanted_spec;
//    wanted_spec.freq = aCodecCtx->sample_rate;
//    wanted_spec.format = AUDIO_S16SYS;
//    wanted_spec.channels = aCodecCtx->channels;
//    wanted_spec.silence = 0;
//    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
//    wanted_spec.callback = audio_callback;
//    wanted_spec.userdata = &mVideoState;
//    if(SDL_OpenAudio(&wanted_spec, &spec) < 0)
//    {
//        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
//        return;
//    }
//    SDL_UnlockAudio();
//    SDL_PauseAudio(0);
//    ///  打开SDL播放设备 - 结束

    /*  打开音频设备，这里使用一个while来循环尝试打开不同的声道数(由上面 */
    /*  next_nb_channels数组指定）直到成功打开，或者全部失败 */
//    while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
    do {

        is->audioID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0,0),0,&wanted_spec, &spec,0);

//        qDebug()<<"audioID"<<audioID;

//        if (audioID >= 1) break;

        fprintf(stderr,"SDL_OpenAudio (%d channels): %s\n",wanted_spec.channels, SDL_GetError());
        qDebug()<<QString("SDL_OpenAudio (%1 channels): %2").arg(wanted_spec.channels).arg(SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            fprintf(stderr,"No more channel combinations to tyu, audio open failed\n");
//            return -1;
            break;
        }
        wanted_channel_layout = av_get_default_channel_layout(
                wanted_spec.channels);
    }while(is->audioID == 0);

    /* 检查实际使用的配置（保存在spec,由SDL_OpenAudio()填充） */
    if (spec.format != AUDIO_S16SYS) {
        fprintf(stderr,"SDL advised audio format %d is not supported!\n",spec.format);
        return -1;
    }

    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            fprintf(stderr,"SDL advised channel count %d is not supported!\n",spec.channels);
            return -1;
        }
    }

    is->audio_hw_buf_size = spec.size;

    /* 把设置好的参数保存到大结构中 */
    is->audio_src_fmt = is->audio_tgt_fmt = AV_SAMPLE_FMT_S16;
    is->audio_src_freq = is->audio_tgt_freq = spec.freq;
    is->audio_src_channel_layout = is->audio_tgt_channel_layout =
            wanted_channel_layout;
    is->audio_src_channels = is->audio_tgt_channels = spec.channels;

    codec = avcodec_find_decoder(codecCtx->codec_id);
    if (!codec || (avcodec_open2(codecCtx, codec, NULL) < 0)) {
        fprintf(stderr,"Unsupported codec!\n");
        return -1;
    }
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (codecCtx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
//        is->audioStream = stream_index;
        is->audio_st = ic->streams[stream_index];
        is->audio_buf_size = 0;
        is->audio_buf_index = 0;
        memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
        packet_queue_init(&is->audioq);
//        SDL_PauseAudio(0); // 开始播放静音
        SDL_PauseAudioDevice(is->audioID,0);
        break;
    default:
        break;
    }

    return 0;
}

int video_thread(void *arg)
{
    VideoState *is = (VideoState *) arg;
    AVPacket pkt1, *packet = &pkt1;

    int ret, got_picture, numBytes;

    double video_pts = 0; //当前视频的pts
    double audio_pts = 0; //音频pts


    ///解码视频相关
    AVFrame *pFrame, *pFrameRGB;
    uint8_t *out_buffer_rgb; //解码后的rgb数据
    struct SwsContext *img_convert_ctx;  //用于解码后的视频格式转换

    AVCodecContext *pCodecCtx = is->video_st->codec; //视频解码器

    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();

    ///这里我们改成了 将解码后的YUV数据转换成RGB32
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
            pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
            PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

    numBytes = avpicture_get_size(PIX_FMT_RGB32, pCodecCtx->width,pCodecCtx->height);

    out_buffer_rgb = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *) pFrameRGB, out_buffer_rgb, PIX_FMT_RGB32,
            pCodecCtx->width, pCodecCtx->height);

    while(1)
    {
        if (is->quit)
        {
            break;
        }

        if (is->isPause == true) //判断暂停
        {
            SDL_Delay(10);
            continue;
        }

        if (packet_queue_get(&is->videoq, packet, 0) <= 0)
        {
            if (is->readFinished)
            {//队列里面没有数据了且读取完毕了
                break;
            }
            else
            {
                SDL_Delay(1); //队列只是暂时没有数据而已
                continue;
            }
        }

        //收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
        if(strcmp((char*)packet->data,FLUSH_DATA) == 0)
        {
            avcodec_flush_buffers(is->video_st->codec);
            av_free_packet(packet);
            continue;
        }

        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet);

        if (ret < 0) {
            qDebug()<<"decode error.\n";
            av_free_packet(packet);
            continue;
        }

        if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque&& *(uint64_t*) pFrame->opaque != AV_NOPTS_VALUE)
        {
            video_pts = *(uint64_t *) pFrame->opaque;
        }
        else if (packet->dts != AV_NOPTS_VALUE)
        {
            video_pts = packet->dts;
        }
        else
        {
            video_pts = 0;
        }

        video_pts *= av_q2d(is->video_st->time_base);
        video_pts = synchronize_video(is, pFrame, video_pts);

        if (is->seek_flag_video)
        {
            //发生了跳转 则跳过关键帧到目的时间的这几帧
           if (video_pts < is->seek_time)
           {
               av_free_packet(packet);
               continue;
           }
           else
           {
               is->seek_flag_video = 0;
           }
        }

        while(1)
        {
            if (is->quit)
            {
                break;
            }

            audio_pts = is->audio_clock;

            //主要是 跳转的时候 我们把video_clock设置成0了
            //因此这里需要更新video_pts
            //否则当从后面跳转到前面的时候 会卡在这里
            video_pts = is->video_clock;


            if (video_pts <= audio_pts) break;

            int delayTime = (video_pts - audio_pts) * 1000;

            delayTime = delayTime > 5 ? 5:delayTime;

            SDL_Delay(delayTime);
        }

        if (got_picture) {
            sws_scale(img_convert_ctx,
                    (uint8_t const * const *) pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
                    pFrameRGB->linesize);

            //把这个RGB数据 用QImage加载
            QImage tmpImg((uchar *)out_buffer_rgb,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB32);
            QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
            is->player->disPlayVideo(image); //调用激发信号的函数
        }

        av_free_packet(packet);

    }

    av_free(pFrame);
    av_free(pFrameRGB);
    av_free(out_buffer_rgb);

    if (!is->quit)
    {
        is->quit = true;
    }

    is->videoThreadFinished = true;

    return 0;
}


VideoPlayer_Thread::VideoPlayer_Thread()
{

    mVideoWidget = new VideoPlayer_ShowVideoWidget;
    connect(this,SIGNAL(sig_GetOneFrame(QImage)),mVideoWidget,SLOT(slotGetOneFrame(QImage)));

    mPlayerState = Stop;
}

VideoPlayer_Thread::~VideoPlayer_Thread()
{

}

bool VideoPlayer_Thread::setFileName(QString path)
{
    if (mPlayerState != Stop)
    {
        return false;
    }

    mFileName = path;

    mPlayerState = Playing;
    emit sig_StateChanged(Playing);

    this->start(); //启动线程

    return true;

}

bool VideoPlayer_Thread::play()
{
    mVideoState.isPause = false;

    if (mPlayerState != Pause)
    {
        return false;
    }

    mPlayerState = Playing;
    emit sig_StateChanged(Playing);

    return true;
}

bool VideoPlayer_Thread::pause()
{
    mVideoState.isPause = true;

    if (mPlayerState != Playing)
    {
        return false;
    }

    mPlayerState = Pause;

    emit sig_StateChanged(Pause);

    return true;
}

bool VideoPlayer_Thread::stop(bool isWait)
{
    if (mPlayerState == Stop)
    {
        return false;
    }

    mVideoState.quit = 1;

    mPlayerState = Stop;
    emit sig_StateChanged(Stop);

    if (isWait)
    {
        while(!mVideoState.readThreadFinished || !mVideoState.videoThreadFinished)
        {
            SDL_Delay(10);
        }
    }

    ///关闭SDL音频播放设备
    if (mVideoState.audioID != 0)
    {
        SDL_LockAudio();
        SDL_PauseAudioDevice(mVideoState.audioID,1);
        SDL_UnlockAudio();

        mVideoState.audioID = 0;
    }


    return true;
}

void VideoPlayer_Thread::seek(int64_t pos)
{
    if(!mVideoState.seek_req)
    {
        mVideoState.seek_pos = pos;
        mVideoState.seek_req = 1;
    }
}

double VideoPlayer_Thread::getCurrentTime()
{
    return mVideoState.audio_clock;
}

int64_t VideoPlayer_Thread::getTotalTime()
{
    return mVideoState.ic->duration;
}

void VideoPlayer_Thread::disPlayVideo(QImage img)
{
    emit sig_GetOneFrame(img);  //发送信号
}

void VideoPlayer_Thread::run()
{
    char file_path[1280] = {0};;

    strcpy(file_path,mFileName.toUtf8().data());

    memset(&mVideoState,0,sizeof(VideoState)); //为了安全起见  先将结构体的数据初始化成0了

    VideoState *is = &mVideoState;

    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    AVCodecContext *aCodecCtx;
    AVCodec *aCodec;

    int audioStream ,videoStream, i;


    //Allocate an AVFormatContext.
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, file_path, NULL, NULL) != 0) {
        printf("can't open the file. \n");
        return;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Could't find stream infomation.\n");
        return;
    }

    videoStream = -1;
    audioStream = -1;

    ///循环查找视频中包含的流信息，
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
        }
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO  && audioStream < 0)
        {
            audioStream = i;
        }
    }

    ///如果videoStream为-1 说明没有找到视频流
    if (videoStream == -1) {
        printf("Didn't find a video stream.\n");
        return;
    }

    if (audioStream == -1) {
        printf("Didn't find a audio stream.\n");
        return;
    }

    is->ic = pFormatCtx;
    is->videoStream = videoStream;
    is->audioStream = audioStream;

    emit sig_TotalTimeChanged(getTotalTime());

    if (audioStream >= 0) {
        /* 所有设置SDL音频流信息的步骤都在这个函数里完成 */
        audio_stream_component_open(&mVideoState, audioStream);
    }

    ///查找音频解码器
    aCodecCtx = pFormatCtx->streams[audioStream]->codec;
    aCodec = avcodec_find_decoder(aCodecCtx->codec_id);

    if (aCodec == NULL) {
        printf("ACodec not found.\n");
        return;
    }

    ///打开音频解码器
    if (avcodec_open2(aCodecCtx, aCodec, NULL) < 0) {
        printf("Could not open audio codec.\n");
        return;
    }

    is->audio_st = pFormatCtx->streams[audioStream];

    ///查找视频解码器
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (pCodec == NULL) {
        printf("PCodec not found.\n");
        return;
    }

    ///打开视频解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open video codec.\n");
        return;
    }

    is->video_st = pFormatCtx->streams[videoStream];
    packet_queue_init(&is->videoq);

    ///创建一个线程专门用来解码视频
    is->video_tid = SDL_CreateThread(video_thread, "video_thread", &mVideoState);


    is->player = this;

//    int y_size = pCodecCtx->width * pCodecCtx->height;

    AVPacket *packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet 用来存放读取的视频
//    av_new_packet(packet, y_size); //av_read_frame 会给它分配空间 因此这里不需要了

    av_dump_format(pFormatCtx, 0, file_path, 0); //输出视频信息

    while (1)
    {
        if (is->quit)
        {
            //停止播放了
            break;
        }

        if (is->seek_req)
        {
            int stream_index = -1;
            int64_t seek_target = is->seek_pos;

            if (is->videoStream >= 0)
                stream_index = is->videoStream;
            else if (is->audioStream >= 0)
                stream_index = is->audioStream;

            AVRational aVRational = {1, AV_TIME_BASE};
            if (stream_index >= 0) {
                seek_target = av_rescale_q(seek_target, aVRational,
                        pFormatCtx->streams[stream_index]->time_base);
            }

            if (av_seek_frame(is->ic, stream_index, seek_target, AVSEEK_FLAG_BACKWARD) < 0) {
                fprintf(stderr, "%s: error while seeking\n",is->ic->filename);
            } else {
                if (is->audioStream >= 0) {
                    AVPacket *packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
                    av_new_packet(packet, 10);
                    strcpy((char*)packet->data,FLUSH_DATA);
                    packet_queue_flush(&is->audioq); //清除队列
                    packet_queue_put(&is->audioq, packet); //往队列中存入用来清除的包
                }
                if (is->videoStream >= 0) {
                    AVPacket *packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
                    av_new_packet(packet, 10);
                    strcpy((char*)packet->data,FLUSH_DATA);
                    packet_queue_flush(&is->videoq); //清除队列
                    packet_queue_put(&is->videoq, packet); //往队列中存入用来清除的包
                    is->video_clock = 0;
                }
            }
            is->seek_req = 0;
            is->seek_time = is->seek_pos / 1000000.0;
            is->seek_flag_audio = 1;
            is->seek_flag_video = 1;
        }

        //这里做了个限制  当队列里面的数据超过某个大小的时候 就暂停读取  防止一下子就把视频读完了，导致的空间分配不足
        /* 这里audioq.size是指队列中的所有数据包带的音频数据的总量或者视频数据总量，并不是包的数量 */
        //这个值可以稍微写大一些
        if (is->audioq.size > MAX_AUDIO_SIZE || is->videoq.size > MAX_VIDEO_SIZE) {
            SDL_Delay(10);
            continue;
        }

        if (is->isPause == true)
        {
            SDL_Delay(10);
            continue;
        }

        if (av_read_frame(pFormatCtx, packet) < 0)
        {
            is->readFinished = true;

            if (is->quit)
            {
                break; //解码线程也执行完了 可以退出了
            }

            SDL_Delay(10);
            continue;
        }

        if (packet->stream_index == videoStream)
        {
            packet_queue_put(&is->videoq, packet);
            //这里我们将数据存入队列 因此不调用 av_free_packet 释放
        }
        else if( packet->stream_index == audioStream )
        {
            packet_queue_put(&is->audioq, packet);
            //这里我们将数据存入队列 因此不调用 av_free_packet 释放
        }
        else
        {
            // Free the packet that was allocated by av_read_frame
            av_free_packet(packet);
        }
    }

    ///文件读取结束 跳出循环的情况
    ///等待播放完毕
    while (!is->quit) {
        SDL_Delay(100);
    }

    stop();

    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    is->readThreadFinished = true;

}
