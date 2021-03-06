/**
 * @file
 * libavformat/libavcodec demuxing and muxing API example.
 *
 * Remux streams from one container format to another.
 * @example remuxing.c
 *
 * copy from ffmpeg demos
 * https://ffmpeg.org/doxygen/3.4/remuxing_8c_source.html
 * 不是最新的，最新的可以下载ffmpeg源码查看
 */

#include "../util.h"

int main(int argc, char **argv)
{
    const AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket *pkt = NULL;

    const char *in_filename, *out_filename;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;

    if (argc < 3)
    {
        printf("usage: %s input output\n"
               "API example program to remux a media file with libavformat and libavcodec.\n"
               "The output format is guessed according to the file extension.\n"
               "\n",
               argv[0]);
        return 1;
    }

    in_filename = argv[1];
    out_filename = argv[2];

    pkt = av_packet_alloc(); // alloc packet and reuse packet
    if (!pkt)
    {
        fprintf(stderr, "Could not allocate AVPacket\n");
        return 1;
    }

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0)
    {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
    {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx)
    {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    
    stream_mapping_size = ifmt_ctx->nb_streams;
    stream_mapping = av_calloc(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping)
    {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ofmt = ofmt_ctx->oformat;
    int v_stream_index = -1;
    for (i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        AVStream *out_stream;
        AVStream *in_stream = ifmt_ctx->streams[i];

        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
        {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream)
        {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0)
        {
            fprintf(stderr, "Failed to copy codec parameters\n");
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;
        if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            v_stream_index = i;
        }
    }
    if (v_stream_index == -1)
    {
        printf("v_stream is NULL\n");
        return 1;
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    if (!(ofmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }
    AVRational base = ifmt_ctx->streams[v_stream_index]->time_base;
    double ticker_t = (double)base.num / (double)base.den;
    printf(">>>>>> 1: %d, 2: %d ticker_time: %f %d\n", base.num, base.den, ticker_t, v_stream_index);
    int frame_rate = ifmt_ctx->streams[v_stream_index]->r_frame_rate.num;
    
    printf("frame_rate: %d\n", frame_rate);
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0)
    {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }
    int frame_cnt = 0;
    int64_t start_pts = 0;
    while (1)
    {
        AVStream *in_stream, *out_stream;

        ret = av_read_frame(ifmt_ctx, pkt);
        if (ret < 0)
            break;

        in_stream = ifmt_ctx->streams[pkt->stream_index];
        if (pkt->stream_index >= stream_mapping_size ||
            stream_mapping[pkt->stream_index] < 0)
        {
            av_packet_unref(pkt);
            continue;
        }

        pkt->stream_index = stream_mapping[pkt->stream_index];
        out_stream = ofmt_ctx->streams[pkt->stream_index];
        log_packet(ifmt_ctx, pkt, "in");
        
        /* copy packet */
        av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;
        log_packet(ofmt_ctx, pkt, "out");

        if (pkt->stream_index == v_stream_index)
        {
            // pkt is from video stream
            if (frame_cnt == 0)
            {
                start_pts = pkt->pts;
            }
            else
            {
                int64_t pkt_pts_diff = pkt->pts - start_pts;

                // printf("[%d]processed pkt: %lld %d/%d,pos:%lld\n", frame_cnt, pkt->pts, in_stream->time_base.num,
                //        in_stream->time_base.den,pkt->pos);
                // double sec = pkt_pts_diff * ticker_t;
                // if (sec >= 30) {
                //     printf("pkt saved ok!\n");
                //     break;
                // } else {
                //     printf("diff: %f-->%f\n", sec, v_stream->time_base.num/(double)v_stream->time_base.den);
                // }
            }
            frame_cnt++;
            double t = frame_cnt/(double)frame_rate;
            printf("time now: %f\n", t);
            if (t >= 10) {
                break;
            }
        }
        // ret = av_interleaved_write_frame(ofmt_ctx, pkt);
        /* pkt is now blank (av_interleaved_write_frame() takes ownership of
         * its contents and resets pkt), so that no unreferencing is necessary.
         * This would be different if one used av_write_frame(). */
        if (ret < 0)
        {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
    }

    av_write_trailer(ofmt_ctx);
end:
    av_packet_free(&pkt);

    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    av_freep(&stream_mapping);

    if (ret < 0 && ret != AVERROR_EOF)
    {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
