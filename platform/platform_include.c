#ifndef PLATFORM_INCLUDE_C
#define PLATFORM_INCLUDE_C


#include "platform.c"


#if OS_WINDOWS

#error unsupported platform

#elif OS_LINUX

#include "platform_linux.c"

#elif OS_MAC

#include "platform_macos.c"

#elif OS_WEB

#error unsupported platform

#else
#error unsupported platform
#endif



#endif
