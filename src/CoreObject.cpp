// *****************************************************
//    Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include "CoreObject.h"
#include "CoreContext.h"
namespace ffpp
{
CoreObject::CoreObject() { CoreContext::instance(); }

CoreObject::~CoreObject() { }
} // namespace ffpp