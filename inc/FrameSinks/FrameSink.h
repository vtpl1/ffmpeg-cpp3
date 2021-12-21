// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef FrameSink_h
#define FrameSink_h
#include "FrameSinkStream.h"
#include "ffmpeg.h"
namespace ffpp
{
class FrameSink
{
public:
  virtual FrameSinkStream* CreateStream() = 0;

  virtual AVMediaType GetMediaType() = 0;

  virtual ~FrameSink() {}
};

} // namespace ffpp

#endif // FrameSink_h
