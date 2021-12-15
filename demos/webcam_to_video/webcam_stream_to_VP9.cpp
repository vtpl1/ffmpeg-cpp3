
/*
 * File webcam_stream_to_VP9.cpp
 * Copyright Eric Bachard / 2020 05 08
 * This document is under GPL v3 license
 * see : http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <iostream>

#include <ffmpegcpp.h>

using namespace ffmpegcpp;
using std::string;
using std::cerr;

void record_VP9()
{

    // Create a muxer that will output the video as MKV.
    Muxer* muxer = new Muxer("../videos/output_VP9.avi");

    // These are example video and audio sources used below.
    const char* videoDevice = "/dev/video0";

    const char* audioDevice = "hwplug:1,0"; // testing
    //const char* audioDevice = "hw:1,0"; // first webcam
    //const char* audioDevice = "default";

    const char * audioDeviceFormat = "alsa";
    int audioSampleRate = 44100;
    int audioChannels   = 2;

    AudioCodec * audioCodec = new AudioCodec(AV_CODEC_ID_AAC);

    // audioCodec->Open(40000, AV_SAMPLE_FMT_FLTP, audioSampleRate);
    /*
        AAC profile should be : 
        Name = "AAC_Profile_128k",
        Bitrate = 128000,
        Rate = 48000
    */

    //  OUTPUT CODEC, linked to the encoder ...
    VP9Codec  * lcodec = new VP9Codec();

    /*
        -speed 6 	Speed 5 to 8 should be used for live / real-time encoding. Lower numbers (5 or 6) are higher quality but require more CPU power. Higher numbers (7 or 8) will be lower quality but more manageable for lower latency use cases and also for lower CPU power devices such as mobile.
        -tile-columns 4 	Tiling splits the video into rectangular regions, which allows multi-threading for encoding and decoding. The number of tiles is always a power of two. 0 = 1 tile, 1 = 2, 2 = 4, 3 = 8, 4 = 16, 5 = 32.
        -frame-parallel 1 	Enable parallel decodability features.
        -threads 8 	Maximum number of threads to use.
        -static-thresh 0 	Motion detection threshold.
        -max-intra-rate 300 	Maximum i-Frame bitrate (pct)
        -deadline realtime 	Alternative (legacy) version of -quality realtime
        -lag-in-frames 0 	Maximum number of frames to lag
        -qmin 4 -qmax 48 	Minimum and maximum values for the quantizer. The values here are merely a suggestion and adjusting this will help increase/decrease video quality at the expense of compression efficiency.
        -row-mt 1 	Enable row-multithreading. Allows use of up to 2x thread as tile columns. 0 = off, 1 = on.
        -error-resilient 1 	Enable error resiliency features.
    */

    lcodec->SetGenericOption("speed", "6");
    lcodec->SetGenericOption("threads", "4");
    lcodec->SetGenericOption("row-mt", "1");

    lcodec->SetGenericOption("qmin", "12");
    lcodec->SetGenericOption("qmax", "34");


    // https://slhck.info/video/2017/03/01/rate-control.html
    lcodec->SetGenericOption("vbv-maxrate", "1000");  // 1M
    lcodec->SetGenericOption("vbv-bufsize", "2000"); // 2M 

    lcodec->SetGenericOption("movflags", "+faststart");

    // Configure the codec to not do compression, to use multiple CPU's and to go as fast as possible.
    lcodec->SetLossless(false);
    lcodec->SetCpuUsed(5);
    lcodec->SetDeadline("realtime"); // other possibility : "good"
    lcodec->SetGenericOption("low_power", "true");

    //   0       <------   18  <-------  23 -----> 28 ------> 51
    //  lossless       better                 worse          worst
    lcodec->SetCrf(23);  // 28 is default for VP9, while 23 is prefered for H264

    // create the different components that make this come together
    try
    {
        int width  = 1280;
        int height = 720;
        int fps = 24;

        // FIXME : timing is not precise, and probably wrong
        AVRational frameRate = { 24, 1 };

        // All seem to work
        //AVPixelFormat input_pix_fmt= AV_PIX_FMT_NV12;
        //AVPixelFormat outputPixFormat= AV_PIX_FMT_RGBA;
        AVPixelFormat outputPixFormat= AV_PIX_FMT_YUV420P;

        VideoEncoder *   videoEncoder = new VideoEncoder(lcodec, muxer, frameRate, outputPixFormat);
        AudioEncoder *   audioEncoder = new AudioEncoder(audioCodec, muxer);

        RawAudioFileSource* audioFile = new RawAudioFileSource(audioDevice, audioDeviceFormat, audioSampleRate, audioChannels, audioEncoder);
        Demuxer *           demuxer   = new Demuxer(videoDevice, width, height, fps);

        // FIXME : how to sync videoand audio ?
        //audioFile->DecodeBestAudioStream(audioEncoder);
        demuxer->DecodeBestVideoStream(videoEncoder);

        audioFile->PreparePipeline();
        demuxer->PreparePipeline();

        int frameNumber = 0;

        while (!demuxer->IsDone() && (frameNumber < 100))
        {
            frameNumber = demuxer->GetFrameCount(demuxer->getVideoStreamIndx());
            std::cerr << "frame number : " << frameNumber << "\n";

            if (frameNumber > 100)
                demuxer->Stop();

            demuxer->Step();
            audioFile->Step();

        }

        // close the muxer and save the file to disk
        muxer->Close();

        if (lcodec != nullptr)
            delete lcodec;

        if (videoEncoder != nullptr)
            delete videoEncoder;


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
    std::cout << "Video + sound encoding complete!" << "\n";

    return 0;
}