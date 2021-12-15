
#include <iostream>

#include "ffmpegcpp.h"

using namespace std;
using namespace ffmpegcpp;

int main()
{
	// This example will apply some filters to a video and write it back.
	try
	{
		// Create a muxer that will output the video as MKV.
		Muxer* muxer = new Muxer("filtered_video.mp4");

		// Create a MPEG2 codec that will encode the raw data.
//		VideoCodec* vcodec = new VideoCodec(AV_CODEC_ID_MPEG2VIDEO);

		VP9Codec* vcodec = new VP9Codec();
//		H264_VAAPICodec* vcodec = new H264_VAAPICodec();

		AudioCodec* acodec = new AudioCodec(AV_CODEC_ID_AAC);

		// Set the global quality of the video encoding. This maps to the command line
		// parameter -qscale and must be within range [0,31].
		vcodec->SetQualityScale(20);

		vcodec->SetGlobalQuality(100);
// hw bazaar
		vcodec->InitHardwareDevice("vaapi=intel:/dev/dri/renderD128");
		vcodec->SetHardwareAccelAPI("vaapi");
		vcodec->SetVAAPIDevice("/dev/dri/renderD128"); // can be  0: , /dev/dri/renderD128
		vcodec->SetHardwareAccelOutputFormat("vaapi");  //can be vaapi, yuv420p
		vcodec->SetHardwareAccelDevice("intel");
		vcodec->SetFilterHardwareDevice("intel");
		// Create an encoder that will encode the raw audio data as MP3.
		// Tie it to the muxer so it will be written to the file.
		VideoEncoder* vEncoder = new VideoEncoder(vcodec, muxer);

		AudioEncoder* aEncoder = new AudioEncoder(acodec, muxer);

		// Create a video filter and do some funny stuff with the video data.
//		Filter* filter = new Filter("format=nv12|vaapi,hwupload,vaapi_device:/dev/dri/renderD128,scale_vaapi=w=1280:h=720:format=yuv420p,hwdownload,transpose=cclock,vignette", vEncoder);
//		Filter* filter = new Filter("format=nv12|vaapi,hwupload,scale_vaapi=w=1280:h=720,transpose_vaapi=cclock", vEncoder);
//		Filter* filter = new Filter("format=nv12|vaapi,hwupload,/dev/dri/renderD128,scale_vaapi=w=1280:h=720,transpose=cclock,vignette", vEncoder);
		Filter* filter = new Filter("scale=640:250,transpose=cclock,vignette", vEncoder);

		// Load a video from a container and send it to the filter first.
		Demuxer* demuxer = new Demuxer("../../samples/big_buck_bunny.mp4");
                Demuxer* audioDemuxer = new Demuxer("../../samples/big_buck_bunny.mp4");

		demuxer->DecodeBestVideoStream(filter);
                audioDemuxer->DecodeBestAudioStream(aEncoder);

		// Prepare the output pipeline. This will push a small amount of frames to the file sink until it IsPrimed returns true.
		demuxer->PreparePipeline();
                audioDemuxer->PreparePipeline();

		// Push all the remaining frames through.
		while (!demuxer->IsDone())
		{
			demuxer->Step();
                        audioDemuxer->Step();
		}
		
		// Save everything to disk by closing the muxer.
		muxer->Close();
	}
	catch (FFmpegException e)
	{
		cerr << "Exception caught!" << endl;
		cerr << e.what() << endl;
		throw e;
	}

	cout << "Encoding complete!" << endl;
}
