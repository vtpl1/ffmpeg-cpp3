// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef StreamData_h
#define StreamData_h
#include "ffmpeg.h"
namespace ffpp
{
struct StreamData {
  AVMediaType type;

  AVRational timeBase;
  AVRational frameRate;
};

} // namespace ffpp

#endif // StreamData_h
