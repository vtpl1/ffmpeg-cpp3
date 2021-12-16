#include <iostream>
#include "ffmpegcpp.h"
#include <memory>
using namespace ffmpegcpp;


int main()
{
    // This example will take a raw audio file and encode it into a Matroska container.
    try
    {
//D:\WorkFiles\py_vtpl_agent_2022\lib\fflivehlsstreamer.exe -i rtsp://admin:AdmiN1234@192.168.0.72:554/live1s1.sdp -o rtmp://192.168.2.21:9105
        // Load both audio and video from a container
        std::unique_ptr<Demuxer> videoContainer(new Demuxer("rtsp://admin:AdmiN1234@192.168.0.72:554/live1s1.sdp"));

        // Prepare the pipeline. We want to call this before the rest of the loop
        // to ensure that the muxer will be fully ready to receive data from
        // multiple sources.
        videoContainer->PreparePipeline();

        // Pump the audio and video fully through.
        // To avoid big buffers, we interleave these calls so that the container
        // can be written to disk efficiently.
        while ( (!videoContainer->IsDone()))
        {
            //  timestamps issue ?
            if (!videoContainer->IsDone()) videoContainer->Step();
        }

        // Save everything to disk by closing the muxer.
    }
    catch (FFmpegException e)
    {
        std::cerr << "Exception caught! " << e.what() << "\n";
        throw e;
    }
    std::cout << "Encoding complete!" << "\n";
}
