// *****************************************************
//  Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef FFmpegException_h
#define FFmpegException_h
#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
namespace ffpp
{
class FFmpegException : public std::exception
{
public:
  explicit FFmpegException(const std::string& error);
  explicit FFmpegException(const std::string& error, int ffmpeg_error_code);

  virtual const char* what() const noexcept { return _msg.c_str(); }

private:
  std::string _msg;
};

class FFmpegNotImplementedException : public FFmpegException
{
public:
  FFmpegNotImplementedException() : FFmpegException("Not implemented yet ") {}
};
auto ThrowOnFfmpegErrorInternal = [](int res, const char* file_name = nullptr, int lineNum = -1) {
  if (0 > res) {
    std::stringstream ss;
    if (lineNum > 0) {
      if (file_name != nullptr) {
        ss << file_name << ":";
      }
      ss << lineNum;
      ss << " return :";
      ss << res;
    }
    throw FFmpegException(ss.str(), res);
  }
};
auto ThrowOnFfmpegErrorWithAllowedErrorInternal = [](int res, int allowed_error, int allowed_error2,
                                                     const char* file_name = nullptr, int lineNum = -1) {
  if (allowed_error2 != 0) {
    if (res == allowed_error2) {
      return;
    }
  }
  if (res == allowed_error) {
    return;
  }

  ThrowOnFfmpegErrorInternal(res, file_name, lineNum);
};
auto ThrowOnFfmpegReturnNullptrInternal = [](void* ptr, const char* file_name = nullptr, int lineNum = -1) {
  if (nullptr == ptr) {
    std::stringstream ss;
    ss << "Returned nullptr, ";
    if (lineNum > 0) {
      if (nullptr != file_name) {
        ss << file_name << ":";
      }
      ss << lineNum;
    }

    throw FFmpegException(ss.str());
  }
};
class Timer
{
public:
  Timer() : beg_(clock_::now()) {}
  ~Timer() { std::cout << "Elapsed: " << elapsed() << std::endl; }
  void reset() { beg_ = clock_::now(); }
  double elapsed() const { return std::chrono::duration_cast<second_>(clock_::now() - beg_).count(); }

private:
  typedef std::chrono::high_resolution_clock clock_;
  typedef std::chrono::duration<double, std::ratio<1>> second_;
  std::chrono::time_point<clock_> beg_;
};

} // namespace ffpp
#define ThrowOnFfmpegError(res) ::ffpp::ThrowOnFfmpegErrorInternal(res, __FILE__, __LINE__)
#define ThrowOnFfmpegErrorWithAllowedError2(res, allowed_error, allowed_error2)                                        \
  ::ffpp::ThrowOnFfmpegErrorWithAllowedErrorInternal(res, allowed_error, allowed_error2, __FILE__, __LINE__)
#define ThrowOnFfmpegErrorWithAllowedError(res, allowed_error)                                                         \
  ThrowOnFfmpegErrorWithAllowedError2(res, allowed_error, 0)
#define ThrowOnFfmpegReturnNullptr(ptr) ::ffpp::ThrowOnFfmpegReturnNullptrInternal(ptr, __FILE__, __LINE__)
#endif // FFmpegException_h
