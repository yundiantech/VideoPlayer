#include "VideoFrame.h"

VideoFrame::VideoFrame()
{

}

VideoFrame::~VideoFrame()
{

}

void VideoFrame::setFrame(std::shared_ptr<VideoEncodedFrame> frame)
{
    if (frame->nalu()->type == T_NALU_H264)
    {
        m_type = VIDEOFRAME_TYPE_H264;
    }
    else if (frame->nalu()->type == T_NALU_H265)
    {
        m_type = VIDEOFRAME_TYPE_H265;
    }

    m_pts = frame->pts();

    m_encoded_frame = frame;
}

void VideoFrame::setFrame(std::shared_ptr<VideoRawFrame> frame)
{
    m_type = (FrameType)frame->type();

    m_width  = frame->width();
    m_hegiht = frame->height();
    m_pts = frame->pts();

    m_raw_frame = frame;
}