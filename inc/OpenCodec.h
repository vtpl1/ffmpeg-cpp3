#pragma once
#ifndef OpenCodec_h
#define OpenCodec_h



#include "ffmpeg.h"
namespace ffpp {
class OpenCodec {
 private:
  AVCodecContext* context{nullptr};

 public:
  OpenCodec(AVCodecContext* openCodecContext);
  ~OpenCodec();
  AVCodecContext* GetContext();
};

}  // namespace ffmpegcpp3

#endif