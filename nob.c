#include "base/platform_info.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

#include "third_party/raylib/nob_raylib.c"

int win32_build_hot_reload_no_cradle(void) {
  Nob_Cmd cmd = {0};

  nob_cmd_append(&cmd,
    "cl",
    "/nologo",
    "/W4",
    "/wd4100",
    "/Zi",
    "/Od",
    "/DHANDMADE_INTERNAL",
    "/DHANDMADE_HOTRELOAD",
    "/LD",
    "handmade_module.c",
    "/link",
    "/INCREMENTAL:NO",
    "/PDB:handmade_module.pdb",
    "/OUT:game.dll",
    ""
  );
  if(!nob_cmd_run_sync_and_reset(&cmd)) return 0;

  return 1;
}

int win32_build_hot_reload(void) {
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd,
    "cl",
    "/nologo",
    "/W4",
    "/wd4100",
    "/Zi",
    "/Od",
    "/DHANDMADE_INTERNAL",
    "/DHANDMADE_HOTRELOAD",
    "/Fe:handmade.exe",
    "handmade_cradle.c",
    "user32.lib",
    "gdi32.lib",
    "Dsound.lib",
    "dxguid.lib",
    "winmm.lib",
    "ole32.lib",
    "/link",
    "/INCREMENTAL:NO",
    ""
  );
  if(!nob_cmd_run_sync_and_reset(&cmd)) return 0;

  return win32_build_hot_reload_no_cradle();
}

int win32_build_static(void) {
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd,
    "cl",
    "/nologo",
    "/W4",
    "/wd4100",
    "/Zi",
    "/Od",
    "/DHANDMADE_INTERNAL",
    "/Fe:handmade.exe",
    "handmade.c",
    "user32.lib",
    "gdi32.lib",
    "Dsound.lib",
    "dxguid.lib",
    "winmm.lib",
    "ole32.lib",
    "/link",
    "/INCREMENTAL:NO",
    ""
  );
  if(!nob_cmd_run_sync_and_reset(&cmd)) return 0;
  return 1;
}

int macos_build(void) {
  Nob_Cmd cmd = {0};

  nob_cmd_append(&cmd,
    "clang",
    "-std=c99",
    "--target=arm64-apple-macos",
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter",
    "-Wno-unused-function",
    // "-Wno-format",
    "-g",
    "-fno-omit-frame-pointer",
    // "-no-pie",
    "-O0",
    // "-pthread",
    "-fsanitize=address",
    "-o",
    "prompt",
    "prompt_build.c",
    "-lcurl",
  );

  if(!nob_cmd_run_sync(cmd)) return 0;

  return 1;
}

int linux_build(void) {
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd,
    "clang",
    "-std=c99",
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter",
    "-g",
    "-fno-omit-frame-pointer",
    "-no-pie",
    "-O0",
    "-pthread",
    "-o",
    "job_system_example",
    "job_system_example.c");

  if(!nob_cmd_run_sync(cmd)) return 0;

  return 1;
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);



  if(!macos_build()) return 1;

  return 0;

  if(!build_raylib_mac()) return 1;

  #if 0
  if(!win32_build_hot_reload()) return 1;
  #else
  if(!win32_build_hot_reload_no_cradle()) return 1;
  #endif

  if(!win32_build_static()) return 1;

  return 0;
}

