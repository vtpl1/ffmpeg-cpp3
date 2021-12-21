// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef ContainerInfo_h
#define ContainerInfo_h
#include "AudioStreamInfo.h"
#include "VideoStreamInfo.h"
#include "ffmpeg.h"
#include <vector>

namespace ffpp
{
struct ContainerInfo {
  long durationInMicroSeconds;
  float durationInSeconds;
  float start;
  float bitRate;
  const AVInputFormat* format;

  std::vector<VideoStreamInfo> videoStreams;
  std::vector<AudioStreamInfo> audioStreams;
};

} // namespace ffpp

#endif // ContainerInfo_h
