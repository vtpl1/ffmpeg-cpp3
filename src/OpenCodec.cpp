// *****************************************************
//  Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "OpenCodec.h"

#include <string>

#include "FFmpegException.h"

namespace ffpp
{
OpenCodec::OpenCodec(AVCodecContext* openCodecContext)
{
  if (!avcodec_is_open(context)) {
    throw FFmpegException(
        std::string("Codec context for " + std::string(context->codec->name) + " hasn't been opened yet"));
  }
  this->context = context;
}
OpenCodec::~OpenCodec() { avcodec_free_context(&context); }
AVCodecContext* OpenCodec::GetContext() { return context; }
} // namespace ffpp
