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
        Muxer* muxer = new Muxer("output_MPEG2.mpg");

        // Create a MPEG2 codec that will encode the raw data.
        VideoCodec * codec = new VideoCodec("mpeg2video");
        //VideoCodec * codec = new VideoCodec(AV_CODEC_ID_MJPEG);
        // Set the global quality of the video encoding. This maps to the command line
        // parameter -qscale and must be within range [0,31].
        codec->SetQualityScale(23);
        // Set the bit rate option -b:v 2M
        codec->SetGenericOption("b", "2M");

        // Create an encoder that will encode the raw audio data as MP3. Tie it to the muxer so it will be written to the file.
        VideoEncoder* encoder = new VideoEncoder(codec, muxer);

        //RawVideoFileSource* videoFile = new RawVideoFileSource(videoContainer->GetFileName(), encoder);
        RawVideoFileSource* videoFile = new RawVideoFileSource("../../samples/big_buck_bunny.mp4", encoder);

        // Prepare the output pipeline. This will push a small amount of frames to the file sink until it IsPrimed returns true.
        videoFile->PreparePipeline();
        std::cerr << "videoFile->PreparePipeline() done ..." << "\n";

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
