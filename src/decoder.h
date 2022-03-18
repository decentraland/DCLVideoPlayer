#pragma once

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <libswresample/swresample.h>

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
    int loop;
    float video_duration_in_sec;
    float audio_duration_in_sec;

    int audio_frequency;
    int audio_channels;
} DecoderContext;

typedef struct ProcessOutput {
    AVFrame *videoFrame;
    AVFrame *audioFrame;
} ProcessOutput;

DecoderContext *decoder_create(const char *url);

int decoder_process_frame(DecoderContext *dectx, ProcessOutput *processOutput);

void decoder_destroy(DecoderContext *dectx);

void decoder_seek(DecoderContext *dectx, float timeInSeconds);