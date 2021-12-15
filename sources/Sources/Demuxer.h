#pragma once

#include "ffmpeg.h"

#include "Demuxing/AudioInputStream.h"
#include "Demuxing/VideoInputStream.h"
#include "Demuxing/InputStream.h"
#include "Sources/InputSource.h"
#include "Info/ContainerInfo.h"

#include <string>

namespace ffmpegcpp
{
    class Demuxer : public InputSource
    {
    public:

        Demuxer(const std::string & fileName);
        Demuxer(const std::string & fileName, int width, int height, int framerate);
        Demuxer(const std::string & fileName, AVInputFormat* inputFormat, AVDictionary *inputFormatOptions);
        Demuxer(const std::string & fileName, const std::string & inputFormat, int sampleRate, int channels, AudioFrameSink * p_audio_frameSink);
        Demuxer(const std::string & fileName, AVInputFormat* inputFormat, AVDictionary *inputFormatOptions, AVFormatContext * aContainerContext);
        ~Demuxer();

        void DecodeBestAudioStream(FrameSink* frameSink);
        void DecodeBestVideoStream(FrameSink* frameSink);

        void DecodeAudioStream(int streamId, FrameSink* frameSink);
        void DecodeVideoStream(int streamId, FrameSink* frameSink);

        virtual void PreparePipeline();
        virtual bool IsDone();
        virtual void Step();

        void Stop() { done = true;}

        ContainerInfo GetInfo();
        int GetFrameCount(int streamId);

        int  getVideoStreamIndx() { return m_VideoStreamIndx ;}

        void setAudioStreamDevice();
        void setVideoStreamDevice();

        const char* GetFileName();

    private:

        bool done = false;

        const char* m_fileName;

        int m_width;
        int m_height;
        int m_framerate;

        int m_VideoStreamIndx = 0;

        int frameCount = 0;

        AVFormatContext* pAVFormatContextIn;

        AVDictionary * options;
        AVCodec * pAVCodec;
        AVInputFormat * m_inputFormat;

        int setStreamIndexAndCopyParameters(AVMediaType aMediaType);

        const char* m_audio_device;

        AVDictionary   *  m_audio_opts;
        AVInputFormat  *  m_file_iformat;
        AudioFrameSink *  m_audio_frameSink;
        int               m_sampleRate;
        int               m_channels;
        int               m_AudioStreamIndx = 0;

        InputStream* GetInputStream(int index);
        InputStream* GetInputStreamById(int streamId);

        //std::vector<StreamInfo> GetStreamInfo(AVMediaType mediaType);
        //StreamInfo CreateInfo(int streamIndex, AVStream* stream, AVCodec* codec);

        InputStream** inputStreams = nullptr;

        AVFormatContext* containerContext = nullptr;
        AVPacket* pkt = nullptr;

        void DecodePacket();

        void CleanUp();
    };
}
