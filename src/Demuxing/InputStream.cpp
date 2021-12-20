// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "InputStream.h"
namespace ffpp
{
InputStream::InputStream(AVFormatContext* format, AVStream* stream) : _format(format), _stream(stream) {}

InputStream::~InputStream() { Stop(); }
void InputStream::Stop() {}

} // namespace ffpp
