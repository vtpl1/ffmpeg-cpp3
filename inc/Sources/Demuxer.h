// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef Demuxer_h
#define Demuxer_h
#include <atomic>
#include <map>
#include <string>
#include <vector>

#include "InputSource.h"
#include "ffmpeg.h"

namespace ffpp
{
class Demuxer : public InputSource
{
private:
  std::atomic_bool _do_shutdown{false};
  std::atomic_bool _is_internal_shutdown{false};
  bool _is_already_shutting_down{false};
  inline bool _do_shutdown_composite() { return (_do_shutdown || _is_internal_shutdown); }

  std::string _fileName;
  AVDictionary* _inputFormatAvDictionary{nullptr};
  AVInputFormat* _inputFormat{nullptr};
  AVFormatContext* _containerContext{nullptr};

public:
  Demuxer(const std::string& fileName, const std::string& input_format = std::string(),
          const std::multimap<std::string, std::string>& ffmpeg_options = std::multimap<std::string, std::string>());
  ~Demuxer();
  void SignalToStop();
  void Stop();
  bool IsDone();
  void Step();
  void PreparePipeline();
};

} // namespace ffpp

#endif // Demuxer_h
