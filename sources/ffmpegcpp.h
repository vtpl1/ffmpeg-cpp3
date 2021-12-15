
#include "ffmpeg.h"

#include "Muxing/Muxer.h"

#include "Sources/Demuxer.h"
#include "Sources/InputSource.h"
#include "Sources/RawAudioFileSource.h"
#include "Sources/RawVideoFileSource.h"
#include "Sources/RawAudioDataSource.h"
#include "Sources/RawVideoDataSource.h"
#include "Sources/EncodedFileSource.h"

#include "FrameSinks/VideoEncoder.h"
#include "FrameSinks/AudioEncoder.h"
#include "FrameSinks/Filter.h"

#include "Codecs/AudioCodec.h"

#include "codecs/H265NVEncCodec.h"
#include "codecs/H264NVEncCodec.h"
#include "Codecs/VP9Codec.h"
#include "Codecs/PNGCodec.h"
#include "Codecs/JPGCodec.h"
#include "Codecs/MJPEGCodec.h"
#include "Codecs/H264Codec.h"
#include "Codecs/H264_VAAPICodec.h"
#include "Codecs/MPEG4Codec.h"
#include "Codecs/VideoCodec.h"
#include "Codecs/Codec.h"

#include "FFmpegException.h"

#ifdef __linux__
#define AV_PIX_FMT_VAAPI_VLD AV_PIX_FMT_VAAPI
#endif
