/**
 * Macros used in this source code
 *
 * SHOW_PACKET_INFO       Logs packet informations
 *
 * COPY_ONLY              Will not transcode audio and video frames
 *                        for Video, only H.264 input will be acepted.
 *                        for Audio, only AAC input will be accepted.
 *                        Any other input will be discarded.
 *
 * SEND_STATUS_INFO       Allow the MonitoringThread to start functioning.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
#include <libavutil/threadmessage.h>
#include <libavutil/time.h>
#include <signal.h>
#ifdef __cplusplus
}
#endif
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include "global_alive.hpp"

#ifdef SEND_STATUS_INFO
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 23000
#include "monitoring_thread.hpp"
#endif

// See https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/transcode_aac.c

// From: https://github.com/joncampbell123/composite-video-simulator/issues/5
#if _MSC_VER
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char *)_malloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)
#else
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char *)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)
#endif  // _MSC_VER

#ifndef MSGQ_LENGTH
#define MSGQ_LENGTH 512
#endif

typedef struct _StreamContext {
  AVCodecContext *dec_ctx;
} StreamContext;

typedef enum _AVTranscodeFlag { av_no, av_copy = 1, av_encode, av_unknown } AVTranscodeFlag;

typedef struct _DataSpace {
  _DataSpace() {}
  ~_DataSpace() {
    if (g_input) {
      free(g_input);
    }
    if (g_output) {
      free(g_output);
    }
#ifdef SEND_STATUS_INFO
    if (g_target_monitoring_host_address) {
      free(g_target_monitoring_host_address);
    }
#endif
  }
  char *g_input = NULL;
  char *g_output = NULL;
  AVFormatContext *g_av_in_fmt_ctx = NULL;
  AVFormatContext *g_av_out_fmt_ctx = NULL;
  AVThreadMessageQueue *g_av_in_queue = NULL;
  bool is_input_thread_started = false;
  std::thread input_thread;
  bool is_output_thread_started = false;
  std::thread output_thread;
  int32_t g_wrote_header_info = 0;
  time_t g_read_time;
  time_t g_write_time;
  AVTranscodeFlag g_video_transcode_flag = av_no;
  AVTranscodeFlag g_audio_transcode_flag = av_no;
  StreamContext *g_stream_ctx = NULL;
  int32_t g_out_video_idx = -1;
  int32_t g_out_audio_idx = -1;
#ifdef SEND_STATUS_INFO
  char *g_target_monitoring_host_address = NULL;
  uint16_t g_target_monitoring_host_port = 0;
#endif
#ifndef COPY_ONLY
  AVThreadMessageQueue *g_av_out_queue = NULL;
  bool is_transcode_thread_started = false;
  std::thread transcode_thread;
  int32_t g_audio_resampling_needed = 0;
  AVCodecContext *g_av_out_audio_enc_ctx = NULL;
  AVFilterGraph *g_av_audio_filter_graph = NULL;
  AVFilterContext *g_av_audio_src_ctx = NULL;
  AVFilterContext *g_av_audio_sink_ctx = NULL;
  int64_t g_audio_out_pts = 0;
#endif
} DataSpace;

void sigHandlerAppClose(int l_signo) {
  if (l_signo == SIGINT) {
    av_log(NULL, AV_LOG_ERROR, "Signal: %d:SIGINT(Interactive attention signal) received.\n", l_signo);
  }
  GlobalAlive::getInstance()->setShutdownCommand();
}

static int readTimer(void *opaque) {
  DataSpace *p = (DataSpace *)opaque;
  return time(NULL) - p->g_read_time >= 5 ? 1 : 0;  // 5 seconds timeout
}

static int writeTimer(void *opaque) {
  std::cout << "Write Timer: UNEXPECTED." << std::endl;
  DataSpace *p = (DataSpace *)opaque;
  return time(NULL) - p->g_write_time >= 5 ? 1 : 0;  // 5 seconds timeout
}

int32_t openRTSPInput(DataSpace *l_dataspace) {
  AVDictionary *l_opts = NULL;
  int32_t l_ret = 0;
  if (strstr(l_dataspace->g_input, "rtsp")) {
    av_dict_set(&l_opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&l_opts, "rtsp_flags", "prefer_tcp", 0);
  }
  do {
    l_dataspace->g_av_in_fmt_ctx = avformat_alloc_context();
    if (!l_dataspace->g_av_in_fmt_ctx) {
      av_log(NULL, AV_LOG_ERROR, "Failed avformat_alloc_context. %s %d\n", av_err2str(l_ret), __LINE__);
      break;
    }
    l_dataspace->g_av_in_fmt_ctx->interrupt_callback.callback = readTimer;  // Set timeout callback
    l_dataspace->g_av_in_fmt_ctx->interrupt_callback.opaque = l_dataspace;
    l_dataspace->g_read_time = time(NULL);
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    if ((l_ret = avformat_open_input(&l_dataspace->g_av_in_fmt_ctx, l_dataspace->g_input, NULL /*l_av_infmt*/, &l_opts)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed avformat_open_input. %s %d\n", av_err2str(l_ret), __LINE__);
      break;
    }
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    if ((l_ret = avformat_find_stream_info(l_dataspace->g_av_in_fmt_ctx, NULL)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed avformat_find_stream_info. %s %d\n", av_err2str(l_ret), __LINE__);
      break;
    }

    StreamContext *l_p_stream_ctx = (StreamContext *)av_mallocz_array(l_dataspace->g_av_in_fmt_ctx->nb_streams, sizeof(*l_p_stream_ctx));
    if (!l_p_stream_ctx) {
      l_ret = AVERROR(ENOMEM);
      av_log(NULL, AV_LOG_ERROR, "Failed av_mallocz_array. %s %d\n", av_err2str(l_ret), __LINE__);
      break;
    }
    l_dataspace->g_stream_ctx = l_p_stream_ctx;  // Save Stream Context

    for (int32_t l_index = 0; l_index < l_dataspace->g_av_in_fmt_ctx->nb_streams; l_index++) {
      AVStream *l_stream = l_dataspace->g_av_in_fmt_ctx->streams[l_index];
      if (l_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (l_stream->codecpar->codec_id == AV_CODEC_ID_NONE) {
          l_dataspace->g_video_transcode_flag = av_no;
          l_dataspace->g_stream_ctx[l_index].dec_ctx = NULL;
        } else if (l_stream->codecpar->codec_id == AV_CODEC_ID_H264) {
          l_dataspace->g_video_transcode_flag = av_copy;
          l_dataspace->g_stream_ctx[l_index].dec_ctx = NULL;
        } else {
#ifdef COPY_ONLY
          l_dataspace->g_video_transcode_flag = av_no;
          l_dataspace->g_stream_ctx[l_index].dec_ctx = NULL;
#else
          // Add transcode to video code here

#endif
        }
      } else if (l_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        if (l_stream->codecpar->codec_id == AV_CODEC_ID_NONE) {
          l_dataspace->g_audio_transcode_flag = av_no;
          l_dataspace->g_stream_ctx[l_index].dec_ctx = NULL;
        } else if (l_stream->codecpar->codec_id == AV_CODEC_ID_AAC) {
          l_dataspace->g_audio_transcode_flag = av_copy;
          l_dataspace->g_stream_ctx[l_index].dec_ctx = NULL;
        } else {
#ifdef COPY_ONLY
          l_dataspace->g_audio_transcode_flag = av_no;
          l_dataspace->g_stream_ctx[l_index].dec_ctx = NULL;
#else
          l_dataspace->g_audio_transcode_flag = av_encode;
          AVCodec *l_dec = avcodec_find_decoder(l_stream->codecpar->codec_id);
          if (!l_dec) {
            l_ret = AVERROR_DECODER_NOT_FOUND;
            av_log(NULL, AV_LOG_ERROR, "Failed avcodec_find_decoder : to find decoder for stream #%u, codec ID: %d. %s %d\n", l_index,
                   l_stream->codecpar->codec_id, av_err2str(l_ret), __LINE__);
            l_dataspace->g_audio_transcode_flag = av_no;
            l_dataspace->g_stream_ctx[l_index].dec_ctx = NULL;
            goto END;
          }
          AVCodecContext *l_codec_ctx = avcodec_alloc_context3(l_dec);
          if (!l_codec_ctx) {
            l_ret = AVERROR(ENOMEM);
            av_log(NULL, AV_LOG_ERROR, "Failed avcodec_alloc_context3: to allocate the decoder context for stream #%u. %s %d\n", l_index,
                   av_err2str(l_ret), __LINE__);
            l_dataspace->g_audio_transcode_flag = av_no;
            l_dataspace->g_stream_ctx[l_index].dec_ctx = NULL;
            goto END;
          }
          l_ret = avcodec_parameters_to_context(l_codec_ctx, l_stream->codecpar);
          if (l_ret < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Failed avcodec_parameters_to_context: to copy decoder parameters to"
                   "input decoder context for stream #%u. %s %d\n",
                   l_index, av_err2str(l_ret), __LINE__);
            goto END;
          }
          l_ret = avcodec_open2(l_codec_ctx, l_dec, NULL);
          if (l_ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed avcodec_open2: to open audio decoder for stream #%u. %s %d\n", l_index, av_err2str(l_ret), __LINE__);
            goto END;
          }
          l_dataspace->g_stream_ctx[l_index].dec_ctx = l_codec_ctx;
#endif
        }
      }
    }
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    av_dump_format(l_dataspace->g_av_in_fmt_ctx, 0, l_dataspace->g_input, 0);
  } while (0);

