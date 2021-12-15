#include "Filter.h"
#include "FFmpegException.h"
#include <iostream>

using namespace std;

namespace ffmpegcpp
{

	Filter::Filter(const std::string &  p_filterString, FrameSink* p_target)
	{
		this->targetMediaType = p_target->GetMediaType();
		this->m_target = p_target->CreateStream();
		this->m_filterString = p_filterString.c_str();
	}

	AVMediaType Filter::GetMediaType()
	{
		return targetMediaType;
	}

	Filter::~Filter()
	{
		CleanUp();
	}

	void Filter::CleanUp()
	{
		avfilter_graph_free(&filter_graph);
		av_frame_free(&filt_frame);
		// TODO filter inputs in derived class!
	}

	void Filter::ConfigureFilterGraph()
	{
		int ret;
		char args[512];

		filt_frame = av_frame_alloc();
		if (!filt_frame)
		{
			throw FFmpegException(string("Could not allocate intermediate video frame for filter").c_str());
		}

		try
		{
			// create the filter graph
			filter_graph = avfilter_graph_alloc();
			if (!filter_graph)
			{
				throw FFmpegException(std::string(string("Failed to allocate filter graph")).c_str());
			}

			// create the filter string based on the defined inputs & one output
			string fullFilterString = "";

			// fetch one frame from each input and use it to construct the filter
			AVFrame *frame;
			for (int i = inputs.size() - 1; i >= 0; --i)
			{
				if (!inputs[i]->PeekFrame(&frame))
				{
					throw new FFmpegException(string(string("No frame found for input ") + to_string(i)).c_str());
				}

				// get the meta data for this input stream
				StreamData* metaData = inputs[i]->GetMetaData();

				// based on the type of data, we fill in the relevant info
				FillArguments(args, sizeof(args), frame, metaData);

				char bufferString[1000];
				snprintf(bufferString, sizeof(bufferString), "%s=%s [in_%d]; ", GetBufferName(metaData->type), args, i + 1);
				fullFilterString = bufferString + fullFilterString; // prepend the buffer string

			}

			// append an output sink to the buffer string
			for (unsigned int i = 0; i < inputs.size(); ++i)
			{
				fullFilterString += "[in_" + to_string(i + 1) + "] ";
			}
			fullFilterString += m_filterString;
			fullFilterString += " [result]; [result] ";
			fullFilterString += GetBufferSinkName(targetMediaType);

			// let avfilter generate the entire filter graph based on this string, including all
			// inputs and outputs. There are other ways to do this, but this is by far the easiest
			// one.
			AVFilterInOut *gis = NULL;
			AVFilterInOut *gos = NULL;
			ret = avfilter_graph_parse2(filter_graph, fullFilterString.c_str(), &gis, &gos);
			if (ret < 0)
			{
				throw FFmpegException(string("Failed to parse and generate filters").c_str(), ret);
			}

			// we don't use these
			avfilter_inout_free(&gis);
			avfilter_inout_free(&gos);

			// Fetch all input buffer sources and the output buffer sink from the graph.
			for (unsigned int i = 0; i < filter_graph->nb_filters; ++i)
			{
				AVFilterContext* ctx = filter_graph->filters[i];
				if (ctx->nb_inputs == 0)
				{
					bufferSources.push_back(ctx);
				}
				if (ctx->nb_outputs == 0)
				{
					buffersink_ctx = ctx;
				}
			}

			// Finally configure (initialize) the graph.
			if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
			{
				throw FFmpegException(string("Failed to configure filter graph").c_str(), ret);
			}

			// we configure our output meta data based on the sink's data
			outputMetaData.timeBase = buffersink_ctx->inputs[0]->time_base;
			outputMetaData.frameRate = buffersink_ctx->inputs[0]->frame_rate;
			outputMetaData.type = targetMediaType;
		}
		catch (FFmpegException e)
		{
			throw e;
		}
	}

	void Filter::FillArguments(char* args, int argsLength, AVFrame* frame, StreamData *metaData)
	{
		// this is a video input stream
		if (metaData->type == AVMEDIA_TYPE_VIDEO)
		{
			/* buffer video source: the decoded frames from the decoder will be inserted here. */
			// FIXME : unused
			//AVPixelFormat pixelFormat = (AVPixelFormat)frame->format;
			snprintf(args, argsLength,
				"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:frame_rate=%d/%d",
				frame->width, frame->height, frame->format,
				metaData->timeBase.num, metaData->timeBase.den,
				frame->sample_aspect_ratio.num, frame->sample_aspect_ratio.den,
				metaData->frameRate.num, metaData->frameRate.den);
		}

		// this is an audio input stream
		else if (metaData->type == AVMEDIA_TYPE_AUDIO)
		{
			uint64_t channelLayout = frame->channel_layout;
			if (channelLayout == 0) channelLayout = av_get_default_channel_layout(frame->channels);
			snprintf(args, argsLength,
				"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%lu",
				metaData->timeBase.num, metaData->timeBase.den, frame->sample_rate,
				av_get_sample_fmt_name((AVSampleFormat)frame->format), channelLayout);
		}

		// not supported
		else
		{
			throw new FFmpegException(std::string(std::string("Media type ") + av_get_media_type_string(metaData->type) + " is not supported by filters.").c_str());
		}
	}

