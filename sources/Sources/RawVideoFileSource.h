#pragma once

#include "ffmpeg.h"

#include "InputSource.h"
#include "Demuxer.h"


namespace ffmpegcpp
{
	class RawVideoFileSource : public InputSource
	{
	public:

		RawVideoFileSource(const std::string & fileName, FrameSink* a_frameSink);

                RawVideoFileSource(const std::string &, int, int, int);
		// I couldn't get this to work. The thing is that it also crashes weirdly when I run ffmpeg directly,
		// so I think it's more an issue of ffmpeg than one of my library.
		//RawVideoFileSource(const char* fileName, int width, int height, const char* frameRate, AVPixelFormat format, VideoFrameSink* frameSink);
		virtual ~RawVideoFileSource();

		virtual void PreparePipeline();
		virtual bool IsDone();
		virtual void Step();
		Demuxer            * demuxer;
                void setFrameSink(FrameSink * aframeSink);

	private:
		AVFormatContext    * pAVFormatContextIn;
//		AVDictionary       * options;
//		enum AVPixelFormat   m_format;
//		AVCodec            * pAVCodec;
//		AVInputFormat      * inputFormat;
		int                  width;
		int                  height;
//		int                  m_framerate;
		void                 CleanUp();
		FrameSink          * m_frameSink;
	};
}
