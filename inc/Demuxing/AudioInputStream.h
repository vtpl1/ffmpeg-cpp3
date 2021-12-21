// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef AudioInputStream_h
#define AudioInputStream_h
#include "ContainerInfo.h"
#include "InputStream.h"
#include "ffmpeg.h"

namespace ffpp
{
class AudioInputStream : public InputStream
{
public:
  AudioInputStream(AVFormatContext* format, AVStream* stream);
  void AddStreamInfo(ContainerInfo* info);

protected:
  virtual void ConfigureCodecContext();
};

} // namespace ffpp

#endif // AudioInputStream_h
