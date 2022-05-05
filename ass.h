/*
 * @Author: hongqianhui hongqianhui@bytedance.com
 * @Date: 2022-05-03 18:57:30
 * @LastEditors: hongqianhui hongqianhui@bytedance.com
 * @LastEditTime: 2022-05-03 22:31:29
 * @FilePath: /c_pro/sdl_demo/ass.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <ass/ass.h>

typedef struct image_s
{
    int width, height, stride;
    uint8_t *buffer; // RGB24
} ImageT;

ASS_Image *ass_img(ASS_Renderer *render, ASS_Track *track, long long ts);

int blend(ASS_Image *img, ImageT *frame);

int write_png(char *fname, ImageT *img);