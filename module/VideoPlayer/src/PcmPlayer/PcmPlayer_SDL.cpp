#include "PcmPlayer_SDL.h"

#include "PcmVolumeControl.h"

#include <stdio.h>

void PcmPlayer_SDL::sdlAudioCallBackFunc(void *userdata, Uint8 *stream, int len)
{
    PcmPlayer_SDL *player = (PcmPlayer_SDL*)userdata;
    player->playAudioBuffer(stream, len);
}

PcmPlayer_SDL::PcmPlayer_SDL()
{
//    ///SDL初始化需要放入子线程中，否则有些电脑会有问题。
//    if (SDL_Init(SDL_INIT_AUDIO))
//    {
//        fprintf(stderr, "Could not initialize SDL - %s. \n", SDL_GetError());
//    }
}

PcmPlayer_SDL::~PcmPlayer_SDL()
{
    SDL_Quit();
}

std::list<AudioDevice> PcmPlayer_SDL::getAudiDeviceList()
{
    std::list<AudioDevice> deviceList;

    int num = SDL_GetNumAudioDevices(0);
    for (int i=0;i<num;i++)
    {
        AudioDevice device;
        device.deviceId = i;
        device.deviceName = SDL_GetAudioDeviceName(i, 0);

        deviceList.push_back(device);
    }

    return deviceList;
}

bool PcmPlayer_SDL::openDevice()
{
    static bool is_inited = false;

//    if (!is_inited)
    {
        is_inited = true;

        ///SDL初始化需要放入子线程中，否则有些电脑会有问题。
        if (SDL_Init(SDL_INIT_AUDIO))
        {
            fprintf(stderr, "Could not initialize SDL - %s. \n", SDL_GetError());
        }
    }

    ///打开SDL，并设置播放的格式为:AUDIO_S16LSB 双声道，44100hz
    ///后期使用ffmpeg解码完音频后，需要重采样成和这个一样的格式，否则播放会有杂音
    SDL_AudioSpec wanted_spec, spec;
//    int wanted_nb_channels = 2;
//    int samplerate = 44100;
    int wanted_nb_channels = m_channel;
    int samplerate = m_sample_rate;

    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq = samplerate;
    wanted_spec.format = AUDIO_S16SYS; // 具体含义请查看“SDL宏定义”部分
    wanted_spec.silence = 0;            // 0指示静音
    wanted_spec.samples = 1024;  // 自定义SDL缓冲区大小
    wanted_spec.callback = sdlAudioCallBackFunc;  // 回调函数
    wanted_spec.userdata = this;                  // 传给上面回调函数的外带数据

    int num = SDL_GetNumAudioDevices(0);
    for (int i=0;i<num;i++)
    {
        mAudioID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(i,0), false, &wanted_spec, &spec,0);
        if (mAudioID > 0)
        {
            break;
        }
    }
// qDebug()<<__FUNCTION__<<"NUM="<<num<<" mAudioID="<<mAudioID;
    /* 检查实际使用的配置（保存在spec,由SDL_OpenAudio()填充） */
//    if (spec.format != AUDIO_S16SYS)
    if (mAudioID <= 0)
    {
        return false;
    }
    else
    {
        SDL_LockAudioDevice(mAudioID);
        SDL_PauseAudioDevice(mAudioID, 0);
        SDL_UnlockAudioDevice(mAudioID);
    }
//fprintf(stderr, "mAudioID=%d\n\n\n\n\n\n", mAudioID);
    return true;
}

bool PcmPlayer_SDL::closeDevice()
{
    if (mAudioID > 0)
    {
        SDL_LockAudioDevice(mAudioID);
        SDL_PauseAudioDevice(mAudioID, 1);
        SDL_UnlockAudioDevice(mAudioID);

        SDL_CloseAudioDevice(mAudioID);
    }

    SDL_Quit();

    mAudioID = 0;

    return true;
}
