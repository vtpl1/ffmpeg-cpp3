// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef InputSource_h
#define InputSource_h
namespace ffpp {
class InputSource {
 private:
 public:
  virtual ~InputSource() {}

  virtual void PreparePipeline() = 0;
  virtual bool IsDone() = 0;
  virtual void Step() = 0;
};

}  // namespace ffmpegcpp3

#endif  // InputSource_h
