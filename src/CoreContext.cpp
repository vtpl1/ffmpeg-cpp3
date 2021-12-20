#include "CoreContext.h"
#include "ffmpeg.h"
namespace ffpp
{
CoreContext::CoreContext()
{
  // av_register_all();
  av_log_set_level(AV_LOG_DEBUG);
}

CoreContext& CoreContext::instance()
{
  static CoreContext s_instance;
  return s_instance;
}
} // namespace ffpp
