#include "PCMFrame.h"

#include "util/util.h"

PCMFrame::PCMFrame()
{
    mFrameBuffer = nullptr;
    mFrameBufferSize = 0;
    m_timestamp_ms = Util::GetUtcTime();
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

void PCMFrame::setFrameInfo(int sample_rate, int channels, uint32_t pts, FrameType type)
{
    m_sample_rate = sample_rate;
    m_channels = channels;
    m_pts = pts;
    m_type = type;    
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
