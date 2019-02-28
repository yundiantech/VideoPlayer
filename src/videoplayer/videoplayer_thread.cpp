#include "videoplayer_thread.h"

#include <stdio.h>

#include <QDebug>

#define SDL_AUDIO_BUFFER_SIZE 1024
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

#define FLUSH_DATA "FLUSH"

//清空
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

static void packet_queue_deinit(PacketQueue *q) {
    packet_queue_flush(q);
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}

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
                return -1;
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
        {
            packet_queue_flush(&is->audioq);
            return -1;
        }

        if (is->isPause == true) //判断暂停
        {
            return -1;
//            SDL_Delay(10);
//            continue;
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

typedef     signed char         int8_t;
typedef     signed short        int16_t;
typedef     signed int          int32_t;
typedef     unsigned char       uint8_t;
typedef     unsigned short      uint16_t;
typedef     unsigned int        uint32_t;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

void RaiseVolume(char* buf, int size, int uRepeat, double vol)//buf为需要调节音量的音频数据块首地址指针，size为长度，uRepeat为重复次数，通常设为1，vol为增益倍数,可以小于1
{
    if (!size)
    {
        return;
    }
    for (int i = 0; i < size; i += 2)
    {
        short wData;
        wData = MAKEWORD(buf[i], buf[i + 1]);
        long dwData = wData;
        for (int j = 0; j < uRepeat; j++)
        {
            dwData = dwData * vol;
            if (dwData < -0x8000)
            {
                dwData = -0x8000;
            }
            else if (dwData > 0x7FFF)
            {
                dwData = 0x7FFF;
            }
        }
        wData = LOWORD(dwData);
        buf[i] = LOBYTE(wData);
        buf[i + 1] = HIBYTE(wData);
    }
}

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    VideoState *is = (VideoState *) userdata;

    int len1, audio_data_size;

    double pts;
//qDebug()<<__FUNCTION__<<"111...";
    /*   len是由SDL传入的SDL缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据 */
    while (len > 0) {
        /*  audio_buf_index 和 audio_buf_size 标示我们自己用来放置解码出来的数据的缓冲区，*/
        /*   这些数据待copy到SDL缓冲区， 当audio_buf_index >= audio_buf_size的时候意味着我*/
        /*   们的缓冲为空，没有数据可供copy，这时候需要调用audio_decode_frame来解码出更
         /*   多的桢数据 */
//        qDebug()<<__FUNCTION__<<is->audio_buf_index<<is->audio_buf_size;
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

        if (is->isMute || is->isNeedPause) //静音 或者 是在暂停的时候跳转了
        {
            memset(is->audio_buf + is->audio_buf_index, 0, len1);
        }
        else
        {
            RaiseVolume((char*)is->audio_buf + is->audio_buf_index, len1, 1, is->mVolume);
        }

        memcpy(stream, (uint8_t *) is->audio_buf + is->audio_buf_index, len1);


//        SDL_memset(stream, 0x0, len);// make sure this is silence.
//        SDL_MixAudio(stream, (uint8_t *) is->audio_buf + is->audio_buf_index, len1, SDL_MIX_MAXVOLUME);

//        SDL_MixAudio(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, len1, 50);
//        SDL_MixAudioFormat(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, 50);

        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
//qDebug()<<__FUNCTION__<<"222...";
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

    int64_t wanted_channel_layout = 0;
    int wanted_nb_channels;

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

    /* 把设置好的参数保存到大结构中 */
    is->audio_src_fmt = is->audio_tgt_fmt = AV_SAMPLE_FMT_S16;
    is->audio_src_freq = is->audio_tgt_freq = 44100;
    is->audio_src_channel_layout = is->audio_tgt_channel_layout =
            wanted_channel_layout;
    is->audio_src_channels = is->audio_tgt_channels = 2;

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
//        packet_queue_init(&is->audioq);
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
        {qDebug()<<__FUNCTION__<<"quit!";
            packet_queue_flush(&is->videoq); //清空队列
            break;
        }
//qDebug()<<__FUNCTION__<<"000!";
        if (is->isPause == true) //判断暂停
        {
            SDL_Delay(10);
            continue;
        }
//qDebug()<<__FUNCTION__<<"000!";
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
//qDebug()<<__FUNCTION__<<"111!";
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

            if (is->readFinished && is->audioq.size == 0)
            {//读取完了 且音频数据也播放完了 就剩下视频数据了  直接显示出来了 不用同步了
                break;
            }

            audio_pts = is->audio_clock;

            //主要是 跳转的时候 我们把video_clock设置成0了
            //因此这里需要更新video_pts
            //否则当从后面跳转到前面的时候 会卡在这里
            video_pts = is->video_clock;

//qDebug()<<__FUNCTION__<<video_pts<<audio_pts;
            if (video_pts <= audio_pts) break;

            int delayTime = (video_pts - audio_pts) * 1000;

            delayTime = delayTime > 5 ? 5:delayTime;

            if (!is->isNeedPause)
                SDL_Delay(delayTime);
        }

        if (got_picture) {
            sws_scale(img_convert_ctx,
                    (uint8_t const * const *) pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
                    pFrameRGB->linesize);

            //把这个RGB数据 用QImage加载
            QImage tmpImg((uchar *)out_buffer_rgb,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB32);
//            QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
			QImage image = tmpImg.convertToFormat(QImage::Format_RGB888,Qt::NoAlpha); //去掉透明的部分 有些奇葩的视频会透明
            is->player->disPlayVideo(image); //调用激发信号的函数

            if (is->isNeedPause)
            {
                is->isPause = true;
                is->isNeedPause = false;
            }

        }

        av_free_packet(packet);

    }
