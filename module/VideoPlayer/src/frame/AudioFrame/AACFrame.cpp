#include "AACFrame.h"

#include "util/util.h"

AACFrame::AACFrame()
{
    mFrameBuffer = nullptr;
    mFrameBufferSize = 0;
    m_timestamp_ms = Util::GetUtcTime();
}

AACFrame::~AACFrame()
{
    if (mFrameBuffer != nullptr)
    {
        free(mFrameBuffer);

        mFrameBuffer = nullptr;
        mFrameBufferSize = 0;
    }
}

void AACFrame::setAdtsHeader(const ADTS_HEADER &adts)
{
    mAdtsHeader = adts;
}

void AACFrame::setFrameBuffer(const uint8_t * const buffer, const unsigned int &size)
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

void AACFrame::setFrameBuffer(const uint8_t * const adtsBuffer, const unsigned int &adtsSize, const uint8_t * const buffer, const unsigned int &size)
{
    if (mFrameBufferSize < (size+adtsSize))
    {
        if (mFrameBuffer != nullptr)
        {
            free(mFrameBuffer);
        }

        mFrameBuffer = static_cast<uint8_t*>(malloc(size+adtsSize));
    }

    memcpy(mFrameBuffer, adtsBuffer, adtsSize);
    memcpy(mFrameBuffer+adtsSize, buffer, size);
    mFrameBufferSize = (size+adtsSize);
}
