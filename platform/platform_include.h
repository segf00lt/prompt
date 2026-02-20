#ifndef PLATFORM_INCLUDE_H
#define PLATFORM_INCLUDE_H


#include "platform.h"


#if OS_WINDOWS

#error unsupported platform

#elif OS_LINUX

#include "platform_linux.h"

#elif OS_MAC

#include "platform_macos.h"

#elif OS_WEB

#error unsupported platform

#else
#error unsupported platform
#endif



#endif
