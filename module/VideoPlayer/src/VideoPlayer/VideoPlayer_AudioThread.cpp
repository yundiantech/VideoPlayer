/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "VideoPlayer/VideoPlayer.h"

#include "PcmPlayer/PcmVolumeControl.h"

#include <iostream>
#include <stdio.h>

void VideoPlayer::decodeAudioThread()
{
    fprintf(stderr, "%s start \n", __FUNCTION__);
    mIsAudioThreadFinished = false;
    
    int64_t pts_ms = 0; //时间戳(毫秒)

    while (1)
    {
        if (mIsQuit)
        {
            clearAudioQuene(); //清空队列
            break;
        }

        if (mIsPause == true) //判断暂停
        {
            mSleep(10);
            continue;
        }

        std::unique_lock<std::mutex> lck(m_mutex_audio);
        while (!mIsQuit && m_audio_pkt_list.empty())
        {
            m_cond_audio.wait(lck);
        }

        if (m_audio_pkt_list.empty())
        {
            if (mIsReadFinished)
            {
                break;
            }
            continue;
        }

        AVPacket packet = m_audio_pkt_list.front();
        m_audio_pkt_list.pop_front();
        lck.unlock();

        AVPacket *pkt = &packet;

        /* if update, update the audio clock w/pts */
        if (pkt->pts != AV_NOPTS_VALUE)
        {
            pts_ms = av_q2d(mAudioStream->time_base) * pkt->pts * 1000;
        }

        //收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
        if(strcmp((char*)pkt->data,FLUSH_DATA) == 0)
        {
            avcodec_flush_buffers(aCodecCtx);
            av_packet_unref(pkt);
            continue;
        }

        if (seek_flag_audio)
        {
            //发生了跳转 则跳过关键帧到目的时间的这几帧
           if (pts_ms < seek_time)
           {
               continue;
           }
           else
           {
               seek_flag_audio = 0;
           }
        }

        ///直播流为了保证降低延迟，当音频播放队列过大时，会提高播放的采样率
        ///因此，非直播流，不能一次性把所有音频塞入队列，需要限制播放队列中的数据量
        if (!m_is_live_mode)
        {
            while (m_pcm_player->getPcmFrameSize() > 15)
            {
                mSleep(5);
            }
        }

        //解码AVPacket->AVFrame
        if (int ret = avcodec_send_packet(aCodecCtx, &packet) && ret != 0)
        {
           char buffer[1024] = {0};
           av_strerror(ret, buffer, 1024);
           fprintf(stderr, "input AVPacket to decoder failed! ret = %d %s\n", ret, buffer);
        }
        else
        {
            while(1)
            {
                int ret = avcodec_receive_frame(aCodecCtx, aFrame);
                if (ret != 0)
                {
        //            char buffer[1024] = {0};
        //            av_strerror(ret, buffer, 1024);
        //            fprintf(stderr, "avcodec_receive_frame = %d %s\n", ret, buffer);
                    break;
                }

                // /* if update, update the audio clock w/pts */
                // if (packet.pts != AV_NOPTS_VALUE)
                // {
                //     audio_clock = (uint64_t)(av_q2d(mAudioStream->time_base) * packet.pts * 1000);
                // }

                int out_sample_rate = m_out_sample_rate;

                /// ffmpeg解码之后得到的音频数据不是SDL想要的，
                /// 因此这里需要重采样成44100 双声道 AV_SAMPLE_FMT_S16

                /// 需要保证重采样后音频的时间是相同的，不同采样率下同样时间采集的数据采样点个数肯定不一样。
                /// 因此就需要重新计算采样点个数（使用下面的函数）
                /// 将in_sample_rate的采样次数换算成out_sample_rate对应的采样次数
                int nb_samples = av_rescale_rnd(swr_get_delay(swrCtx, out_sample_rate) + aFrame->nb_samples, out_sample_rate, m_in_sample_rate, AV_ROUND_UP);
// std::cout<<swr_get_delay(swrCtx, out_sample_rate) + aFrame->nb_samples<<aFrame->nb_samples<<nb_samples<<std::endl;
    //            int nb_samples = av_rescale_rnd(aFrame->nb_samples, out_sample_rate, m_in_sample_rate, AV_ROUND_INF);
                if (aFrame_ReSample != nullptr)
                {
                    if (aFrame_ReSample->nb_samples != nb_samples || aFrame_ReSample->sample_rate != out_sample_rate)
                    {
                        av_frame_free(&aFrame_ReSample);
                        aFrame_ReSample = nullptr;
                    }
                }

                ///解码一帧后才能获取到采样率等信息，因此将初始化放到这里
                if (aFrame_ReSample == nullptr)
                {
                    aFrame_ReSample = av_frame_alloc();

                    aFrame_ReSample->format = out_sample_fmt;
                    aFrame_ReSample->channel_layout = out_ch_layout;
                    aFrame_ReSample->sample_rate = out_sample_rate;
                    aFrame_ReSample->nb_samples = nb_samples;

                    int ret = av_samples_fill_arrays(aFrame_ReSample->data, aFrame_ReSample->linesize, audio_buf, audio_tgt_channels, aFrame_ReSample->nb_samples, out_sample_fmt, 0);
    //                int ret = av_frame_get_buffer(aFrame_ReSample, 0);
                    if (ret < 0)
                    {
                        fprintf(stderr, "Error allocating an audio buffer\n");
    //                        exit(1);
                    }
                }

                ///2024-09-16
                //当重采样的采样率不一致的时候，原先的方法会存在杂音问题，解决方法如下：
                // 1) 如果没有提供足够的空间用于保存输出数据，采样数据会缓存在swr中。可以通过 swr_get_out_samples()来获取下一次调用swr_convert在给定输入样本数量下输出样本数量的上限，来提供足够的空间。
                // 2)如果是采样频率转换，转换完成后采样数据可能会缓存在swr中，它希望你能给它更多的输入数据。
                // 3)如果实际上并不需要更多输入数据，通过调用swr_convert()，其中参数in_count设置为0来获取缓存在swr中的数据。
                // 4)转换结束之后需要冲刷swr_context的缓冲区，通过调用swr_convert()，其中参数in设置为NULL，参数in_count设置为0。
                for(int i=0;i<2;i++)
                {
                    ///执行重采样
                    int len2;

                    if (i == 0)
                    {
                        len2 = swr_convert(swrCtx, aFrame_ReSample->data, aFrame_ReSample->nb_samples, (const uint8_t**)aFrame->data, aFrame->nb_samples);
                    }
                    else
                    {
                        len2 = swr_convert(swrCtx, aFrame_ReSample->data, aFrame_ReSample->nb_samples, NULL, 0);
                    }

                    if (len2 <= 0)
                    {
                        break;
                    }

                    ///必须根据swr_convert实际返回的值计算数据大小
                    int resampled_data_size = len2 * audio_tgt_channels * av_get_bytes_per_sample(out_sample_fmt);
                    int OneChannelDataSize = resampled_data_size / audio_tgt_channels;

                    PCMFramePtr pcmFramePtr = std::make_shared<PCMFrame>();
                    pcmFramePtr->setFrameBuffer(audio_buf, resampled_data_size);
                    pcmFramePtr->setFrameInfo(m_out_sample_rate, audio_tgt_channels, pts_ms);

                    int audio_frame_size = m_pcm_player->inputPCMFrame(pcmFramePtr);

                    if (audio_frame_size > 0) //大于0 表示音频播放设备处于打开状态
                    {
                        // std::lock_guard<std::mutex> lck(m_mutex_audio_clk);
                        audio_clock = m_pcm_player->getCurrentPts();
                    }
                    
// std::cout<<resampled_data_size<<" "<<audio_frame_size<<" "<<audio_clock<<" "<<pts_ms<<std::endl;
                }

            }
        }

        av_packet_unref(&packet);
    }

    ///等待播放完成
    while (m_pcm_player->getPcmFrameSize() > 0)
    {
        mSleep(5);
    }

    mIsAudioThreadFinished = true;
    fprintf(stderr, "%s finished \n", __FUNCTION__);
}


