#ifndef PLATFORM_LINUX_H
#define PLATFORM_LINUX_H


#undef internal

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dlfcn.h>


#define internal static


typedef struct Platform_linux Platform_linux;
struct Platform_linux {
  Arena *main_arena;
  App *ap;
  b32 do_reload;
  struct stat st;
};

internal Platform_linux* platform_linux_init(void);
internal void            platform_linux_main(Platform_linux *pp);


#endif
