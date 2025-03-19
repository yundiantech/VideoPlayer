/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "VideoPlayer/VideoPlayer.h"

#include <iostream>

//static const struct TextureFormatEntry {
//    enum AVPixelFormat format;
//    int texture_fmt;
//} sdl_texture_format_map[] = {
//    { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
//    { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
//    { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
//    { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
//    { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
//    { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
//    { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
//    { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
//    { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
//    { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
//    { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
//    { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
//    { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
//    { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
//    { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
//    { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
//    { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
//    { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
//    { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
//    { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
//};

#if CONFIG_AVFILTER
int VideoPlayer::configure_filtergraph(AVFilterGraph *graph, const char *filtergraph, AVFilterContext *source_ctx, AVFilterContext *sink_ctx)
{
    int ret, i;
    int nb_filters = graph->nb_filters;
    AVFilterInOut *outputs = NULL, *inputs = NULL;

    if (filtergraph) {
        outputs = avfilter_inout_alloc();
        inputs  = avfilter_inout_alloc();
        if (!outputs || !inputs) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        outputs->name       = av_strdup("in");
        outputs->filter_ctx = source_ctx;
        outputs->pad_idx    = 0;
        outputs->next       = NULL;

        inputs->name        = av_strdup("out");
        inputs->filter_ctx  = sink_ctx;
        inputs->pad_idx     = 0;
        inputs->next        = NULL;

        if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, NULL)) < 0)
            goto fail;
    } else {
        if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
            goto fail;
    }

    /* Reorder the filters to ensure that inputs of the custom filters are merged first */
    for (i = 0; i < graph->nb_filters - nb_filters; i++)
        FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);

    ret = avfilter_graph_config(graph, NULL);
fail:
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    return ret;
}

double get_rotation(AVStream *st)
{
    uint8_t* displaymatrix = av_stream_get_side_data(st,
                                                     AV_PKT_DATA_DISPLAYMATRIX, NULL);
    double theta = 0;
    if (displaymatrix)
        theta = -av_display_rotation_get((int32_t*) displaymatrix);

    theta -= 360*floor(theta/360 + 0.9/360);

    if (fabs(theta - 90*round(theta/90)) > 2)
        av_log(NULL, AV_LOG_WARNING, "Odd rotation angle.\n"
               "If you want to help, upload a sample "
               "of this file to ftp://upload.ffmpeg.org/incoming/ "
               "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");

    return theta;
}

int VideoPlayer::configure_video_filters(AVFilterGraph *graph, const char *vfilters, AVFrame *frame)
{
//    enum AVPixelFormat pix_fmts[FF_ARRAY_ELEMS(sdl_texture_format_map)];
    char sws_flags_str[512] = "";
    char buffersrc_args[256];
    int ret;
    AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
    AVCodecParameters *codecpar = this->mVideoStream->codecpar;
    AVRational fr = av_guess_frame_rate(this->pFormatCtx, this->mVideoStream, NULL);
    AVDictionaryEntry *e = NULL;
    int nb_pix_fmts = 0;

//    int i, j;
//    for (i = 0; i < renderer_info.num_texture_formats; i++)
//    {
//        for (j = 0; j < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; j++)
//        {
//            if (renderer_info.texture_formats[i] == sdl_texture_format_map[j].texture_fmt)
//            {
//                pix_fmts[nb_pix_fmts++] = sdl_texture_format_map[j].format;
//                break;
//            }
//        }
//    }
//    pix_fmts[nb_pix_fmts] = AV_PIX_FMT_NONE;

//    while ((e = av_dict_get(sws_dict, "", e, AV_DICT_IGNORE_SUFFIX)))
//    {
//        if (!strcmp(e->key, "sws_flags"))
//        {
//            av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", e->value);
//        }
//        else
//        {
//            av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", e->key, e->value);
//        }
//    }

    if (strlen(sws_flags_str))
        sws_flags_str[strlen(sws_flags_str)-1] = '\0';

    graph->scale_sws_opts = av_strdup(sws_flags_str);

    snprintf(buffersrc_args, sizeof(buffersrc_args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             frame->width, frame->height, frame->format,
             this->mVideoStream->time_base.num, this->mVideoStream->time_base.den,
             codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));

    if (fr.num && fr.den)
        av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);

    if ((ret = avfilter_graph_create_filter(&filt_src,
                                            avfilter_get_by_name("buffer"),
                                            "ffplay_buffer", buffersrc_args, NULL,
                                            graph)) < 0)
    {
        goto fail;
    }

    ret = avfilter_graph_create_filter(&filt_out,
                                       avfilter_get_by_name("buffersink"),
                                       "ffplay_buffersink", NULL, NULL, graph);
    if (ret < 0)
        goto fail;

//    if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts,  AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
//        goto fail;

    last_filter = filt_out;