	const char* Filter::GetBufferName(AVMediaType mediaType)
	{
		// this is a video input stream
		if (mediaType == AVMEDIA_TYPE_VIDEO) return "buffer";
		else if (mediaType == AVMEDIA_TYPE_AUDIO) return "abuffer";
		else throw new FFmpegException(std::string(std::string("Media type ") + av_get_media_type_string(mediaType) + " is not supported by filters.").c_str());
	}

	const char* Filter::GetBufferSinkName(AVMediaType mediaType)
	{
		// this is a video input stream
		if (mediaType == AVMEDIA_TYPE_VIDEO) return "buffersink";
		else if (mediaType == AVMEDIA_TYPE_AUDIO) return "abuffersink";
		else throw new FFmpegException(std::string(std::string("Media type ") + av_get_media_type_string(mediaType) + " is not supported by filters.").c_str());
	}

	void Filter::DrainInputQueues()
	{
		AVFrame *frame;
		// FIXME : unused
		// AVRational* timeBase;
		for (unsigned int i = 0; i < inputs.size(); ++i)
		{
			while (inputs[i]->FetchFrame(&frame))
			{
				int ret = av_buffersrc_add_frame(bufferSources[i], frame);
                                if (ret <0)
                                    std::cout << "Pb with av_buffersrc_add_frame" << "\n";

				av_frame_free(&frame);
			}
		}
	}

	FrameSinkStream* Filter::CreateStream()
	{

		// each new stream is associated with an input, and we need at least one frame from each input
		// until we can configure the filter graph!
		VideoFilterInput* input = new VideoFilterInput();
		inputs.push_back(input);
		return new FrameSinkStream(this, inputs.size() - 1);
	}

	void Filter::WriteFrame(int streamIndex, AVFrame* frame, StreamData* metaData)
	{
		// lazily initialize because we need the data from the frame to configure our filter graph
		if (!initialized)
		{
			// add to the proper input
			inputs[streamIndex]->SetMetaData(metaData);
			inputs[streamIndex]->WriteFrame(frame);

			// see if all inputs have received a frame - at this point, we can initialize!
			bool allInputsHaveFrames = true;
			for (unsigned int i = 0; i < inputs.size(); ++i)
			{
				if (!inputs[i]->HasFrame())
				{
					allInputsHaveFrames = false;
				}
			}

			// all inputs have a frame - we can finally configure!
			if (allInputsHaveFrames)
			{
				ConfigureFilterGraph();
				DrainInputQueues();
				initialized = true;
			}

			// we don't do anything else if we haven't finished configuring
			else
			{
				return;
			}
		}

		// add to the proper buffer source
		int ret = av_buffersrc_add_frame_flags(bufferSources[streamIndex], frame, AV_BUFFERSRC_FLAG_KEEP_REF);

                if (ret < 0)
                    std::cout << "Pb with av_buffersrc_add_frame_flags()" << "\n";

		PollFilterGraphForFrames();
	}

	void Filter::Close(int streamIndex)
	{
		if (!initialized) return; // can't close if we were never opened

		int ret = av_buffersrc_add_frame_flags(bufferSources[streamIndex], NULL, AV_BUFFERSRC_FLAG_KEEP_REF);

                if (ret < 0)
                    std::cout << "Pb with av_buffersrc_add_frame_flags()" << "\n";

		PollFilterGraphForFrames();

		// close this input
		inputs[streamIndex]->Close();

		// close our target only if all inputs are closed
		bool allClosed = true;
		for (unsigned int i = 0; i < inputs.size(); ++i)
		{
			if (!inputs[i]->IsClosed()) allClosed = false;
		}
		if (allClosed)
		{
			m_target->Close();
		}
	}

	void Filter::PollFilterGraphForFrames()
	{
		int ret = 0;
		while (ret >= 0)
		{
			ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				return;
			}
			else if (ret < 0)
			{
				throw FFmpegException(std::string("Error during filtering").c_str(), ret);
			}

			m_target->WriteFrame(filt_frame, &outputMetaData);

			av_frame_unref(filt_frame);
		}
	}


	bool Filter::IsPrimed()
	{
		return m_target->IsPrimed();
	}
}
