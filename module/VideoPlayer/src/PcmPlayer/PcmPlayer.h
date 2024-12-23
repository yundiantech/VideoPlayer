#ifndef PCMPLAYER_H
#define PCMPLAYER_H

#include <thread>
#include <list>

#include "Mutex/Cond.h"
#include "frame/AudioFrame/PCMFrame.h"

struct AudioDevice
{
    std::string deviceName;
    uint32_t deviceId;
};

class PcmPlayer
{
public:
    PcmPlayer();
    virtual ~PcmPlayer();

    virtual std::list<AudioDevice> getAudiDeviceList() = 0; //获取音频设备列表

    bool startPlay();
    bool stopPlay();

    int inputPCMFrame(PCMFramePtr framePtr);
    int getPcmFrameSize();

    uint32_t getCurrentPts(){return m_current_pts;}

    void setMute(const bool is_mute){m_volume = is_mute;}
    void setVolume(float value){m_volume = value;}
    float getVolume(){return m_volume;}

    bool deviceOpened(){return m_device_opened;}

protected:
    Cond *mCond;
    std::list<PCMFramePtr> mPcmFrameList;

    uint32_t m_current_pts = 0; //当前播放帧的时间戳
    bool m_device_opened = false; //设备是否已经打开了
    uint64_t m_last_try_open_device_time = 0; //上一次尝试打开音频设备的时间
    int m_sample_rate = 0;
    int m_channel = 0;

    ///音量相关变量
    bool  m_is_mute = false;
    float m_volume = 1.0f; //音量 0~1 超过1 表示放大倍数

    virtual bool openDevice()  = 0;
    virtual bool closeDevice() = 0;

protected:
    void playAudioBuffer(void *stream, int len);

};

#endif // AUDIOPLAYER_H
