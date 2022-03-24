#include "decoder.h"
#include "logger.h"
#include "utils.h"

#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>

static int lastID = 0;

int fill_stream_info(DecoderContext *dectx, AVStream *avs, AVCodec **avc, AVCodecContext **avcc) {
  *avc = (AVCodec *) avcodec_find_decoder(avs->codecpar->codec_id);
  if (!*avc) {
    logging("%d failed to find the codec", dectx->id);
    return -1;
  }

  *avcc = avcodec_alloc_context3(*avc);
  if (!*avcc) {
    logging("%d failed to alloc memory for codec context", dectx->id);
    return -1;
  }

  if (avcodec_parameters_to_context(*avcc, avs->codecpar) < 0) {
    logging("%d failed to fill codec context", dectx->id);
    return -1;
  }

  if (avcodec_open2(*avcc, *avc, NULL) < 0) {
    logging("%d failed to open codec", dectx->id);
    return -1;
  }
  return 0;
}

int prepare_swr(DecoderContext *dectx) {
  int errorCode = 0;
  int64_t inChannelLayout = av_get_default_channel_layout(dectx->audio_avcc->channels);
  uint64_t outChannelLayout = inChannelLayout;
  enum AVSampleFormat inSampleFormat = dectx->audio_avcc->sample_fmt;
  enum AVSampleFormat outSampleFormat = AV_SAMPLE_FMT_FLT;
  int inSampleRate = dectx->audio_avcc->sample_rate;
  int outSampleRate = inSampleRate;

  if (dectx->swr_ctx != NULL) {
    swr_close(dectx->swr_ctx);
    swr_free(&dectx->swr_ctx);
    dectx->swr_ctx = NULL;
  }

  dectx->swr_ctx = swr_alloc_set_opts(NULL,
                                      outChannelLayout, outSampleFormat, outSampleRate,
                                      inChannelLayout, inSampleFormat, inSampleRate,
                                      0, NULL);


  if (swr_is_initialized(dectx->swr_ctx) == 0) {
    errorCode = swr_init(dectx->swr_ctx);
  }

  dectx->audio_channels = av_get_channel_layout_nb_channels(outChannelLayout);
  dectx->audio_frequency = outSampleRate;
  return errorCode;
}

int prepare_decoder(DecoderContext *dectx) {
  logging("%d preparing decoder", dectx->id);
  for (int i = 0; i < dectx->av_format_ctx->nb_streams; i++) {
    if (dectx->av_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      dectx->video_avs = dectx->av_format_ctx->streams[i];
      dectx->video_index = i;

      if (fill_stream_info(dectx, dectx->video_avs, &dectx->video_avc, &dectx->video_avcc)) { return -1; }

      dectx->video_frame_rate = av_q2d(dectx->video_avs->r_frame_rate);

      if (dectx->video_avs->duration != AV_NOPTS_VALUE) {
        dectx->video_duration_in_sec =
            dectx->video_avs->duration <= 0 ? (double) (dectx->av_format_ctx->duration) / AV_TIME_BASE :
            dectx->video_avs->duration * av_q2d(dectx->video_avs->time_base);
      } else {
        dectx->video_duration_in_sec = 0.0;
      }

    } else if (dectx->av_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      dectx->audio_avs = dectx->av_format_ctx->streams[i];
      dectx->audio_index = i;

      if (fill_stream_info(dectx, dectx->audio_avs, &dectx->audio_avc, &dectx->audio_avcc)) { return -1; }
      prepare_swr(dectx);

      if (dectx->audio_avs->duration != AV_NOPTS_VALUE) {
        dectx->audio_duration_in_sec =
            dectx->audio_avs->duration <= 0 ? (double) (dectx->av_format_ctx->duration) / AV_TIME_BASE :
            dectx->audio_avs->duration * av_q2d(dectx->audio_avs->time_base);
      } else {
        dectx->audio_duration_in_sec = 0.0;
      }

    } else {
      logging("%d skipping streams other than audio and video", dectx->id);
    }
  }

  logging("%d decoder prepared", dectx->id);
  return 0;
}

