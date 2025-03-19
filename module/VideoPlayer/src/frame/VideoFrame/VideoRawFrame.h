#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <memory>

#define VideoRawFramePtr std::shared_ptr<VideoRawFrame>

class VideoRawFrame
{
public:
    enum FrameType
    {
        FRAME_TYPE_NONE = -1,
        FRAME_TYPE_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
        FRAME_TYPE_RGB8,
        FRAME_TYPE_RGB24,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    };

    VideoRawFrame();
    ~VideoRawFrame();

    void initBuffer(const int &width, const int &height, const FrameType &type, int64_t time = 0);

    void setFramebuf(const uint8_t *buf);
    void setYbuf(const uint8_t *buf);
    void setUbuf(const uint8_t *buf);
    void setVbuf(const uint8_t *buf);

    uint8_t *buffer(){return mFrameBuffer;}
    int width(){return mWidth;}
    int height(){return mHegiht;}
    int size(){return mFrameBufferSize;}

    void setPts(const int64_t &pts){mPts=pts;}
    int64_t pts(){return mPts;}

    void setTimeStamp(uint64_t t){m_timestamp_ms = t;}
    uint64_t timeStamp(){return m_timestamp_ms;}

    FrameType type(){return mType;}

protected:
    FrameType mType;

    uint8_t *mFrameBuffer = nullptr;
    int mFrameBufferSize = 0;

    int mWidth;
    int mHegiht;

    int64_t mPts;
    uint64_t m_timestamp_ms = 0;

};
