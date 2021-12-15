
/*
 * File remux_webcam.cpp
 * Copyright Eric Bachard / 2020 05 08
 * This document is under GPL v3 license
 * see : http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <iostream>
#include <chrono>
#include <ffmpegcpp.h>
#include <thread>         // std::thread
#include <fstream>        // std::remove

static bool bRecording = false;

using namespace ffmpegcpp;
using std::string;
using std::cerr;

//#ifdef ALSA_BUFFER_SIZE_MAX
#undef ALSA_BUFFER_SIZE_MAX
#define ALSA_BUFFER_SIZE_MAX  524288

const char * audio_file = "../videos/audio.mp4";  // aac (s32le ?)
const char * video_file = "../videos/video_H264.mp4";  // h264


void record_Audio()
{
    const char * audioDevice = "pulse";
    //const char * audioDevice = "hw:1,0";
    const char * audioDeviceFormat = "alsa";

    Muxer* Amuxer  = new Muxer(audio_file);
    int audioSampleRate = 44100;
    int audioChannels   = 2;

    AudioCodec         *   audioCodec = new AudioCodec(AV_CODEC_ID_AAC);
    AudioEncoder       *   audioEncoder = new AudioEncoder(audioCodec, Amuxer);

    RawAudioFileSource *   audioFile = new RawAudioFileSource( audioDevice,
                                                            audioDeviceFormat,
                                                            audioSampleRate,
                                                            audioChannels,
                                                            audioEncoder);
    audioFile->PreparePipeline();
    audioFile->Step();

    while (!audioFile->IsDone())
    {
        audioFile->Step();

        if (bRecording == false)
            audioFile->Stop();
    }

    Amuxer->Close();

    if (audioEncoder != nullptr)
        delete audioEncoder;

    delete Amuxer;
}


void record_Video()
{
    int width  = 1280; // 1920;
    int height = 720;  // 1080;
    int fps = 24;  // Logitech prefered fps value

    AVRational frameRate = { 24, 1 };

    // These are example video and audio sources used below.
    const char * videoDevice = "/dev/video0";
    AVPixelFormat outputPixFormat= AV_PIX_FMT_NV12;

    Muxer* Vmuxer = new Muxer(video_file);
    H264Codec  * vcodec = new H264Codec();

    VideoEncoder * videoEncoder = new VideoEncoder(vcodec, Vmuxer, frameRate, outputPixFormat);

    Demuxer * demuxer  = new Demuxer(videoDevice, width, height, fps);
    demuxer->DecodeBestVideoStream(videoEncoder);
    demuxer->PreparePipeline();

    while (!demuxer->IsDone())
    {
        demuxer->Step();

        if (bRecording == false)
        {
            demuxer->Stop();
        }
    }

    // close the first muxers and save separately audio and video files to disk
    Vmuxer->Close();

    if (videoEncoder != nullptr)
        delete videoEncoder;

    delete Vmuxer;
}


void create_final_Video()
{
    const char * final_file = "../videos/final_video.mp4";  // h264 + aac (or vp9 + aac)

    Muxer* AVmuxer = new Muxer(final_file);

    AudioCodec * faudioCodec = new AudioCodec(AV_CODEC_ID_AAC);
    H264Codec  * fvcodec = new H264Codec();

    try
    {
        // Create encoders for both
        VideoEncoder* fvideoEncoder = new VideoEncoder(fvcodec, AVmuxer);
        AudioEncoder* faudioEncoder = new AudioEncoder(faudioCodec, AVmuxer);

        // Load both audio and video from a container
        Demuxer* videoContainer = new Demuxer(video_file);
        Demuxer* audioContainer = new Demuxer(audio_file);

        // Tie the best stream from each container to the output
        videoContainer->DecodeBestVideoStream(fvideoEncoder);
        audioContainer->DecodeBestAudioStream(faudioEncoder);

        // Prepare the pipeline. We want to call this before the rest of the loop
        // to ensure that the muxer will be fully ready to receive data from
        // multiple sources.
        videoContainer->PreparePipeline();
        audioContainer->PreparePipeline();

        // Pump the audio and video fully through.
        // To avoid big buffers, we interleave these calls so that the container
        // can be written to disk efficiently.
        while ( (!videoContainer->IsDone()) || (!audioContainer->IsDone()))
        {
            if (!videoContainer->IsDone())
                videoContainer->Step();

            if (!audioContainer->IsDone())
                audioContainer->Step();
        }

        // Save everything to disk by closing the muxer.
        AVmuxer->Close();
    }
    catch (FFmpegException e)
    {
        cerr << e.what() << "\n";
        throw e;
    }

    delete AVmuxer;
}


int main(void)
{
    avdevice_register_all();
    //avformat_network_init(); // future use
    bRecording = true;

    std::thread first (record_Audio);
    std::thread second (record_Video);

    auto start = std::chrono::steady_clock::now();
    auto current_time = std::chrono::steady_clock::now();
         std::chrono::duration<double> elapsed_seconds = current_time - start;
    do
    {
        current_time = std::chrono::steady_clock::now();
        elapsed_seconds = current_time - start;

    } while ((elapsed_seconds.count()) < (60));

    bRecording = false;

    first.join();
    second.join();

    create_final_Video();

    std::cout << "Encoding complete!" << "\n";

#define TEST
#ifdef TEST
    std::remove(audio_file);
    std::remove(video_file);

     bool failed = (std::ifstream(audio_file) || std::ifstream(video_file));

    if(failed)
    {
        std::perror("Error opening deleted file");
        return 1;
    }
#endif

    return 0;
}