#ifndef H_DECODER
#define H_DECODER

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <libswresample/swresample.h>
#include <pthread.h>

typedef struct DecoderContext {
    // AVFormatContext holds the header information from the format (Container)
    // Allocating memory for this component
    // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
    AVFormatContext *av_format_ctx;

    AVCodec *video_avc;
    AVCodec *audio_avc;
    AVStream *video_avs;
    AVStream *audio_avs;
    AVCodecContext *video_avcc;
    AVCodecContext *audio_avcc;
    int video_index;
    int audio_index;

    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    AVFrame *av_frame;

    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    AVPacket *av_packet;

    SwrContext *swr_ctx;
    uint8_t convert_to_rgb;
    uint8_t id;
    uint8_t loop;
    double video_frame_rate;
    double video_duration_in_sec;
    double audio_duration_in_sec;

    uint8_t loop_id;
    uint8_t last_loop_id;

    int audio_frequency;
    int audio_channels;
    pthread_mutex_t lock;
} DecoderContext;

typedef struct ProcessOutput {
    AVFrame *videoFrame;
    AVFrame *audioFrame;
    uint8_t loop_id;
} ProcessOutput;

DecoderContext *decoder_create(const char *url, uint8_t id, uint8_t convert_to_rgb);

int decoder_process_frame(DecoderContext *dectx, ProcessOutput *processOutput);

void decoder_destroy(DecoderContext *dectx);

void decoder_seek(DecoderContext *dectx, float timeInSeconds);

#endif