/*
 * File MPEG4Codec.cpp, from project https://github.com/ebachard/ffmpeg-cpp
 * Author : Eric Bachard  / Thu Apr 23 18:18:20 CEST 2020
 * This file is under GPL v3 license
 * see : http://www.gnu.org/licenses/gpl-3.0.html
 */


#include "MPEG4Codec.h"

/* TO MAKE IT WORK,  YOU MUST INSTALL LIBX264 AND SET -enable-gpl building FFMPEG (else:  segmentation fault) */

namespace ffmpegcpp
{

    // CTor
    MPEG4Codec::MPEG4Codec() : VideoCodec(AV_CODEC_ID_MPEG4)
    {
    }

// TODO : improve parameters, probably wrong ...
    void MPEG4Codec::SetProfile(const char * profile)
    {
        SetOption("profile", profile);
    }

    void MPEG4Codec::SetTune(const char * tune)
    {
        SetOption("tune", tune);
    }

    void MPEG4Codec::SetPreset(const char * preset)
    {
        SetOption("preset", preset);
    }

    void MPEG4Codec::SetCrf(int crf)
    {
        SetOption("crf", crf);
    }
}

