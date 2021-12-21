// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef VideoInputStream_h
#define VideoInputStream_h
#include "ContainerInfo.h"
#include "InputStream.h"
#include "ffmpeg.h"
namespace ffpp
{
class VideoInputStream : public InputStream
{
public:
  VideoInputStream(AVFormatContext* format, AVStream* stream);
  void AddStreamInfo(ContainerInfo* info);

protected:
  virtual void ConfigureCodecContext() override;
};

} // namespace ffpp

#endif // VideoInputStream_h
