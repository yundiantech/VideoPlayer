#ifndef PCMFRAME_H
#define PCMFRAME_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory>

//extern "C"
//{
//    #include <libavutil/samplefmt.h>
//}

#define PCMFramePtr std::shared_ptr<PCMFrame>

///本程序固定使用AV_SAMPLE_FMT_S16 44100 双声道
class PCMFrame
{
public:
    PCMFrame();
    ~PCMFrame();

    void setFrameBuffer(const uint8_t * const buffer, const unsigned int &size);
    uint8_t *getBuffer(){return mFrameBuffer;}
    unsigned int getSize(){return  mFrameBufferSize;}

    void setFrameInfo(int sample_rate, int channels, uint32_t pts);
    uint32_t pts(){return m_pts;}
    int sampleRate(){return m_sample_rate;}
    int channels(){return m_channels;}

private:
    uint8_t *mFrameBuffer = nullptr; //pcm数据
    unsigned int mFrameBufferSize = 0; //pcm数据长度

    uint32_t m_pts = 0; //时间戳
//    enum AVSampleFormat mSampleFmt = AV_SAMPLE_FMT_S16;//输出的采样格式
    int m_sample_rate = 0;//采样率
    int m_channels = 0; //声道数

};

#endif // PCMFRAME_H
