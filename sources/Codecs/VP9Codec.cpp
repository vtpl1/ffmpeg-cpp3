#include "VP9Codec.h"

namespace ffmpegcpp
{
	VP9Codec::VP9Codec()
#ifdef __linux__
		: VideoCodec(AV_CODEC_ID_VP9)
#else
		: VideoCodec("libvpx-vp9")
#endif
	{

	}

	void VP9Codec::SetDeadline(const char* deadline)
	{
		SetOption("deadline", deadline);
	}

	void VP9Codec::SetCpuUsed(int cpuUsed)
	{
		SetOption("cpu-used", cpuUsed);
	}

	void VP9Codec::SetLossless(bool lossless)
	{
		SetOption("lossless", (lossless ? 1 : 0));
	}

	void VP9Codec::SetCrf(int crf)
	{
		SetOption("crf", crf);
	}

	void VP9Codec::SetGlobalQuality(int global_quality)
	{
		SetOption("global_quality", global_quality);
	}

// Hardware stuff (Intel here)
	void VP9Codec::InitHardwareDevice(const char * init_hw_device)
	{
		SetOption("init_hw_device", init_hw_device);
	}

	void VP9Codec::SetHardwareAccelAPI(const char * hwaccel)
	{
		SetOption("hwaccel", hwaccel);
	}

	void VP9Codec::SetVAAPIDevice(const char * vaapi_device)
	{
		SetOption("vaapi_device", vaapi_device);
	}

	void VP9Codec::SetHardwareAccelOutputFormat(const char * hwaccel_output_format)
	{
		SetOption("hwaccel_output_format", hwaccel_output_format);
	}

	void VP9Codec::SetHardwareAccelDevice(const char * hwaccel_device)
	{
		SetOption("hwaccel_device", hwaccel_device);
	}

	void VP9Codec::SetFilterHardwareDevice(const char * filter_hw_device)
	{
		SetOption("filter_hw_device", filter_hw_device);
	}

// End hardware stuff

}