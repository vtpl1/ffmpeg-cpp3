// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "AudioInputStream.h"
#include "CodecDeducer.h"
#include "ContainerInfo.h"
#include "FFmpegException.h"
#include "FFmpegResource.h"
namespace ffpp
{

AudioInputStream::AudioInputStream(AVFormatContext* format, AVStream* stream) : InputStream(format, stream) {}
void AudioInputStream::ConfigureCodecContext()
{
  // try to guess the channel layout for the decoder
  if (!_codecContext->channel_layout) {
    _codecContext->channel_layout = av_get_default_channel_layout(_codecContext->channels);
  }
}
void AudioInputStream::AddStreamInfo(ContainerInfo* containerInfo)
{
  AudioStreamInfo info;

  info.id =
      _stream->id; // the layout of the id's depends on the container format - it doesn't always start from 0 or 1!

  AVRational tb = _stream->time_base;

  StreamData* metaData = new StreamData();
  info.timeBase = tb;

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

  info.sampleRate = codecContext->sample_rate;
  info.channels = codecContext->channels;
  info.channelLayout = codecContext->channel_layout;
  av_get_channel_layout_string(info.channelLayoutName, sizeof(info.channelLayoutName), codecContext->channels,
                               codecContext->channel_layout);
  containerInfo->audioStreams.push_back(info);
}

} // namespace ffpp