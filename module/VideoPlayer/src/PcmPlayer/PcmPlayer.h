#ifndef PCMPLAYER_H
#define PCMPLAYER_H

#include <thread>
#include <list>
#include <mutex>
#include <condition_variable>

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

    int inputPCMFrame(PCMFramePtr frame);
    int getPcmFrameSize();
    bool isFrameFull(); //缓存是否满了
    void clearFrame();

    uint32_t getCurrentPts(){return m_current_pts;}

    void setSpeed(float speed){m_play_speed = speed;}    
    void setMute(const bool is_mute){m_is_mute = is_mute;}
    void setVolume(float value){m_volume = value;}
    float getVolume(){return m_volume;}

    bool deviceOpened(){return m_device_opened;}
    bool deviceOpenFailed(){return m_device_open_failed;}

protected:
    std::mutex m_mutex_audio;
    std::condition_variable m_cond_audio;
    std::list<PCMFramePtr> m_pcm_frame_list;
    float m_play_speed = 1.0f; //倍速播放
    
    /// 用于存放上一次未处理完的数据
    uint8_t m_last_frame_buffer[10240];
    int m_last_frame_buffer_size = 0;

    uint32_t m_current_pts = 0; //当前播放帧的时间戳
    bool m_device_opened = false; //设备是否已经打开了
    bool m_device_open_failed = false; //设备打开失败
    uint64_t m_last_try_open_device_time = 0; //上一次尝试打开音频设备的时间
    int m_sample_rate = 0;
    int m_channel = 0;
    int m_cache_size = 81920; //缓存大小
    bool m_is_stop = false;

    ///音量相关变量
    bool  m_is_mute = false;
    float m_volume = 1.0f; //音量 0~1 超过1 表示放大倍数

    virtual bool openDevice()  = 0;
    virtual bool closeDevice() = 0;

protected:
    void playAudioBuffer(void *stream, int len);

};

#endif // AUDIOPLAYER_H
