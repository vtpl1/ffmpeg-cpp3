// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef FrameSinkStream_h
#define FrameSinkStream_h
#include "FrameWriter.h"
#include "ffmpeg.h"
namespace ffpp
{

class FrameSinkStream
{
public:
  FrameSinkStream(FrameWriter* frameSink, int streamIdx);
  void WriteFrame(AVFrame* frame, StreamData* metaData);
  void Close();
  bool IsPrimed();

private:
  FrameWriter* frameSink;
  int streamIndex;
};

} // namespace ffpp
#endif // FrameSinkStream_h
