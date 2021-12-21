// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "FrameSinkStream.h"

namespace ffpp
{

FrameSinkStream::FrameSinkStream(FrameWriter* frameSink, int streamIdx)
    : frameSink(frameSink), streamIndex(streamIdx)
{
}

void FrameSinkStream::WriteFrame(AVFrame* frame, StreamData* metaData)
{
  frameSink->WriteFrame(streamIndex, frame, metaData);
}

void FrameSinkStream::Close() { frameSink->Close(streamIndex); }

bool FrameSinkStream::IsPrimed() { return frameSink->IsPrimed(); }

} // namespace ffpp
