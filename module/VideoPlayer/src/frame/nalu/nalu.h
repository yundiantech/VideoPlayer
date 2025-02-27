#ifndef NALU_H
#define NALU_H

#include <stdint.h>
#include <stdlib.h>

#include "h264.h"
#include "h265.h"

enum T_NALU_TYPE
{
    T_NALU_H264 = 0,
    T_NALU_H265,
};

typedef struct
{
    T_NALU_TYPE type;

    union
    {
        T_H264_NALU h264Nalu;
        T_H265_NALU h265Nalu;
    }nalu;

} T_NALU;

///用于从连续的h264/h265数据中解析出nalu
class NALUParsing
{
public:
    NALUParsing();
    ~NALUParsing();

    int inputData(T_NALU_TYPE type, uint8_t *buf, int len, bool clear_old_buffer); //输入h264数据

    ///从H264数据中查找出一帧数据
    T_NALU* getNextFrame();

    int getIDRHeaderBuffer(uint8_t *buf, unsigned int &buf_len); //获取IDR帧头部信息(vps+sps+pps)

private:
    uint8_t *m_buffer = nullptr;
    int m_buffer_size = 0;
    int m_buffer_max_size = 0;

    T_NALU_TYPE m_video_type; //类型 区分是264还是265

public:
    ///为NALU_t结构体分配内存空间
    static T_NALU *AllocNALU(const int &buffer_size, const T_NALU_TYPE &type, const bool &is_alloc_buffer = true);

    ///释放
    static void FreeNALU(T_NALU *n);

};


#endif // NALU_H
