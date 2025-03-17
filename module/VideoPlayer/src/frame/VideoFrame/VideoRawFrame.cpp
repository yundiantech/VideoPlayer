#include "VideoRawFrame.h"
#include <stdio.h>

#include "util/util.h"

#ifdef ENABLE_FFMPEG
extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
}
#endif

VideoRawFrame::VideoRawFrame()
{
    mFrameBuffer = nullptr;
    mFrameBufferSize = 0;
    mPts = 0;
    m_timestamp_ms = Util::GetUtcTime();
}

VideoRawFrame::~VideoRawFrame()
{
    if (mFrameBuffer != nullptr)
    {
        free(mFrameBuffer);
        mFrameBuffer = nullptr;
        mFrameBufferSize = 0;
    }
}

void VideoRawFrame::initBuffer(const int &width, const int &height, const FrameType &type, int64_t time)
{
    if (mFrameBuffer != nullptr)
    {
        free(mFrameBuffer);
        mFrameBuffer = nullptr;
    }

    mWidth  = width;
    mHegiht = height;

    mPts = time;

    mType = type;

    int size = 0;
    if (type == FRAME_TYPE_YUV420P)
    {
#ifdef ENABLE_FFMPEG
		size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);  //按1字节进行内存对齐,得到的内存大小最接近实际大小
#else
        size = width * height * 3 / 2;
#endif
    }
    else if (type == FRAME_TYPE_RGB8)
    {
        size = width * height;
    }
    else if (type == FRAME_TYPE_RGB24)
    {
        size = width * height * 3;
    }

    mFrameBuffer = (uint8_t*)malloc(size);
    mFrameBufferSize = size;
}

void VideoRawFrame::setFramebuf(const uint8_t *buf)
{
    if (mFrameBuffer && buf)
    {
        memcpy(mFrameBuffer, buf, mFrameBufferSize);
    }
    else
    {
        printf("%s line=%d setFramebuf error!\n", __FUNCTION__, __LINE__);
    }
}

void VideoRawFrame::setYbuf(const uint8_t *buf)
{
    int Ysize = mWidth * mHegiht;
    memcpy(mFrameBuffer, buf, Ysize);
}

void VideoRawFrame::setUbuf(const uint8_t *buf)
{
    int Ysize = mWidth * mHegiht;
    memcpy(mFrameBuffer + Ysize, buf, Ysize / 4);
}

void VideoRawFrame::setVbuf(const uint8_t *buf)
{
    int Ysize = mWidth * mHegiht;
    memcpy(mFrameBuffer + Ysize + Ysize / 4, buf, Ysize / 4);
}

