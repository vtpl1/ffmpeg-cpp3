#pragma once
#include "VideoCodec.h"

namespace ffmpegcpp
{

	class MJPEGCodec : public VideoCodec
	{

	public:

		MJPEGCodec();

		void SetCompressionLevel(int compressionLevel);
	};


}