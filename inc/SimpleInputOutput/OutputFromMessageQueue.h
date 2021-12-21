// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef OutputFromMessageQueue_h
#define OutputFromMessageQueue_h
#include <atomic>
#include <memory>
#include <string>
#include <thread>
namespace ffpp
{
class OutputFromMessageQueue
{

public:
  OutputFromMessageQueue(const std::string& input_name);
  ~OutputFromMessageQueue();
  void Start();
  void SignalToStop();
  void Stop();
  inline bool IsDone() { return (_do_shutdown || _is_internal_shutdown); }

private:
  std::atomic_bool _do_shutdown{false};
  std::atomic_bool _is_internal_shutdown{false};
  bool _is_already_shutting_down{false};
  std::string _input_name;
  inline bool _do_shutdown_composite() { return (_do_shutdown || _is_internal_shutdown); }
  std::thread _thread;
  void run();
};

} // namespace ffpp

#endif // OutputFromMessageQueue_h
