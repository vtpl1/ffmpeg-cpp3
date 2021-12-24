// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef InputToMessageQueue_h
#define InputToMessageQueue_h
#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "CoreObject.h"
#include "FFmpegResource.h"
#include "ffmpeg.h"
#ifndef MSGQ_LENGTH
#define MSGQ_LENGTH 512
#endif
namespace ffpp
{
class InputToMessageQueue : public CoreObject
{

public:
  InputToMessageQueue(const std::string& input_name, int open_or_listen_timeout_in_sec);
  ~InputToMessageQueue();
  bool Start();
  void SignalToStop();
  void Stop();
  inline bool IsDone() { return (_do_shutdown || _is_internal_shutdown); }
  AVThreadMessageQueue* GetMessageQueue();
  inline AVFormatContext* GetInputAVFormatContext()
  {
    return _av_format_context;
  } // Valid onllly after successful start

  uint64_t _last_watch_dog_time_in_sec{0};
  int _time_out_in_sec;
  static int resetWatchDogTimer(void* opaque);

private:
  std::atomic_bool _do_shutdown{false};
  std::atomic_bool _is_internal_shutdown{false};
  bool _is_already_shutting_down{false};
  std::string _input_name;
  inline bool _do_shutdown_composite() { return (_do_shutdown || _is_internal_shutdown); }
  std::unique_ptr<std::thread> _thread;
  void run();

  AVFormatContext* _av_format_context{nullptr};
  AVCodecContext* _av_codec_context_audio{nullptr};
  AVThreadMessageQueue* _av_thread_message_queue{nullptr};
};

} // namespace ffpp

#endif // InputToMessageQueue_h
