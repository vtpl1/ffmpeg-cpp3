// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef CodecDeducer_h
#define CodecDeducer_h
#include "ffmpeg.h"
namespace ffpp
{
namespace CodecDeducer
{
AVCodec* DeduceEncoderFromFilename(const char* fileName);

AVCodec* DeduceEncoder(AVCodecID codecId);
AVCodec* DeduceEncoder(const char* codecName);

AVCodec* DeduceDecoder(AVCodecID codecId);
AVCodec* DeduceDecoder(const char* codecName);
} // namespace CodecDeducer

} // namespace ffpp

#endif // CodecDeducer_h
