// *****************************************************
//  Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef OpenCodec_h
#define OpenCodec_h
#include "FFmpegResource.h"
#include "ffmpeg.h"
#include <memory>

namespace ffpp
{
class OpenCodec
{
public:
  OpenCodec(AVCodecContext* context);
  AVCodecContext* GetContext();
private:
  std::unique_ptr<AVCodecContext, Deleter<AVCodecContext>> _context{nullptr};
};

} // namespace ffpp
#endif // OpenCodec_h