END:
  av_dict_free(&l_opts);
  return l_ret;
}

void closeRTSPInput(DataSpace *l_dataspace) {
  if (l_dataspace->g_av_in_fmt_ctx) {
    for (int32_t l_index = 0; l_index < l_dataspace->g_av_in_fmt_ctx->nb_streams; l_index++) {
      if (l_dataspace->g_stream_ctx[l_index].dec_ctx) {
        avcodec_free_context(&l_dataspace->g_stream_ctx[l_index].dec_ctx);
        l_dataspace->g_stream_ctx[l_index].dec_ctx = NULL;
      }
    }
  }
  if (l_dataspace->g_av_in_fmt_ctx) {
    avformat_close_input(&l_dataspace->g_av_in_fmt_ctx);
  }
  l_dataspace->g_av_in_fmt_ctx = NULL;
}

#ifndef COPY_ONLY
int initFilterGraph(AVCodecContext *l_dec_ctx, AVCodecContext *l_enc_ctx, AVFilterGraph **l_av_audio_filter_graph,
                    AVFilterContext **l_av_audio_src_ctx, AVFilterContext **l_av_audio_sink_ctx) {
  AVFilterGraph *filter_graph;
  AVFilterContext *abuffer_ctx;
  const AVFilter *abuffer;
  AVFilterContext *aformat_ctx;
  const AVFilter *aformat;
  AVFilterContext *abuffersink_ctx;
  const AVFilter *abuffersink;

  AVDictionary *options_dict = NULL;
  char options_str[1024];
  char ch_layout[64];

  *l_av_audio_filter_graph = NULL;
  *l_av_audio_src_ctx = NULL;
  *l_av_audio_sink_ctx = NULL;

  int err;

  /* Create a new filtergraph, which will contain all the filters. */
  filter_graph = avfilter_graph_alloc();
  if (!filter_graph) {
    av_log(NULL, AV_LOG_ERROR, "Unable to create filter graph.\n");
    return AVERROR(ENOMEM);
  }

  /* Create the abuffer filter;
   * it will be used for feeding the data into the graph. */
  abuffer = avfilter_get_by_name("abuffer");
  if (!abuffer) {
    av_log(NULL, AV_LOG_ERROR, "Could not find the abuffer filter.\n");
    return AVERROR_FILTER_NOT_FOUND;
  }

  abuffer_ctx = avfilter_graph_alloc_filter(filter_graph, abuffer, "src");
  if (!abuffer_ctx) {
    av_log(NULL, AV_LOG_ERROR, "Could not allocate the abuffer instance.\n");
    return AVERROR(ENOMEM);
  }

  /* Set the filter options through the AVOptions API. */
  av_get_channel_layout_string(ch_layout, sizeof(ch_layout), l_dec_ctx->channels, av_get_default_channel_layout(l_dec_ctx->channels));

  av_opt_set(abuffer_ctx, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
  av_opt_set(abuffer_ctx, "sample_fmt", av_get_sample_fmt_name(l_dec_ctx->sample_fmt), AV_OPT_SEARCH_CHILDREN);
  av_opt_set_q(abuffer_ctx, "time_base", l_dec_ctx->time_base, AV_OPT_SEARCH_CHILDREN);
  av_opt_set_int(abuffer_ctx, "sample_rate", l_dec_ctx->sample_rate, AV_OPT_SEARCH_CHILDREN);

  /* Now initialize the filter; we pass NULL options, since we have already
   * set all the options above. */
  err = avfilter_init_str(abuffer_ctx, NULL);
  if (err < 0) {
    av_log(NULL, AV_LOG_ERROR, "Could not initialize the abuffer filter.\n");
    return err;
  }

  /* Create the aformat filter;
   * it ensures that the output is of the format we want. */
  aformat = avfilter_get_by_name("aformat");
  if (!aformat) {
    av_log(NULL, AV_LOG_ERROR, "Could not find the aformat filter.\n");
    return AVERROR_FILTER_NOT_FOUND;
  }

  aformat_ctx = avfilter_graph_alloc_filter(filter_graph, aformat, "aformat");
  if (!aformat_ctx) {
    av_log(NULL, AV_LOG_ERROR, "Could not allocate the aformat instance.\n");
    return AVERROR(ENOMEM);
  }

  /* A third way of passing the options is in a string of the form
   * key1=value1:key2=value2.... */
  snprintf(options_str, sizeof(options_str), "sample_fmts=%s:sample_rates=%d:channel_layouts=0x%" PRIx64,
           av_get_sample_fmt_name(l_enc_ctx->sample_fmt), l_enc_ctx->sample_rate, (uint64_t)l_enc_ctx->channel_layout);

  err = avfilter_init_str(aformat_ctx, options_str);

  if (err < 0) {
    av_log(NULL, AV_LOG_ERROR, "Could not initialize the aformat filter.\n");
    return err;
  }

  /* Finally create the abuffersink filter;
   * it will be used to get the filtered data out of the graph. */
  abuffersink = avfilter_get_by_name("abuffersink");
  if (!abuffersink) {
    av_log(NULL, AV_LOG_ERROR, "Could not find the abuffersink filter.\n");
    return AVERROR_FILTER_NOT_FOUND;
  }

  abuffersink_ctx = avfilter_graph_alloc_filter(filter_graph, abuffersink, "sink");
  if (!abuffersink_ctx) {
    av_log(NULL, AV_LOG_ERROR, "Could not allocate the abuffersink instance.\n");
    return AVERROR(ENOMEM);
  }

  /* This filter takes no options. */
  err = avfilter_init_str(abuffersink_ctx, NULL);
  if (err < 0) {
    av_log(NULL, AV_LOG_ERROR, "Could not initialize the abuffersink instance.\n");
    return err;
  }

  err = avfilter_link(abuffer_ctx, 0, aformat_ctx, 0);
  if (err >= 0) err = avfilter_link(aformat_ctx, 0, abuffersink_ctx, 0);
  if (err < 0) {
    av_log(NULL, AV_LOG_ERROR, "Error connecting filters\n");
    return err;
  }

  /* Configure the graph. */
  err = avfilter_graph_config(filter_graph, NULL);
  if (err < 0) {
    av_log(NULL, AV_LOG_ERROR, "Error configuring the filter graph\n");
    return err;
  }

  *l_av_audio_filter_graph = filter_graph;
  *l_av_audio_src_ctx = abuffer_ctx;
  *l_av_audio_sink_ctx = abuffersink_ctx;

  return 0;
}
#endif

int32_t openRTMPOutput(DataSpace *l_dataspace) {
  int32_t l_ret = 0;
  do {
    if ((l_ret = avformat_alloc_output_context2(&l_dataspace->g_av_out_fmt_ctx, NULL, "flv", l_dataspace->g_output)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed avformat_alloc_output_context2. %s %d\n", av_err2str(l_ret), __LINE__);
      break;
    }
    if (!l_dataspace->g_av_out_fmt_ctx) {
      av_log(NULL, AV_LOG_ERROR, "Unexpected: Could not create avformat_alloc_output_context2. %s %d\n", av_err2str(l_ret), __LINE__);
      l_ret = AVERROR_UNKNOWN;
      break;
    }
    if (!l_dataspace->g_av_out_fmt_ctx) {
      av_log(NULL, AV_LOG_ERROR, "Failed avformat_alloc_context. %s %d\n", av_err2str(l_ret), __LINE__);
      break;
    }
    l_dataspace->g_av_out_fmt_ctx->interrupt_callback.callback = writeTimer;  // Set timeout callback
    l_dataspace->g_av_out_fmt_ctx->interrupt_callback.opaque = l_dataspace;
    l_dataspace->g_write_time = time(NULL);
    int32_t l_stream_idx = 0;
    for (int32_t l_index = 0; l_index < l_dataspace->g_av_in_fmt_ctx->nb_streams && !GlobalAlive::getInstance()->isShutdownCommanded(); l_index++) {
      AVStream *l_in_stream = l_dataspace->g_av_in_fmt_ctx->streams[l_index];
      if (l_in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        AVStream *l_out_stream = avformat_new_stream(l_dataspace->g_av_out_fmt_ctx, NULL);
        if (!l_out_stream) {
          av_log(NULL, AV_LOG_ERROR, "Failed avformat_new_stream. %s %d\n", av_err2str(l_ret), __LINE__);
          continue;
        }
        l_dataspace->g_out_video_idx = l_stream_idx++;
        if (l_dataspace->g_video_transcode_flag == av_no) {
          // TODO: what?
        } else if (l_dataspace->g_video_transcode_flag == av_copy) {
          l_ret = avcodec_parameters_copy(l_out_stream->codecpar, l_in_stream->codecpar);
          if (l_ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed avcodec_parameters_copy for stream #%u. %s %d\n", l_index, av_err2str(l_ret), __LINE__);
            continue;
          }
          l_out_stream->codecpar->codec_tag = 0;
          l_out_stream->time_base = l_in_stream->time_base;
        } else if (l_dataspace->g_video_transcode_flag == av_encode) {
          // FIXME: add transcoing to video
        }
      } else if (l_in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        AVStream *l_out_stream = NULL;
#ifdef COPY_ONLY
        if (l_dataspace->g_audio_transcode_flag == av_copy) {
#endif
          l_out_stream = avformat_new_stream(l_dataspace->g_av_out_fmt_ctx, NULL);
          if (!l_out_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed avformat_new_stream. %s %d\n", av_err2str(l_ret), __LINE__);
            continue;
          }
          l_dataspace->g_out_audio_idx = l_stream_idx++;
#ifdef COPY_ONLY
        }
#endif
        if (l_dataspace->g_audio_transcode_flag == av_no) {
          // TODO: what?
        } else if (l_dataspace->g_audio_transcode_flag == av_copy) {
          l_ret = avcodec_parameters_copy(l_out_stream->codecpar, l_in_stream->codecpar);
          if (l_ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed avcodec_parameters_copy for stream #%u. %s %d\n", l_index, av_err2str(l_ret), __LINE__);
            continue;
          }
          l_out_stream->codecpar->codec_tag = 0;
          l_out_stream->time_base = l_in_stream->time_base;
        } else if (l_dataspace->g_audio_transcode_flag == av_encode) {
#ifndef COPY_ONLY
          // encode audio
          AVCodecContext *l_dec_ctx = l_dataspace->g_stream_ctx[l_index].dec_ctx;
          // AVCodec *l_encoder = avcodec_find_encoder_by_name("libfdk_aac");
          AVCodec *l_encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
          if (!l_encoder) {
            l_ret = AVERROR_ENCODER_NOT_FOUND;
            av_log(NULL, AV_LOG_FATAL, "Failed avcodec_find_encoder_by_name: Necessary audio encoder not found for stream #%u. %s %d\n", l_index,
                   av_err2str(l_ret), __LINE__);
            continue;
          }
          AVCodecContext *l_enc_ctx = avcodec_alloc_context3(l_encoder);  // TODO: Need to deallocate
          if (!l_enc_ctx) {
            l_ret = AVERROR(ENOMEM);
            av_log(NULL, AV_LOG_FATAL, "Failed avcodec_alloc_context3: to allocate the AAC audio encoder context for stream #%u. %s %d\n", l_index,
                   av_err2str(l_ret), __LINE__);
            continue;
          }

          if (l_in_stream->codecpar->codec_id == AV_CODEC_ID_AAC) {
            l_enc_ctx->sample_rate = l_dec_ctx->sample_rate;
          } else if (l_in_stream->codecpar->codec_id >= AV_CODEC_ID_FIRST_AUDIO && l_in_stream->codecpar->codec_id <= AV_CODEC_ID_PCM_F24LE) {
            // PCMU, PCMA family
            // when i/p was 8KHz PCM, o/p worked well for 16 KHz AAC
            l_enc_ctx->sample_rate = 16000;
          } else if (l_in_stream->codecpar->codec_id == AV_CODEC_ID_OPUS) {
            // fail cases:
            // i/p - 48 KHz, fullband (opus has fullband 48KHz, and narrowband 8 KHz)
            // 8KHz - too slow
            // 12KHz - too slow
            // 16KHz - slow
            // 44100 Hz - good, but o/p is distorted
            l_enc_ctx->sample_rate = 44100;
          } else {
            // FIXME: Here is the unknown world!
            // Proved to fail for LPCM
            l_enc_ctx->sample_rate = l_dec_ctx->sample_rate;
            av_log(NULL, AV_LOG_FATAL, "Failed avcodec_alloc_context3: to allocate the AAC audio encoder context for stream #%u. %s %d\n", l_index,
                   av_err2str(l_ret), __LINE__);
          }
          l_enc_ctx->channels = l_dec_ctx->channels;
          l_enc_ctx->channel_layout = av_get_default_channel_layout(l_enc_ctx->channels);

          l_enc_ctx->sample_fmt = l_encoder->sample_fmts[0];
          l_enc_ctx->time_base.num = 1;
          l_enc_ctx->time_base.den = l_enc_ctx->sample_rate;
          l_out_stream->time_base = l_enc_ctx->time_base;
          l_enc_ctx->bit_rate = l_dec_ctx->bit_rate;
          l_enc_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

          /* Some container formats (like MP4) require global headers to be present.
           * Mark the encoder so that it behaves accordingly. */
          if (l_dataspace->g_av_out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) l_enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

          if (l_dec_ctx->sample_rate != l_enc_ctx->sample_rate || l_dec_ctx->sample_fmt != l_enc_ctx->sample_fmt) {
            l_dataspace->g_audio_resampling_needed = 1;
          }

          /* Open the encoder for the audio stream to use it later. */
          l_ret = avcodec_open2(l_enc_ctx, l_encoder, NULL);
          if (l_ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed avcodec_open2: Cannot open audio encoder for stream #%u. %s %d\n", l_index, av_err2str(l_ret),
                   __LINE__);
            continue;
          }
          l_ret = avcodec_parameters_from_context(l_out_stream->codecpar, l_enc_ctx);
          if (l_ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed avcodec_parameters_from_context: to copy audio encoder parameters to output stream #%u. %s %d\n",
                   l_index, av_err2str(l_ret), __LINE__);
            return l_ret;
          }
          if (l_dataspace->g_av_out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
            l_enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
          }

          l_dataspace->g_av_out_audio_enc_ctx = l_enc_ctx;
          if (l_dataspace->g_audio_resampling_needed) {
            l_ret = initFilterGraph(l_dec_ctx, l_enc_ctx, &l_dataspace->g_av_audio_filter_graph, &l_dataspace->g_av_audio_src_ctx,
                                    &l_dataspace->g_av_audio_sink_ctx);
          }
#endif
        }
      }
    }
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    AVDictionary *l_opts = NULL;
    av_dump_format(l_dataspace->g_av_out_fmt_ctx, 0, l_dataspace->g_output, 1);
    if (!(l_dataspace->g_av_out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
      l_dataspace->g_write_time = time(NULL);
      if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
      l_ret = avio_open2(&l_dataspace->g_av_out_fmt_ctx->pb, l_dataspace->g_output, AVIO_FLAG_WRITE, NULL, &l_opts);
      if (l_ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed avio_open2 for %s. %s %d\n", l_dataspace->g_output, av_err2str(l_ret), __LINE__);
        break;
      }
    }
    l_dataspace->g_write_time = time(NULL);
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    l_ret = avformat_write_header(l_dataspace->g_av_out_fmt_ctx, &l_opts);
    if (l_ret == 0) {
      l_dataspace->g_wrote_header_info = 1;
    } else if (l_ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed avformat_write_header for %s. %s %d\n", l_dataspace->g_output, av_err2str(l_ret), __LINE__);
      av_dict_free(&l_opts);
      break;
    }
    av_dict_free(&l_opts);
  } while (0);
  return l_ret;
}

void closeRTMPOutput(DataSpace *l_dataspace) {
  if (l_dataspace->g_av_out_fmt_ctx) {
    if (l_dataspace->g_wrote_header_info) {
      av_write_trailer(l_dataspace->g_av_out_fmt_ctx);
    }
#ifndef COPY_ONLY
    if (l_dataspace->g_av_out_audio_enc_ctx) {
      avcodec_free_context(&l_dataspace->g_av_out_audio_enc_ctx);
    }
#endif
    avio_close(l_dataspace->g_av_out_fmt_ctx->pb);
    avformat_free_context(l_dataspace->g_av_out_fmt_ctx);
    l_dataspace->g_av_out_fmt_ctx = NULL;
    l_dataspace->g_wrote_header_info = 0;
  }
}

void runInputThread(DataSpace *l_dataspace) {
  std::stringstream ss;
  ss << __FUNCTION__ << " ID: " << std::this_thread::get_id() << " Started." << std::endl;
  av_log(NULL, AV_LOG_INFO, "%s", ss.str().c_str());
  int32_t l_ret = 0;
  AVPacket l_packet;
  while (!GlobalAlive::getInstance()->isShutdownCommanded()) {
#ifdef SEND_STATUS_INFO
    MonitoringThread::getInstance()->setStatus(1);
#endif
    l_dataspace->g_read_time = time(NULL);
    l_ret = av_read_frame(l_dataspace->g_av_in_fmt_ctx, &l_packet);
    if (l_ret < 0) {
      if (l_ret == AVERROR(EAGAIN)) {
        av_log(NULL, AV_LOG_INFO, "Failed av_read_frame. Trying again.%s %d\n", av_err2str(l_ret), __LINE__);
        continue;
      } else if (l_ret == AVERROR_EOF) {
        av_log(NULL, AV_LOG_INFO, "Failed av_read_frame. EOF Found. %s %d\n", av_err2str(l_ret), __LINE__);
        av_thread_message_queue_set_err_recv(l_dataspace->g_av_in_queue, l_ret);
        break;
      } else {
        av_log(NULL, AV_LOG_ERROR, "UNEXPECTED: Failed av_read_frame. %d, %d %s %d\n", l_ret, AVERROR_EOF, av_err2str(l_ret), __LINE__);
        av_thread_message_queue_set_err_recv(l_dataspace->g_av_in_queue, l_ret);
        break;
      }
    }
    if (l_dataspace->g_av_in_fmt_ctx->streams[l_packet.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      if (l_dataspace->g_audio_transcode_flag == av_no || l_dataspace->g_out_audio_idx == -1) {
        continue;
      }
    } else if (l_dataspace->g_av_in_fmt_ctx->streams[l_packet.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (l_dataspace->g_video_transcode_flag == av_no || l_dataspace->g_out_video_idx == -1) {
        continue;
      }
    }
    l_ret = av_thread_message_queue_send(l_dataspace->g_av_in_queue, &l_packet, AV_THREAD_MESSAGE_NONBLOCK);
    if (l_ret == 0) {
      // av_log(NULL, AV_LOG_INFO, "READ INPUT: No drop: audio/video message read and send\n");
    } else if (l_ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed av_thread_message_queue_send. %s %d\n", av_err2str(l_ret), __LINE__);
      av_packet_unref(&l_packet);
      av_thread_message_queue_set_err_recv(l_dataspace->g_av_in_queue, l_ret);
      break;
    }
  }
  GlobalAlive::getInstance()->setShutdownCommand();
  std::stringstream ss1;
  ss.swap(ss1);
  ss << __FUNCTION__ << " ID: " << std::this_thread::get_id() << " Finished." << std::endl;
  av_log(NULL, AV_LOG_INFO, "%s", ss.str().c_str());
}

int32_t copyAVPacket(AVFormatContext *l_av_in_fmt_ctx, AVFormatContext *l_av_out_fmt_ctx, int l_out_stream_index, AVPacket *l_pkt
#ifndef COPY_ONLY
                     ,
                     AVThreadMessageQueue *l_p_out_queue
#endif
) {
  l_pkt->pts = av_rescale_q_rnd(l_pkt->pts, l_av_in_fmt_ctx->streams[l_pkt->stream_index]->time_base,
                                l_av_out_fmt_ctx->streams[l_out_stream_index]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
  l_pkt->dts = av_rescale_q_rnd(l_pkt->dts, l_av_in_fmt_ctx->streams[l_pkt->stream_index]->time_base,
                                l_av_out_fmt_ctx->streams[l_out_stream_index]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
  l_pkt->duration = av_rescale_q(l_pkt->duration, l_av_in_fmt_ctx->streams[l_pkt->stream_index]->time_base,
                                 l_av_out_fmt_ctx->streams[l_out_stream_index]->time_base);
  l_pkt->pos = -1;
  l_pkt->stream_index = l_out_stream_index;
#ifdef SHOW_PACKET_INFO
  av_log(NULL, AV_LOG_INFO, "TO RTMP: Pkt: index: %d, duration: %" PRId64 "pts: %" PRId64 ", dts: %" PRId64 "\n", l_pkt->stream_index,
         l_pkt->duration, l_pkt->pts, l_pkt->dts);
#endif
#ifdef COPY_ONLY
  return 0;
#else
  int32_t l_ret = av_thread_message_queue_send(l_p_out_queue, l_pkt, AV_THREAD_MESSAGE_NONBLOCK);
  if (l_ret == AVERROR(EAGAIN)) {
    av_usleep(1000);
  } else if (l_ret < 0) {
    if (l_ret == AVERROR_EOF) {
      av_log(NULL, AV_LOG_INFO, "EOF Found in Queue. %s %d\n", av_err2str(l_ret), __LINE__);
      av_packet_unref(l_pkt);
      av_thread_message_queue_set_err_recv(l_p_out_queue, l_ret);
    } else {
      av_log(NULL, AV_LOG_ERROR, "UNEXPECTED: Failed av_thread_message_queue_recv. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
    }
  }
  return l_ret;
#endif
}

void runOutputThread(DataSpace *l_dataspace) {
  std::stringstream ss;
  ss << __FUNCTION__ << " ID: " << std::this_thread::get_id() << " Started." << std::endl;
  av_log(NULL, AV_LOG_INFO, "%s", ss.str().c_str());
  int l_ret = 0;
  AVPacket l_pkt;
  while (!GlobalAlive::getInstance()->isShutdownCommanded()) {
#ifdef SEND_STATUS_INFO
    MonitoringThread::getInstance()->setStatus(2);
#endif
#ifndef COPY_ONLY
    l_ret = av_thread_message_queue_recv(l_dataspace->g_av_out_queue, &l_pkt, AV_THREAD_MESSAGE_NONBLOCK);
#else
    l_ret = av_thread_message_queue_recv(l_dataspace->g_av_in_queue, &l_pkt, AV_THREAD_MESSAGE_NONBLOCK);
#endif
    if (l_ret == AVERROR(EAGAIN)) {
      av_usleep(1000);
      continue;
    } else if (l_ret < 0) {
      if (l_ret == AVERROR_EOF) {
        av_log(NULL, AV_LOG_INFO, "EOF Found in Queue. %s %d\n", av_err2str(l_ret), __LINE__);
      } else {
        av_log(NULL, AV_LOG_ERROR, "UNEXPECTED: Failed av_thread_message_queue_recv. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
      }
      break;
    }
#ifdef COPY_ONLY
    copyAVPacket(l_dataspace->g_av_in_fmt_ctx, l_dataspace->g_av_out_fmt_ctx, l_dataspace->g_out_video_idx, &l_pkt);
#endif
    l_dataspace->g_write_time = time(NULL);
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    l_ret = av_interleaved_write_frame(l_dataspace->g_av_out_fmt_ctx, &l_pkt);
    if (l_ret < 0) {
      if (l_ret == AVERROR(EINVAL)) {  // Known good error

      } else {
#if _MSC_VER
        if (l_ret == AVERROR(ENETDOWN) || l_ret == AVERROR(ENETUNREACH) || l_ret == AVERROR(ENETRESET) || l_ret == AVERROR(ECONNABORTED) ||
            l_ret == AVERROR(ECONNRESET) || l_ret == AVERROR(ENOBUFS) || l_ret == AVERROR(EISCONN) || l_ret == AVERROR(ENOTCONN) ||
            l_ret == AVERROR(ETIMEDOUT) || l_ret == AVERROR(ECONNREFUSED) || l_ret == AVERROR(EHOSTUNREACH) || l_ret == AVERROR(EPIPE)) {
#else

        if (l_ret == AVERROR(ENETDOWN) || l_ret == AVERROR(ENETUNREACH) || l_ret == AVERROR(ENETRESET) || l_ret == AVERROR(ECONNABORTED) ||
            l_ret == AVERROR(ECONNRESET) || l_ret == AVERROR(ENOBUFS) || l_ret == AVERROR(EISCONN) || l_ret == AVERROR(ENOTCONN) ||
            l_ret == AVERROR(ESHUTDOWN) || l_ret == AVERROR(ETOOMANYREFS) || l_ret == AVERROR(ETIMEDOUT) || l_ret == AVERROR(ECONNREFUSED) ||
            l_ret == AVERROR(EHOSTDOWN) || l_ret == AVERROR(EHOSTUNREACH) || l_ret == AVERROR(EPIPE)) {
#endif
          av_log(NULL, AV_LOG_INFO, "Failed av_interleaved_write_frame. %s %d\n", av_err2str(l_ret), __LINE__);
        } else {
          av_log(NULL, AV_LOG_ERROR, "UNEXPECTED ERROR: Failed av_interleaved_write_frame. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
        }
        break;
      }
    }
  }
  GlobalAlive::getInstance()->setShutdownCommand();
  std::stringstream ss1;
  ss.swap(ss1);
  ss << __FUNCTION__ << " ID: " << std::this_thread::get_id() << " Finished." << std::endl;
  av_log(NULL, AV_LOG_INFO, "%s", ss.str().c_str());
}

#ifndef COPY_ONLY
int64_t samplesToTimeBase(AVCodecContext *l_ctx, int64_t samples) {
  if (samples == AV_NOPTS_VALUE) {
    return AV_NOPTS_VALUE;
  }
  AVRational s;
  s.num = 1;
  s.den = l_ctx->sample_rate;
  return av_rescale_q(samples, s, l_ctx->time_base);
}

int32_t encodeAACResampleFrame(AVFrame *l_in_frame, int64_t *l_audio_out_pts, AVCodecContext *l_av_out_audio_enc_ctx,
                               AVFormatContext *l_av_out_fmt_ctx, int32_t l_out_audio_idx, AVFilterContext *l_av_audio_src_ctx,
                               AVFilterContext *l_av_audio_sink_ctx, AVThreadMessageQueue *l_p_out_queue) {
  int32_t l_ret = 0;
  AVPacket l_enc_pkt;
  AVFrame *l_frame = NULL;
  l_ret = av_buffersrc_add_frame(l_av_audio_src_ctx, l_in_frame);
  if (l_ret < 0) {
    av_log(NULL, AV_LOG_FATAL,
           "av_buffersrc_add_frame failed: while submitting the frame to the filtergraph."
           " %d %s %d\n",
           l_ret, av_err2str(l_ret), __LINE__);
    return l_ret;
  }
  while (true) {  // TODO: What is the stop condition?
    l_frame = av_frame_alloc();
    if (!l_frame) {
      l_ret = AVERROR(ENOMEM);
      av_log(NULL, AV_LOG_ERROR, "Error av_frame_alloc failed");
      av_log(NULL, AV_LOG_FATAL, "av_frame_alloc failed: %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
      return l_ret;
    }
    // l_ret = av_buffersink_get_samples(l_av_audio_sink_ctx, l_frame, l_av_out_audio_enc_ctx->frame_size);
    l_ret = av_buffersink_get_frame(l_av_audio_sink_ctx, l_frame);  // Sourav Change
    if (l_ret < 0) {
      /* if no more frames for output - returns AVERROR(EAGAIN)
       * if flushed and no more frames for output - returns AVERROR_EOF
       * rewrite retcode to 0 to show it as normal procedure completion
       */
      if (l_ret == AVERROR_EOF) {
        av_log(NULL, AV_LOG_FATAL, "av_buffersink_get_samples failed: Error: Pulling filtered frame EOF: %d %s %d\n", l_ret, av_err2str(l_ret),
               __LINE__);
      } else if (l_ret == AVERROR(EAGAIN)) {
      } else {
        av_log(NULL, AV_LOG_FATAL, "av_buffersink_get_samples failed: Error: Unknown, av_buffersink_get_samples: %d %s %d\n", l_ret,
               av_err2str(l_ret), __LINE__);
      }
      av_frame_free(&l_frame);
      return l_ret;
    }
    l_frame->pict_type = AV_PICTURE_TYPE_NONE;
    l_enc_pkt.data = NULL;
    l_enc_pkt.size = 0;
    av_init_packet(&l_enc_pkt);

    AVRational s;
    s.num = 1;
    s.den = l_av_out_audio_enc_ctx->sample_rate;
    l_frame->pts = av_rescale_q(*l_audio_out_pts, s, l_av_out_audio_enc_ctx->time_base);
    *l_audio_out_pts = *l_audio_out_pts + l_frame->nb_samples;
    int64_t l_duration = samplesToTimeBase(l_av_out_audio_enc_ctx, l_frame->nb_samples);

    l_ret = avcodec_send_frame(l_av_out_audio_enc_ctx, l_frame);
    if (l_ret < 0) {
      av_frame_free(&l_frame);
      av_log(NULL, AV_LOG_FATAL, "avcodec_send_frame failed: for AAC encoding. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
      return l_ret;
    }
    l_ret = avcodec_receive_packet(l_av_out_audio_enc_ctx, &l_enc_pkt);
    if (l_ret == AVERROR(EAGAIN)) {
      av_log(NULL, AV_LOG_FATAL, "avcodec_receive_packet failed: for AAC encoding. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
      av_frame_free(&l_frame);
      return l_ret;
    } else if (l_ret < 0) {
      av_log(NULL, AV_LOG_FATAL, "avcodec_receive_packet failed: for AAC encoding. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
      av_frame_free(&l_frame);
      return l_ret;
    }
    l_enc_pkt.stream_index = l_av_out_fmt_ctx->streams[l_out_audio_idx]->index;
    l_enc_pkt.pts = l_enc_pkt.dts = *l_audio_out_pts;
    av_packet_rescale_ts(&l_enc_pkt, l_av_out_audio_enc_ctx->time_base, l_av_out_fmt_ctx->streams[l_out_audio_idx]->time_base);

    l_ret = av_thread_message_queue_send(l_p_out_queue, &l_enc_pkt, AV_THREAD_MESSAGE_NONBLOCK);
    if (l_ret == 0) {
      // av_log(NULL, AV_LOG_INFO, "READ INPUT: No drop: audio/video message read and send\n");
    } else if (l_ret == AVERROR(EAGAIN)) {
      av_log(NULL, AV_LOG_ERROR, "Failed av_thread_message_queue_send. %s %d\n", av_err2str(l_ret), __LINE__);
    } else if (l_ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed av_thread_message_queue_send. %s %d\n", av_err2str(l_ret), __LINE__);
      av_packet_unref(&l_enc_pkt);
      av_thread_message_queue_set_err_recv(l_p_out_queue, l_ret);
      break;
    }
  }
  return l_ret;
}

int32_t encodeAACFrame(AVFrame *l_frame, int64_t *l_audio_out_pts, AVCodecContext *l_av_out_audio_enc_ctx, AVFormatContext *l_av_in_fmt_ctx,
                       AVFormatContext *l_av_out_fmt_ctx, int32_t l_out_audio_idx, AVThreadMessageQueue *l_p_out_queue) {
  int32_t l_ret = 0;
  AVPacket l_enc_pkt;

  l_enc_pkt.data = NULL;
  l_enc_pkt.size = 0;
  av_init_packet(&l_enc_pkt);

  AVRational s;
  s.num = 1;
  s.den = l_av_out_audio_enc_ctx->sample_rate;
  l_frame->pts = av_rescale_q(*l_audio_out_pts, s, l_av_out_audio_enc_ctx->time_base);
  *l_audio_out_pts += l_frame->nb_samples;
  // int64_t l_duration = samplesToTimeBase(l_av_out_audio_enc_ctx, l_frame->nb_samples);

  l_ret = avcodec_send_frame(l_av_out_audio_enc_ctx, l_frame);
  if (l_ret < 0) {
    av_log(NULL, AV_LOG_FATAL, "avcodec_send_frame failed: for AAC encoding. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
    return l_ret;
  }
  l_ret = avcodec_receive_packet(l_av_out_audio_enc_ctx, &l_enc_pkt);
  if (l_ret == AVERROR(EAGAIN)) {
    av_log(NULL, AV_LOG_FATAL, "avcodec_receive_packet failed: for AAC encoding. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
    return l_ret;
  } else if (l_ret < 0) {
    av_log(NULL, AV_LOG_FATAL, "avcodec_receive_packet failed: for AAC encoding. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
    return l_ret;
  }
  av_packet_rescale_ts(&l_enc_pkt, l_av_out_audio_enc_ctx->time_base, l_av_out_fmt_ctx->streams[l_out_audio_idx]->time_base);
  l_enc_pkt.stream_index = l_av_out_fmt_ctx->streams[l_out_audio_idx]->index;

  l_ret = copyAVPacket(l_av_in_fmt_ctx, l_av_out_fmt_ctx, l_out_audio_idx, &l_enc_pkt, l_p_out_queue);
  return l_ret;
}

void transcodeAVPacket(DataSpace *l_dataspace) {
  std::stringstream ss;
  ss << __FUNCTION__ << " ID: " << std::this_thread::get_id() << " Started." << std::endl;
  av_log(NULL, AV_LOG_INFO, "%s", ss.str().c_str());
  int l_ret = 0;
  while (!GlobalAlive::getInstance()->isShutdownCommanded()) {
#ifdef SEND_STATUS_INFO
    MonitoringThread::getInstance()->setStatus(3);
#endif
    AVPacket l_pkt;
    l_ret = av_thread_message_queue_recv(l_dataspace->g_av_in_queue, &l_pkt, AV_THREAD_MESSAGE_NONBLOCK);
    if (l_ret == AVERROR(EAGAIN)) {
      av_usleep(10000);
      continue;
    } else if (l_ret < 0) {
      if (l_ret == AVERROR_EOF) {
        av_log(NULL, AV_LOG_INFO, "EOF Found in Queue. %s %d\n", av_err2str(l_ret), __LINE__);
        break;
      } else {
        av_log(NULL, AV_LOG_ERROR, "UNEXPECTED: Failed av_thread_message_queue_recv. %d %s %d\n", l_ret, av_err2str(l_ret), __LINE__);
        av_usleep(10000);
        continue;
      }
    }
    AVMediaType l_codec_type = l_dataspace->g_av_in_fmt_ctx->streams[l_pkt.stream_index]->codecpar->codec_type;
    if (l_codec_type == AVMEDIA_TYPE_VIDEO) {
      if (l_dataspace->g_video_transcode_flag == av_copy) {
        l_ret = copyAVPacket(l_dataspace->g_av_in_fmt_ctx, l_dataspace->g_av_out_fmt_ctx, l_dataspace->g_out_video_idx, &l_pkt,
                             l_dataspace->g_av_out_queue);
      } else if (l_dataspace->g_video_transcode_flag == av_encode) {
        // FIXME: to add video transcode
      }
    } else if (l_codec_type == AVMEDIA_TYPE_AUDIO) {
      if (l_dataspace->g_audio_transcode_flag == av_copy) {
        l_ret = copyAVPacket(l_dataspace->g_av_in_fmt_ctx, l_dataspace->g_av_out_fmt_ctx, l_dataspace->g_out_audio_idx, &l_pkt,
                             l_dataspace->g_av_out_queue);
      } else if (l_dataspace->g_audio_transcode_flag == av_encode) {
        av_packet_rescale_ts(&l_pkt, l_dataspace->g_av_in_fmt_ctx->streams[l_pkt.stream_index]->time_base,
                             l_dataspace->g_stream_ctx[l_pkt.stream_index].dec_ctx->time_base);
        AVFrame *l_frame = av_frame_alloc();
        if (!l_frame) {
          av_packet_unref(&l_pkt);  // Not needed
          l_ret = AVERROR(ENOMEM);
          av_log(NULL, AV_LOG_ERROR, "Failed av_frame_alloc. %s %d\n", av_err2str(l_ret), __LINE__);
          break;
        }
        int32_t l_got_pic_ptr = 0;
        l_ret = avcodec_decode_audio4(l_dataspace->g_stream_ctx[l_pkt.stream_index].dec_ctx, l_frame, &l_got_pic_ptr, &l_pkt);
        av_packet_unref(&l_pkt);
        if (l_got_pic_ptr) {
          if (l_dataspace->g_audio_resampling_needed) {
            l_ret = encodeAACResampleFrame(l_frame, &l_dataspace->g_audio_out_pts, l_dataspace->g_av_out_audio_enc_ctx, l_dataspace->g_av_out_fmt_ctx,
                                           l_dataspace->g_out_audio_idx, l_dataspace->g_av_audio_src_ctx, l_dataspace->g_av_audio_sink_ctx,
                                           l_dataspace->g_av_out_queue);
          } else {
            l_ret = encodeAACFrame(l_frame, &l_dataspace->g_audio_out_pts, l_dataspace->g_av_out_audio_enc_ctx, l_dataspace->g_av_in_fmt_ctx,
                                   l_dataspace->g_av_out_fmt_ctx, l_dataspace->g_out_audio_idx, l_dataspace->g_av_out_queue);
          }
        }
        av_frame_free(&l_frame);
      }
    }
  }
  GlobalAlive::getInstance()->setShutdownCommand();
  std::stringstream ss1;
  ss.swap(ss1);
  ss << __FUNCTION__ << " ID: " << std::this_thread::get_id() << " Finished." << std::endl;
  av_log(NULL, AV_LOG_INFO, "%s", ss.str().c_str());
}
#endif

int32_t createInputThread(DataSpace *l_dataspace) {
  int32_t l_ret = 0;
  l_dataspace->input_thread = std::thread(runInputThread, l_dataspace);
  l_dataspace->is_input_thread_started = true;
  return l_ret;
}

void closeInputThread(DataSpace *l_dataspace) {
  if (l_dataspace->is_input_thread_started) {
    l_dataspace->input_thread.join();
  }
}

int32_t createOutputThread(DataSpace *l_dataspace) {
  int32_t l_ret = 0;
  l_dataspace->output_thread = std::thread(runOutputThread, l_dataspace);
  l_dataspace->is_output_thread_started = true;
  return l_ret;
}

void closeOutputThread(DataSpace *l_dataspace) {
  if (l_dataspace->is_output_thread_started) {
    l_dataspace->output_thread.join();
  }
}

#ifndef COPY_ONLY
int32_t createTranscodeThread(DataSpace *l_dataspace) {
  int32_t l_ret = 0;
  l_dataspace->transcode_thread = std::thread(transcodeAVPacket, l_dataspace);
  l_dataspace->is_transcode_thread_started = true;
  return l_ret;
}

void closeTranscodeThread(DataSpace *l_dataspace) {
  if (l_dataspace->is_transcode_thread_started) {
    l_dataspace->transcode_thread.join();
  }
}
#endif

int32_t createPacketQueue(AVThreadMessageQueue **l_pp_queue) { return av_thread_message_queue_alloc(l_pp_queue, MSGQ_LENGTH, sizeof(AVPacket)); }

int32_t closePacketQueue(AVThreadMessageQueue **l_pp_queue) {
  int32_t l_ret = 0;
  if (*l_pp_queue) {
    av_thread_message_queue_free(l_pp_queue);
  }
  return l_ret;
}

int readArguments(DataSpace *l_dataspace, int a_argc, char *a_argv[]) {
  for (int32_t l_index = 0; l_index < a_argc; l_index++) {
    if (strcmp(a_argv[l_index], "-i") == 0) {
      if (l_index + 1 >= a_argc) {
        av_log(NULL, AV_LOG_ERROR, "Error: No video input specified after  -i \n");
        return -1;
      }
      l_index++;
      int32_t in_str_len = strlen(a_argv[l_index]);
      if (!l_dataspace->g_input) {
        l_dataspace->g_input = (char *)calloc(in_str_len + 1, sizeof(char));
      }
      strncpy(l_dataspace->g_input, a_argv[l_index], in_str_len);
      l_dataspace->g_input[in_str_len] = '\0';
    } else if (strcmp(a_argv[l_index], "-o") == 0) {
      if (l_index + 1 >= a_argc) {
        av_log(NULL, AV_LOG_ERROR, "Error No output RTMP specified after -o \n");
        return -2;
      }
      l_index++;
      int32_t in_str_len = strlen(a_argv[l_index]);
      if (!l_dataspace->g_output) {
        l_dataspace->g_output = (char *)calloc(in_str_len + 1, sizeof(char));
      }
      strncpy(l_dataspace->g_output, a_argv[l_index], in_str_len);
      l_dataspace->g_output[in_str_len] = '\0';
    }
#ifdef SEND_STATUS_INFO
    else if (strcmp(a_argv[l_index], "-mh") == 0) {  // target monitoring host: optional
      if (l_index + 1 < a_argc) {
        l_index++;
        int32_t in_str_len = strlen(a_argv[l_index]);
        if (!l_dataspace->g_target_monitoring_host_address) {
          l_dataspace->g_target_monitoring_host_address = (char *)calloc(in_str_len + 1, sizeof(char));
        }
        strncpy(l_dataspace->g_target_monitoring_host_address, a_argv[l_index], in_str_len);
        l_dataspace->g_target_monitoring_host_address[in_str_len] = '\0';
      }
    } else if (strcmp(a_argv[l_index], "-mp") == 0) {  // target monitoring port: optional
      if (l_index + 1 < a_argc) {
        l_index++;
        l_dataspace->g_target_monitoring_host_port = std::stoi(a_argv[l_index]);
      }
    }
#endif
  }
  return 0;
}

std::string ver_string(int a, int b, int c) {
  std::ostringstream ss;
  ss << a << '.' << b << '.' << c;
  return ss.str();
}

void show_banner() {
  const char *const COMPILED = __DATE__ " @ " __TIME__;
  std::string true_cxx =
#if defined(__clang__)
      "clang++";
#elif defined(MSVC)
      "msvc";
#else
      "g++";
#endif
  std::string true_cxx_ver =
#if defined(__clang__)
      ver_string(__clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(MSVC)
      ver_string(0, 1, 2);
#else
      ver_string(0, 1, 2);
#endif
  av_log(NULL, AV_LOG_INFO, "Binary  \t : fflivehlsstreamer\n");
  av_log(NULL, AV_LOG_INFO, "Owner   \t : Videonetics Technology Private Limited\n");
  av_log(NULL, AV_LOG_INFO, "Description \t : Receives media packets from RTSP and pushes to RTMP\n");
  av_log(NULL, AV_LOG_INFO, "Compiled at \t : %s\n", COMPILED);
  av_log(NULL, AV_LOG_INFO, "Compiler \t : %s %s\n", true_cxx.data(), true_cxx_ver.data());
  av_log(NULL, AV_LOG_INFO, "Flag Status \t : ");
  av_log(NULL, AV_LOG_INFO, "Transcoding: %s\n",
#ifdef COPY_ONLY
         "Disabled. This binary only accepts H.264 and AAC"
#else
         "Enabled"
#endif
  );
  av_log(NULL, AV_LOG_INFO, " \t\t : Status Send: %s\n",
#ifdef SEND_STATUS_INFO
         "Enabled"
#else
         "Disabled"
#endif
  );
  av_log(NULL, AV_LOG_INFO, "Usage\n");
  av_log(NULL, AV_LOG_INFO, " \t\t : Required:\n\t\t\t -i rtsp url\n\t\t\t -o rtmp url\n");
  av_log(NULL, AV_LOG_INFO, " \t\t : Optional:\n\t\t\t -mh monitoring host address\n\t\t\t -mp monitoring host port\n\n");
  av_log(NULL, AV_LOG_INFO,
         "Example \t : ./fflivehlsstreamer -i rtsp://admin:admin@192.168.0.31/live.sdp -o rtmp://192.168.1.114:9110 -mh 192.168.1.114 -mp 23000\n\n");
}

int main(int argc, char **argv) {
  DataSpace l_dataspace;

  // Set Signal handlers
  // See https://stackoverflow.com/questions/36387815/signals-and-sleep-not-working-properly for another way of handling signals.
  /*
  signal(SIGHUP, SIG_IGN);
  if (signal(SIGUSR1, sigHandlerAppClose) == SIG_ERR || signal(SIGINT, sigHandlerAppClose) == SIG_ERR ||
      signal(SIGSEGV, sigHandlerAppClose) == SIG_ERR) {
      av_log(NULL, AV_LOG_ERROR, "Can't catch sigHandlerAppClose signal\n");
      return 0;
  }
  */
  // if (signal(SIGINT, sigHandlerAppClose) == SIG_ERR || signal(SIGHUP, sigHandlerAppClose) == SIG_ERR ||
  //     signal(SIGQUIT, sigHandlerAppClose) == SIG_ERR || signal(SIGTRAP, sigHandlerAppClose) == SIG_ERR ||
  //     signal(SIGKILL, sigHandlerAppClose) == SIG_ERR || signal(SIGBUS, sigHandlerAppClose) == SIG_ERR ||
  //     signal(SIGSYS, sigHandlerAppClose) == SIG_ERR || signal(SIGPIPE, sigHandlerAppClose) == SIG_ERR ||
  //     signal(SIGTERM, sigHandlerAppClose) == SIG_ERR || signal(SIGSEGV, sigHandlerAppClose) == SIG_ERR ||
  //     signal(SIGABRT, sigHandlerAppClose) == SIG_ERR || signal(SIGILL, sigHandlerAppClose) == SIG_ERR) {
  //     av_log(NULL, AV_LOG_ERROR, "Can't catch sigHandlerAppClose signal\n");
  //     return 0;
  // }

  // signal(SIGPIPE, SIG_IGN);
  if (signal(SIGINT, sigHandlerAppClose) == SIG_ERR) {
    av_log(NULL, AV_LOG_ERROR, "Can't catch sigHandlerAppClose signal\n");
    return 0;
  }

  // Logger
  av_log_set_level(AV_LOG_INFO);

  show_banner();

  readArguments(&l_dataspace, argc, argv);
  if (!l_dataspace.g_input || !l_dataspace.g_output) return -1;
  av_log(NULL, AV_LOG_INFO,
         "Received arg: IN-[%s], Out-[%s]"
#ifdef SEND_STATUS_INFO
         ", MH-[%s], MP-[%d]"
#endif
         "\n",
         l_dataspace.g_input, l_dataspace.g_output
#ifdef SEND_STATUS_INFO
         ,
         l_dataspace.g_target_monitoring_host_address, l_dataspace.g_target_monitoring_host_port
#endif
  );
  GlobalAlive *alive = GlobalAlive::getInstance();
#ifdef SEND_STATUS_INFO
  std::string l_host(DEFAULT_HOST);
  uint16_t l_port(DEFAULT_PORT);
  if (l_dataspace.g_target_monitoring_host_address != NULL && strlen(l_dataspace.g_target_monitoring_host_address) > 0) {
    l_host = std::string(l_dataspace.g_target_monitoring_host_address);
  }
  if (l_dataspace.g_target_monitoring_host_port > 0) {
    l_port = l_dataspace.g_target_monitoring_host_port;
  }
  MonitoringThread::getInstance(l_host, l_port);
#endif

  av_log(NULL, AV_LOG_INFO,
         "Final Param List : IN-[%s], Out-[%s]"
#ifdef SEND_STATUS_INFO
         ", MH-[%s], MP-[%d]"
#endif
         "\n",
         l_dataspace.g_input, l_dataspace.g_output
#ifdef SEND_STATUS_INFO
         ,
         l_host.data(), l_port
#endif
  );

  // Init
  av_register_all();        // Initialize libavdevice and register all the input and
                            // output devices.
  avcodec_register_all();   // Register all the codecs, parsers and bitstream
                            // filters which were enabled at configuration time. Internally called by av_register_all()
  avdevice_register_all();  // Initialize libavdevice and register all the input
                            // and output devices.
  avformat_network_init();  // Do global initialization of network libraries.
  avfilter_register_all();  // Registers all filters

  do {
    if (openRTSPInput(&l_dataspace) < 0) break;
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    if (openRTMPOutput(&l_dataspace) < 0) break;
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    if (createPacketQueue(&l_dataspace.g_av_in_queue) < 0) break;
#ifndef COPY_ONLY
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    if (createPacketQueue(&l_dataspace.g_av_out_queue) < 0) break;
#endif
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    if (createInputThread(&l_dataspace) < 0) break;
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    if (createOutputThread(&l_dataspace) < 0) break;
#ifndef COPY_ONLY
    if (GlobalAlive::getInstance()->isShutdownCommanded()) break;
    if (createTranscodeThread(&l_dataspace) < 0) break;
#endif
    while (!GlobalAlive::getInstance()->isShutdownCommanded()) {
      av_usleep(1000);
#ifdef SEND_STATUS_INFO
      MonitoringThread::getInstance()->setStatus(0);
#endif
    }
  } while (0);

  av_log(NULL, AV_LOG_INFO, "Cleaning up: Effect of signal caught.\n");
#ifndef COPY_ONLY
  closeTranscodeThread(&l_dataspace);
#endif
  closeOutputThread(&l_dataspace);
  closeInputThread(&l_dataspace);
  closePacketQueue(&l_dataspace.g_av_in_queue);
#ifndef COPY_ONLY
  closePacketQueue(&l_dataspace.g_av_out_queue);
#endif
  closeRTSPInput(&l_dataspace);
  closeRTMPOutput(&l_dataspace);
#ifdef SEND_STATUS_INFO
  MonitoringThread::deleteInstance();
#endif
  avformat_network_deinit();
  GlobalAlive::deleteInstance();
  av_log(NULL, AV_LOG_INFO, "Gracefully shutdown: Bye.\n");
  return 0;
}