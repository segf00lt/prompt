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

    {
      struct stat st;
      if(stat("./"MODULE".so", &st) != 0) {
        printf(MODULE".so not found\n");
        return 1;
      }
    }

    if(rename(MODULE".so", MODULE".so.live") != 0) {
      printf("module file rename failed\n");
      return 1;
    }

    module = dlopen("./"MODULE".so.live", RTLD_NOW | RTLD_LOCAL);
    if(!module) {
      printf("%s\n", dlerror());
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

    dlclose(module);

    if(!reload_module) {
      break;
    }

  }

  return 0;
}


#endif
