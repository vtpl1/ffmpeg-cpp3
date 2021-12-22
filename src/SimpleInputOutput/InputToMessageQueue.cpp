// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include <iostream>

#include "FFmpegException.h"
#include "InputToMessageQueue.h"
#include "logging.h"
namespace ffpp
{
InputToMessageQueue::InputToMessageQueue(const std::string& input_name) : _input_name(input_name)
{
  ThrowOnFfmpegError(av_thread_message_queue_alloc(&_av_thread_message_queue, MSGQ_LENGTH, sizeof(AVPacket)));
  ThrowOnFfmpegReturnNullptr(_av_thread_message_queue);
}
InputToMessageQueue::~InputToMessageQueue()
{
  Stop();
  av_thread_message_queue_free(&_av_thread_message_queue);
}
void InputToMessageQueue::Start() { _thread.reset(new std::thread(&InputToMessageQueue::run, this)); }
void InputToMessageQueue::SignalToStop() { _do_shutdown = true; }
void InputToMessageQueue::Stop()
{
  if (_is_already_shutting_down)
    return;
  _is_already_shutting_down = true;
  SignalToStop();
  if (_thread->joinable()) {
    _thread->join();
  }
}
int InputToMessageQueue::resetWatchDogTimer(void* opaque)
{
  InputToMessageQueue* p = static_cast<InputToMessageQueue*>(opaque);
  return (time(NULL) - p->_last_watch_dog_time_in_sec) >= 5 ? 1 : 0; // 5 seconds timeout
}
AVThreadMessageQueue* InputToMessageQueue::GetMessageQueue()
{
  return _av_thread_message_queue;
}
void InputToMessageQueue::run()
{
  RAY_LOG(INFO) << "Started : InputToMessageQueue";
  AVDictionary* av_dictionary_opts{nullptr};
  try {

    if (_input_name.rfind("rtsp", 0) == 0) {
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "rtsp_transport", "tcp", 0));
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "rtsp_flags", "prefer_tcp", 0));
    } else if (_input_name.rfind("rtmp", 0) == 0) {
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "listen", "1", 0));
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "timeout", "10", 0));
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "rtmp_buffer", "100", 0));
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "rtmp_live", "live", 0));
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "analyzeduration", "32", 0));
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "probesize", "32", 0));
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "sync", "1", 0));
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "fflags", "nobuffer", 0));
      ThrowOnFfmpegError(av_dict_set(&av_dictionary_opts, "fflags", "flush_packets", 0));
    }
    ThrowOnFfmpegReturnNullptr(_av_format_context = avformat_alloc_context());
    _last_watch_dog_time_in_sec = time(NULL);
    _av_format_context->interrupt_callback.callback = InputToMessageQueue::resetWatchDogTimer;
    _av_format_context->interrupt_callback.opaque = this;
    if (!_do_shutdown_composite()) {
      RAY_LOG(INFO) << "Trying to open: " << _input_name;
      ThrowOnFfmpegError(avformat_open_input(&_av_format_context, _input_name.c_str(), NULL, &av_dictionary_opts));
      ThrowOnFfmpegError(avformat_find_stream_info(_av_format_context, NULL));
      bool is_valid_video_found{false};
      for (int32_t l_index = 0; l_index < _av_format_context->nb_streams; ++l_index) {
        AVStream* l_stream = _av_format_context->streams[l_index];
        if (l_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
          if (l_stream->codecpar->codec_id == AV_CODEC_ID_H264) {
            RAY_LOG(INFO) << "H264 Video found, proceeding";
            is_valid_video_found = true;
          }
        } else if (l_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
          if (l_stream->codecpar->codec_id != AV_CODEC_ID_AAC) {
            RAY_LOG(WARNING) << "Non AAC audio codec found: " << l_stream->codecpar->codec_id;
            AVCodec* l_dec{nullptr};
            ThrowOnFfmpegReturnNullptr(l_dec = avcodec_find_decoder(l_stream->codecpar->codec_id));
            ThrowOnFfmpegReturnNullptr(_av_codec_context_audio = avcodec_alloc_context3(l_dec));
            ThrowOnFfmpegError(avcodec_parameters_to_context(_av_codec_context_audio, l_stream->codecpar));
          }
        } else {
        }
      }
      if (!is_valid_video_found) {
        RAY_LOG(ERROR) << "No suitable (H264) video format found";
        _is_internal_shutdown = true;
      }
      av_dump_format(_av_format_context, 0, _input_name.c_str(), 0);
      int32_t l_ret = 0;
      AVPacket l_packet;
      int frame_count = 0;
      while (!_do_shutdown_composite()) {
        _last_watch_dog_time_in_sec = time(NULL);
        try {
          ThrowOnFfmpegErrorWithAllowedError(l_ret = av_read_frame(_av_format_context, &l_packet), AVERROR(EAGAIN));
          if (l_ret == AVERROR(EAGAIN))
            continue;
          ThrowOnFfmpegError(
              av_thread_message_queue_send(_av_thread_message_queue, &l_packet, AV_THREAD_MESSAGE_NONBLOCK));
        } catch (const std::exception& e) {
          _is_internal_shutdown = true;
          av_packet_unref(&l_packet);
          RAY_LOG(ERROR) << "Send Receive error: " << e.what();
        }
      }
      av_thread_message_queue_set_err_recv(_av_thread_message_queue, AVERROR_EOF);
      avformat_close_input(&_av_format_context);
      avcodec_free_context(&_av_codec_context_audio);
    }
  } catch (const std::exception& e) {
    _is_internal_shutdown = true;
    RAY_LOG(ERROR) << "Closing due to: " << e.what();
  }
  av_dict_free(&av_dictionary_opts);
  RAY_LOG(INFO) << "End : InputToMessageQueue";
}
} // namespace ffpp
