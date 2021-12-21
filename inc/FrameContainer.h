// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef FrameContainer_h
#define FrameContainer_h
#include "FFmpegResource.h"
#include "ffmpeg.h"
#include <memory>
namespace ffpp
{
class FrameContainer
{
public:
  FrameContainer(AVFrame* frame, AVRational* timeBase);
  AVFrame* GetFrame();
  AVRational* GetTimeBase();

private:
  std::unique_ptr<AVFrame, Deleter<AVFrame>> _frame;
  AVRational* _timeBase;
};

} // namespace ffpp

#endif // FrameContainer_h