DecoderContext *decoder_create(const char *url, uint8_t id, uint8_t convert_to_rgb) {
  DecoderContext *dectx = (DecoderContext *) calloc(1, sizeof(DecoderContext));
  dectx->id = id;
  dectx->loop_id = 0;
  dectx->last_loop_id = 0;
  logging("%d initializing all the containers, codecs and protocols.", dectx->id);

  dectx->loop = 0;

  if (pthread_mutex_init(&dectx->lock, NULL) != 0) {
    logging("%d ERROR decoder mutex init failed", dectx->id);
    goto free;
  }

  // AVFormatContext holds the header information from the format (Container)
  // Allocating memory for this component
  // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
  dectx->av_format_ctx = avformat_alloc_context();
  if (!dectx->av_format_ctx) {
    logging("%d ERROR could not allocate memory for Format Context", dectx->id);
    goto free;
  }

  logging("%d opening the input file (%s) and loading format (container) header", dectx->id, url);
  // Open the file and read its header. The codecs are not opened.
  // The function arguments are:
  // AVFormatContext (the component we allocated memory for),
  // url (filename),
  // AVInputFormat (if you pass NULL it'll do the auto detect)
  // and AVDictionary (which are options to the demuxer)
  // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
  if (avformat_open_input(&dectx->av_format_ctx, url, NULL, NULL) != 0) {
    logging("%d ERROR could not open the file", dectx->id);
    goto free;
  }

  // now we have access to some information about our file
  // since we read its header we can say what format (container) it's
  // and some other information related to the format itself.
  logging("%d format %s, duration %lld us, bit_rate %lld", dectx->id, dectx->av_format_ctx->iformat->name,
          dectx->av_format_ctx->duration, dectx->av_format_ctx->bit_rate);

  logging("%d finding stream info from format", dectx->id);
  // read Packets from the Format to get stream information
  // this function populates dectx->av_format_ctx->streams
  // (of size equals to dectx->av_format_ctx->nb_streams)
  // the arguments are:
  // the AVFormatContext
  // and options contains options for codec corresponding to i-th stream.
  // On return each dictionary will be filled with options that were not found.
  // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
  if (avformat_find_stream_info(dectx->av_format_ctx, NULL) < 0) {
    logging("%d ERROR could not get the stream info", dectx->id);
    goto free;
  }

  if (prepare_decoder(dectx)) {
    logging("%d ERROR could not prepare the decoder", dectx->id);
    goto free;
  }
  // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
  AVFrame *pFrame = av_frame_alloc();
  if (!pFrame) {
    logging("%d failed to allocated memory for AVFrame", dectx->id);
    goto free;
  }
  // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
  AVPacket *pPacket = av_packet_alloc();
  if (!pPacket) {
    logging("%d failed to allocated memory for AVPacket", dectx->id);
    goto free;
  }

  dectx->av_frame = pFrame;
  dectx->av_packet = pPacket;

  logging("%d video_duration=%lf audio_duration=%lf fps=%lf", dectx->id, dectx->video_duration_in_sec, dectx->audio_duration_in_sec, dectx->video_frame_rate);
  return dectx;
free:
  decoder_destroy(dectx);
  return NULL;
}

#ifdef SAVE_FRAME_TO_FILE
static void save_ppm_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
  FILE *f;
  int i;
  f = fopen(filename, "w");
  // writing the minimal required header for a pgm file format
  // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
  fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);

  // writing line by line
  for (i = 0; i < ysize; i++)
      fwrite(buf + i * wrap, 3, xsize, f);
  fclose(f);
}
#endif


AVFrame *convert_to_rgb24(AVFrame *srcFrame, int frameNumber) {
  int width = srcFrame->width;
  int height = srcFrame->height;

  enum AVPixelFormat dstFormat = AV_PIX_FMT_RGB24;
  int bufSize = av_image_get_buffer_size(dstFormat, width, height, 1);
  AVBufferRef *buf = av_buffer_alloc(bufSize);

  AVFrame *dstFrame = av_frame_alloc();

  av_frame_copy_props(dstFrame, srcFrame);
  dstFrame->format = dstFormat;
  dstFrame->buf[0] = buf;

  av_image_fill_arrays(dstFrame->data, dstFrame->linesize, buf->data, dstFormat, width, height, 1);

  struct SwsContext *conversion = sws_getContext(width,
                                                 height,
                                                 srcFrame->format,
                                                 width,
                                                 height,
                                                 dstFormat,
                                                 SWS_FAST_BILINEAR,
                                                 NULL,
                                                 NULL,
                                                 NULL);
  sws_scale(conversion, (const uint8_t **) srcFrame->data, srcFrame->linesize, 0, height, dstFrame->data,
            dstFrame->linesize);
  sws_freeContext(conversion);

  dstFrame->format = dstFormat;
  dstFrame->width = srcFrame->width;
  dstFrame->height = srcFrame->height;

#ifdef SAVE_FILE_TO_FILE
  char frame_filename[1024];
  snprintf(frame_filename, sizeof(frame_filename), "out/%s-%d.ppm", "frame", frameNumber);
  save_ppm_frame(dstFrame->data[0], dstFrame->linesize[0], dstFrame->width, dstFrame->height, frame_filename);
#endif
  return dstFrame;
}

int decode_packet(DecoderContext *dectx, AVCodecContext *avcc, AVPacket *av_packet, AVFrame *av_frame) {
  // Supply raw packet data as input to a decoder
  // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
  int response = avcodec_send_packet(avcc, av_packet);

  if (response < 0) {
    logging("%d Error while sending a packet to the decoder: %s", dectx->id, av_err2str(response));
    return response;
  }

  while (response >= 0) {
    // Return decoded output data (into a frame) from a decoder
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
    response = avcodec_receive_frame(avcc, av_frame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      logging("%d Error while receiving a frame from the decoder: %s", dectx->id, av_err2str(response));
      return response;
    }

    if (response >= 0) {
      /*logging(
          "Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame %d [DTS %d]",
          avcc->frame_number,
          av_get_picture_type_char(av_frame->pict_type),
          av_frame->pkt_size,
          av_frame->format,
          av_frame->pts,
          av_frame->key_frame,
          av_frame->coded_picture_number
      );*/
      return 0;
    }
  }
  return -1;
}