/* Note: this macro adds a filter before the lastly added filter, so the
 * processing order of the filters is in reverse */
#define INSERT_FILT(name, arg) do {                                          \
    AVFilterContext *filt_ctx;                                               \
                                                                             \
    ret = avfilter_graph_create_filter(&filt_ctx,                            \
                                       avfilter_get_by_name(name),           \
                                       "ffplay_" name, arg, NULL, graph);    \
    if (ret < 0)                                                             \
        goto fail;                                                           \
                                                                             \
    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                        \
    if (ret < 0)                                                             \
        goto fail;                                                           \
                                                                             \
    last_filter = filt_ctx;                                                  \
} while (0)

    if (autorotate) {
        double theta  = get_rotation(this->mVideoStream);
//        theta = 90.0f; //测试用
// fprintf(stderr, "%s get_rotation：%d \n", __FUNCTION__, theta);
        if (fabs(theta - 90) < 1.0) {
            INSERT_FILT("transpose", "clock");
        } else if (fabs(theta - 180) < 1.0) {
            INSERT_FILT("hflip", NULL);
            INSERT_FILT("vflip", NULL);
        } else if (fabs(theta - 270) < 1.0) {
            INSERT_FILT("transpose", "cclock");
        } else if (fabs(theta) > 1.0) {
            char rotate_buf[64];
            snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
            INSERT_FILT("rotate", rotate_buf);
        }
    }

    if ((ret = configure_filtergraph(graph, vfilters, filt_src, last_filter)) < 0)
        goto fail;

    this->in_video_filter  = filt_src;
    this->out_video_filter = filt_out;

fail:
    return ret;
}

#endif  /* CONFIG_AVFILTER */

