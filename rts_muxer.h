/**
 * Description:
 * Author:created by hhbk on 17-7-24.
 */
//

#ifndef IJKPLAYER_RTS_MUXER_H
#define IJKPLAYER_RTS_MUXER_H
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"

enum RTS_TYPE {
    TYPE_RTS_PCM  = 1,//pcm
    TYPE_RTS_JPEG  = 2,//JPG
    TYPE_RTS_H264  = 3,//h264
};
typedef struct {
    long pts_time;
    AVOutputFormat *ofmt;
    AVFormatContext *ofctx;
    AVStream *audio_st;
    AVStream *video_st;
    AVCodec *audio_codec;
    AVCodec *video_codec;
}rts_muxer_t;

int rts_muxer_create(rts_muxer_t *client, uint8_t *path);
int rts_muxer_write(rts_muxer_t *client, int type, uint8_t *data, int size);
int rts_muxer_close(rts_muxer_t *client);
#endif //IJKPLAYER_RTS_MUXER_H
