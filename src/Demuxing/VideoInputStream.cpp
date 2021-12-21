// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "VideoInputStream.h"
#include "CodecDeducer.h"
#include "FFmpegException.h"
#include "VideoStreamInfo.h"
namespace ffpp
{
VideoInputStream::VideoInputStream(AVFormatContext* format, AVStream* stream) : InputStream(format, stream) {}
void VideoInputStream::ConfigureCodecContext() {}
void VideoInputStream::AddStreamInfo(ContainerInfo* containerInfo)
{
  VideoStreamInfo info;

  info.id =
      _stream->id; // the layout of the id's depends on the container format - it doesn't always start from 0 or 1!

  AVRational overrideFrameRate;
  overrideFrameRate.num = 0;

  AVRational tb = overrideFrameRate.num ? av_inv_q(overrideFrameRate) : _stream->time_base;
  AVRational fr = overrideFrameRate;
  if (!fr.num)
    fr = av_guess_frame_rate(_format, _stream, NULL);

  StreamData* metaData = new StreamData();
  info.timeBase = tb;
  info.frameRate = fr;

  std::unique_ptr<AVCodecContext, Deleter<AVCodecContext>> codecContext;

  AVCodecContext* t_codecContext{nullptr};
  ThrowOnFfmpegReturnNullptr(t_codecContext = avcodec_alloc_context3(NULL));
  codecContext.reset(t_codecContext);
  ThrowOnFfmpegError(avcodec_parameters_to_context(_codecContext.get(), _stream->codecpar));

  codecContext->properties = _stream->codec->properties;
  codecContext->codec = _stream->codec->codec;
  codecContext->qmin = _stream->codec->qmin;
  codecContext->qmax = _stream->codec->qmax;
  codecContext->coded_width = _stream->codec->coded_width;
  codecContext->coded_height = _stream->codec->coded_height;

  info.bitRate = CalculateBitRate(codecContext.get());

  AVCodec* codec = CodecDeducer::DeduceDecoder(codecContext->codec_id);
  info.codec = codec;
  info.format = codecContext->pix_fmt;
  info.formatName = av_get_pix_fmt_name(info.format);

  info.width = codecContext->width;
  info.height = codecContext->height;
  containerInfo->videoStreams.push_back(info);
}
} // namespace ffpp
