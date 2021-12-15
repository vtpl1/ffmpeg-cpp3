#include "RawAudioFileSource.h"
#include "FFmpegException.h"


using namespace std;

namespace ffmpegcpp
{
	RawAudioFileSource::RawAudioFileSource(const std::string & fileName, const std::string & inputFormat, int sampleRate, int channels, FrameSink* frameSink)
	{
            // fileName = hw::1,0 input format = alsa, sampleRate = 48000 (default)  channels= 2 (default)  frameSink = ? AudioDecoder() ?

		// try to deduce the input format from the input format name
		AVInputFormat *file_iformat;
		if (!(file_iformat = av_find_input_format(inputFormat.c_str())))
		{
			CleanUp();
			throw FFmpegException(std::string("Unknown input format: " + string(inputFormat)).c_str());
		}

		AVDictionary* format_opts = NULL;

		av_dict_set_int(&format_opts, "sample_rate", sampleRate, 0);
		av_dict_set_int(&format_opts, "channels", channels, 0);
		av_dict_set_int(&format_opts, "b", 256000, 0);
		av_dict_set_int(&format_opts, "minrate", 400000, 0);
		av_dict_set_int(&format_opts, "maxrate", 400000, 0);

		// create the demuxer
		try
		{
			demuxer = new Demuxer(fileName, file_iformat, format_opts);
			demuxer->DecodeBestAudioStream(frameSink);
		}
		catch (FFmpegException e)
		{
			CleanUp();
			throw e;
		}
	}


	RawAudioFileSource::~RawAudioFileSource()
	{
		CleanUp();
	}

	void RawAudioFileSource::CleanUp()
	{
		if (demuxer != nullptr)
		{
			delete demuxer;
			demuxer = nullptr;
		}
	}

	void RawAudioFileSource::PreparePipeline()
	{
		demuxer->PreparePipeline();
	}

	bool RawAudioFileSource::IsDone()
	{
		return demuxer->IsDone();
	}

	void RawAudioFileSource::Step()
	{
		demuxer->Step();
	}
}

