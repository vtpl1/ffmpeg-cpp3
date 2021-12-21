// *****************************************************
//  Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "OpenCodec.h"

#include <string>

#include "FFmpegException.h"

namespace ffpp
{
OpenCodec::OpenCodec(AVCodecContext* context)
{
  ThrowOnFfmpegError(avcodec_is_open(context));
  _context.reset(context);
}

AVCodecContext* OpenCodec::GetContext() { return _context.get(); }
} // namespace ffpp