void VideoPlayer::decodeVideoThread()
{
    fprintf(stderr, "%s start pointer=%d file_path=%s \n", __FUNCTION__, this, m_file_path.c_str());

    mIsVideoThreadFinished = false;

    int videoWidth  = 0;
    int videoHeight =  0;

    int64_t video_pts = 0; //当前视频的pts
    int64_t audio_pts = 0; //音频pts

    ///解码视频相关
    AVFrame *pFrame = nullptr;
    AVFrame *pFrameYUV = nullptr;
    uint8_t *yuv420pBuffer = nullptr; //解码后的yuv数据
    struct SwsContext *imgConvertCtx = nullptr;  //用于解码后的视频格式转换

//    AVCodecContext *pCodecCtx = mVideoStream->codec; //视频解码器

    pFrame = av_frame_alloc();

#if CONFIG_AVFILTER
    AVFilterGraph *graph = NULL;
    AVFilterContext *filt_out = NULL, *filt_in = NULL;
    int last_w = 0;
    int last_h = 0;
//    AVPixelFormat last_format = -2;
    int last_format = -2;
//    int last_serial = -1;
    int last_vfilter_idx = 0;
#endif

    bool is_key_frame_getted = false;

    auto avSyncFunc = [&]
    {
        ///视频同步到音频
        ///音视频同步，实现的原理就是，判断是否到显示此帧图像的时间了，没到则休眠5ms，然后继续判断
        while(1)
        {
            if (mIsQuit)
            {
                break;
            }

            if (mAudioStream != NULL && !mIsAudioThreadFinished 
#ifdef USE_PCM_PLAYER
            && !m_pcm_player->deviceOpenFailed()
#endif
            )
            {
                if (mIsReadFinished && m_audio_pkt_list.size() <= 0 
#ifdef USE_PCM_PLAYER
                && m_pcm_player->getPcmFrameSize() <= 0
#endif
                )
                {//读取完了 且音频数据也播放完了 就剩下视频数据了  直接显示出来了 不用同步了
                    break;
                }

                ///有音频的情况下，将视频同步到音频
                ///跟音频的pts做对比，比视频快则做延时
                // std::lock_guard<std::mutex> lck(m_mutex_audio_clk);
                audio_pts = audio_clock;
            }
            else
            {
                ///没有音频或者音频设备打开失败的情况下，直接同步到外部时钟
                audio_pts = (av_gettime() - mVideoStartTime) * m_speed / 1000; //毫秒
                // std::lock_guard<std::mutex> lck(m_mutex_audio_clk);
                audio_clock = audio_pts;
            }

// printf("%s %lld %lld \n", __FUNCTION__, video_pts, audio_pts);
// std::cout<<video_pts<<" "<<audio_pts <<" "<<video_clock<<std::endl;
            //主要是 跳转的时候 我们把video_clock设置成0了
            //因此这里需要更新video_pts
            //否则当从后面跳转到前面的时候 会卡在这里
            video_pts = video_clock;

            if (video_pts <= audio_pts) break;

            int delayTime = (video_pts - audio_pts);

            delayTime = delayTime > 5 ? 5:delayTime; //最长休眠5ms

            if (!mIsNeedPause)
            {
                mSleep(delayTime);
            }
        }
    };

    while(1)
    {
        if (mIsQuit)
        {
            clearVideoQuene(); //清空队列
            break;
        }

        if (mIsPause == true) //判断暂停
        {
            mSleep(10);
            continue;
        }

        std::unique_lock<std::mutex> lck(m_mutex_video);
        while (!mIsQuit && !mIsReadFinished && m_video_pkt_list.empty())
        {
            m_cond_video.wait(lck);
        }

        if (m_video_pkt_list.empty())
        {
            if (mIsReadFinished)
            {
                break;
            }
            continue;
        }

        AVPacket pkt = m_video_pkt_list.front();
        m_video_pkt_list.pop_front();
        lck.unlock();

        AVPacket *packet = &pkt;

        if (!m_enable_video_decode)
        {
            if (packet->dts != AV_NOPTS_VALUE)
            {
                video_pts = packet->dts;
            }
            else
            {
                video_pts = 0;
            }

            video_pts *= (av_q2d(mVideoStream->time_base) * 1000); //毫秒
            video_clock = video_pts;
// printf("%s %lld %lld %lld %lld\n", __FUNCTION__, video_pts, video_clock, audio_pts, packet->dts);
// printf("%s %d m_audio_pkt_list.size()=%d m_video_pkt_list.size()=%d \n", __FILE__, __LINE__, m_audio_pkt_list.size(), m_video_pkt_list.size());
            avSyncFunc(); //音视频同步
        }

        if (m_event_handle && m_enable_encoded_video_callback)
        {
            //处理视频数据，直接回调未解码前的数据
            int key_frame = (packet->flags & AV_PKT_FLAG_KEY);

            T_NALU_TYPE nalu_type = T_NALU_H264;

            if (mVideoStream->codecpar->codec_id == AV_CODEC_ID_H264) 
            {
                nalu_type = T_NALU_H264;
            } 
            else if (mVideoStream->codecpar->codec_id == AV_CODEC_ID_HEVC) 
            {
                nalu_type = T_NALU_H265;
            }

            VideoEncodedFramePtr videoFrame = std::make_shared<VideoEncodedFrame>();

            if (key_frame && mVideoStream->codecpar->extradata_size > 0)
            {
                int buffer_size = mVideoStream->codecpar->extradata_size + packet->size;
                uint8_t *buffer = (uint8_t*)malloc(buffer_size);
                // printf("mVideoStream->codecpar->extradata_size=%d \n", mVideoStream->codecpar->extradata_size);
                memcpy(buffer, mVideoStream->codecpar->extradata, mVideoStream->codecpar->extradata_size);
                memcpy(buffer + mVideoStream->codecpar->extradata_size, packet->data, packet->size);

                videoFrame->setNalu(buffer, buffer_size, false, nalu_type, video_clock);
            }
            else
            {
                videoFrame->setNalu(packet->data, packet->size, true, nalu_type, video_clock);
            }   

            videoFrame->setIsKeyFrame(key_frame);
            m_event_handle->onVideoBuffer(videoFrame);
        }

        if (!m_enable_video_decode)
        {
            av_packet_unref(packet);
            continue;
        }

        //收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
        if(strcmp((char*)packet->data, FLUSH_DATA) == 0)
        {
            avcodec_flush_buffers(pCodecCtx);
            av_packet_unref(packet);
            continue;
        }

        if (avcodec_send_packet(pCodecCtx, packet) != 0)
        {
           printf("input AVPacket to decoder failed!\n");
           av_packet_unref(packet);
           continue;
        }

        while (0 == avcodec_receive_frame(pCodecCtx, pFrame))
        {
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

            video_pts *= (av_q2d(mVideoStream->time_base) * 1000); //毫秒
            video_clock = video_pts;
    //OUTPUT("%s %f \n", __FUNCTION__, video_pts);
            if (seek_flag_video)
            {
                //发生了跳转 则跳过关键帧到目的时间的这几帧
               if (video_pts < seek_time)
               {
                   av_packet_unref(packet);
                   continue;
               }
               else
               {
                   seek_flag_video = 0;
               }
            }

            avSyncFunc();

#if CONFIG_AVFILTER
        if (   last_w != pFrame->width
            || last_h != pFrame->height
            || last_format != pFrame->format
//            || last_serial != is->viddec.pkt_serial
            || last_vfilter_idx != vfilter_idx)
        {
//            av_log(NULL, AV_LOG_DEBUG,
//                   "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
//                   last_w, last_h,
//                   (const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial, pFrame->width, pFrame->height,
//                   (const char *)av_x_if_null(av_get_pix_fmt_name(pFrame->format), "none"), is->viddec.pkt_serial);

            avfilter_graph_free(&graph);
            graph = avfilter_graph_alloc();
            if (!graph)
            {
//                ret = AVERROR(ENOMEM);
//                goto the_end;
                continue;
            }
            graph->nb_threads = filter_nbthreads;

            int ret = configure_video_filters(graph, vfilters_list ? vfilters_list[vfilter_idx] : NULL, pFrame);
            if (ret < 0)
            {
//                SDL_Event event;
//                event.type = FF_QUIT_EVENT;
//                event.user.data1 = is;
//                SDL_PushEvent(&event);
//                goto the_end;
                continue;
            }
            filt_in  = in_video_filter;
            filt_out = out_video_filter;
            last_w = pFrame->width;
            last_h = pFrame->height;
            last_format = pFrame->format;
//            last_serial = is->viddec.pkt_serial;
            last_vfilter_idx = vfilter_idx;
//            frame_rate = av_buffersink_get_frame_rate(filt_out);
        }

        int ret = av_buffersrc_add_frame(filt_in, pFrame);
        if (ret < 0)
        {
//            goto the_end;
            continue;
        }

        while (ret >= 0)
        {
//            is->frame_last_returned_time = av_gettime_relative() / 1000000.0;

            ret = av_buffersink_get_frame_flags(filt_out, pFrame, 0);
            if (ret < 0)
            {
//                if (ret == AVERROR_EOF)
//                    is->viddec.finished = is->viddec.pkt_serial;
                ret = 0;
                break;
            }

//            frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
//            if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
//                is->frame_last_filter_delay = 0;
//            tb = av_buffersink_get_time_base(filt_out);
#endif

            if (pFrame->width != videoWidth || pFrame->height != videoHeight)
            {
                videoWidth  = pFrame->width;
                videoHeight = pFrame->height;

                if (pFrameYUV != nullptr)
                {
                    av_free(pFrameYUV);
                }

                if (yuv420pBuffer != nullptr)
                {
                    av_free(yuv420pBuffer);
                }

                if (imgConvertCtx != nullptr)
                {
                    sws_freeContext(imgConvertCtx);
                }

                pFrameYUV = av_frame_alloc();

                int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, videoWidth, videoHeight, 1);  //按1字节进行内存对齐,得到的内存大小最接近实际大小
            //    int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 0);  //按0字节进行内存对齐，得到的内存大小是0
            //    int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 4);   //按4字节进行内存对齐，得到的内存大小稍微大一些

                unsigned int numBytes = static_cast<unsigned int>(yuvSize);
                yuv420pBuffer = static_cast<uint8_t *>(av_malloc(numBytes * sizeof(uint8_t)));
                av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, yuv420pBuffer, AV_PIX_FMT_YUV420P, videoWidth, videoHeight, 1);

                ///由于解码后的数据不一定都是yuv420p，因此需要将解码后的数据统一转换成YUV420P
                imgConvertCtx = sws_getContext(videoWidth, videoHeight,
                        (AVPixelFormat)pFrame->format, videoWidth, videoHeight,
                        AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

            }

            sws_scale(imgConvertCtx,
                    (uint8_t const * const *) pFrame->data,
                    pFrame->linesize, 0, videoHeight, pFrameYUV->data,
                    pFrameYUV->linesize);

// printf("(packet->flags & AV_PKT_FLAG_KEY) = %d\n", packet->flags & AV_PKT_FLAG_KEY);
            if (!is_key_frame_getted && (packet->flags & AV_PKT_FLAG_KEY)) // is keyframe
            {
                is_key_frame_getted = true;
            }

            if (!m_is_live_mode || is_key_frame_getted) //只有获取到第一帧关键帧后才进行显示，rtsp流最开始的部分会花屏
            {
                doDisplayVideo(yuv420pBuffer, videoWidth, videoHeight);
            }

#if CONFIG_AVFILTER
//            if (is->videoq.serial != is->viddec.pkt_serial)
//                break;
        }
#endif
            if (mIsNeedPause)
            {
                mIsPause = true;
                mIsNeedPause = false;
            }

        }
        av_packet_unref(packet);
    }

#if CONFIG_AVFILTER
    avfilter_graph_free(&graph);
#endif

    av_free(pFrame);

    if (pFrameYUV != nullptr)
    {
        av_free(pFrameYUV);
    }

    if (yuv420pBuffer != nullptr)
    {
        av_free(yuv420pBuffer);
    }

    if (imgConvertCtx != nullptr)
    {
        sws_freeContext(imgConvertCtx);
    }

    if (!mIsQuit)
    {
        mIsQuit = true;
    }

    mIsVideoThreadFinished = true;

    fprintf(stderr, "%s finished \n", __FUNCTION__);
}
