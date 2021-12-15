#include <iostream>
#include "ffmpegcpp.h"

using namespace ffmpegcpp;


int main()
{
    // This example will take a raw audio file and encode it into a Matroska container.
    try
    {
        // Create a muxer that will output the video as MKV.
        Muxer* muxer = new Muxer("output.mkv");

#define VP9_VIDEO

#if defined( VP9_VIDEO )
        // Create a codec that will encode video as VP9
        VP9Codec* videoCodec = new VP9Codec();

        // Configure the codec to not do compression, to use multiple CPU's and to go as fast as possible.
        videoCodec->SetLossless(false);
        videoCodec->SetCpuUsed(5);
        videoCodec->SetDeadline("realtime");
        // videoCodec->SetCrf(23);

#elif defined( H264_VIDEO )

        // Default is : 
        // profile = main, crf = 10, preset = medium (default), tune = film or animation

        H264Codec* videoCodec = new H264Codec();
        // H264_VAAPICodec* codec = new H264_VAAPICodec();

        // FIXME : the buffer size seems to NOT being set ?
        videoCodec->SetGenericOption("b", "4M");
        //codec->SetGenericOption("low_power", true);
        videoCodec->SetProfile("high10"); // baseline, main, high, high10, high422
        videoCodec->SetTune("film");  // film animation grain stillimage psnr ssim fastdecode zerolatency
        videoCodec->SetPreset("veryslow"); // fast, medium, slow slower, veryslow placebo

        // https://slhck.info/video/2017/03/01/rate-control.html
        //https://slhck.info/video/2017/02/24/crf-guide.html
        // about crf (very important !!
        //
        //   0       <------   18  <-------  23 -----> 28 ------> 51
        //  lossless       better                 worse          worst
        videoCodec->SetCrf(23);

#endif
        // Create a codec that will encode audio as AAC
        AudioCodec* audioCodec = new AudioCodec(AV_CODEC_ID_AAC);

        // Create encoders for both
        VideoEncoder* videoEncoder = new VideoEncoder(videoCodec, muxer);
        AudioEncoder* audioEncoder = new AudioEncoder(audioCodec, muxer);

        // Load both audio and video from a container
        Demuxer* videoContainer = new Demuxer("../../samples/big_buck_bunny.mp4");
        Demuxer* audioContainer = new Demuxer("../../samples/DesiJourney.wav");

        // Tie the best stream from each container to the output
        videoContainer->DecodeBestVideoStream(videoEncoder);
        audioContainer->DecodeBestAudioStream(audioEncoder);

        // Prepare the pipeline. We want to call this before the rest of the loop
        // to ensure that the muxer will be fully ready to receive data from
        // multiple sources.
        videoContainer->PreparePipeline();
        audioContainer->PreparePipeline();

        // Pump the audio and video fully through.
        // To avoid big buffers, we interleave these calls so that the container
        // can be written to disk efficiently.
        while ( (!videoContainer->IsDone()) && (!audioContainer->IsDone()))
        {
            //  timestamps issue ?
            if (!videoContainer->IsDone()) videoContainer->Step();
            if (!audioContainer->IsDone()) audioContainer->Step();
        }

        // Save everything to disk by closing the muxer.
        muxer->Close();
    }
    catch (FFmpegException e)
    {
        std::cerr << "Exception caught!" << "\n";
        throw e;
    }
    std::cout << "Encoding complete!" << "\n";
}
