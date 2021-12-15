#pragma once

#include "ffmpeg.h"

#include "InputSource.h"
#include "Demuxer.h"


namespace ffmpegcpp
{
	class RawAudioFileSource : public InputSource
	{
	public:

		RawAudioFileSource(const std::string & fileName, const std::string & inputFormat, int sampleRate, int channels, FrameSink* frameSink);
		virtual ~RawAudioFileSource();

		virtual void PreparePipeline();
		virtual bool IsDone();
		virtual void Step();

                void Stop() { demuxer->Stop(); }

	private:
		void CleanUp();

		Demuxer* demuxer = nullptr;
	};


}