// int VideoPlayer::decodeAudioFrame(bool isBlock)
// {
//     int audioBufferSize = 0;
//     float pts_s = 0.0f; //时间戳(秒)

//     while(1)
//     {
//         if (mIsQuit)
//         {
//             mIsAudioThreadFinished = true;
//             clearAudioQuene(); //清空队列
//             break;
//         }

//         if (mIsPause == true) //判断暂停
//         {
//             break;
//         }

//         mConditon_Audio->Lock();
// int audio_list_size = mAudioPacktList.size();
// //fprintf(stderr, "mAudioPacktList.size()=%d \n", audio_list_size);
//         // if (mAudioPacktList.size() <= 0)
//         // {
//         //     if (isBlock)
//         //     {
//         //         mConditon_Audio->Wait();
//         //     }
//         //     else
//         //     {
//         //         mConditon_Audio->Unlock();
//         //         break;
//         //     }
//         // }

//         AVPacket packet = mAudioPacktList.front();
//         mAudioPacktList.pop_front();
// //qDebug()<<__FUNCTION__<<mAudioPacktList.size();
//         mConditon_Audio->Unlock();

//         AVPacket *pkt = &packet;

//         /* if update, update the audio clock w/pts */
//         if (pkt->pts != AV_NOPTS_VALUE)
//         {
//             pts_s = av_q2d(mAudioStream->time_base) * pkt->pts;
//         }