qDebug()<<__FUNCTION__<<"quit 222!";
    av_free(pFrame);
    av_free(pFrameRGB);
    av_free(out_buffer_rgb);

    sws_freeContext(img_convert_ctx);

    if (!is->quit)
    {
        is->quit = true;
    }

    is->videoThreadFinished = true;
qDebug()<<__FUNCTION__<<"finished!";

//qDebug()<<__FUNCTION__<<"videoThreadFinished"<<is->videoThreadFinished;

    return 0;
}

VideoPlayer_Thread::VideoPlayer_Thread()
{
//    mVideoState.readThreadFinished = true;
//    mVideoState.videoThreadFinished = true;

    memset(&mVideoState,0,sizeof(VideoState)); //为了安全起见  先将结构体的数据初始化成0了

    mVideoWidget = NULL;
    mPlayerState = Stop;

    mAudioID = 0;
    mIsMute = false;

    mVolume = 1;

//    packet_queue_init(&mVideoState.audioq);
//    packet_queue_init(&mVideoState.videoq);

//    while (1)
//    {
//       openSDL();
//       if (mAudioID > 0)
//       {
//           break;
//       }
//       SDL_Delay(1);
//    }


}

VideoPlayer_Thread::~VideoPlayer_Thread()
{
    ///关闭SDL音频播放设备
    qDebug()<<__FUNCTION__<<"111...";
    if (mAudioID != 0)
    {
        SDL_LockAudioDevice(mAudioID);
//        SDL_PauseAudioDevice(mAudioID,1);
        SDL_CloseAudioDevice(mAudioID);
        SDL_UnlockAudioDevice(mAudioID);

        mAudioID = 0;
    }

    deInit();

    qDebug()<<__FUNCTION__<<"222...";
}

void VideoPlayer_Thread::deInit()
{
//    packet_queue_deinit(&mVideoState.videoq);
//    packet_queue_deinit(&mVideoState.audioq);

    if (mVideoState.swr_ctx != NULL)
    {
        swr_free(&mVideoState.swr_ctx);
        mVideoState.swr_ctx = NULL;
    }

    if (mVideoState.audio_frame!= NULL) {
        avcodec_free_frame(&mVideoState.audio_frame);
        mVideoState.audio_frame = NULL;
    }
}

