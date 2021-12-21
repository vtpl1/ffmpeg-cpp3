// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef FrameWriter_h
#define FrameWriter_h
#include "StreamData.h"
namespace ffpp
{
class FrameWriter
{
public:
  virtual void WriteFrame(int streamIndex, AVFrame* frame, StreamData* metaData) = 0;
  virtual void Close(int streamIndex) = 0;
  virtual bool IsPrimed() = 0;
};

} // namespace ffpp

#endif // FrameWriter_h
