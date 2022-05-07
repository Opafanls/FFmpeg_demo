/*
 * @Author: hongqianhui hongqianhui@bytedance.com
 * @Date: 2022-05-05 23:02:04
 * @LastEditors: hongqianhui hongqianhui@bytedance.com
 * @LastEditTime: 2022-05-05 23:02:37
 * @FilePath: /c_pro/ffmpeg_demos/util.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */


#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>


void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag);