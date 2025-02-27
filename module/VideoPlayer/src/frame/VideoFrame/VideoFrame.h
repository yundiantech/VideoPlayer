#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <memory>

#include "VideoRawFrame.h"
#include "VideoEncodedFrame.h"

#define VideoFramePtr std::shared_ptr<VideoFrame>

class VideoFrame
{
public:
    enum FrameType
    {
        VIDEOFRAME_TYPE_NONE = -1,
        VIDEOFRAME_TYPE_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
        VIDEOFRAME_TYPE_RGB8,
        VIDEOFRAME_TYPE_RGB24,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
        VIDEOFRAME_TYPE_H264, 
        VIDEOFRAME_TYPE_H265, 
    };
    VideoFrame();
    ~VideoFrame();

    void setFrame(std::shared_ptr<VideoEncodedFrame> frame);
    void setFrame(std::shared_ptr<VideoRawFrame> frame);

    uint8_t *buffer();
    int size();

    int width(){return m_width;}
    int height(){return m_hegiht;}

protected:
    FrameType m_type = VIDEOFRAME_TYPE_NONE;

    int m_width = -100;
    int m_hegiht= -100;

    int64_t m_pts = 0;

    std::shared_ptr<VideoEncodedFrame> m_encoded_frame = nullptr;
    std::shared_ptr<VideoRawFrame> m_raw_frame = nullptr;

};

#endif // VIDEOFRAME_H
