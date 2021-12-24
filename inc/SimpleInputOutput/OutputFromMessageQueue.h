// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef OutputFromMessageQueue_h
#define OutputFromMessageQueue_h
#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include "CoreObject.h"
#include "ffmpeg.h"
namespace ffpp
{
class OutputFromMessageQueue : public CoreObject
{

public:
  OutputFromMessageQueue(const std::string& input_name, void* av_thread_message_queue, void* input_av_format_context,
                         int open_or_listen_timeout_in_sec);
  ~OutputFromMessageQueue();
  bool Start();
  void SignalToStop();
  void Stop();
  inline bool IsDone() { return (_do_shutdown || _is_internal_shutdown); }

  uint64_t _last_watch_dog_time_in_sec{0};
  int _time_out_in_sec;

  static int resetWatchDogTimer(void* opaque);

private:
  std::atomic_bool _do_shutdown{false};
  std::atomic_bool _is_internal_shutdown{false};
  bool _is_already_shutting_down{false};
  std::string _output_name;
  inline bool _do_shutdown_composite() { return (_do_shutdown || _is_internal_shutdown); }
  std::unique_ptr<std::thread> _thread;
  void run();

  AVFormatContext* _av_format_context{nullptr};
  AVFormatContext* _input_av_format_context{nullptr};
  AVThreadMessageQueue* _av_thread_message_queue{nullptr};
  std::map<int, int> _input_output_index_map;
};

} // namespace ffpp

#endif // OutputFromMessageQueue_h
