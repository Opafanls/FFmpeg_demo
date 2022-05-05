/*
 * @Author: hongqianhui hongqianhui@bytedance.com
 * @Date: 2022-05-03 18:57:26
 * @LastEditors: hongqianhui hongqianhui@bytedance.com
 * @LastEditTime: 2022-05-04 22:27:40
 * @FilePath: /c_pro/sdl_demo/ass.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#include "ass.h"
#include "sdl_util.h"
#include <png.h>
#include <stdlib.h>

void blend_single(ImageT *frame, ASS_Image *img);

ASS_Image *ass_img(ASS_Renderer *render, ASS_Track *track, long long ts)
{
    int detect_change = 0;
    ASS_Image *img =
        ass_render_frame(render, track, ts, &detect_change);

    return img;
}


int blend(ASS_Image *img, ImageT *frame)
{
    int img_cnt = 0;
    while (img)
    {
        img_cnt++;
        blend_single(frame, img);
        img = img->next;
    }
    return img_cnt;
}

void blend_single(ImageT *frame, ASS_Image *img)
{
    int x, y;
    // unsigned char opacity = 255 - AA(img->color);
    unsigned char opacity = 255;
    unsigned char r = AR(img->color);
    unsigned char g = AG(img->color);
    unsigned char b = AB(img->color);

    unsigned char *src;
    unsigned char *dst;

    src = img->bitmap;  //ass img
    dst = frame->buffer + img->dst_y * frame->stride + img->dst_x * 3;
    for (y = 0; y < img->h; ++y)
    {
        for (x = 0; x < img->w; ++x)
        {
            unsigned k = ((unsigned)src[x]) * opacity / 255;
            // possible endianness problems
            dst[x * 3] = (k * b + (255 - k) * dst[x * 3]) / 255;
            dst[x * 3 + 1] = (k * g + (255 - k) * dst[x * 3 + 1]) / 255;
            dst[x * 3 + 2] = (k * r + (255 - k) * dst[x * 3 + 2]) / 255;
        }
        src += img->stride;
        dst += frame->stride;
    }
}


int write_png(char *fname, ImageT *img)
{
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_byte **row_pointers;
    int k;

    png_ptr =
        png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);
    fp = NULL;

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return -1;
    }

    fp = fopen(fname, "wb");
    if (fp == NULL)
    {
        printf("PNG Error opening %s for writing!\n", fname);
        return -2;
    }

    png_init_io(png_ptr, fp);
    png_set_compression_level(png_ptr, 0);

    png_set_IHDR(png_ptr, info_ptr, img->width, img->height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    png_set_bgr(png_ptr);

    row_pointers = malloc(img->height * sizeof(png_byte *));
    for (k = 0; k < img->height; k++)
        row_pointers[k] = img->buffer + img->stride * k;

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(row_pointers);

    fclose(fp);
    return 0;
}