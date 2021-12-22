// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include <iostream>

#include "FFmpegException.h"
#include "OutputFromMessageQueue.h"
#include "logging.h"
namespace ffpp
{
OutputFromMessageQueue::OutputFromMessageQueue(const std::string& output_name, void* av_thread_message_queue)
    : _output_name(output_name), _av_thread_message_queue(static_cast<AVThreadMessageQueue*>(av_thread_message_queue))
{
}
OutputFromMessageQueue::~OutputFromMessageQueue() { Stop(); }
void OutputFromMessageQueue::Start() { _thread.reset(new std::thread(&OutputFromMessageQueue::run, this)); }
void OutputFromMessageQueue::SignalToStop() { _do_shutdown = true; }
void OutputFromMessageQueue::Stop()
{
  if (_is_already_shutting_down)
    return;
  _is_already_shutting_down = true;
  SignalToStop();
  if (_thread->joinable()) {
    _thread->join();
  }
}
int OutputFromMessageQueue::resetWatchDogTimer(void* opaque)
{
  OutputFromMessageQueue* p = static_cast<OutputFromMessageQueue*>(opaque);
  return (time(NULL) - p->_last_watch_dog_time_in_sec) >= 5 ? 1 : 0; // 5 seconds timeout
}
void OutputFromMessageQueue::run()
{
  RAY_LOG(INFO) << "Started : OutputFromMessageQueue";
  std::string format_hint;
  AVDictionary* l_opts = NULL;
  try {
    if (_output_name.rfind("rtmp", 0) == 0) {
      format_hint = "flv";
    }
    ThrowOnFfmpegError(avformat_alloc_output_context2(
        &_av_format_context, NULL, (format_hint.empty() ? NULL : format_hint.c_str()), _output_name.c_str()));
    ThrowOnFfmpegReturnNullptr(_av_format_context);
    _last_watch_dog_time_in_sec = time(NULL);
    _av_format_context->interrupt_callback.callback = OutputFromMessageQueue::resetWatchDogTimer;
    _av_format_context->interrupt_callback.opaque = this;
    if (!_do_shutdown_composite()) {
      for (int32_t l_index = 0; l_index < _av_format_context->nb_streams; ++l_index) {
        AVStream* l_in_stream = _av_format_context->streams[l_index];
        if (l_in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
          AVStream* l_out_stream = avformat_new_stream(_av_format_context, NULL);
          ThrowOnFfmpegReturnNullptr(l_out_stream);
          ThrowOnFfmpegError(avcodec_parameters_copy(l_out_stream->codecpar, l_in_stream->codecpar));
          l_out_stream->codecpar->codec_tag = 0;
          l_out_stream->time_base = l_in_stream->time_base;
        } else if (l_in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
          AVStream* l_out_stream = avformat_new_stream(_av_format_context, NULL);
          ThrowOnFfmpegReturnNullptr(l_out_stream);
          ThrowOnFfmpegError(avcodec_parameters_copy(l_out_stream->codecpar, l_in_stream->codecpar));
          l_out_stream->codecpar->codec_tag = 0;
          l_out_stream->time_base = l_in_stream->time_base;
        }
      }
      av_dump_format(_av_format_context, 0, _output_name.c_str(), 1);
      if (!(_av_format_context->oformat->flags & AVFMT_NOFILE)) {
        _last_watch_dog_time_in_sec = time(NULL);
        ThrowOnFfmpegError(avio_open2(&_av_format_context->pb, _output_name.c_str(), AVIO_FLAG_WRITE, NULL, &l_opts));
        ThrowOnFfmpegError(avformat_write_header(_av_format_context, &l_opts));
      }
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
        ThrowOnFfmpegErrorWithAllowedError(l_ret = av_interleaved_write_frame(_av_format_context, &l_pkt),
                                           AVERROR(EAGAIN));
      }
      av_write_trailer(_av_format_context);
      avio_close(_av_format_context->pb);
      avformat_free_context(_av_format_context);
    }
  } catch (const std::exception& e) {
    _is_internal_shutdown = true;
    RAY_LOG(ERROR) << "Closing due to: " << e.what();
  }
  av_dict_free(&l_opts);
  RAY_LOG(INFO) << "End : OutputFromMessageQueue";
}
} // namespace ffpp
