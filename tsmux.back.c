#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libavutil/opt.h>
//#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
//#include <libswscale/swscale.h>
//#include <libswresample/swresample.h>

#define STREAM_FRAME_RATE 25 /* 25 images/s */

/* Add an output stream. */
static AVStream *add_streams(AVFormatContext *oc, AVCodec **codec,
                            enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    st = avformat_new_stream(oc, *codec);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    st->id = oc->nb_streams-1;
    c = st->codec;

    switch ((*codec)->type) {
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
static AVStream *add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    st = avformat_new_stream(oc, *codec);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    st->id = oc->nb_streams-1;
   	c = st->codec;
   	
   	printf("== nb_streams=%d, codec_id=%s, index=%d type=%d\n", oc->nb_streams, avcodec_get_name(codec_id), st->index, (*codec)->type);

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

/**************************************************************/
/* media file output */

int main(int argc, char **argv)
{
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *audio_st,*video_st;
    AVCodec *audio_codec,*video_codec;

    int ret;
    char * str_length = NULL;

    char filename[32];
    sprintf(filename,"rm.mov");

    /* Initialize libavcodec, and register all codecs and formats. */
    av_register_all();

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("\nCould not deduce output format from file extension.\n");
        return 0;
    }
    if (!oc) {
        return 1;
    }
    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    video_st = NULL;
    audio_st = NULL;

    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        fmt->video_codec = AV_CODEC_ID_H264;
        video_st = add_stream(oc, &video_codec, fmt->video_codec);
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        fmt->audio_codec = AV_CODEC_ID_AAC;
        audio_st = add_stream(oc, &audio_codec, fmt->audio_codec);
    }

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    av_err2str(ret));
            return 0;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        return 0;
    }

    video_st->time_base.den = 90000;
    video_st->time_base.num = 1;

    long pts_time = 0;

    FILE * fp_hdr = fopen("test.hdr","rb");
    FILE * fp_h264 = fopen("test.h264","rb");
    FILE * fp_adr = fopen("test.adr","rb");
    FILE * fp_aac = fopen("test.aac","rb");

    FILE * fp_ts = fopen("test.ts","wb");

    printf("ffmpeg mux start : \n");
    
    int video_tag;

	while(1){
	    AVPacket pkt = { 0 };
        av_init_packet(&pkt);
        
        
        pkt.dts = pts_time;
        pkt.pts = pts_time;
        //pkt.pts = av_rescale_q(pkt.pts, audio_st->codec->time_base, oc->streams[1]->time_base);
		//pkt.dts = av_rescale_q(pkt.dts, audio_st->codec->time_base, oc->streams[1]->time_base);
		
        char str_buffer[1000*1024] = "";
        pkt.data = (unsigned char *)str_buffer;
        pkt.size = 0;
        char msg[128] = "";

        if(video_tag == 1)
        {
            pts_time += 23*90;
            pkt.stream_index = video_st->index;
            str_length = fgets(msg,sizeof(msg),fp_hdr);
            if(str_length == NULL)
                break;
            pkt.size = atoi(msg);
            ret = fread(pkt.data,pkt.size,1,fp_h264);
            if(ret <= 0)
                break;
            
            video_tag = 0;
        }else{
            pkt.stream_index = audio_st->index;
            str_length = fgets(msg,sizeof(msg),fp_adr);
            if(str_length == NULL)
                break;
            pkt.size = atoi(msg);
            ret = fread(pkt.data,pkt.size,1,fp_aac);
            if(ret <= 0)
                break;
            
            video_tag = 1;
        }

        ret = av_interleaved_write_frame(oc, &pkt);
        if (ret != 0) {
            fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
            exit(1);
        }

        fwrite(oc->pb->buffer,oc->pb->buffer_size,1,fp_ts);

	}
	
	av_write_trailer(oc);
	avio_close(oc->pb);
	avformat_free_context(oc);

    fclose(fp_hdr);
    fclose(fp_h264);
    fclose(fp_adr);
    fclose(fp_aac);
    fclose(fp_ts);

	printf("\n---FILE:%s-line %d ---\n",__FILE__,__LINE__);
	printf("ffmpeg save file done !!!\n");
	
	return 0;
}


