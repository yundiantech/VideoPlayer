#include "PCMFrame.h"

PCMFrame::PCMFrame()
{
    mFrameBuffer = nullptr;
    mFrameBufferSize = 0;
}

PCMFrame::~PCMFrame()
{
    if (mFrameBuffer != nullptr)
    {
        free(mFrameBuffer);

        mFrameBuffer = nullptr;
        mFrameBufferSize = 0;
    }
}

void PCMFrame::setFrameInfo(int sample_rate, int channels, uint32_t pts)
{
    m_sample_rate = sample_rate;
    m_channels = channels;
    m_pts = pts;
}

void PCMFrame::setFrameBuffer(const uint8_t * const buffer, const unsigned int &size)
{
    if (mFrameBufferSize < size)
    {
        if (mFrameBuffer != nullptr)
        {
            free(mFrameBuffer);
        }

        mFrameBuffer = static_cast<uint8_t*>(malloc(size));
    }

    memcpy(mFrameBuffer, buffer, size);
    mFrameBufferSize = size;
}
