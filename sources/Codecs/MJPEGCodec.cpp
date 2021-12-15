#include "MJPEGCodec.h"

namespace ffmpegcpp
{

	MJPEGCodec::MJPEGCodec()
		: VideoCodec(AV_CODEC_ID_MJPEG)
//		: VideoCodec("mjpeg")
	{

		codecContext->pix_fmt = AV_PIX_FMT_YUVJ420P;
	}

// TODO : fixme, wrong parameters
	void MJPEGCodec::SetCompressionLevel(int compressionLevel)
	{
		SetOption("compression_level", compressionLevel);
	}
}