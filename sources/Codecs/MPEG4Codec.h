/*
 * Header file H264Codec.h, from project https://github.com/ebachard/ffmpeg-cpp
 * Author : Eric Bachard  / Thu Apr 23 18:18:20 CEST 2020
 * This file is under GPL v3 license
 * see : http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef __MPEG4CODEC_H__
#define __MPEG4CODEC_H__

#pragma once
#include "VideoCodec.h"

/* TO MAKE IT WORK,  YOU MUST INSTALL LIBX264 AND SET -enable-gpl building FFMPEG */


namespace ffmpegcpp
{
    class MPEG4Codec : public VideoCodec
    {

        public:

            MPEG4Codec();


// SOURCE : https://trac.ffmpeg.org/wiki/Encode/H.264
/*

  profile

  baseline
  main
  High

  (automatic)

  The -profile:v option limits the output to a specific H.264 profile.
  Some devices (mostly very old or obsolete) only support the more limited 
  Constrained Baseline or Main profiles. 
  You can set these profiles with -profile:v baseline or -profile:v main.
  Most modern devices support the more advanced High profile.
  Unless you need to support limited devices the recommendation is to omit setting the profile which will allow x264 to automatically select the appropriate profile. 
*/

        void SetProfile(const char * profile);

/*
  tune

  You can optionally use -tune to change settings based upon the specifics of your input. Current tunings include:

    film – use for high quality movie content; lowers deblocking
    animation – good for cartoons; uses higher deblocking and more reference frames
    grain – preserves the grain structure in old, grainy film material
    stillimage – good for slideshow-like content
    fastdecode – allows faster decoding by disabling certain filters
    zerolatency – good for fast encoding and low-latency streaming
    psnr – ignore this as it is only used for codec development
    ssim – ignore this as it is only used for codec development 
		void SetCpuUsed(int cpuUsed);
*/

        void SetTune(const char * tune);

/*

  preset

  Use the slowest preset that you have patience for. The available presets in descending order of speed are:

    ultrafast   // poor quality, fastest
    superfast
    veryfast
    faster
    fast
    medium – default preset
    slow
    slower
    veryslow  // slow, better quality
*/

        void SetPreset(const char * preset);

/*
  crf

  The range of the CRF scale is 0–51, where 0 is lossless, 23 is the default.
  51 is worst quality possible. A lower value generally leads to higher quality,
  and a subjectively sane range is 17–28.
  Consider 17 or 18 to be visually lossless or nearly so; it should look the same 
  or nearly the same as the input but it isn't technically lossless. 

  When compiled with 10-bit support, x264's quantizer scale is 0–63

*/
        void SetCrf(int crf);
    };
}

#endif /* __MPEG4CODEC_H__ */
