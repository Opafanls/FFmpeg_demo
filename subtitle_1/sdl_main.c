/*
 * @Author: hongqianhui
 * @Date: 2022-05-01 23:40:34
 * @LastEditTime: 2022-05-05 21:28:16
 * @LastEditors: hongqianhui hongqianhui@bytedance.com
 * @Description: ffmpeg解码,sdl显示数据
 * @FilePath: /c_pro/sdl_demo/sdl_main.c
 */
#include <SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>
#include <ass/ass.h>
#include <libswscale/swscale.h>

#include "sdl_util.h"
#include "ass.h"

static AVFormatContext *fmt_ctx = NULL;
static AVCodec *codec = NULL;
static AVStream *v_stream = NULL;
static AVCodecContext *av_codec_ctx = NULL;
static int mWidth, mHeight;
static int index_video_stream = -1;
// SDL
static SDL_Window *mainWindow = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static SDL_Event event;
// ass
ASS_Library *ass_library;
ASS_Renderer *ass_render;
ASS_Track *ass_track;
// png
//  static image_t *gen_image(AVFrame *frame, int width, int height);

// process
typedef struct ProcessCtx
{
    ASS_Image *img;
    ImageT *image_t;
    int frame_cnt;
} ProcessCtx;

static int process_frame(ProcessCtx *pCtx, AVFrame *src, AVFrame *dst);
static int init_ass(char *ass_file_name);
static void print_frame_info(AVFrame *frame);

void init_ffmpeg()
{
    avformat_network_init();
}

int init_sdl()
{
    int res = SDL_Init(SDL_INIT_EVERYTHING);
    if (res < 0)
    {
        printf("sdl lib init failed, ret < 0\n");
        return -1;
    }
    mainWindow = SDL_CreateWindow("Hello World For SDL.H 2",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, mWidth, mHeight, SDL_WINDOW_ALLOW_HIGHDPI);
    if (mainWindow == NULL)
    {
        printf("main window init failed, is NULL\n");
        return -1;
    }
    //从窗体创建渲染器
    renderer = SDL_CreateRenderer(mainWindow, -1, 0);
    //创建渲染器纹理，我的视频编码格式是SDL_PIXELFORMAT_IYUV，可以从FFmpeg探测到的实际结果设置颜色格式
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, mWidth, mHeight);
    if (renderer == NULL || texture == NULL)
    {
        return -1;
    }
    return 0;
}

int free_sdl()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(mainWindow);
    SDL_Quit();
    return 0;
}

