#pragma once

#include "ffmpeg.h"
#include "FrameSinks/FrameSink.h"
#include "InputSource.h"

#include <string>

namespace ffmpegcpp
{
	// EncodedFileSource takes a file that is already encoded but not in a container (ie .mp3, .h264)
	// and feeds it to the system.
	class EncodedFileSource : public InputSource
	{

	public:
		EncodedFileSource(const std::string & inFileName, AVCodecID codecId, FrameSink* output);
		EncodedFileSource(const std::string & inFileName, const std::string & codecName, FrameSink* output);
		virtual ~EncodedFileSource();

		virtual void PreparePipeline();
		virtual bool IsDone();
		virtual void Step();

	private:

		void CleanUp();

		bool done = false;

		FrameSinkStream* output;
		
		AVCodecParserContext* parser = nullptr;

		AVCodec* codec;
		AVCodecContext* codecContext = nullptr;

		int bufferSize;
                int refillThreshold;


		AVFrame* decoded_frame = nullptr;
		AVPacket* pkt = nullptr;
		uint8_t* buffer = nullptr;

		FILE* file;

		void Init(const std::string & inFileName, AVCodec* codec, FrameSink* output);

		void Decode(AVPacket *packet, AVFrame* targetFrame);

		AVRational timeBaseCorrectedByTicksPerFrame;

		StreamData* metaData = nullptr;
	};
}
