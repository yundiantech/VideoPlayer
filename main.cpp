
/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/pixfmt.h"
    #include "libswscale/swscale.h"
}

#include <stdio.h>

///现在我们需要做的是让SaveFrame函数能把RGB信息定稿到一个PPM格式的文件中。
///我们将生成一个简单的PPM格式文件，请相信，它是可以工作的。
void SaveFrame(AVFrame *pFrame, int width, int height,int index)
{

  FILE *pFile;
  char szFilename[32];
  int  y;

  // Open file
  sprintf(szFilename, "frame%d.ppm", index);
  pFile=fopen(szFilename, "wb");

  if(pFile==NULL)
    return;

  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  // Write pixel data
  for(y=0; y<height; y++)
  {
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  }

  // Close file
  fclose(pFile);

}


int main(int argc, char *argv[])
{
    char *file_path = "E:\\in.mp4";

    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameRGB;
    AVPacket *packet;
    uint8_t *out_buffer;

    static struct SwsContext *img_convert_ctx;

    int videoStream, i, numBytes;
    int ret, got_picture;

    av_register_all(); //初始化FFMPEG  调用了这个才能正常适用编码器和解码器

    //Allocate an AVFormatContext.
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, file_path, NULL, NULL) != 0) {
        printf("can't open the file. \n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Could't find stream infomation.\n");
        return -1;
    }

    videoStream = -1;

    ///循环查找视频中包含的流信息，直到找到视频类型的流
    ///便将其记录下来 保存到videoStream变量中
    ///这里我们现在只处理视频流  音频流先不管他
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
        }
    }

    ///如果videoStream为-1 说明没有找到视频流
    if (videoStream == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    ///查找解码器
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (pCodec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }

    ///打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
            pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
            PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,pCodecCtx->height);

    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *) pFrameRGB, out_buffer, PIX_FMT_RGB24,
            pCodecCtx->width, pCodecCtx->height);

    int y_size = pCodecCtx->width * pCodecCtx->height;

    packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
    av_new_packet(packet, y_size); //分配packet的数据

    av_dump_format(pFormatCtx, 0, file_path, 0); //输出视频信息

    int index = 0;

    while (1)
    {
        if (av_read_frame(pFormatCtx, packet) < 0)
        {
            break; //这里认为视频读取完了
        }

        if (packet->stream_index == videoStream) {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet);

            if (ret < 0) {
                printf("decode error.\n");
                return -1;
            }

            if (got_picture) {
                sws_scale(img_convert_ctx,
                        (uint8_t const * const *) pFrame->data,
                        pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
                        pFrameRGB->linesize);

                SaveFrame(pFrameRGB, pCodecCtx->width,pCodecCtx->height,index++); //保存图片
                if (index > 50) return 0; //这里我们就保存50张图片
            }
        }
        av_free_packet(packet);
    }
    av_free(out_buffer);
    av_free(pFrameRGB);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}