int main(int argv, char **args)
{
    if (argv < 3)
    {
        printf("input 1.source url;2.ass file\n");
        return 1;
    }
    init_ffmpeg();
    char *source_url = args[1];
    char *ass_file = args[2];
    printf("source url is: %s, ass: %s\n", source_url, ass_file);
    if (avformat_open_input(&fmt_ctx, source_url, NULL, NULL) < 0)
    {
        printf("open %s failed", source_url);
        exit(1);
    }
    if ((avformat_find_stream_info(fmt_ctx, NULL)) < 0)
    {
        printf("Cannot find stream information\n");
        return 1;
    }
    index_video_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    v_stream = fmt_ctx->streams[index_video_stream];
    mWidth = v_stream->codecpar->width;
    mHeight = v_stream->codecpar->height;
    if (init_ass(ass_file) < 0)
    {
        printf("init ass file failed\n");
        exit(1);
    }
    else
    {
        printf("init ass_file ok!\n");
    }

    codec = avcodec_find_decoder(v_stream->codecpar->codec_id);
    av_codec_ctx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(av_codec_ctx, v_stream->codecpar) < 0)
    { // 拷贝参数
        printf("avcodec param to ctx failed\n");
        exit(1);
    }
    av_codec_ctx->thread_count = 8;
    if (avcodec_open2(av_codec_ctx, codec, NULL) < 0)
    {
        printf("avcodec_open2 failed");
        exit(1);
    }

    if (init_sdl() < 0)
    {
        printf("init sdl failed");
        exit(1);
    }
    // open finished
    ProcessCtx *pCtx = (ProcessCtx *)malloc(sizeof(ProcessCtx));
    int buf_size = av_image_get_buffer_size(av_codec_ctx->pix_fmt, av_codec_ctx->width, av_codec_ctx->height, 1);
    printf("alloc: %d bytes for img buffer\n", buf_size);
    ImageT *image_t = (ImageT *)malloc(sizeof(ImageT));
    image_t->height = av_codec_ctx->height;
    image_t->width = av_codec_ctx->width;
    image_t->stride = av_codec_ctx->width * 3;
    pCtx->image_t = image_t;
    pCtx->frame_cnt = 0;

    // alloc packet
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *dst = av_frame_alloc();
    int ret = fill_frame(dst, AV_PIX_FMT_RGB24, av_codec_ctx->width, av_codec_ctx->height);
    if (ret < 0)
    {
        printf("fill dst frame failed\n");
        exit(1);
    }
    printf("alloc dst frame ok!\n");
    while (1)
    {
        int ret = av_read_frame(fmt_ctx, pkt);
        if (ret < 0)
        {
            printf("reach end of file?\n");
            break;
        }
        if (pkt->stream_index != index_video_stream)
        {
            // printf("pkt is not video stream\n");
            av_packet_unref(pkt); // reuse pkt
            continue;
        }
        // send the packet to decoder thread, and the packet will be copied
        // so you should unref the packet at this interate finish
        if (avcodec_send_packet(av_codec_ctx, pkt) != 0)
        {
            printf("decode the feame %d failed\n", pCtx->frame_cnt);
            av_packet_unref(pkt);
            continue;
        }
        av_packet_unref(pkt);
        while (1)
        {
            ret = avcodec_receive_frame(av_codec_ctx, frame);
            if (ret == AVERROR(EAGAIN))
            {
                //当前这次没有解码后的音视频帧输出,但是后续可以继续读取
                break;
            }
            if (ret == AVERROR_EOF)
            {
                //解码缓冲区已经刷新完成,后续不再有数据输出
                break;
            }
            if (ret < 0)
            {
                printf("recv frame failed: code:[%d]", ret);
                break;
            }
            if (frame->format == AV_PIX_FMT_YUV420P)
            {
                print_frame_info(frame);
                pCtx->frame_cnt++;
                int ret;
                if (pCtx->frame_cnt != 0 /*&& pCtx->frame_cnt % 30 == 0*/)
                {
                    AVRational al;
                    al = v_stream->time_base;
                    double v = al.num / (double) al.den;
                    long long ts = frame->pts*v*1000;
                    printf("ts: %lld,pts:%lld,%d:%d\n", ts,frame->pts,v_stream->time_base.num, v_stream->time_base.den);
                    pCtx->img = ass_img(ass_render, ass_track, ts);
                    ret = process_frame(pCtx, frame, dst);
                    if (ret < 0)
                    {
                        printf("process frame failed %d", ret);
                    }
                    // reset pCtx
                    pCtx->img = NULL;
                }
                // printf("render %d yuvframe\n", vframeCnt);
                if (SDL_UpdateYUVTexture(texture, NULL, frame->data[0], frame->linesize[0],
                                         frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]) != 0)
                {
                    printf("update yuv texture failed!\n");
                    continue;
                }
                ret = SDL_RenderClear(renderer);
                if (ret < 0)
                {
                    printf("sdl clear render failed %s", SDL_GetError());
                    continue;
                }
                ret = SDL_RenderCopy(renderer, texture, NULL, NULL);
                if (ret < 0)
                {
                    printf("sdl copy render failed, %s", SDL_GetError());
                    continue;
                }
                SDL_RenderPresent(renderer);
                SDL_Delay(1000/av_codec_ctx->framerate.den);
            }
            else
            {
                printf("frame format is not yuv420p!but: %d\n", frame->format);
            }
            SDL_PollEvent(&event);
            switch (event.type)
            {
            case SDL_QUIT:
                SDL_Quit();
                exit(0);
                break;
            default:
                break;
            }
            av_frame_unref(frame);
            // av_frame_unref(dst);
        }
    }
    if (av_codec_ctx)
    {
        avcodec_close(av_codec_ctx);
    }
    av_frame_unref(dst);
    avformat_close_input(&fmt_ctx);
    av_packet_free(&pkt);
    free(pCtx);
    free_sdl();
    return 0;
}

