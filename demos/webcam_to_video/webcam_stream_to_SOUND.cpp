
/*
 * File webcam_stream_to_SOUND.cpp
 * This example code helps to record webcam sound, and create a sound file (audio Codec is AAC currently, but can be modified)
 * Copyright Eric Bachard / 2020 05 08
 * This document is under GPL v3 license
 * see : http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <iostream>
#include <chrono>

#include <ffmpegcpp.h>

using namespace ffmpegcpp;
using std::string;
using std::cerr;

void record_VP9()
{
    // Create a muxer that will output the video as MKV.
    Muxer* muxer = new Muxer("../videos/output_VP9.mkv");

    // These are example video and audio sources used below.
    // const char* videoDevice = "/dev/video0";
    //const char* audioDevice = "hw:1,0";  // first webcam
    const char* audioDevice = "pulse";  // first webcam
    //const char* audioDevice = "default"; // the current selected source

    const char * audioDeviceFormat = "alsa";
    int audioSampleRate = 44100;
    int audioChannels   = 2;

    AudioCodec * audioCodec = new AudioCodec(AV_CODEC_ID_AAC);

    // create the different components that make this come together
    try
    {
        AudioEncoder *   audioEncoder = new AudioEncoder(audioCodec, muxer);

        RawAudioFileSource* audioFile = new RawAudioFileSource( audioDevice,
                                                                audioDeviceFormat,
                                                                audioSampleRate,
                                                                audioChannels,
                                                                audioEncoder);
        audioFile->PreparePipeline();

        // https://en.cppreference.com/w/cpp/chrono
        auto start = std::chrono::steady_clock::now();

        while (!audioFile->IsDone())
        {
            // of course, this will replaced by the start / stop button
            // waiting, it helps a lot to see the difficulties ffmpegs has to solve with timings
            auto current_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_seconds = current_time - start;

            std::cout << "elapsed time  :   "<< elapsed_seconds.count() << '\n';

            audioFile->Step();

            if ((elapsed_seconds.count()) > (20)) // 20s. If 1min it gives 1440 video frames exactly at 24 fps
                audioFile->Stop();
        }

        // close the muxer and save the file to disk
        muxer->Close();

        if (audioFile != nullptr)
            delete audioFile;

        delete muxer;
    }
    catch (FFmpegException e)
    {
        cerr << e.what() << "\n";
        throw e;
    }
}

int main(void)
{
    avdevice_register_all();
    record_VP9();
    std::cout << "Sound encoding complete!" << "\n";

    return 0;
}