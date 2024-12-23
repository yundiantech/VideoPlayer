#include "PcmPlayer.h"

#include "PcmVolumeControl.h"

#include <stdio.h>

PcmPlayer::PcmPlayer()
{
    mCond = new Cond();
}

PcmPlayer::~PcmPlayer()
{

}

bool PcmPlayer::startPlay()
{
    auto now = std::chrono::system_clock::now();
    auto tick = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    m_last_try_open_device_time = tick;

    bool is_succeed = openDevice();
    m_device_opened = is_succeed;
    return is_succeed;
}

bool PcmPlayer::stopPlay()
{
    bool isSucceed = closeDevice();
    m_device_opened = false;
    return isSucceed;
}

int PcmPlayer::inputPCMFrame(PCMFramePtr framePtr)
{
    int frame_size = 0;

    if (m_device_opened)
    {
        //音频设备打开的情况下才处理播放

        mCond->Lock();
        mPcmFrameList.push_back(framePtr);
        frame_size = mPcmFrameList.size();
        mCond->Unlock();
        mCond->Signal();

    //    qDebug()<<m_sample_rate<<m_channel<<m_device_opened<<framePtr->sampleRate()<<framePtr->channels();

        int channels = framePtr->channels();
        int sample_rate = framePtr->sampleRate();

        ///用于实现 播放队列太大的时候，提高播放采样率，用于直播消延时
        static int use_sample_rate = framePtr->sampleRate();
        if (frame_size > 20)
        {
        //    use_sample_rate = 32000;
            use_sample_rate = sample_rate*2;
            // qDebug()<<__FUNCTION__<<"frame_size="<<frame_size<<"make sample_rate bigger>>"<<use_sample_rate;
        }
        else if (frame_size < 10 && use_sample_rate != sample_rate)
        {
            use_sample_rate = sample_rate;
            // qDebug()<<__FUNCTION__<<"frame_size="<<frame_size<<"make sample_rate normal>>"<<use_sample_rate;
        }
    //qDebug()<<__FUNCTION__<<"frame_size="<<frame_size<<"make sample_rate bigger>>"<<sample_rate;
        if (m_sample_rate != use_sample_rate || m_channel != channels || !m_device_opened)
        {
            m_sample_rate = use_sample_rate;
            m_channel = channels;

            stopPlay();
            startPlay();
        }
    }
    else
    {
        auto now = std::chrono::system_clock::now();
        auto tick = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        if ((tick - m_last_try_open_device_time) > 3000)
        {
            //音频设备未打开，则每隔3秒尝试打开一次
            stopPlay();
            startPlay();
        }
    }
    return frame_size;
}

int PcmPlayer::getPcmFrameSize()
{
//    mCond->Lock();
    int size = mPcmFrameList.size();
//    mCond->Unlock();

    return size;
}

void PcmPlayer::playAudioBuffer(void *stream, int len)
{
    PCMFramePtr pcmFramePtr = nullptr;
//fprintf(stderr, "%s %d %d \n", __FUNCTION__, len, mPcmFrameList.size());
    mCond->Lock();
    if (!mPcmFrameList.empty())
    {
        pcmFramePtr = mPcmFrameList.front();
        mPcmFrameList.pop_front();
    }
    mCond->Unlock();

    if (pcmFramePtr != nullptr)
    {
        /// 这里我的PCM数据都是AV_SAMPLE_FMT_S16 44100 双声道的，且采样都是1024，因此pcmFramePtr->getSize()和len值相等都为4096.
        /// 所以这里直接拷贝过去就好了。
//        fprintf(stderr, "%s %d %d \n", __FUNCTION__, pcmFramePtr->getSize(), len);

        if (m_is_mute)// || mIsNeedPause) //静音 或者 是在暂停的时候跳转了
        {
            memset(stream, 0x0, pcmFramePtr->getSize());
        }
        else
        {
            PcmVolumeControl::RaiseVolume((char*)pcmFramePtr->getBuffer(), pcmFramePtr->getSize(), 1, m_volume);
            memcpy(stream, (uint8_t *)pcmFramePtr->getBuffer(), pcmFramePtr->getSize());
        }

        len -= pcmFramePtr->getSize();

        m_current_pts = pcmFramePtr->pts();
    }

}
