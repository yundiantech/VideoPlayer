#ifndef VIDEOENCODEDFRAME_H
#define VIDEOENCODEDFRAME_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory>

#include "nalu/nalu.h"

#define VideoEncodedFramePtr std::shared_ptr<VideoEncodedFrame>

class VideoEncodedFrame
{
public:
    VideoEncodedFrame();
    ~VideoEncodedFrame();

    void setNalu(T_NALU *nalu, const int64_t &time = 0);
    void setNalu(uint8_t *buffer, const int &len, const bool & isAllocBuffer, const T_NALU_TYPE &type, const uint64_t &time = 0);

    void setIsKeyFrame(const bool &isKeyFrame){mIsKeyFrame = isKeyFrame;}

    T_NALU *getNalu(){return mNalu;}
    bool getIsKeyFrame(){return mIsKeyFrame;}
    uint64_t getPts(){return mPts;}

    void setTimeStamp(uint64_t t){m_timestamp_ms = t;}
    uint64_t getTimeStamp(){return m_timestamp_ms;}

    uint8_t *getBuffer();
    unsigned int getSize();

    void setId(uint64_t id){m_id = id;}
    uint64_t getId(){return m_id;}

private:
    T_NALU *mNalu;

    bool mIsKeyFrame;

    uint64_t mPts;
    uint64_t m_timestamp_ms = 0; //本地绝对时间(UTC时间戳-毫秒)

    uint64_t m_id = 0;
};

#endif // VIDEOFRAME_H
