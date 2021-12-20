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
auto ThrowOnFfmpegErrorInternal = [](int res, const char* file_name = nullptr, int lineNum = -1) {
  if (0 != res) {
    std::stringstream ss;
    if (lineNum > 0) {
      if (file_name != nullptr) {
        ss << file_name << ":";
      }
      ss << lineNum << std::endl;
    }
    throw FFmpegException(ss.str(), res);
  }
};
auto ThrowOnFfmpegReturnNullptrInternal = [](void* ptr, const char* file_name = nullptr, int lineNum = -1) {
  if (ptr == nullptr) {
    std::stringstream ss;
    if (lineNum > 0) {
      if (file_name != nullptr) {
        ss << file_name << ":";
      }
      ss << lineNum << std::endl;
    }
    ss << "Returned nullptr";
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
#define ThrowOnFfmpegReturnNullptr(ptr) ::ffpp::ThrowOnFfmpegReturnNullptrInternal(ptr, __FILE__, __LINE__)
#endif // FFmpegException_h
