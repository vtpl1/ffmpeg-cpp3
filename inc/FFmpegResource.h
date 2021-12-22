// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef FFmpegResource_h
#define FFmpegResource_h
#include "ffmpeg.h"
namespace ffpp
{
template <typename value_type> struct Deleter {
};
template <> struct Deleter<AVFormatContext> {
  void operator()(AVFormatContext* ptr) { avformat_free_context(ptr); }
};
template <> struct Deleter<AVPacket> {
  void operator()(AVPacket* ptr) { av_packet_free(&ptr); }
};
template <> struct Deleter<AVCodecContext> {
  void operator()(AVCodecContext* ptr) { avcodec_free_context(&ptr); }
};
template <> struct Deleter<AVFrame> {
  void operator()(AVFrame* ptr) { av_frame_free(&ptr); }
};
template <> struct Deleter<AVDictionary> {
  void operator()(AVDictionary* ptr) { av_dict_free(&ptr); }
};
} // namespace ffpp

#endif // FFmpegResource_h
