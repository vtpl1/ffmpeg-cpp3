// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "FFmpegException.h"

#include "ffmpeg.h"
#if _MSC_VER
#undef av_err2str
#define av_err2str(errnum)                                                                                             \
  av_make_error_string((char*)_malloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)
#else
#undef av_err2str
#define av_err2str(errnum)                                                                                             \
  av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)
#endif // _MSC_VER

namespace ffpp
{
FFmpegException::FFmpegException(const std::string& error) : _msg(error) {}

FFmpegException::FFmpegException(const std::string& error, int ffmpegErrorCode)
{
  _msg = error + "[" + av_err2str(ffmpegErrorCode) + "]";
}
} // namespace ffpp