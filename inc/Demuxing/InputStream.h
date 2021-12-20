// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef InputStream_h
#define InputStream_h
#include "CoreObject.h"
#include "ffmpeg.h"
namespace ffpp
{
class InputStream : public CoreObject
{
private:
  AVCodecContext* _codecContext{nullptr};
  AVFormatContext* _format;
  AVStream* _stream;

  void Stop();

public:
  InputStream(AVFormatContext* format, AVStream* stream);
  ~InputStream();
};
} // namespace ffpp

#endif // InputStream_h
