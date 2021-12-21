// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "FrameContainer.h"
#include "FFmpegException.h"
namespace ffpp
{
FrameContainer::FrameContainer(AVFrame* frame, AVRational* timeBase)
{

  //   AVFrame* tmp = av_frame_clone(frame);
  AVFrame* tmp(nullptr);
  ThrowOnFfmpegReturnNullptr(tmp = av_frame_clone(frame));
  av_frame_unref(frame);
  _frame.reset(tmp);
  _timeBase = timeBase;
}
AVFrame* FrameContainer::GetFrame() { return _frame.get(); }
AVRational* FrameContainer::GetTimeBase() { return _timeBase; }
} // namespace ffpp
