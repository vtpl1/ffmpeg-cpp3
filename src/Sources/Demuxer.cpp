// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "Demuxer.h"

#include "FFmpegException.h"
#ifndef WIN32
#define _strdup strdup
#endif
namespace ffpp
{
Demuxer::Demuxer(const std::string& fileName, const std::string& input_format,
                 const std::multimap<std::string, std::string>& ffmpeg_options)
    : _fileName(fileName)
{
  for (auto& pair : ffmpeg_options) {
    // std::cout << "Key:\t" << pair.first << "\t\tvalue:\t" << pair.second << std::endl;
    ThrowOnFfmpegError(
        av_dict_set(&_inputFormatAvDictionary, _strdup(pair.first.c_str()), _strdup(pair.second.c_str()), 0));
  }
  if (!input_format.empty()) {
    ThrowOnFfmpegReturnNullptr(_inputFormat = av_find_input_format(input_format.c_str()));
  }
  {
    Timer t;
    ThrowOnFfmpegError(
        avformat_open_input(&_containerContext, _fileName.c_str(), _inputFormat, &_inputFormatAvDictionary));
    // ThrowOnFfmpegError(av_opt_set(_containerContext, "codec:v", "h264", AV_OPT_SEARCH_CHILDREN));
    ThrowOnFfmpegError(avformat_find_stream_info(_containerContext, nullptr));
    _inputStreams.resize(_containerContext->nb_streams);
    // av_dump_format(_containerContext, )
    ThrowOnFfmpegReturnNullptr(_pkt = av_packet_alloc());
    av_init_packet(_pkt);
    _pkt->data = NULL;
    _pkt->size = 0;
  }
}
Demuxer::~Demuxer() { Stop(); }
void Demuxer::SignalToStop() { _do_shutdown = true; }
void Demuxer::Stop()
{
  if (_is_already_shutting_down)
    return;
  _is_already_shutting_down = true;
  SignalToStop();
  if (_inputFormatAvDictionary) {
    av_dict_free(&_inputFormatAvDictionary);
    _inputFormatAvDictionary = nullptr;
  }
  _inputStreams.clear();
  if (_containerContext) {
    avformat_close_input(&_containerContext);
    _containerContext = nullptr;
  }
  if (_pkt != nullptr) {
    av_packet_free(&_pkt);
    _pkt = nullptr;
  }
}
bool Demuxer::IsDone() { return _do_shutdown_composite(); }
void Demuxer::Step() {}
void Demuxer::PreparePipeline()
{
  do {
  } while (!IsDone());
}
} // namespace ffpp
