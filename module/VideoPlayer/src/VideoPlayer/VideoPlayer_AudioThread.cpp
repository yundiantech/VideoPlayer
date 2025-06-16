/**
 * 叶海辉
 * QQ群321159586
 * http://blog.yundiantech.com/
 */

#include "VideoPlayer/VideoPlayer.h"

#include "PcmPlayer/PcmVolumeControl.h"

#include <iostream>
#include <stdio.h>

static void addADTStoPacket(uint8_t* packet, int packetLen, int channel, int sample_rate)
{
    int profile = 2;  //AAC LC，MediaCodecInfo.CodecProfileLevel.AACObjectLC;
    int freqIdx = 8;  //16K, 见后面注释avpriv_mpeg4audio_sample_rates中32000对应的数组下标，来自ffmpeg源码
    int chanCfg = channel;  //见后面注释channel_configuration，Stero双声道立体声

    int avpriv_mpeg4audio_sample_rates[] = {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000, 7350
    };
    /*channel_configuration: 表示声道数chanCfg
    0: Defined in AOT Specifc Config
    1: 1 channel: front-center
    2: 2 channels: front-left, front-right
    3: 3 channels: front-center, front-left, front-right
    4: 4 channels: front-center, front-left, front-right, back-center
    5: 5 channels: front-center, front-left, front-right, back-left, back-right
    6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
    7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
    8-15: Reserved
    */

    for (int i=0; i<13; i++)
    {
        if (avpriv_mpeg4audio_sample_rates[i] == sample_rate)
        {
            freqIdx = i;
            break;
        }
    }

    // fill in ADTS data
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)0xF9;
    packet[2] = (uint8_t)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
    packet[3] = (uint8_t)(((chanCfg&3)<<6) + (packetLen>>11));
    packet[4] = (uint8_t)((packetLen&0x7FF) >> 3);
    packet[5] = (uint8_t)(((packetLen&7)<<5) + 0x1F);
    packet[6] = (uint8_t)0xFC;
}

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
        while (!mIsQuit && !mIsReadFinished && m_audio_pkt_list.empty())
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

        AVPacket pkt = m_audio_pkt_list.front();
        m_audio_pkt_list.pop_front();
        lck.unlock();

        AVPacket *packet = &pkt;

        /* if update, update the audio clock w/pts */
        if (packet->pts != AV_NOPTS_VALUE)
        {
            pts_ms = av_q2d(mAudioStream->time_base) * packet->pts * 1000;
        }

        if (!m_enable_audio_play) //音频播放未启用，则把音频同步到外部时钟
        {
            int64_t audio_pts = 0; //音频pts
            if (packet->dts != AV_NOPTS_VALUE)
            {
                audio_pts = packet->dts;
            }
            else
            {
                audio_pts = 0;
            }

            if (audio_pts < 0)
            {
                audio_pts = 0;
            }

            audio_pts *= av_q2d(mAudioStream->time_base) * m_speed * 1000;
            audio_clock = audio_pts;
            ///音频同步到外部时钟
            uint64_t realTime = 0;
            do
            {
                mSleep(5);
                realTime = (av_gettime() - mVideoStartTime) * m_speed / 1000; //毫秒
            }while(!m_is_stop && audio_pts > (realTime-10));
        }

        if (m_event_handle && m_enable_encoded_audio_callback)
        {
            // printf("%s:%d mAudioStream->codecpar->codec_id=%d %d\n", __FILE__, __LINE__, mAudioStream->codecpar->codec_id, AV_CODEC_ID_AAC);
            if (mAudioStream->codecpar->codec_id == AV_CODEC_ID_AAC) 
            {
                //回调AAC数据
                uint8_t adtsBuffer[7] = {0};
                addADTStoPacket(adtsBuffer, 7 + packet->size, aCodecCtx->channels, aCodecCtx->sample_rate);

                AACFramePtr aac_frame = std::make_shared<AACFrame>();
                aac_frame->setFrameBuffer(adtsBuffer, 7, packet->data, packet->size);
                aac_frame->setPts(audio_clock);
                m_event_handle->onAudioBuffer(aac_frame);
            }
            else if(mAudioStream->codecpar->codec_id == AV_CODEC_ID_PCM_MULAW)
            {
                //回调PCMU数据
                PCMFramePtr audio_frame = std::make_shared<PCMFrame>();
                audio_frame->setFrameBuffer(packet->data, packet->size);
                audio_frame->setFrameInfo(aCodecCtx->sample_rate, aCodecCtx->channels, audio_clock, PCMFrame::PCMFRAME_TYPE_PCMU);
                m_event_handle->onAudioBuffer(audio_frame);
            }
            else if(mAudioStream->codecpar->codec_id == AV_CODEC_ID_PCM_ALAW)
            {
                //回调PCMA数据
                PCMFramePtr audio_frame = std::make_shared<PCMFrame>();
                audio_frame->setFrameBuffer(packet->data, packet->size);
                audio_frame->setFrameInfo(aCodecCtx->sample_rate, aCodecCtx->channels, audio_clock, PCMFrame::PCMFRAME_TYPE_PCMA);
                m_event_handle->onAudioBuffer(audio_frame);
            }
        }

        if (!m_enable_audio_play)
        {
            av_packet_unref(packet);
            continue;
        }

        //收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
        if(strcmp((char*)packet->data,FLUSH_DATA) == 0)
        {
            avcodec_flush_buffers(aCodecCtx);
            av_packet_unref(packet);
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
#ifdef USE_PCM_PLAYER
        ///直播流为了保证降低延迟，当音频播放队列过大时，会提高播放的采样率
        ///因此，非直播流，不能一次性把所有音频塞入队列，需要限制播放队列中的数据量
        if (!m_is_live_mode)
        {
            while (m_pcm_player->getPcmFrameSize() > 15)
            {
                mSleep(5);
            }
        }
#endif
        //解码AVPacket->AVFrame
        if (int ret = avcodec_send_packet(aCodecCtx, packet) && ret != 0)
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
#ifdef USE_PCM_PLAYER
                    int audio_frame_size = m_pcm_player->inputPCMFrame(pcmFramePtr);

                    if (audio_frame_size > 0) //大于0 表示音频播放设备处于打开状态
                    {
                        // std::lock_guard<std::mutex> lck(m_mutex_audio_clk);
                        audio_clock = m_pcm_player->getCurrentPts();
                    }
#endif
// std::cout<<resampled_data_size<<" "<<audio_frame_size<<" "<<audio_clock<<" "<<pts_ms<<std::endl;
                }
            }
        }

        av_packet_unref(packet);
    }
#ifdef USE_PCM_PLAYER
    ///等待播放完成
    while (m_pcm_player->getPcmFrameSize() > 0)
    {
        mSleep(5);
    }
#endif
    mIsAudioThreadFinished = true;
    fprintf(stderr, "%s finished \n", __FUNCTION__);
}
