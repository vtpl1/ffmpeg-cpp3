//
//    Copyright 2021 Videonetics Technology Pvt Ltd
//

#pragma once
#ifndef ffmpeg_h
#define ffmpeg_h

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include <libavdevice/avdevice.h>
#ifdef __cplusplus
}
#endif
#endif