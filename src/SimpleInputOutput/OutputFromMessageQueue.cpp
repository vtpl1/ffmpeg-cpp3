// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "OutputFromMessageQueue.h"
#include <iostream>
namespace ffpp
{
OutputFromMessageQueue::OutputFromMessageQueue(const std::string& input_name) : _input_name(input_name) {}
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
void OutputFromMessageQueue::run()
{
  std::cout << "Started : OutputFromMessageQueue" << std::endl;
  while (!_do_shutdown_composite()) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    _is_internal_shutdown = true;
  }
  std::cout << "End : OutputFromMessageQueue" << std::endl;
}
} // namespace ffpp
