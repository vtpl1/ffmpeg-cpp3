// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef AudioStreamInfo_h
#define AudioStreamInfo_h
#include "ffmpeg.h"

namespace ffpp
{
struct AudioStreamInfo {
  int id;
  AVRational timeBase;
  const AVCodec* codec;
  float bitRate;

  int sampleRate;
  int channels;

  uint64_t channelLayout;
  char channelLayoutName[255];
};

} // namespace ffpp

#endif // AudioStreamInfo_h
