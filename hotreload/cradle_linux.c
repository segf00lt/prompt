#ifndef HOTRELOAD_CRADLE_LINUX_C
#define HOTRELOAD_CRADLE_LINUX_C


#define _POSIX_C_SOURCE 200809L

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

#ifndef MODULE
#error must define MODULE
#endif


typedef void* Module_init_func(void);
typedef int   Module_main_func(void *);

int main(int argc, char **argv) {

  void *module_state = 0;

  for(;;) {

    void *module = 0;
    Module_init_func *module_init = 0;
    Module_main_func *module_main = 0;

    int module_file_exists = 0;
    {
      struct stat st;
      if(stat("./"MODULE".so", &st) == 0) {
        module_file_exists = 1;
      }
    }

    if(module_file_exists) {
      for(;;) {
        if(rename(MODULE".so", MODULE".so.live") == 0) {
          break;
        }

        if(errno != EBUSY) {
          printf("error loading app code\n");
          return 1;
        }

        unsigned int ms = 10;
        struct timespec req, rem;
        req.tv_sec = (time_t)(ms / 1000);
        req.tv_nsec = (long)((ms % 1000) * 1000000L);
        while(nanosleep(&req, &rem) == -1 && errno == EINTR) {
          req = rem;
        }

      }
    }

    module = dlopen("./"MODULE".so.live", RTLD_NOW | RTLD_LOCAL);
    if(!module) {
      printf("%s", dlerror());
      return 1;
    }

    module_init = (Module_init_func*)dlsym(module, "module_init");
    if(!module_init) {
      printf("%s\n", dlerror());
      return 1;
    }

    module_main = (Module_main_func*)dlsym(module, "module_main");
    if(!module_main) {
      printf("%s\n", dlerror());
      return 1;
    }

    if(!module_state) {
      module_state = module_init();
    }

    int reload_module = module_main(module_state);

    if(!reload_module) {
      break;
    }

    dlclose(module);

  }

  return 0;
}


#endif
