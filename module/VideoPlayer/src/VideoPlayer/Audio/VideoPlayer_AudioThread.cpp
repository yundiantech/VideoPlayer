/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "VideoPlayer/VideoPlayer.h"

#include "PcmVolumeControl.h"

#include <stdio.h>

void VideoPlayer::sdlAudioCallBackFunc(void *userdata, Uint8 *stream, int len)
{
    VideoPlayer *player = (VideoPlayer*)userdata;
    player->sdlAudioCallBack(stream, len);
}

void VideoPlayer::sdlAudioCallBack(Uint8 *stream, int len)
{
    int len1, audio_data_size;

    /*   len是由SDL传入的SDL缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据 */
    while (len > 0)
    {
        /*  audio_buf_index 和 audio_buf_size 标示我们自己用来放置解码出来的数据的缓冲区，*/
        /*   这些数据待copy到SDL缓冲区， 当audio_buf_index >= audio_buf_size的时候意味着我*/
        /*   们的缓冲为空，没有数据可供copy，这时候需要调用audio_decode_frame来解码出更
         /*   多的桢数据 */
        if (audio_buf_index >= audio_buf_size)
        {
            audio_data_size = decodeAudioFrame();

            /* audio_data_size < 0 标示没能解码出数据，我们默认播放静音 */
            if (audio_data_size <= 0)
            {
                /* silence */
                audio_buf_size = 1024;
                /* 清零，静音 */
                memset(audio_buf, 0, audio_buf_size);
            }
            else
            {
                audio_buf_size = audio_data_size;
            }
            audio_buf_index = 0;
        }
        /*  查看stream可用空间，决定一次copy多少数据，剩下的下次继续copy */
        len1 = audio_buf_size - audio_buf_index;

        if (len1 > len)
        {
            len1 = len;
        }

        if (audio_buf == NULL) return;

        if (mIsMute || mIsNeedPause) //静音 或者 是在暂停的时候跳转了
        {
            memset(audio_buf + audio_buf_index, 0, len1);
        }
        else
        {
            PcmVolumeControl::RaiseVolume((char*)audio_buf + audio_buf_index, len1, 1, mVolume);
        }

        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);

//        SDL_memset(stream, 0x0, len);// make sure this is silence.
//        SDL_MixAudio(stream, (uint8_t *) audio_buf + audio_buf_index, len1, SDL_MIX_MAXVOLUME);

//        SDL_MixAudio(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, len1, 50);
//        SDL_MixAudioFormat(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, 50);

        len -= len1;
        stream += len1;
        audio_buf_index += len1;

    }

}

int VideoPlayer::decodeAudioFrame(bool isBlock)
{
    int audioBufferSize = 0;

    while(1)
    {
        if (mIsQuit)
        {
            mIsAudioThreadFinished = true;
            clearAudioQuene(); //清空队列
            break;
        }

        if (mIsPause == true) //判断暂停
        {
            break;
        }

        mConditon_Audio->Lock();

        if (mAudioPacktList.size() <= 0)
        {
            if (isBlock)
            {
                mConditon_Audio->Wait();
            }
            else
            {
                mConditon_Audio->Unlock();
                break;
            }
        }

        AVPacket packet = mAudioPacktList.front();
        mAudioPacktList.pop_front();

        mConditon_Audio->Unlock();

        AVPacket *pkt = &packet;

        /* if update, update the audio clock w/pts */
        if (pkt->pts != AV_NOPTS_VALUE)
        {
            audio_clock = av_q2d(mAudioStream->time_base) * pkt->pts;
        }

        //收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
        if(strcmp((char*)pkt->data,FLUSH_DATA) == 0)
        {
            avcodec_flush_buffers(mAudioStream->codec);
            av_packet_unref(pkt);
            continue;
        }

        if (seek_flag_audio)
        {
            //发生了跳转 则跳过关键帧到目的时间的这几帧
           if (audio_clock < seek_time)
           {
               continue;
           }
           else
           {
               seek_flag_audio = 0;
           }
        }

        //解码AVPacket->AVFrame
        int got_frame = 0;
        int size = avcodec_decode_audio4(aCodecCtx, aFrame, &got_frame, &packet);

        av_packet_unref(&packet);

        if (got_frame)
        {
            /// ffmpeg解码之后得到的音频数据不是SDL想要的，
            /// 因此这里需要重采样成44100 双声道 AV_SAMPLE_FMT_S16
            if (aFrame_ReSample == NULL)
            {
                aFrame_ReSample = av_frame_alloc();

                aFrame_ReSample->nb_samples = av_rescale_rnd(swr_get_delay(swrCtx, out_sample_rate) + aFrame->nb_samples,
                            out_sample_rate, in_sample_rate, AV_ROUND_UP);

                av_samples_fill_arrays(aFrame_ReSample->data, aFrame_ReSample->linesize, audio_buf, audio_tgt_channels, aFrame_ReSample->nb_samples, out_sample_fmt, 0);

            }

            int len2 = swr_convert(swrCtx, aFrame_ReSample->data, aFrame_ReSample->nb_samples, (const uint8_t**)aFrame->data, aFrame->nb_samples);
            int resampled_data_size = len2 * audio_tgt_channels * av_get_bytes_per_sample(out_sample_fmt);

            audioBufferSize = resampled_data_size;
            break;
        }
    }

    return audioBufferSize;
}
