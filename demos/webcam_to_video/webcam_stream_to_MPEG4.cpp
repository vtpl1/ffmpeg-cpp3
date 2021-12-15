
/*
 * file : webcam_stream_to_MPEG4.cpp
 * Copyright Eric Bachard / 2020 05 08
 * This document is under GPL v3 license
 * see : http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <iostream>
#include <ffmpegcpp.h>

using namespace ffmpegcpp;
using std::string;
using std::cerr;

void record_MPEG4()
{
    avdevice_register_all();

    // These are example video and audio sources used below.
    const char* videoDevice = "/dev/video0";

    Muxer * muxer = new Muxer("../videos/output_MPEG4.avi");

    //  OUTPUT CODEC, linked to the encoder ...

    MPEG4Codec* mcodec = new MPEG4Codec();

    // Default is : 
    // profile = main, crf = 10, preset = medium (default), tune = film or animation
    // FIXME : the buffer size seems to NOT being set ?
    mcodec->SetGenericOption("b", "4M");
    mcodec->SetGenericOption("bit_rate", "2M");
    mcodec->SetProfile("high10"); // baseline, main, high, high10, high422
    mcodec->SetTune("film");  // film animation grain stillimage psnr ssim fastdecode zerolatency
    mcodec->SetPreset("veryslow"); // fast, medium, slow slower, veryslow placebo
    mcodec->SetCrf(23);

    // create the different components that make this come together
    try
    {

        ///AVPixelFormat pix_format = AV_PIX_FMT_YUVJ420P; // = V4L2_PIX_FMT_MJPEG
        //AVPixelFormat pix_format = AV_PIX_FMT_NV12; // = V4L2_PIX_FMT_MJPEG
        int width  = 1280;
        int height = 720;
        int fps = 30;

        // FIXME : probably plain wrong (lot of issues with timings)
        AVRational frameRate = { 30, 1 };

        AVPixelFormat outputPixFormat= AV_PIX_FMT_YUV420P;

        VideoEncoder * videoEncoder = new VideoEncoder(mcodec, muxer, frameRate, outputPixFormat);

        Demuxer * demuxer = new Demuxer(videoDevice, width, height, fps);
        demuxer->DecodeBestVideoStream(videoEncoder);
        demuxer->PreparePipeline();

        int frameNumber = 0;

        while (!demuxer->IsDone() && (frameNumber < 100))
        {
            frameNumber = demuxer->GetFrameCount(demuxer->getVideoStreamIndx());
            std::cerr << "frame number : " << frameNumber << "\n";

            if (frameNumber > 100)
                demuxer->Stop();

            demuxer->Step();
        }

        // close the muxer and save the file to disk
        muxer->Close();

        if (mcodec != nullptr)
            delete mcodec;

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
	record_MPEG4();

	std::cout << "Encoding complete!" << "\n";

	return 0;
}