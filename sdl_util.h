/*
 * @Author: hongqianhui hongqianhui@bytedance.com
 * @Date: 2022-05-03 14:04:21
 * @LastEditors: hongqianhui hongqianhui@bytedance.com
 * @LastEditTime: 2022-05-03 21:52:42
 * @FilePath: /c_pro/sdl_demo/sdl_util.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

/* libass stores an RGBA color in the format RRGGBBTT, where TT is the transparency level */
#define AR(c) ((c) >> 24)
#define AG(c) (((c) >> 16) & 0xFF)
#define AB(c) (((c) >> 8) & 0xFF)
#define AA(c) ((0xFF - (c)) & 0xFF)

#include <libavformat/avformat.h>

int fill_frame(AVFrame *f, enum AVPixelFormat pix_fmt, int width, int height);