//         //收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
//         if(strcmp((char*)pkt->data,FLUSH_DATA) == 0)
//         {
//             avcodec_flush_buffers(aCodecCtx);
//             av_packet_unref(pkt);
//             continue;
//         }

//         if (seek_flag_audio)
//         {
//             //发生了跳转 则跳过关键帧到目的时间的这几帧
//            if (pts_s < seek_time)
//            {
//                continue;
//            }
//            else
//            {
//                seek_flag_audio = 0;
//            }
//         }

//         int got_frame = 0;

//         //解码AVPacket->AVFrame
//         if (int ret = avcodec_send_packet(aCodecCtx, &packet) && ret != 0)
//         {
//            char buffer[1024] = {0};
//            av_strerror(ret, buffer, 1024);
//            fprintf(stderr, "input AVPacket to decoder failed! ret = %d %s\n", ret, buffer);
//         }
//         else
//         {
//             while(1)
//             {
//                 int ret = avcodec_receive_frame(aCodecCtx, aFrame);
//                 if (ret != 0)
//                 {
//         //            char buffer[1024] = {0};
//         //            av_strerror(ret, buffer, 1024);
//         //            fprintf(stderr, "avcodec_receive_frame = %d %s\n", ret, buffer);
//                     break;
//                 }

//                 /* if update, update the audio clock w/pts */
//                 if (packet.pts != AV_NOPTS_VALUE)
//                 {
//                     audio_clock = 1000 * av_q2d(mAudioStream->time_base) * packet.pts;
//                 }

//                 int out_sample_rate = m_out_sample_rate;

//                 /// ffmpeg解码之后得到的音频数据不是SDL想要的，
//                 /// 因此这里需要重采样成44100 双声道 AV_SAMPLE_FMT_S16

//                 /// 需要保证重采样后音频的时间是相同的，不同采样率下同样时间采集的数据采样点个数肯定不一样。
//                 /// 因此就需要重新计算采样点个数（使用下面的函数）
//                 /// 将in_sample_rate的采样次数换算成out_sample_rate对应的采样次数
//                 int nb_samples = av_rescale_rnd(swr_get_delay(swrCtx, out_sample_rate) + aFrame->nb_samples, out_sample_rate, m_in_sample_rate, AV_ROUND_UP);
//     //qDebug()<<swr_get_delay(swrCtx, out_sample_rate) + aFrame->nb_samples<<aFrame->nb_samples<<nb_samples;
//     //            int nb_samples = av_rescale_rnd(aFrame->nb_samples, out_sample_rate, m_in_sample_rate, AV_ROUND_INF);
//                 if (aFrame_ReSample != nullptr)
//                 {
//                     if (aFrame_ReSample->nb_samples != nb_samples || aFrame_ReSample->sample_rate != out_sample_rate)
//                     {
//                         av_frame_free(&aFrame_ReSample);
//                         aFrame_ReSample = nullptr;
//                     }
//                 }

