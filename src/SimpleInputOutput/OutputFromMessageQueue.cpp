// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include <iostream>

#include "FFmpegException.h"
#include "OutputFromMessageQueue.h"
#include "logging.h"

constexpr int HLS_SEGMENT_DURATION = 3;
constexpr int HLS_LIST_SIZE = 2;
constexpr int HLS_WRAP = 10;
constexpr int HLS_START_NUMBER = 1;
namespace ffpp
{
OutputFromMessageQueue::OutputFromMessageQueue(const std::string& output_name, void* av_thread_message_queue,
                                               void* input_av_format_context)
    : _output_name(output_name), _av_thread_message_queue(static_cast<AVThreadMessageQueue*>(av_thread_message_queue)),
      _input_av_format_context(static_cast<AVFormatContext*>(input_av_format_context))
{
}
OutputFromMessageQueue::~OutputFromMessageQueue() { Stop(); }
bool OutputFromMessageQueue::Start()
{
  bool ret = true;
  AVDictionary* av_dictionary_opts{nullptr};
  do {
    std::string format_hint;
    if (_output_name.rfind("rtmp", 0) == 0) {
      format_hint = "flv";
    } else if (_output_name.find(".m3u8") != std::string::npos) {
      format_hint = "hls";
    }
    try {
      ThrowOnFfmpegError(avformat_alloc_output_context2(
          &_av_format_context, NULL, (format_hint.empty() ? NULL : format_hint.c_str()), _output_name.c_str()));
      ThrowOnFfmpegReturnNullptr(_av_format_context);
      _last_watch_dog_time_in_sec = time(NULL);
      _av_format_context->interrupt_callback.callback = OutputFromMessageQueue::resetWatchDogTimer;
      _av_format_context->interrupt_callback.opaque = this;
    } catch (const ffpp::FFmpegException& e) {
      RAY_LOG_INF << "Not starting: " << e.what();
      ret = false;
      break;
    }
    try {
      _last_watch_dog_time_in_sec = time(NULL);
      if (format_hint == "hls") {
        // ThrowOnFfmpegError(av_opt_set(_av_format_context->priv_data, "event", "1", 0));
        ThrowOnFfmpegError(av_opt_set_int(_av_format_context->priv_data, "start_number", HLS_START_NUMBER, 0));
        ThrowOnFfmpegError(av_opt_set_double(_av_format_context->priv_data, "hls_time", HLS_SEGMENT_DURATION, 0));
        ThrowOnFfmpegError(av_opt_set_int(_av_format_context->priv_data, "hls_list_size", HLS_LIST_SIZE, 0));
        ThrowOnFfmpegError(av_opt_set_int(_av_format_context->priv_data, "hls_wrap", HLS_WRAP, 0));
      }
    } catch (const ffpp::FFmpegException& e) {
      RAY_LOG_INF << "Not starting: " << e.what();
      ret = false;
      avformat_free_context(_av_format_context);
      _av_format_context = nullptr;
      break;
    }
    bool is_valid_video_found{false};
    try {
      int l_out_index_generator = 0;
      for (int l_index = 0; l_index < _input_av_format_context->nb_streams; ++l_index) {
        AVStream* l_in_stream = _input_av_format_context->streams[l_index];
        if (l_in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
          AVStream* l_out_stream = avformat_new_stream(_av_format_context, NULL);
          ThrowOnFfmpegReturnNullptr(l_out_stream);
          ThrowOnFfmpegError(avcodec_parameters_copy(l_out_stream->codecpar, l_in_stream->codecpar));
          l_out_stream->codecpar->codec_tag = 0;
          l_out_stream->time_base = l_in_stream->time_base;
          l_out_stream->index = l_out_index_generator++;
          _input_output_index_map.emplace(std::make_pair(l_index, l_out_stream->index));
          is_valid_video_found = true;
        } else if (l_in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
          AVStream* l_out_stream = avformat_new_stream(_av_format_context, NULL);
          ThrowOnFfmpegReturnNullptr(l_out_stream);
          ThrowOnFfmpegError(avcodec_parameters_copy(l_out_stream->codecpar, l_in_stream->codecpar));
          l_out_stream->codecpar->codec_tag = 0;
          l_out_stream->time_base = l_in_stream->time_base;
          l_out_stream->index = l_out_index_generator++;
          _input_output_index_map.emplace(std::make_pair(l_index, l_out_stream->index));
        }
      }
      av_dump_format(_av_format_context, 0, _output_name.c_str(), 1);
    } catch (const ffpp::FFmpegException& e) {
      RAY_LOG_INF << "Not starting: " << e.what();
      ret = false;
      break;
    }
    if (!is_valid_video_found) {
      avformat_free_context(_av_format_context);
      _av_format_context = nullptr;
      break;
    }
    try {
      RAY_LOG_INF << "Trying to open : " << _output_name;
      _last_watch_dog_time_in_sec = time(NULL);
      ThrowOnFfmpegError(
          avio_open2(&_av_format_context->pb, _output_name.c_str(), AVIO_FLAG_WRITE, NULL, &av_dictionary_opts));
      RAY_LOG_INF << "Success to open : " << _output_name;
    } catch (const ffpp::FFmpegException& e) {
      RAY_LOG_INF << "Not starting: " << e.what();
      ret = false;
      avformat_free_context(_av_format_context);
      _av_format_context = nullptr;
      break;
    }
    try {
      _last_watch_dog_time_in_sec = time(NULL);
      ThrowOnFfmpegError(avformat_write_header(_av_format_context, &av_dictionary_opts));
    } catch (const ffpp::FFmpegException& e) {
      RAY_LOG_INF << "Not starting: " << e.what();
      ret = false;
      avio_close(_av_format_context->pb);
      avformat_free_context(_av_format_context);
      _av_format_context = nullptr;
      break;
    }
  } while (0);
  av_dict_free(&av_dictionary_opts);
  if (ret) {
    RAY_LOG_INF << "Starting InputToMessageQueue thread";
    _last_watch_dog_time_in_sec = time(NULL);
    _thread.reset(new std::thread(&OutputFromMessageQueue::run, this));
  }
  return ret;
}
void OutputFromMessageQueue::SignalToStop()
{
  av_thread_message_queue_set_err_send(_av_thread_message_queue, AVERROR_EOF);
  _do_shutdown = true;
}
void OutputFromMessageQueue::Stop()
{
  if (_is_already_shutting_down)
    return;
  _is_already_shutting_down = true;
  SignalToStop();
  if (_thread) {
    if (_thread->joinable()) {
      _thread->join();
    }
  }
}
int OutputFromMessageQueue::resetWatchDogTimer(void* opaque)
{
  OutputFromMessageQueue* p = static_cast<OutputFromMessageQueue*>(opaque);
  int ret = (time(NULL) - p->_last_watch_dog_time_in_sec) >= 5 ? 1 : 0; // 5 seconds timeout
  if (ret > 0) {
    std::cout << "Returning OutputFromMessageQueue zero for callback" << std::endl;
  }
  return ret;
}
void OutputFromMessageQueue::run()
{
  RAY_LOG_INF << "Started : OutputFromMessageQueue";
  try {
    AVPacket l_pkt;
    int l_ret = 0;
    while (!_do_shutdown_composite()) {
      ThrowOnFfmpegErrorWithAllowedError(
          l_ret = av_thread_message_queue_recv(_av_thread_message_queue, &l_pkt, AV_THREAD_MESSAGE_NONBLOCK),
          AVERROR(EAGAIN));
      if (l_ret == AVERROR(EAGAIN)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }
      _last_watch_dog_time_in_sec = time(NULL);
      RAY_LOG_DBG << "IN  index: " << l_pkt.stream_index << " PTS: " << l_pkt.pts << " DTS: " << l_pkt.dts
                  << " DURATION: " << l_pkt.duration;
      l_pkt.pts = av_rescale_q_rnd(l_pkt.pts, _input_av_format_context->streams[l_pkt.stream_index]->time_base,
                                   _av_format_context->streams[_input_output_index_map[l_pkt.stream_index]]->time_base,
                                   (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
      l_pkt.dts = av_rescale_q_rnd(l_pkt.dts, _input_av_format_context->streams[l_pkt.stream_index]->time_base,
                                   _av_format_context->streams[_input_output_index_map[l_pkt.stream_index]]->time_base,
                                   (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
      l_pkt.duration =
          av_rescale_q(l_pkt.duration, _input_av_format_context->streams[l_pkt.stream_index]->time_base,
                       _av_format_context->streams[_input_output_index_map[l_pkt.stream_index]]->time_base);
      l_pkt.pos = -1;
      l_pkt.stream_index = _input_output_index_map[l_pkt.stream_index];
      RAY_LOG_DBG << "OUT index: " << l_pkt.stream_index << " PTS: " << l_pkt.pts << " DTS: " << l_pkt.dts
                  << " DURATION: " << l_pkt.duration;
      ThrowOnFfmpegErrorWithAllowedError2(l_ret = av_interleaved_write_frame(_av_format_context, &l_pkt),
                                          AVERROR(EAGAIN), AVERROR(EINVAL));
    }
    av_packet_unref(&l_pkt);
  } catch (const std::exception& e) {
    _is_internal_shutdown = true;
    RAY_LOG_ERR << "Closing due to: " << e.what();
  }
  av_thread_message_queue_set_err_send(_av_thread_message_queue, AVERROR_EOF);
  av_write_trailer(_av_format_context);
  avio_close(_av_format_context->pb);
  avformat_free_context(_av_format_context);
  RAY_LOG_INF << "End : OutputFromMessageQueue";
}
} // namespace ffpp
