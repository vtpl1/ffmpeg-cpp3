#include "VideoCodec.h"
#include "FFmpegException.h"
#ifdef __linux__
#include <limits>
#endif
#include <iostream>

/*
// Source : https://www.hardening-consulting.com/posts/20170625-utiliser-vaapi-avec-ffmpeg.html
static enum AVPixelFormat vaapi_get_format(AVCodecContext *ctx, const enum AVPixelFormat *fmt)
{
    const enum AVPixelFormat *fmtIt = fmt;

    while(*fmtIt != AV_PIX_FMT_NONE) {
        if (*fmtIt == AV_PIX_FMT_VAAPI_VLD) {
            if (!vaapi_decode_init(ctx))
                WLog_ERR(TAG, "error when initializing VAAPI");
            else
                return AV_PIX_FMT_VAAPI_VLD;
        }

        fmtIt++;
    }

    WLog_ERR(TAG, "expecting VAAPI format");
    return AV_PIX_FMT_NONE;
}

*/
using namespace std;

namespace ffmpegcpp
{
	VideoCodec::VideoCodec(const char* codecName)
		: Codec(codecName)
	{
	}

	VideoCodec::VideoCodec(AVCodecID codecId)
		: Codec(codecId)
	{
	}

	VideoCodec::~VideoCodec()
	{
	}

	void VideoCodec::SetQualityScale(int /* qscale */)
	{

		codecContext->flags |= AV_CODEC_FLAG_QSCALE;
		codecContext->global_quality = FF_QP2LAMBDA * 0;
	}

	bool VideoCodec::IsPixelFormatSupported(AVPixelFormat format)
	{
/*
                AVPixelFormat format2 = vaapi_get_format(codecContext, format)
*/
#ifdef DEBUG
                std::cout << "pixel format  : " << format << "\n";
                std::cout << "pixel format  AV_PIX_FMT_NONE = " << AV_PIX_FMT_NONE << "\n";
#endif
		if (format == AV_PIX_FMT_NONE) return true; // let the codec deal with this

		const enum AVPixelFormat *p = codecContext->codec->pix_fmts;
		while (*p != AV_PIX_FMT_NONE)
		{
			if (*p == format) return true;
			p++;
		}
		return false;
	}

	bool VideoCodec::IsFrameRateSupported(AVRational* frameRate)
	{
#ifdef DEBUG
            std::cerr  << "I'm in : "<<  __func__ << "...  questionning framerate :  " <<  frameRate->num <<  " / " << frameRate->den << "\n";
#endif

		if (!codecContext->codec->supported_framerates) return true; // all frame rates are fair game
		const AVRational *p = codecContext->codec->supported_framerates;
		while (p->num)
		{
#ifdef DEBUG
            std::cerr  << "supported framerates are "<<  p->num <<  " / " << p->den << "\n";
#endif

			if (av_cmp_q(*p, *frameRate) == 0) return true;
			p++;
		}
		return false;
	}

	OpenCodec* VideoCodec::Open(int width, int height, AVRational* frameRate, AVPixelFormat format)
	{
#ifdef DEBUG
            std::cerr  << "I'm in : "<<  __func__ << ", trying to set (width, height, framerate,pixel format) : " <<  width << ", " << height << ", " <<  frameRate->num  << "/" << frameRate->den << ", " << format <<"\n";
#endif

		// sanity checks
		if (!IsPixelFormatSupported(format)) throw FFmpegException(std::string("Pixel format " + string(av_get_pix_fmt_name(format)) + " is not supported by codec " + codecContext->codec->name).c_str());

                  std::cerr  <<  "IsPixelFormatSupported() done " << "\n";

		if (!IsFrameRateSupported(frameRate)) throw FFmpegException(std::string("Frame rate " + to_string(frameRate->num) + "/" + to_string(frameRate->den) + " is not supported by codec " + codecContext->codec->name).c_str());

                  std::cerr  <<  "IsFrameRateSupported() done " << "\n";

		// if the codec is not an audio codec, we are doing it wrong!
		if (codecContext->codec->type != AVMEDIA_TYPE_VIDEO) throw FFmpegException(std::string("A video output stream must be initialized with a video codec").c_str());

		// set everything & open
		codecContext->width = width;
		codecContext->height = height;
		codecContext->pix_fmt = format;

		// FPS
		AVRational time_base;
		time_base.num = frameRate->den;
		time_base.den = frameRate->num;
		codecContext->time_base = time_base;
		AVRational framerate;
		framerate.num = frameRate->num;
		framerate.den = frameRate->den;
		codecContext->framerate = framerate;

		return Codec::Open();
	}

	AVPixelFormat VideoCodec::GetDefaultPixelFormat()
	{
		const enum AVPixelFormat *p = codecContext->codec->pix_fmts;
		if (*p == AV_PIX_FMT_NONE) throw FFmpegException(std::string("Codec " + string(codecContext->codec->name) + " does not have a default pixel format, you have to specify one").c_str());
		return *p;
	}

	AVRational VideoCodec::GetClosestSupportedFrameRate(AVRational originalFrameRate)
	{
		if (!codecContext->codec->supported_framerates)
		{
			// make up a frame rate - there is no supported frame rate
			return originalFrameRate;
		};

		const AVRational *p = codecContext->codec->supported_framerates;
		AVRational bestFrameRate;
		bestFrameRate.num = 0;
		bestFrameRate.den = 1;
		double bestDiff = std::numeric_limits<double>::max();
		double fVal = av_q2d(originalFrameRate);
		while (p->num)
		{
			double pVal = av_q2d(*p);
			double diff = abs(pVal - fVal);
			if (diff < bestDiff)
			{
				bestDiff = diff;
				bestFrameRate.num = p->num;
				bestFrameRate.den = p->den;
			}
			p++;
		}

		// There were no valid frame rates in the list... this should never happen unless ffmpeg screws up.
		if (bestFrameRate.num == 0) return originalFrameRate;

		return bestFrameRate;
	}
}
