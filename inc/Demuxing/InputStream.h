// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef InputStream_h
#define InputStream_h
#include <memory>

#include "ContainerInfo.h"
#include "CoreObject.h"
#include "FFmpegResource.h"
#include "FrameSink.h"
#include "FrameSinkStream.h"
#include "StreamData.h"
#include "ffmpeg.h"
namespace ffpp
{
class InputStream : public CoreObject
{
public:
  InputStream(AVFormatContext* format, AVStream* stream);
  void Open(FrameSink* frameSink);
  virtual void DecodePacket(AVPacket* pkt);
  void Close();
  bool IsPrimed();
  int GetFramesProcessed();
  virtual void AddStreamInfo(ContainerInfo* info) = 0;

protected:
  std::unique_ptr<AVCodecContext, Deleter<AVCodecContext>> _codecContext{nullptr};
  virtual void ConfigureCodecContext();
  AVFormatContext* _format;
  AVStream* _stream;
  float CalculateBitRate(AVCodecContext* ctx);

private:
  AVRational _timeBaseCorrectedByTicksPerFrame;

  FrameSinkStream* _output{nullptr};

  std::unique_ptr<AVFrame, Deleter<AVFrame>> _frame{nullptr};

  std::unique_ptr<StreamData> _metaData{nullptr};
  int _nFramesProcessed{0};

  StreamData* DiscoverMetaData();
};
} // namespace ffpp

#endif // InputStream_h
