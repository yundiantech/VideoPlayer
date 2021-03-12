#include "VideoFrame.h"

VideoFrame::VideoFrame()
{
    mYuv420Buffer = nullptr;
}

VideoFrame::~VideoFrame()
{
    if (mYuv420Buffer != nullptr)
    {
        free(mYuv420Buffer);
        mYuv420Buffer = nullptr;
    }
}

void VideoFrame::initBuffer(const int &width, const int &height)
{
    if (mYuv420Buffer != nullptr)
    {
        free(mYuv420Buffer);
        mYuv420Buffer = nullptr;
    }

    mWidth  = width;
    mHegiht = height;

    mYuv420Buffer = (uint8_t*)malloc(width * height * 3 / 2);

}

void VideoFrame::setYUVbuf(const uint8_t *buf)
{
    int Ysize = mWidth * mHegiht;
    memcpy(mYuv420Buffer, buf, Ysize * 3 / 2);
}

void VideoFrame::setYbuf(const uint8_t *buf)
{
    int Ysize = mWidth * mHegiht;
    memcpy(mYuv420Buffer, buf, Ysize);
}

void VideoFrame::setUbuf(const uint8_t *buf)
{
    int Ysize = mWidth * mHegiht;
    memcpy(mYuv420Buffer + Ysize, buf, Ysize / 4);
}

void VideoFrame::setVbuf(const uint8_t *buf)
{
    int Ysize = mWidth * mHegiht;
    memcpy(mYuv420Buffer + Ysize + Ysize / 4, buf, Ysize / 4);
}

