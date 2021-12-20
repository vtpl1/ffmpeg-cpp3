// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef CoreContext_h
#define CoreContext_h
namespace ffpp
{
class CoreContext
{
private:
  CoreContext();
  ~CoreContext() = default;

public:
  static CoreContext& instance();
  CoreContext(const CoreContext&) = delete;
  CoreContext& operator=(const CoreContext&) = delete;
};
} // namespace ffpp

#endif // CoreContext_h