bool VideoPlayer_Thread::setFileName(QString path)
{
    if (mPlayerState != Stop)
    {
        return false;
    }

    mFileName = path;

//    mPlayerState = Playing;
//    emit sig_StateChanged(Playing);

    memset(&mVideoState,0,sizeof(VideoState)); //为了安全起见  先将结构体的数据初始化成0了

    this->start(); //启动线程

    return true;

}

bool VideoPlayer_Thread::replay()
{
    while (this->isRunning())
    {
        SDL_Delay(5);
    }

    if (mPlayerState != Stop)
    {
        return false;
    }
//    memset(&mVideoState,0,sizeof(VideoState)); //为了安全起见  先将结构体的数据初始化成0了

    this->start(); //启动线程
    return true;
}

bool VideoPlayer_Thread::play()
{
    mVideoState.isNeedPause = false;
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
    qDebug()<<__FUNCTION__<<"111...";
    if (mPlayerState == Stop)
    {
        qDebug()<<__FUNCTION__<<"333...";
        return false;
    }

    mPlayerState = Stop;
    mVideoState.quit = true;
qDebug()<<__FUNCTION__<<"222...";
    if (isWait)
    {
        while(!mVideoState.readThreadFinished)
        {
//            qDebug()<<mVideoState.readThreadFinished<<mVideoState.videoThreadFinished;
            SDL_Delay(3);
        }
    }

//    ///关闭SDL音频播放设备
//    qDebug()<<__FUNCTION__<<mAudioID;
//    if (mAudioID != 0)
//    {
//        SDL_LockAudio();
//        SDL_PauseAudioDevice(mAudioID,1);
//        SDL_CloseAudioDevice(mAudioID);
//        SDL_UnlockAudio();

//        mAudioID = 0;
//    }

qDebug()<<__FUNCTION__<<"999...";

//    emit sig_StateChanged(Stop);

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

void VideoPlayer_Thread::setVolume(float value)
{
    mVolume = value;
    mVideoState.mVolume = value;
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

void VideoPlayer_Thread::setVideoWidget(VideoPlayer_ShowVideoWidget*widget)
{
    mVideoWidget = widget;
    connect(this,SIGNAL(sig_GetOneFrame(QImage)),mVideoWidget,SLOT(slotGetOneFrame(QImage)));
}

int VideoPlayer_Thread::openSDL()
{

    VideoState *is = &mVideoState;

    SDL_AudioSpec wanted_spec, spec;
    int64_t wanted_channel_layout = 0;
    int wanted_nb_channels = 2;
    int samplerate = 44100;
    /*  SDL支持的声道数为 1, 2, 4, 6 */
//    /*  后面我们会使用这个数组来纠正不支持的声道数目 */
//    const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };

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
    wanted_spec.freq = samplerate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        //fprintf(stderr,"Invalid sample rate or channel count!\n");
        return -1;
    }
    wanted_spec.format = AUDIO_S16SYS; // 具体含义请查看“SDL宏定义”部分
    wanted_spec.silence = 0;            // 0指示静音
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;  // 自定义SDL缓冲区大小
    wanted_spec.callback = audio_callback;        // 音频解码的关键回调函数
    wanted_spec.userdata = is;                    // 传给上面回调函数的外带数据

    int num = SDL_GetNumAudioDevices(0);
    for (int i=0;i<num;i++)
    {
        mAudioID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(i,0), false, &wanted_spec, &spec,0);
        if (mAudioID > 0)
        {
            break;
        }
    }

    /* 检查实际使用的配置（保存在spec,由SDL_OpenAudio()填充） */
    if (spec.format != AUDIO_S16SYS) {
        qDebug()<<"SDL advised audio format %d is not supported!"<<spec.format;
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
    is->audio_src_channel_layout = is->audio_tgt_channel_layout = wanted_channel_layout;
    is->audio_src_channels = is->audio_tgt_channels = spec.channels;

    is->audio_buf_size = 0;
    is->audio_buf_index = 0;
    memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));

    return 0;
}