static int init_ass(char *ass_file_name)
{
    ass_library = ass_library_init();
    if (!ass_library)
    {
        printf("init ass libbrary failed\n");
        return -1;
    }
    ass_set_extract_fonts(ass_library, 1);
    ass_render = ass_renderer_init(ass_library);
    if (!ass_render)
    {
        printf("init ass render failed\n");
        return -1;
    }
    ass_set_frame_size(ass_render, mWidth, mHeight);
    ass_set_fonts(ass_render, NULL, "sans-serif",
                  ASS_FONTPROVIDER_AUTODETECT, NULL, 1);
    ass_track = ass_read_file(ass_library, ass_file_name, NULL);
    return 0;
}

static int process_frame(ProcessCtx *pCtx, AVFrame *src, AVFrame *dst)
{
    int ret;
    struct SwsContext *sws_ctx;
    sws_ctx = sws_getContext(av_codec_ctx->width, av_codec_ctx->height, av_codec_ctx->pix_fmt, av_codec_ctx->width,
                             av_codec_ctx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
    if (sws_ctx == NULL)
    {
        return -2;
    }
    // convert to rgb
    uint8_t *dst_data[4];
    int dst_linesize[4];
    ret = av_image_alloc(dst_data, dst_linesize, av_codec_ctx->width, av_codec_ctx->height, AV_PIX_FMT_RGB24, 1);
    if (ret < 0)
    {
        printf("alloc image buf failed\n");
        return ret;
    }
    ret = sws_scale(sws_ctx, (const uint8_t *const *)src->data, src->linesize, 0, av_codec_ctx->height, dst_data, dst_linesize);
    sws_freeContext(sws_ctx); // free the mem
    if (ret > 0)
    {
        pCtx->image_t->buffer = dst_data[0];
        char str[20];
        int cnt = pCtx->frame_cnt;
        sprintf(str, "output%d.png", cnt);
        // blend ass img and raw frame data
        ASS_Image *as_img = pCtx->img;
        int res = blend(as_img, pCtx->image_t);
        if (res < 0)
        {
            printf("blend img failed\n");
        }
        else
        {
            // printf("blend [%d] img to raw frame\n", res);
        }
        // if (write_png(str, pCtx->image_t) < 0)
        // {
        //     printf("write png failed\n");
        //     ret = -3;
        // }
        // set the subtitle frame to video
        if (ret > 0)
        {
            sws_ctx = sws_getContext(av_codec_ctx->width, av_codec_ctx->height, AV_PIX_FMT_RGB24, av_codec_ctx->width,
                                     av_codec_ctx->height, av_codec_ctx->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
            if (sws_ctx == NULL)
            {
                printf("[subtitle_video] sws_ctx failed\n");
            }
            else
            {
                ret = sws_scale(sws_ctx, (const uint8_t *const *)dst_data, dst_linesize, 0, av_codec_ctx->height, src->data, src->linesize);
                sws_freeContext(sws_ctx); // free the mem
                if (res < 0)
                {
                    printf("[subtitle_video] sws_scale failed\n");
                }
            }
        }
    }
    av_freep(&dst_data[0]);

    return ret;
}

static void print_frame_info(AVFrame *frame) {
    int gop_size = av_codec_ctx->gop_size;
    
}