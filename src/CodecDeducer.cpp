// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "CodecDeducer.h"
#include "FFmpegException.h"
namespace ffpp
{
namespace CodecDeducer
{
AVCodec* DeduceEncoderFromFilename(const char* fileName) { throw FFmpegNotImplementedException(); }

AVCodec* DeduceEncoder(AVCodecID codecId)
{
  AVCodec* codec = avcodec_find_encoder(codecId);
  ThrowOnFfmpegReturnNullptr(codec);
  return codec;
}
AVCodec* DeduceEncoder(const char* codecName)
{
  AVCodec* codec = avcodec_find_encoder_by_name(codecName);
  ThrowOnFfmpegReturnNullptr(codec);
  return codec;
}

AVCodec* DeduceDecoder(AVCodecID codecId)
{
  if (codecId == AV_CODEC_ID_NONE)
    return nullptr;
  AVCodec* codec = avcodec_find_decoder(codecId);
  ThrowOnFfmpegReturnNullptr(codec);
  return codec;
}
AVCodec* DeduceDecoder(const char* codecName)
{
  AVCodec* codec = avcodec_find_decoder_by_name(codecName);
  ThrowOnFfmpegReturnNullptr(codec);
  return codec;
}
} // namespace CodecDeducer
} // namespace ffpp