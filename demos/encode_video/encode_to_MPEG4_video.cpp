#include <iostream>
#include "ffmpegcpp.h"

using std::cerr;
using std::cout;

using namespace ffmpegcpp;


int main()
{
    avdevice_register_all();

    // This example will take a raw audio file and encode it into as MP3.
    try
    {
        // Create a muxer that will output the video as MKV.
        Muxer* muxer = new Muxer("output_MPEG4.mpg");

        MPEG4Codec* codec = new MPEG4Codec();

        // Default is : 
        // profile = main, crf = 10, preset = medium (default), tune = film or animation
        // FIXME : the buffer size seems to NOT being set ?
        codec->SetGenericOption("b", "4M");
        codec->SetGenericOption("bit_rate", "2M");
        codec->SetProfile("high10"); // baseline, main, high, high10, high422
        codec->SetTune("film");  // film animation grain stillimage psnr ssim fastdecode zerolatency
        codec->SetPreset("veryslow"); // fast, medium, slow slower, veryslow placebo
        codec->SetCrf(23);

        // Create an encoder that will encode the raw audio data as MP3. Tie it to the muxer so it will be written to the file.
        VideoEncoder* encoder = new VideoEncoder(codec, muxer);

        RawVideoFileSource* videoFile = new RawVideoFileSource("../../samples/big_buck_bunny.mp4", encoder);

        // Prepare the output pipeline. This will push a small amount of frames to the file sink until it IsPrimed returns true.
        videoFile->PreparePipeline();

        // Push all the remaining frames through.
        while (!videoFile->IsDone())
        {
            videoFile->Step();
        }
        // Save everything to disk by closing the muxer.
        muxer->Close();
    }

    catch (FFmpegException e)
    {
        cerr << "Exception caught!" << "\n";
        cerr << e.what() << "\n";
        throw e;
    }
    cout << "Encoding complete!" << "\n";
}