AVFrame *process_video_frame(DecoderContext *dectx, AVFrame *frame, int frameNumber) {
  if (dectx->convert_to_rgb == 1) {
    return convert_to_rgb24(frame, frameNumber);
  } else {
    AVFrame *new_frame = av_frame_alloc();
    dectx->av_frame = new_frame; // Don't convert, create a new frame for decoding and use current frame to process (it will be released by the player)
    return frame;
  }
}

AVFrame *process_audio_frame(DecoderContext *dectx) {
  AVFrame *frame = dectx->av_frame;
  AVFrame *frameConverted = av_frame_alloc();
  frameConverted->sample_rate = frame->sample_rate;
  frameConverted->channel_layout = av_get_default_channel_layout(dectx->audio_channels);
  frameConverted->format = AV_SAMPLE_FMT_FLT;  //	For Unity format.
  frameConverted->best_effort_timestamp = frame->best_effort_timestamp;
  swr_convert_frame(dectx->swr_ctx, frameConverted, frame);
  return frameConverted;
}

int decoder_process_frame(DecoderContext *dectx, ProcessOutput *processOutput) {
  pthread_mutex_lock(&dectx->lock);
  processOutput->videoFrame = NULL;
  processOutput->audioFrame = NULL;
  int res = -1;
  // fill the Packet with data from the Stream
  // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
  if (av_read_frame(dectx->av_format_ctx, dectx->av_packet) >= 0) {
    if (dectx->av_packet->stream_index == dectx->video_index) {
      if (dectx->loop_id == dectx->last_loop_id) {
        dectx->loop_id = get_next_id(&dectx->last_loop_id);
      }
      processOutput->loop_id = dectx->loop_id;

      double timeInSec = (double) (av_q2d(dectx->video_avs->time_base) *
                                   (double) dectx->av_frame->best_effort_timestamp);
      //logging("[VIDEO] AVPacket frame-number=%d timeInSec=%lf", dectx->video_avcc->frame_number, timeInSec);
      res = decode_packet(dectx, dectx->video_avcc, dectx->av_packet, dectx->av_frame);
      if (res < 0) {
        pthread_mutex_unlock(&dectx->lock);
        return -1;
      }
      processOutput->videoFrame = process_video_frame(dectx, dectx->av_frame, dectx->video_avcc->frame_number);
      res = 0;
    } else if (dectx->av_packet->stream_index == dectx->audio_index) {
      //logging("[AUDIO] AVPacket->pts %" PRId64, dectx->av_packet->pts);
      res = decode_packet(dectx, dectx->audio_avcc, dectx->av_packet, dectx->av_frame);
      if (res < 0) {
        pthread_mutex_unlock(&dectx->lock);
        return -1;
      }
      processOutput->audioFrame = process_audio_frame(dectx);
      res = 0;
    }
    // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
    av_packet_unref(dectx->av_packet);
    pthread_mutex_unlock(&dectx->lock);
  } else {

    if (dectx->loop == 1) {
      if (dectx->loop_id != dectx->last_loop_id) {
        logging("%d decoder: loop", dectx->id);
        dectx->loop_id = dectx->last_loop_id;

        pthread_mutex_unlock(&dectx->lock); // Avoid deadlock
        decoder_seek(dectx, 0.0f);
      }
    } else {
      pthread_mutex_unlock(&dectx->lock);
    }
  }

  return res;
}

void decoder_seek(DecoderContext *dectx, float timeInSeconds) {
  pthread_mutex_lock(&dectx->lock);
  uint64_t timestamp = (uint64_t) timeInSeconds * AV_TIME_BASE;
  av_seek_frame(dectx->av_format_ctx, -1, timestamp, AVSEEK_FLAG_BACKWARD);
  pthread_mutex_unlock(&dectx->lock);
}

void decoder_destroy(DecoderContext *dectx) {
  logging("%d releasing all the resources", dectx->id);

  if (dectx->av_format_ctx)
    avformat_close_input(&dectx->av_format_ctx);
  if (dectx->av_packet)
    av_packet_free(&dectx->av_packet);
  if (dectx->av_frame)
    av_frame_free(&dectx->av_frame);
  if (dectx->video_avcc)
    avcodec_free_context(&dectx->video_avcc);
  if (dectx->audio_avcc)
    avcodec_free_context(&dectx->audio_avcc);

  pthread_mutex_destroy(&dectx->lock);

  if (dectx->swr_ctx != NULL) {
    swr_close(dectx->swr_ctx);
    swr_free(&dectx->swr_ctx);
  }

  free(dectx);
}
