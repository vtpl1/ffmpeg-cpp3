#pragma once
#include "VideoCodec.h"

namespace ffmpegcpp
{

	class VP9Codec : public VideoCodec
	{

	public:

		VP9Codec();

		void SetDeadline(const char* deadline);
		void SetCpuUsed(int cpuUsed);

		void SetLossless(bool lossless);
		void SetCrf(int crf);
		void SetGlobalQuality(int global_quality);
// TODO : specific #ifdef ... #endif ?
		void InitHardwareDevice(const char * init_hw_device);
		void SetHardwareAccelAPI(const char * hwaccel);
		void SetVAAPIDevice(const char * vaapi_device);
		void SetHardwareAccelOutputFormat(const char * hwaccel_output_format);
		void SetHardwareAccelDevice(const char * hwaccel_device);
		void SetFilterHardwareDevice(const char * filter_hw_device);
//
	};


}