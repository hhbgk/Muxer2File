/**
 * Description:
 * Author:created by hhbgk on 17-7-24.
 */
//

#include "rts_muxer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define STREAM_FRAME_RATE 30 /* 25 images/s */

/* Add an output stream. */
static AVStream *add_streams(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id) {
    AVCodecContext *c;
    AVStream *st;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        //exit(1);
    }

    st = avformat_new_stream(oc, *codec);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    st->id = oc->nb_streams-1;
    c = st->codec;

//    switch ((*codec)->type) {
    switch (codec_id) {
        case AVMEDIA_TYPE_AUDIO:
            c->sample_fmt  = AV_SAMPLE_FMT_FLTP;
            c->bit_rate    = 64000;
            c->sample_rate = 44100;
            c->channels    = 2;
            break;

        case AVMEDIA_TYPE_VIDEO:
            c->codec_id = codec_id;

            c->bit_rate = 400000;
            /* Resolution must be a multiple of two. */
            c->width    = 1920;
            c->height   = 1080;
            /* timebase: This is the fundamental unit of time (in seconds) in terms
             * of which frame timestamps are represented. For fixed-fps content,
             * timebase should be 1/framerate and timestamp increments should be
             * identical to 1. */
            c->time_base.den = STREAM_FRAME_RATE;
            c->time_base.num = 1;
            c->gop_size      = STREAM_FRAME_RATE; /* emit one intra frame every twelve frames at most */
            c->pix_fmt       = AV_PIX_FMT_YUV420P;
            if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
                /* just for testing, we also add B frames */
                c->max_b_frames = 2;
            }
            if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
                /* Needed to avoid using macroblocks in which some coeffs overflow.
                 * This does not happen with normal video, it just happens here as
                 * the motion of the chroma plane does not match the luma plane. */
                c->mb_decision = 2;
            }
            break;

        default:
            break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

/* Add an output stream. */
static AVStream *add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id) {
    AVCodecContext *c;
    AVStream *st;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        AVOutputFormat *oformat = av_guess_format(NULL, "test.mov", NULL);
        loge("video %s, audio %s", avcodec_get_name(oformat->video_codec), avcodec_get_name(oformat->audio_codec));
        *codec = avcodec_find_encoder(oformat->video_codec);
        if (!(*codec)) {
            loge("Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
            return NULL;
        }
    }

    st = avformat_new_stream(oc, *codec);
    if (!st) {
        loge("Could not allocate stream\n");
        return NULL;
    }
    st->id = oc->nb_streams-1;
    c = st->codec;

    //logi("== nb_streams=%d, codec_id=%s, index=%d type=%d\n", oc->nb_streams, avcodec_get_name(codec_id), st->index, (*codec)->type);

    switch ((*codec)->type) {
        case AVMEDIA_TYPE_AUDIO:
            c->sample_fmt  = (*codec)->sample_fmts ?
                             (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
            c->bit_rate    = 64000;
            c->sample_rate = 44100;
            if ((*codec)->supported_samplerates) {
                c->sample_rate = (*codec)->supported_samplerates[0];
                for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                    if ((*codec)->supported_samplerates[i] == 44100)
                        c->sample_rate = 44100;
                }
            }
            c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
            c->channel_layout = AV_CH_LAYOUT_STEREO;
            if ((*codec)->channel_layouts) {
                c->channel_layout = (*codec)->channel_layouts[0];
                for (i = 0; (*codec)->channel_layouts[i]; i++) {
                    if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                        c->channel_layout = AV_CH_LAYOUT_STEREO;
                }
            }
            c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
            st->time_base = (AVRational){ 1, c->sample_rate };
            break;

        case AVMEDIA_TYPE_VIDEO:
            c->codec_id = codec_id;

            c->bit_rate = 400000;
            /* Resolution must be a multiple of two. */
            c->width    = 640;
            c->height   = 480;
            /* timebase: This is the fundamental unit of time (in seconds) in terms
             * of which frame timestamps are represented. For fixed-fps content,
             * timebase should be 1/framerate and timestamp increments should be
             * identical to 1. */
            st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
            c->time_base       = st->time_base;

            c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
            c->pix_fmt       = AV_PIX_FMT_YUV420P;
            if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
                /* just for testing, we also add B-frames */
                c->max_b_frames = 2;
            }
            if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
                /* Needed to avoid using macroblocks in which some coeffs overflow.
                 * This does not happen with normal video, it just happens here as
                 * the motion of the chroma plane does not match the luma plane. */
                c->mb_decision = 2;
            }
            break;

        default:
            break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    return st;
}

