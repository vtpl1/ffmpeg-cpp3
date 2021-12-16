#include "FFmpegException.h"

#if _MSC_VER
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char *)_malloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)
#else
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char *)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)
#endif  // _MSC_VER


namespace ffmpegcpp
{
	FFmpegException::FFmpegException(const std::string & error) : _msg(error)
	{
	}

	FFmpegException::FFmpegException(const std::string & error, int returnValue)
	{
		_msg = error + " " + av_err2str(returnValue);
			//std::string(std::string(error) + ": " + av_make_error_string(this->error, AV_ERROR_MAX_STRING_SIZE, returnValue)).c_str();
	}
}

/*
namespace ffmpegcpp
{
	FFmpegException::FFmpegException(const char * error) : exception ()
	{
	}

	FFmpegException::FFmpegException(const char * error, int returnValue)
		: exception(
			std::string(std::string(error) + ": " + av_make_error_string(this->error, AV_ERROR_MAX_STRING_SIZE, returnValue)).c_str(), returnValue )
	{
	}
}
*/