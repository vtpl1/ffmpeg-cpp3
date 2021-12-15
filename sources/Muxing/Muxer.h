#pragma once

#include "ffmpeg.h"
#include <vector>
#include <string>

namespace ffmpegcpp {

	class OutputStream;

	class Muxer
	{
	public:

		Muxer(const std::string & fileName);
		~Muxer();

		void AddOutputStream(OutputStream* stream);

		void WritePacket(AVPacket* pkt);

		void Close();
		
		bool IsPrimed();

		AVCodec* GetDefaultVideoFormat();
		AVCodec* GetDefaultAudioFormat();


	private:

		void Open();
		
		std::vector<OutputStream*> outputStreams;
		std::vector<AVPacket*> packetQueue;

		AVOutputFormat* containerFormat;

		AVFormatContext* containerContext = nullptr;

		std::string fileName;

		void CleanUp();

		bool opened = false;
	};
}
