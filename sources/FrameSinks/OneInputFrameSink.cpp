#include "OneInputFrameSink.h"
#include "FFmpegException.h"

using namespace std;

namespace ffmpegcpp
{

	OneInputFrameSink::OneInputFrameSink(FrameWriter* p_writer, AVMediaType p_mediaType)
	{
		this->writer = p_writer;
		this->mediaType = p_mediaType;
	}

	FrameSinkStream* OneInputFrameSink::CreateStream()
	{
		++nStreamsGenerated;
		if (nStreamsGenerated > 1)
		{
			throw new FFmpegException("This frame sink only supports one input");
		}
		stream = new FrameSinkStream(writer, 0);
		return stream;
	}

	OneInputFrameSink::~OneInputFrameSink()
	{
		if (stream != nullptr) delete stream;
	}

	AVMediaType OneInputFrameSink::GetMediaType()
	{
		return mediaType;
	}
}

