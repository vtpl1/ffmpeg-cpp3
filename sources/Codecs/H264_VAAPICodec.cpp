/*
 * File H264_VAAPICodec.cpp, from project https://github.com/ebachard/ffmpeg-cpp
 * Author : Eric Bachard  / Thu Apr 23 18:18:20 CEST 2020
 * This file is under GPL v3 license
 * see : http://www.gnu.org/licenses/gpl-3.0.html
 */


#include "H264_VAAPICodec.h"


// WORK IN PROGRESS, NOT COMPLETED !!   (it won't work ... )


namespace ffmpegcpp
{
    //VAProfileH264Main ?

    // CTor
    H264_VAAPICodec::H264_VAAPICodec() : VideoCodec("h264_vaapi") //	: VideoCodec("h264_v4l2m2m")
    {
    }

    void H264_VAAPICodec::SetProfile(const char * profile)
    {
        SetOption("profile", profile);  // vainfo will return lot of things ...
    }

    void H264_VAAPICodec::SetTune(const char * tune)
    {
        SetOption("tune", tune);
    }

    void H264_VAAPICodec::SetPreset(const char * preset)
    {
        SetOption("preset", preset);
    }

    void H264_VAAPICodec::SetCrf(int crf)
    {
        SetOption("crf", crf);
    }
// Hardware stuff (Intel here)
    void H264_VAAPICodec::InitHardwareDevice(const char * init_hw_device)
    {
	SetOption("init_hw_device", init_hw_device);
    }

    void H264_VAAPICodec::SetHardwareAccelAPI(const char * hwaccel)
    {
	SetOption("hwaccel", hwaccel);
    }

    void H264_VAAPICodec::SetVAAPIDevice(const char * vaapi_device)
    {
	SetOption("vaapi_device", vaapi_device);
    }

    void H264_VAAPICodec::SetHardwareAccelOutputFormat(const char * hwaccel_output_format)
    {
	SetOption("hwaccel_output_format", hwaccel_output_format);
    }

    void H264_VAAPICodec::SetHardwareAccelDevice(const char * hwaccel_device)
    {
	SetOption("hwaccel_device", hwaccel_device);
    }

    void H264_VAAPICodec::SetFilterHardwareDevice(const char * filter_hw_device)
    {
	SetOption("filter_hw_device", filter_hw_device);
    }

// End hardware stuff

}

