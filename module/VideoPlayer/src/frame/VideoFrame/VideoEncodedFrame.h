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

    void setIsKeyFrame(const bool &is_key_frame){m_key_frame = is_key_frame;}

    T_NALU *nalu(){return m_nalu;}
    bool isKeyFrame(){return m_key_frame;}
    uint64_t pts(){return m_pts;}

    void setTimeStamp(uint64_t t){m_timestamp_ms = t;}
    uint64_t timeStamp(){return m_timestamp_ms;}

    uint8_t *buffer();
    unsigned int size();

    // void setId(uint64_t id){m_id = id;}
    // uint64_t id(){return m_id;}

private:
    T_NALU *m_nalu = nullptr;

    bool m_key_frame = false;

    uint64_t m_pts = 0;
    uint64_t m_timestamp_ms = 0; //本地绝对时间(UTC时间戳-毫秒)

    // uint64_t m_id = 0;
};

#endif // VIDEOFRAME_H