int rts_muxer_create(rts_muxer_t *client, uint8_t *path) {
    logi("%s: path=%s", __func__, path);
    int ret;
    const char *filename = (const char *)path;
    /* Initialize libavcodec, and register all codecs and formats. */
    av_register_all();
    avcodec_register_all();

    /* allocate the output media context */
    avformat_alloc_output_context2(&client->ofctx, NULL, NULL, filename);
    if (!client->ofctx) {
        loge("Could not deduce output format from file extension.");
        return -1;
    }

    client->ofmt = client->ofctx->oformat;
    client->ofmt->video_codec = AV_CODEC_ID_H264;
    client->ofmt->audio_codec = AV_CODEC_ID_PCM_S16LE;//AV_CODEC_ID_AAC;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (client->ofmt->video_codec != AV_CODEC_ID_NONE) {
        client->video_st = add_stream(client->ofctx, &client->video_codec, client->ofmt->video_codec);
    }
    if (client->ofmt->audio_codec != AV_CODEC_ID_NONE) {
        client->audio_st = add_stream(client->ofctx, &client->audio_codec, client->ofmt->audio_codec);
    }
    logw("Add stream : video %x, audio %x", client->video_st, client->audio_st);
    loge("video %s, audio %s", avcodec_get_name(client->video_st->codec->codec_id), avcodec_get_name(client->audio_st->codec->codec_id));

    if (!client->video_st || !client->audio_st) {
        loge("Add stream failed: video %x, audio %x", client->video_st, client->audio_st);
        return -3;
    }

    /* open the output file, if needed */
    if (!(client->ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&client->ofctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            loge("Could not open '%s': %s\n", filename, av_err2str(ret));
            return -4;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(client->ofctx, NULL);
    if (ret < 0) {
        loge("Error occurred when opening output file: %s\n", av_err2str(ret));
        return -5;
    }

    client->video_st->time_base.den = 90000;
    client->video_st->time_base.num = 1;
    return 0;
}
/* media file output */
int rts_muxer_write(rts_muxer_t *client, int type, uint8_t *data, int size)
{
    logi("%s:", __func__);

    int ret = -1;

    AVPacket pkt = { 0 };
    av_init_packet(&pkt);

    pkt.dts = client->pts_time;
    pkt.pts = client->pts_time;
    pkt.data = data;
    pkt.size = size;

    switch(type) {
        case TYPE_RTS_PCM:
            pkt.stream_index = client->audio_st->index;
        break;
        case TYPE_RTS_JPEG:
        case TYPE_RTS_H264:
            client->pts_time += 23*90;
            pkt.stream_index = client->video_st->index;
        break;

        default:
            loge("Error type: %d", type);
            return -1;
    }

    ret = av_interleaved_write_frame(client->ofctx, &pkt);

    if (ret != 0) {
        loge("Error while writing video frame: %s", av_err2str(ret));
        return ret;
    }
    return 0;
}

int rts_muxer_close(rts_muxer_t *client) {
    if (client && client->ofctx) {
        av_write_trailer(client->ofctx);
        avio_close(client->ofctx->pb);
        avformat_free_context(client->ofctx);
        
        logi("ffmpeg save file done !!!\n");
        return 0;
    }
    return -1;
}
