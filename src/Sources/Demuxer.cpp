// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "Demuxer.h"

#include "AudioInputStream.h"
#include "CodecDeducer.h"
#include "FFmpegException.h"
#include "VideoInputStream.h"
#ifndef WIN32
#define _strdup strdup
#endif
namespace ffpp
{
Demuxer::Demuxer(const std::string& fileName, const std::string& input_format,
                 const std::multimap<std::string, std::string>& ffmpeg_options)
    : _fileName(fileName)
{
  AVDictionary* t_inputFormatAvDictionary{nullptr};
  try {
    for (auto& pair : ffmpeg_options) {
      // std::cout << "Key:\t" << pair.first << "\t\tvalue:\t" << pair.second << std::endl;
      ThrowOnFfmpegError(
          av_dict_set(&t_inputFormatAvDictionary, _strdup(pair.first.c_str()), _strdup(pair.second.c_str()), 0));
    }

  } catch (const ffpp::FFmpegException& e) {
    if (t_inputFormatAvDictionary) {
      av_dict_free(&t_inputFormatAvDictionary);
      t_inputFormatAvDictionary = nullptr;
    }
    throw e;
  }

  if (!input_format.empty()) {
    ThrowOnFfmpegReturnNullptr(_inputFormat = av_find_input_format(input_format.c_str()));
  }
  AVFormatContext* t_containerContext{nullptr};
  ThrowOnFfmpegError(
      avformat_open_input(&t_containerContext, _fileName.c_str(), _inputFormat, &t_inputFormatAvDictionary));
  _containerContext.reset(t_containerContext);
  if (t_inputFormatAvDictionary) {
    av_dict_free(&t_inputFormatAvDictionary);
    t_inputFormatAvDictionary = nullptr;
  }
  // ThrowOnFfmpegError(av_opt_set(_containerContext, "codec:v", "h264", AV_OPT_SEARCH_CHILDREN));
  ThrowOnFfmpegError(avformat_find_stream_info(_containerContext.get(), nullptr));
  _inputStreams.resize(_containerContext->nb_streams);
  AVPacket* t_pkt;
  ThrowOnFfmpegReturnNullptr(t_pkt = av_packet_alloc());
  _pkt.reset(t_pkt);
}

InputStream* Demuxer::GetInputStream(int streamIndex)
{
  // already exists
  if (_inputStreams[streamIndex] != nullptr)
    return _inputStreams[streamIndex].get();

  // The stream doesn't exist but we already processed all our frames, so it makes no sense
  // to add it anymore.
  if (IsDone())
    return nullptr;

  AVStream* stream = _containerContext->streams[streamIndex];
  AVCodec* codec = CodecDeducer::DeduceDecoder(stream->codecpar->codec_id);
  if (codec == nullptr)
    return nullptr; // no codec found - we can't really do anything with this stream!
  switch (codec->type) {
  case AVMEDIA_TYPE_VIDEO:
    _inputStreams[streamIndex].reset(new VideoInputStream(_containerContext.get(), stream));
    break;
  case AVMEDIA_TYPE_AUDIO:
    _inputStreams[streamIndex].reset(new AudioInputStream(_containerContext.get(), stream));
    break;
  }

  // return the created stream
  return _inputStreams[streamIndex].get();
}

InputStream* Demuxer::GetInputStreamById(int streamId)
{
  // map the stream id to an index by going over all the streams and comparing the id
  for (int i = 0; i < _containerContext->nb_streams; ++i) {
    AVStream* stream = _containerContext->streams[i];
    if (stream->id == streamId)
      return GetInputStream(i);
  }

  // no match found
  return nullptr;
}
int Demuxer::GetFrameCount(int streamId)
{
  // Make sure all streams exist, so we can query them later.
  for (int i = 0; i < _containerContext->nb_streams; ++i) {
    GetInputStream(i);
  }

  // Process the entire container so we can know how many frames are in each
  while (!IsDone()) {
    Step();
  }

  // Return the right stream's frame count.
  return GetInputStreamById(streamId)->GetFramesProcessed();
}
std::string Demuxer::GetFileName() { return _fileName; }
Demuxer::~Demuxer() { Stop(); }
void Demuxer::SignalToStop() { _do_shutdown = true; }
void Demuxer::Stop()
{
  if (_is_already_shutting_down)
    return;
  _is_already_shutting_down = true;
  SignalToStop();

  _inputStreams.clear();
}
void Demuxer::DecodeBestAudioStream(FrameSink* frameSink)
{
  int ret = av_find_best_stream(_containerContext.get(), AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
  if (ret < 0) {
    throw FFmpegException("Could not find " + std::string(av_get_media_type_string(AVMEDIA_TYPE_AUDIO)) +
                              " stream in input file " + _fileName,
                          ret);
  }
  int streamIndex = ret;
  return DecodeAudioStream(streamIndex, frameSink);
}
void Demuxer::DecodeAudioStream(int streamIndex, FrameSink* frameSink)
{
  // each input stream can only be used once
  if (_inputStreams[streamIndex] != nullptr) {
    throw FFmpegException(
        "That stream is already tied to a frame sink, you cannot process the same stream multiple times");
  }

  // create the stream
  InputStream* inputStream = GetInputStream(streamIndex);
  inputStream->Open(frameSink);

  // remember and return
  _inputStreams[streamIndex].reset(inputStream);
}
void Demuxer::DecodeBestVideoStream(FrameSink* frameSink)
{
  int ret = av_find_best_stream(_containerContext.get(), AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (ret < 0) {
    throw FFmpegException("Could not find " + std::string(av_get_media_type_string(AVMEDIA_TYPE_VIDEO)) +
                              " stream in input file " + _fileName,
                          ret);
  }
  int streamIndex = ret;
  return DecodeVideoStream(streamIndex, frameSink);
}
void Demuxer::DecodeVideoStream(int streamIndex, FrameSink* frameSink)
{
  // each input stream can only be used once
  if (_inputStreams[streamIndex] != nullptr) {
    throw FFmpegException(
        "That stream is already tied to a frame sink, you cannot process the same stream multiple times");
  }

  // create the stream
  InputStream* inputStream = GetInputStream(streamIndex);
  inputStream->Open(frameSink);

  // remember and return
  _inputStreams[streamIndex].reset(inputStream);
}

bool Demuxer::IsDone() { return _do_shutdown_composite(); }
void Demuxer::Step()
{
  // read frames from the file
  int ret = av_read_frame(_containerContext.get(), _pkt.get());

  // EOF
  if (ret == AVERROR_EOF) {
    _pkt->data = NULL;
    _pkt->size = 0;
    for (int i = 0; i < _containerContext->nb_streams; ++i) {
      InputStream* stream = _inputStreams[i].get();
      if (stream != nullptr) {
        _pkt->stream_index = i;
        DecodePacket();
        stream->Close();
      }
    }

    _is_internal_shutdown = true;
    return;
  }

  // not ready yet
  if (ret == AVERROR(EAGAIN))
    return;

  // error
  if (ret < 0) {
    throw FFmpegException("Error during demuxing", ret);
  }

  // decode the finished packet
  DecodePacket();
}

void Demuxer::DecodePacket()
{
  int streamIndex = _pkt->stream_index;
  InputStream* inputStream = _inputStreams[streamIndex].get();

  if (inputStream != nullptr) {
    inputStream->DecodePacket(_pkt.get());
  }

  // We need to unref the packet here because packets might pass by here
  // that don't have a stream attached to them. We want to dismiss them!
  av_packet_unref(_pkt.get());
}

ContainerInfo Demuxer::GetInfo()
{

  ContainerInfo info;

  // general data
  // the duration is calculated like this... why?
  int64_t duration = _containerContext->duration + (_containerContext->duration <= INT64_MAX - 5000 ? 5000 : 0);
  info.durationInMicroSeconds = duration;
  info.durationInSeconds = (float)info.durationInMicroSeconds / AV_TIME_BASE;
  info.start = (float)_containerContext->start_time / AV_TIME_BASE;
  info.bitRate = _containerContext->bit_rate;
  info.format = _containerContext->iformat;

  // go over all streams and get their info
  for (int i = 0; i < _containerContext->nb_streams; ++i) {
    InputStream* stream = GetInputStream(i);
    if (stream == nullptr)
      continue; // no valid stream
    stream->AddStreamInfo(&info);
  }

  return info;
}

void Demuxer::PreparePipeline()
{
  bool allPrimed = false;
  do {
    Step();

    // see if all input streams are primed
    allPrimed = true;
    for (int i = 0; i < _containerContext->nb_streams; ++i) {
      InputStream* stream = _inputStreams[i].get();
      if (stream != nullptr) {
        if (!stream->IsPrimed())
          allPrimed = false;
      }
    }

  } while (!allPrimed && !IsDone());
}
} // namespace ffpp
