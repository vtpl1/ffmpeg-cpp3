// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef Demuxer_h
#define Demuxer_h
#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ContainerInfo.h"
#include "FFmpegResource.h"
#include "InputSource.h"
#include "InputStream.h"
#include "ffmpeg.h"
namespace ffpp
{

class Demuxer : public InputSource
{
public:
  Demuxer(const std::string& fileName, const std::string& input_format = std::string(),
          const std::multimap<std::string, std::string>& ffmpeg_options = std::multimap<std::string, std::string>());
  ~Demuxer();

  void DecodeBestAudioStream(FrameSink* frameSink);
  void DecodeBestVideoStream(FrameSink* frameSink);

  void DecodeAudioStream(int streamId, FrameSink* frameSink);
  void DecodeVideoStream(int streamId, FrameSink* frameSink);

  void PreparePipeline() override;
  bool IsDone() override;
  void Step() override;

  ContainerInfo GetInfo();
  int GetFrameCount(int streamId);

  std::string GetFileName();

  void SignalToStop();
  void Stop();

private:
  std::atomic_bool _do_shutdown{false};
  std::atomic_bool _is_internal_shutdown{false};
  bool _is_already_shutting_down{false};
  inline bool _do_shutdown_composite() { return (_do_shutdown || _is_internal_shutdown); }

  std::string _fileName;
  AVInputFormat* _inputFormat{nullptr};

  std::unique_ptr<AVFormatContext, Deleter<AVFormatContext>> _containerContext{nullptr};
  std::vector<std::unique_ptr<InputStream>> _inputStreams;
  std::unique_ptr<AVPacket, Deleter<AVPacket>> _pkt{nullptr};

  InputStream* GetInputStream(int index);
  InputStream* GetInputStreamById(int streamId);
  void DecodePacket();
};

} // namespace ffpp

#endif // Demuxer_h
