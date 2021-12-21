// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef VideoStreamInfo_h
#define VideoStreamInfo_h
#include "ffmpeg.h"
#include <vector>
namespace ffpp
{
struct VideoStreamInfo {
  int id;
  AVRational frameRate;
  AVRational timeBase;
  const AVCodec* codec;
  float bitRate;

  AVPixelFormat format;
  const char* formatName;

  int width;
  int height;
};

} // namespace ffpp

#endif // VideoStreamInfo_h
