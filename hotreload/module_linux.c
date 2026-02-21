#ifndef HOTRELOAD_MODULE_LINUX_C
#define HOTRELOAD_MODULE_LINUX_C


void* module_init(void) {

  Platform_linux *pp_linux = platform_linux_init();
  ASSERT(pp_linux);

  return (void*)pp_linux;

}


int module_main(void *state) {

  Platform_linux *pp_linux = (Platform_linux*)state;

  platform_linux_main(pp_linux);

  int do_reload = pp_linux->do_reload;
  pp_linux->do_reload = 0;

  return do_reload;

}


#endif
