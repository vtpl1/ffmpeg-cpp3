/*
 * File H264Codec.cpp, from project https://github.com/ebachard/ffmpeg-cpp
 * Author : Eric Bachard  / Thu Apr 23 18:18:20 CEST 2020
 * This file is under GPL v3 license
 * see : http://www.gnu.org/licenses/gpl-3.0.html
 */


#include "H264Codec.h"

/* TO MAKE IT WORK,  YOU MUST INSTALL LIBX264 AND SET -enable-gpl building FFMPEG (else:  segmentation fault) */

namespace ffmpegcpp
{

    // CTor
    H264Codec::H264Codec() : VideoCodec(AV_CODEC_ID_H264)
    {
    }

    void H264Codec::SetProfile(const char * profile)
    {
        SetOption("profile", profile);
    }

    void H264Codec::SetTune(const char * tune)
    {
        SetOption("tune", tune);
    }

    void H264Codec::SetPreset(const char * preset)
    {
        SetOption("preset", preset);
    }

    void H264Codec::SetCrf(int crf)
    {
        SetOption("crf", crf);
    }

    void H264Codec::SetMovFlags(const char * movflags)
    {
        SetOption("movflags", movflags);
    }

}

