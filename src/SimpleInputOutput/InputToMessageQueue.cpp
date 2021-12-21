// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "InputToMessageQueue.h"
#include <iostream>
namespace ffpp
{
InputToMessageQueue::InputToMessageQueue(const std::string& input_name) : _input_name(input_name) {}
InputToMessageQueue::~InputToMessageQueue() { Stop(); }
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
void InputToMessageQueue::run()
{
  std::cout << "Started : InputToMessageQueue" << std::endl;
  while (!_do_shutdown_composite()) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    _is_internal_shutdown = true;
  }
  std::cout << "End : InputToMessageQueue" << std::endl;
}
} // namespace ffpp
