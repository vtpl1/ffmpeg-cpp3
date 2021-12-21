// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "InputStream.h"
#include "CodecDeducer.h"
#include "FFmpegException.h"
namespace ffpp
{
InputStream::InputStream(AVFormatContext* format, AVStream* stream) : _format(format), _stream(stream)
{
  // find decoder for the stream
  AVCodec* codec = CodecDeducer::DeduceDecoder(stream->codecpar->codec_id);
  // Allocate a codec context for the decoder
  AVCodecContext* t_codecContext{nullptr};
  ThrowOnFfmpegReturnNullptr(t_codecContext = avcodec_alloc_context3(codec));
  _codecContext.reset(t_codecContext);
  _codecContext->framerate = _stream->avg_frame_rate;
  // Copy codec parameters from input stream to output codec context
  ThrowOnFfmpegError(avcodec_parameters_to_context(_codecContext.get(), _stream->codecpar));
  // before we open it, we let our subclasses configure the codec context as well
  ConfigureCodecContext();
  // Init the decoders
  ThrowOnFfmpegError(avcodec_open2(_codecContext.get(), codec, NULL));
  // calculate the "correct" time_base
  // TODO(Monotosh): this is definitely an ugly hack but right now I have no idea on how to fix this properly.
  _timeBaseCorrectedByTicksPerFrame.num = _codecContext->time_base.num;
  _timeBaseCorrectedByTicksPerFrame.den = _codecContext->time_base.den;
  _timeBaseCorrectedByTicksPerFrame.num *= _codecContext->ticks_per_frame;
  // assign the frame that will be read from the container
  AVFrame* t_frame{nullptr};
  ThrowOnFfmpegReturnNullptr(t_frame = av_frame_alloc());
  _frame.reset(t_frame);
}
void InputStream::ConfigureCodecContext()
{
  // does nothing by default
}
void InputStream::Open(FrameSink* frameSink) { _output = frameSink->CreateStream(); }
StreamData* InputStream::DiscoverMetaData()
{
  /*metaData = new StreamData();

  AVRational* time_base = &timeBaseCorrectedByTicksPerFrame;
  if (!timeBaseCorrectedByTicksPerFrame.num)
  {
          time_base = &stream->time_base;
  }

  metaData->timeBase.num = time_base->num;
  metaData->timeBase.den = time_base->den;*/

  AVRational overrideFrameRate;
  overrideFrameRate.num = 0;

  AVRational tb = overrideFrameRate.num ? av_inv_q(overrideFrameRate) : _stream->time_base;
  AVRational fr = overrideFrameRate;
  if (!fr.num)
    fr = av_guess_frame_rate(_format, _stream, NULL);

  StreamData* metaData = new StreamData();
  metaData->timeBase = tb;
  metaData->frameRate = fr;

  metaData->type = _codecContext->codec->type;

  return std::move(metaData);
}
void InputStream::DecodePacket(AVPacket* pkt)
{
  int ret;

  /* send the packet with the compressed data to the decoder */
  ThrowOnFfmpegError(ret = avcodec_send_packet(_codecContext.get(), pkt));

  /* read all the output frames (in general there may be any number of them */
  while (ret >= 0) {
    ThrowOnFfmpegError(ret = avcodec_receive_frame(_codecContext.get(), _frame.get()));
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;

    // put default settings from the stream into the frame
    if (!_frame->sample_aspect_ratio.num) {
      _frame->sample_aspect_ratio = _stream->sample_aspect_ratio;
    }

    // the meta data does not exist yet - we figure it out!
    if (_metaData == nullptr) {
      _metaData.reset(DiscoverMetaData());
    }

    // push the frame to the next stage.
    // The time_base is filled in in the codecContext after the first frame is decoded
    // so we can fetch it from there.
    if (_output == nullptr) {
      // No frame sink specified - just release the frame again.
    } else {
      _output->WriteFrame(_frame.get(), _metaData.get());
    }
    ++_nFramesProcessed;
  }
}
int InputStream::GetFramesProcessed() { return _nFramesProcessed; }

void InputStream::Close()
{
  if (_output != nullptr)
    _output->Close();
}

bool InputStream::IsPrimed() { return _output->IsPrimed(); }

float InputStream::CalculateBitRate(AVCodecContext* ctx)
{
  int64_t bit_rate;
  int bits_per_sample;

  switch (ctx->codec_type) {
  case AVMEDIA_TYPE_VIDEO:
  case AVMEDIA_TYPE_DATA:
  case AVMEDIA_TYPE_SUBTITLE:
  case AVMEDIA_TYPE_ATTACHMENT:
    bit_rate = ctx->bit_rate;
    break;
  case AVMEDIA_TYPE_AUDIO:
    bits_per_sample = av_get_bits_per_sample(ctx->codec_id);
    bit_rate = bits_per_sample ? ctx->sample_rate * (int64_t)ctx->channels * bits_per_sample : ctx->bit_rate;
    break;
  default:
    bit_rate = 0;
    break;
  }
  return bit_rate / 1000.0f;
}

} // namespace ffpp
