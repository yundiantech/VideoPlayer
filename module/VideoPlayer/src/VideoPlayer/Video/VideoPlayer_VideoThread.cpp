/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "VideoPlayer/VideoPlayer.h"

void VideoPlayer::decodeVideoThread()
{
    fprintf(stderr, "%s start \n", __FUNCTION__);

    mIsVideoThreadFinished = false;

    int videoWidth  = 0;
    int videoHeight =  0;

    double video_pts = 0; //当前视频的pts
    double audio_pts = 0; //音频pts

    ///解码视频相关
    AVFrame *pFrame = nullptr;
    AVFrame *pFrameYUV = nullptr;
    uint8_t *yuv420pBuffer = nullptr; //解码后的yuv数据
    struct SwsContext *imgConvertCtx = nullptr;  //用于解码后的视频格式转换

    AVCodecContext *pCodecCtx = mVideoStream->codec; //视频解码器

    pFrame = av_frame_alloc();

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

        mConditon_Video->Lock();

        if (mVideoPacktList.size() <= 0)
        {
            mConditon_Video->Unlock();
            if (mIsReadFinished)
            {
                //队列里面没有数据了且读取完毕了
                break;
            }
            else
            {
                mSleep(1); //队列只是暂时没有数据而已
                continue;
            }
        }

        AVPacket pkt1 = mVideoPacktList.front();
        mVideoPacktList.pop_front();

        mConditon_Video->Unlock();

        AVPacket *packet = &pkt1;

        //收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
        if(strcmp((char*)packet->data, FLUSH_DATA) == 0)
        {
            avcodec_flush_buffers(mVideoStream->codec);
            av_packet_unref(packet);
            continue;
        }

        if (avcodec_send_packet(pCodecCtx, packet) != 0)
        {
           qDebug("input AVPacket to decoder failed!\n");
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

            video_pts *= av_q2d(mVideoStream->time_base);
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

            ///音视频同步，实现的原理就是，判断是否到显示此帧图像的时间了，没到则休眠5ms，然后继续判断
            while(1)
            {
                if (mIsQuit)
                {
                    break;
                }

                if (mAudioStream != NULL && !mIsAudioThreadFinished)
                {
                    if (mIsReadFinished && mAudioPacktList.size() <= 0)
                    {//读取完了 且音频数据也播放完了 就剩下视频数据了  直接显示出来了 不用同步了
                        break;
                    }

                    ///有音频的情况下，将视频同步到音频
                    ///跟音频的pts做对比，比视频快则做延时
                    audio_pts = audio_clock;
                }
                else
                {
                    ///没有音频的情况下，直接同步到外部时钟
                    audio_pts = (av_gettime() - mVideoStartTime) / 1000000.0;
                    audio_clock = audio_pts;
                }

    //OUTPUT("%s %f %f \n", __FUNCTION__, video_pts, audio_pts);
                //主要是 跳转的时候 我们把video_clock设置成0了
                //因此这里需要更新video_pts
                //否则当从后面跳转到前面的时候 会卡在这里
                video_pts = video_clock;

                if (video_pts <= audio_pts) break;

                int delayTime = (video_pts - audio_pts) * 1000;

                delayTime = delayTime > 5 ? 5:delayTime;

                if (!mIsNeedPause)
                {
                    mSleep(delayTime);
                }
            }

            if (pCodecCtx->width != videoWidth || pCodecCtx->height != videoHeight)
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

                int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);  //按1字节进行内存对齐,得到的内存大小最接近实际大小
            //    int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 0);  //按0字节进行内存对齐，得到的内存大小是0
            //    int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 4);   //按4字节进行内存对齐，得到的内存大小稍微大一些

                unsigned int numBytes = static_cast<unsigned int>(yuvSize);
                yuv420pBuffer = static_cast<uint8_t *>(av_malloc(numBytes * sizeof(uint8_t)));
                av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, yuv420pBuffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

                ///由于解码后的数据不一定都是yuv420p，因此需要将解码后的数据统一转换成YUV420P
                imgConvertCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                        pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                        AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

            }

            sws_scale(imgConvertCtx,
                    (uint8_t const * const *) pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data,
                    pFrameYUV->linesize);

            doDisplayVideo(yuv420pBuffer, pCodecCtx->width, pCodecCtx->height);

            if (mIsNeedPause)
            {
                mIsPause = true;
                mIsNeedPause = false;
            }

        }
        av_packet_unref(packet);
    }

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

    return;
}