//                 ///解码一帧后才能获取到采样率等信息，因此将初始化放到这里
//                 if (aFrame_ReSample == nullptr)
//                 {
//                     aFrame_ReSample = av_frame_alloc();

//                     aFrame_ReSample->format = out_sample_fmt;
//                     aFrame_ReSample->channel_layout = out_ch_layout;
//                     aFrame_ReSample->sample_rate = out_sample_rate;
//                     aFrame_ReSample->nb_samples = nb_samples;

//                     int ret = av_samples_fill_arrays(aFrame_ReSample->data, aFrame_ReSample->linesize, audio_buf, audio_tgt_channels, aFrame_ReSample->nb_samples, out_sample_fmt, 0);
//     //                int ret = av_frame_get_buffer(aFrame_ReSample, 0);
//                     if (ret < 0)
//                     {
//                         fprintf(stderr, "Error allocating an audio buffer\n");
//     //                        exit(1);
//                     }
//                 }

//                 ///2024-09-16
//                 //当重采样的采样率不一致的时候，原先的方法会存在杂音问题，解决方法如下：
//                 // 1) 如果没有提供足够的空间用于保存输出数据，采样数据会缓存在swr中。可以通过 swr_get_out_samples()来获取下一次调用swr_convert在给定输入样本数量下输出样本数量的上限，来提供足够的空间。
//                 // 2)如果是采样频率转换，转换完成后采样数据可能会缓存在swr中，它希望你能给它更多的输入数据。
//                 // 3)如果实际上并不需要更多输入数据，通过调用swr_convert()，其中参数in_count设置为0来获取缓存在swr中的数据。
//                 // 4)转换结束之后需要冲刷swr_context的缓冲区，通过调用swr_convert()，其中参数in设置为NULL，参数in_count设置为0。
//                 for(int i=0;i<2;i++)
//                 {
//                     ///执行重采样
//                     int len2;

//                     if (i == 0)
//                     {
//                         len2 = swr_convert(swrCtx, aFrame_ReSample->data, aFrame_ReSample->nb_samples, (const uint8_t**)aFrame->data, aFrame->nb_samples);
//                     }
//                     else
//                     {
//                         len2 = swr_convert(swrCtx, aFrame_ReSample->data, aFrame_ReSample->nb_samples, NULL, 0);
//                     }

//                     if (len2 <= 0)
//                     {
//                         break;
//                     }

//                     ///必须根据swr_convert实际返回的值计算数据大小
//                     int resampled_data_size = len2 * audio_tgt_channels * av_get_bytes_per_sample(out_sample_fmt);
//                     int OneChannelDataSize = resampled_data_size / audio_tgt_channels;

//                     audioBufferSize = resampled_data_size;

//                     PCMFramePtr pcmFramePtr = std::make_shared<PCMFrame>();
//                     pcmFramePtr->setFrameBuffer(audio_buf, resampled_data_size);
//                     pcmFramePtr->setFrameInfo(m_out_sample_rate, audio_tgt_channels, pts_s*1000);

//                     int audio_frame_size = m_pcm_player->inputPCMFrame(pcmFramePtr);
// //qDebug()<<resampled_data_size<<audio_frame_size;
//                     audio_clock = m_pcm_player->getCurrentPts() / 1000.0;
//                 }

//                 got_frame = 1;
//             }
//         }

//         av_packet_unref(&packet);

//         if (got_frame)
//         {
//             break;
//         }
//     }

//     return audioBufferSize;
// }
