#pragma once

#include "ffmpeg.h"
#include "Demuxing/InputStream.h"
#include "FrameSinks/VideoFrameSink.h"
#include "Info/VideoStreamInfo.h"

namespace ffmpegcpp
{
	class VideoInputStream : public InputStream
	{

	public:

		VideoInputStream(AVFormatContext* format, AVStream* stream);
		~VideoInputStream();

		void AddStreamInfo(ContainerInfo* info);

	protected:

		virtual void ConfigureCodecContext();

	};
}
