/*
 * @Author: hongqianhui hongqianhui@bytedance.com
 * @Date: 2022-05-03 14:04:26
 * @LastEditors: hongqianhui hongqianhui@bytedance.com
 * @LastEditTime: 2022-05-03 21:52:32
 * @FilePath: /c_pro/sdl_demo/sdl_util.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "sdl_util.h"


int fill_frame(AVFrame *f, enum AVPixelFormat pix_fmt, int width, int height)
{    
    f->format = pix_fmt;
    f->height = height;
    f->width = width;
    if (av_frame_get_buffer(f, 0) < 0)
    {
        return -1;
    }
    return 0;
}