void VideoPlayer_Thread::closeSDL()
{
    if (mAudioID > 0)
    {
        SDL_CloseAudioDevice(mAudioID);
    }

    mAudioID = -1;
}

void VideoPlayer_Thread::run()
{
    char file_path[1280] = {0};
    strcpy(file_path,mFileName.toUtf8().data());

    memset(&mVideoState,0,sizeof(VideoState)); //为了安全起见  先将结构体的数据初始化成0了

    mVideoState.isMute = mIsMute;
    mVideoState.mVolume = mVolume;

//    mVideoState.Init();

//    seek((23*60+50)*1000000);

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

    packet_queue_init(&mVideoState.audioq);
    packet_queue_init(&mVideoState.videoq);


    ///创建一个线程专门用来解码视频
    is->video_tid = SDL_CreateThread(video_thread, "video_thread", &mVideoState);


    is->player = this;

//    int y_size = pCodecCtx->width * pCodecCtx->height;

    AVPacket *packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet 用来存放读取的视频
//    av_new_packet(packet, y_size); //av_read_frame 会给它分配空间 因此这里不需要了

//    av_dump_format(pFormatCtx, 0, file_path, 0); //输出视频信息

    qDebug()<<__FUNCTION__<<is->quit;
    mPlayerState = Playing;
    emit sig_StateChanged(Playing);

    openSDL();

    SDL_LockAudioDevice(mAudioID);
    SDL_PauseAudioDevice(mAudioID,0);
    SDL_UnlockAudioDevice(mAudioID);

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

            if (is->isPause)
            {
                is->isNeedPause = true;
                is->isPause = false;
            }

        }

        //这里做了个限制  当队列里面的数据超过某个大小的时候 就暂停读取  防止一下子就把视频读完了，导致的空间分配不足
        /* 这里audioq.size是指队列中的所有数据包带的音频数据的总量或者视频数据总量，并不是包的数量 */
        //这个值可以稍微写大一些
//        qDebug()<<__FUNCTION__<<is->audioq.size<<MAX_AUDIO_SIZE<<is->videoq.size<<MAX_VIDEO_SIZE;
        if (is->audioq.size > MAX_AUDIO_SIZE || is->videoq.size > MAX_VIDEO_SIZE) {
            SDL_Delay(10);
            continue;
        }

//        qDebug()<<__FUNCTION__<<"is->isPause"<<is->isPause;

        if (is->isPause == true)
        {
            SDL_Delay(10);
            continue;
        }

        if (av_read_frame(pFormatCtx, packet) < 0)
        {
            is->readFinished = true;

//            qDebug()<<__FUNCTION__<<"av_read_frame<0";

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

    qDebug()<<__FUNCTION__<<"333";

    ///文件读取结束 跳出循环的情况
    ///等待播放完毕
    while (!is->quit) {
        SDL_Delay(100);
    }

    if (mPlayerState != Stop) //不是外部调用的stop 是正常播放结束
    {
        stop();
    }

    SDL_LockAudioDevice(mAudioID);
    SDL_PauseAudioDevice(mAudioID,1);
    SDL_UnlockAudioDevice(mAudioID);

    closeSDL();

 qDebug()<<__FUNCTION__<<"444";
    while(!mVideoState.videoThreadFinished)
    {
//        qDebug()<<__FUNCTION__<<"videoThreadFinished"<<mVideoState.videoThreadFinished;
        msleep(10);
    } //确保视频线程结束后 再销毁队列

    qDebug()<<__FUNCTION__<<"555";

    avcodec_close(aCodecCtx);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);

    free(packet);

    packet_queue_deinit(&mVideoState.videoq);
    packet_queue_deinit(&mVideoState.audioq);

    is->readThreadFinished = true;

    emit sig_StateChanged(Stop);

qDebug()<<__FUNCTION__<<"finished!";


//    SDL_Quit();
}
