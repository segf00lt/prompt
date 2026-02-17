
#ifndef NOB_IMPLEMENTATION

#define NOB_IMPLEMENTATION
#include "nob.h"

#endif


enum {
  RAYLIB_BUILD_FIRST = 0,
  RAYLIB_BUILD_STATIC = 0,
  RAYLIB_BUILD_SHARED,
  RAYLIB_BUILD_DEBUG,
  RAYLIB_BUILD_WEB,
  RAYLIB_BUILD_COUNT,
};

#define RAYLIB_PATH "./third_party/raylib"

char *raylib_compiler_linux = "clang";
char *raylib_compiler_mac = "clang";
char *raylib_compiler_windows = "cl";
char *raylib_compiler_web = "emcc";

char *raylib_linker_linux = "ar";
char *raylib_linker_mac = "ar";

char *shared_build_cflag_mac = "-dynamiclib";
char *shared_build_ext_mac = ".dylib";

char *shared_build_cflag_linux = "-shared";
char *shared_build_ext_linux = ".so";

char *raylib_cflags_mac[] = {
  "-Wall",
  "-D_GNU_SOURCE",
  "-DPLATFORM_DESKTOP_GLFW",
  "-DGRAPHICS_API_OPENGL_33",
  "-Wno-missing-braces",
  "-Werror=pointer-arith",
  "-fno-strict-aliasing",
  "-std=c99",
  "-O1",
  "-Werror=implicit-function-declaration",
};

char *raylib_cflags_linux[] = {
  "-Wall",
  "-D_GNU_SOURCE",
  "-DPLATFORM_DESKTOP_GLFW",
  "-DGRAPHICS_API_OPENGL_33",
  "-Wno-missing-braces",
  "-Werror=pointer-arith",
  "-fno-strict-aliasing",
  "-std=c99",
  "-fPIC",
  "-O1",
  "-Werror=implicit-function-declaration",
  "-D_GLFW_X11",
};

char *raylib_cflags_web[] = {
  "-Wall",
  "-D_GNU_SOURCE",
  "-DPLATFORM_WEB",
  "-DGRAPHICS_API_OPENGL_ES2",
  "-Wno-missing-braces",
  "-Werror=pointer-arith",
  "-fno-strict-aliasing",
  "-std=gnu99",
  "-fPIC",
  "-Os",
  "-Os",
  "-sWASM_WORKERS=1",
  "-matomics",
  "-mbulk-memory",
  "-sUSE_PTHREADS=1",
};

char *raylib_debug_cflags_mac[] = {
  "-g",
  "-Wall",
  "-D_GNU_SOURCE",
  "-DPLATFORM_DESKTOP_GLFW",
  "-DGRAPHICS_API_OPENGL_33",
  "-Wno-missing-braces",
  "-Werror=pointer-arith",
  "-fno-strict-aliasing",
  "-std=c99",
  "-O0",
  "-Werror=implicit-function-declaration",
};

char *raylib_debug_cflags_linux[] = {
  "-g",
  "-Wall",
  "-D_GNU_SOURCE",
  "-DPLATFORM_DESKTOP_GLFW",
  "-DGRAPHICS_API_OPENGL_33",
  "-Wno-missing-braces",
  "-Werror=pointer-arith",
  "-fno-strict-aliasing",
  "-std=c99",
  "-fPIC",
  "-O0",
  "-Werror=implicit-function-declaration",
  "-D_GLFW_X11",
};

char *raylib_ldflags_mac[] = {
  "-install_name",
  "@rpath/libraylib.dylib",
  "-framework",
  "OpenGL",
  "-framework",
  "Cocoa",
  "-framework",
  "IOKit",
  "-framework",
  "CoreAudio",
  "-framework",
  "CoreVideo",
};

char *raylib_ldflags_linux[] = {
  "-Wl,-soname,libraylib.so",
  "-lGL",
  "-lc",
  "-lm",
  "-lpthread",
  "-ldl",
  "-lrt",
};

char *raylib_include_flags[] = {
  "-I./third_party/raylib",
  "-I./third_party/raylib/external/glfw/include",
};


#define RAYLIB_FILES \
X(rcore) \
X(rshapes) \
X(rtextures) \
X(rtext) \
X(utils) \
X(rglfw) \
X(rmodels) \
X(raudio) \

#define RAYLIB_FILES_WEB \
X(rcore) \
X(rshapes) \
X(rtextures) \
X(rtext) \
X(utils) \
X(rmodels) \
X(raudio) \

char *raylib_src_files[] = {
  #define X(x) RAYLIB_PATH"/"#x".c",
  RAYLIB_FILES
  #undef X
};

char *raylib_object_files[] = {
  #define X(x) RAYLIB_PATH"/"#x".o",
  RAYLIB_FILES
  #undef X
};

char *raylib_files[] = {
  #define X(x) #x,
  RAYLIB_FILES
  #undef X
};

char *raylib_src_files_web[] = {
  #define X(x) RAYLIB_PATH"/"#x".c",
  RAYLIB_FILES_WEB
  #undef X
};

char *raylib_object_files_web[] = {
  #define X(x) RAYLIB_PATH"/"#x".o",
  RAYLIB_FILES_WEB
  #undef X
};

char *raylib_files_web[] = {
  #define X(x) #x,
  RAYLIB_FILES_WEB
  #undef X
};



#if OS_WINDOWS

#define RAYLIB_SHARED_LIB_NAME "raylib.dll"

#elif OS_MAC

#define RAYLIB_SHARED_LIB_NAME "libraylib.dylib"

#elif OS_LINUX

#define RAYLIB_SHARED_LIB_NAME "libraylib.so"

#else
#error unsupported build platform
#endif


#define RAYLIB_BUILD_DIR             RAYLIB_PATH"/build"

#define RAYLIB_STATIC_BUILD_DIR      RAYLIB_BUILD_DIR"/static"
#define RAYLIB_SHARED_BUILD_DIR      RAYLIB_BUILD_DIR"/shared"
#define RAYLIB_DEBUG_BUILD_DIR       RAYLIB_BUILD_DIR"/debug"
#define RAYLIB_WEB_BUILD_DIR         RAYLIB_BUILD_DIR"/web"


#define RAYLIB_STATIC_LIB_PATH       RAYLIB_STATIC_BUILD_DIR"/libraylib.a"
#define RAYLIB_SHARED_LIB_PATH       RAYLIB_SHARED_BUILD_DIR"/"RAYLIB_SHARED_LIB_NAME
#define RAYLIB_DEBUG_LIB_PATH        RAYLIB_DEBUG_BUILD_DIR"/"RAYLIB_SHARED_LIB_NAME
#define RAYLIB_WEB_LIB_PATH          RAYLIB_WEB_BUILD_DIR"/libraylib.web.a"




int build_raylib_mac(void) {
  nob_log(NOB_INFO, "building raylib");

  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_BUILD_DIR));
  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_STATIC_BUILD_DIR));
  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_SHARED_BUILD_DIR));
  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_DEBUG_BUILD_DIR));
  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_WEB_BUILD_DIR));

  Nob_Cmd compile_cmds[RAYLIB_BUILD_COUNT * NOB_ARRAY_LEN(raylib_files)] = {0};
  int compile_cmds_count = 0;

  Nob_Cmd linker_cmds[RAYLIB_BUILD_COUNT] = {0};
  int linker_cmds_count = 0;

  for(int build_type = RAYLIB_BUILD_FIRST; build_type < RAYLIB_BUILD_COUNT; build_type++) {

    if(build_type == RAYLIB_BUILD_WEB) {
      continue; // TODO jfd: raylib web build
    } else {

      for(int i = 0; i < NOB_ARRAY_LEN(raylib_files); i++) {
        NOB_ASSERT(compile_cmds_count < NOB_ARRAY_LEN(compile_cmds));

        Nob_Cmd *compile_cmd = &compile_cmds[compile_cmds_count++];

        nob_cmd_append(compile_cmd,
          raylib_compiler_mac,
          "-x",
          "objective-c",
          "-c",
          raylib_src_files[i],
          "-o",
          raylib_object_files[i],
        );

        if(build_type == RAYLIB_BUILD_DEBUG) {
          nob_da_append_many(compile_cmd, raylib_debug_cflags_mac, NOB_ARRAY_LEN(raylib_debug_cflags_mac));
        } else {
          nob_da_append_many(compile_cmd, raylib_cflags_mac, NOB_ARRAY_LEN(raylib_cflags_mac));
        }

        nob_da_append_many(compile_cmd, raylib_include_flags, NOB_ARRAY_LEN(raylib_include_flags));

        if(!strcmp("rglfw", raylib_files[i])) {
          nob_cmd_append(compile_cmd, "-U_GNU_SOURCE");
        }

      }

    }


    NOB_ASSERT(linker_cmds_count < NOB_ARRAY_LEN(linker_cmds));
    Nob_Cmd *linker_cmd = &linker_cmds[linker_cmds_count++];

    if(build_type == RAYLIB_BUILD_DEBUG) {
      nob_cmd_append(linker_cmd,
        raylib_compiler_mac,
        shared_build_cflag_mac,
        "-o",
        RAYLIB_DEBUG_LIB_PATH
      );
      nob_da_append_many(linker_cmd, raylib_object_files, NOB_ARRAY_LEN(raylib_object_files));
      nob_da_append_many(linker_cmd, raylib_ldflags_mac, NOB_ARRAY_LEN(raylib_ldflags_mac));

    } else if(build_type == RAYLIB_BUILD_SHARED) {
      nob_cmd_append(linker_cmd,
        raylib_compiler_mac,
        shared_build_cflag_mac,
        "-o",
        RAYLIB_SHARED_LIB_PATH
      );
      nob_da_append_many(linker_cmd, raylib_object_files, NOB_ARRAY_LEN(raylib_object_files));
      nob_da_append_many(linker_cmd, raylib_ldflags_mac, NOB_ARRAY_LEN(raylib_ldflags_mac));

    } else if(build_type == RAYLIB_BUILD_STATIC) {
      nob_cmd_append(linker_cmd,
        raylib_linker_mac,
        "rcs",
        RAYLIB_STATIC_LIB_PATH
      );
      nob_da_append_many(linker_cmd, raylib_object_files, NOB_ARRAY_LEN(raylib_object_files));

    } else if(build_type == RAYLIB_BUILD_WEB) {
      NOB_UNREACHABLE("web builds unimplemented");
    } else {
      NOB_UNREACHABLE("unsupported build type");
    }

  }

  nob_log(NOB_INFO, "compiling all builds of raylib");
  Nob_Procs procs = {0};

  for(int i = 0; i < compile_cmds_count; i++) {
    nob_procs_append_with_flush(&procs, nob_cmd_run_async(compile_cmds[i]), 512);
  }

  NOB_ASSERT(nob_procs_wait_and_reset(&procs));

  nob_log(NOB_INFO, "linking all builds of raylib");

  for(int i = 0; i < linker_cmds_count; i++) {
    nob_procs_append_with_flush(&procs, nob_cmd_run_async(linker_cmds[i]), 512);
  }

  NOB_ASSERT(nob_procs_wait_and_reset(&procs));

  return 1;
}


int build_raylib_linux(void) {
  nob_log(NOB_INFO, "building raylib");

  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_BUILD_DIR));
  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_STATIC_BUILD_DIR));
  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_SHARED_BUILD_DIR));
  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_DEBUG_BUILD_DIR));
  NOB_ASSERT(nob_mkdir_if_not_exists(RAYLIB_WEB_BUILD_DIR));

  Nob_Cmd compile_cmds[RAYLIB_BUILD_COUNT * NOB_ARRAY_LEN(raylib_files)] = {0};
  int compile_cmds_count = 0;

  Nob_Cmd linker_cmds[RAYLIB_BUILD_COUNT] = {0};
  int linker_cmds_count = 0;

  for(int build_type = RAYLIB_BUILD_FIRST; build_type < RAYLIB_BUILD_COUNT; build_type++) {

    if(build_type == RAYLIB_BUILD_WEB) {
      continue; // TODO jfd: raylib web build
    } else {

      for(int i = 0; i < NOB_ARRAY_LEN(raylib_files); i++) {
        NOB_ASSERT(compile_cmds_count < NOB_ARRAY_LEN(compile_cmds));

        Nob_Cmd *compile_cmd = &compile_cmds[compile_cmds_count++];

        nob_cmd_append(compile_cmd,
          raylib_compiler_linux,
          "-x",
          "objective-c",
          "-c",
          raylib_src_files[i],
          "-o",
          raylib_object_files[i],
        );

        if(build_type == RAYLIB_BUILD_DEBUG) {
          nob_da_append_many(compile_cmd, raylib_debug_cflags_linux, NOB_ARRAY_LEN(raylib_debug_cflags_linux));
        } else {
          nob_da_append_many(compile_cmd, raylib_cflags_linux, NOB_ARRAY_LEN(raylib_cflags_linux));
        }

        nob_da_append_many(compile_cmd, raylib_include_flags, NOB_ARRAY_LEN(raylib_include_flags));

        if(!strcmp("rglfw", raylib_files[i])) {
          nob_cmd_append(compile_cmd, "-U_GNU_SOURCE");
        }

      }

    }


    NOB_ASSERT(linker_cmds_count < NOB_ARRAY_LEN(linker_cmds));
    Nob_Cmd *linker_cmd = &linker_cmds[linker_cmds_count++];

    if(build_type == RAYLIB_BUILD_DEBUG) {
      nob_cmd_append(linker_cmd,
        raylib_compiler_linux,
        shared_build_cflag_linux,
        "-o",
        RAYLIB_DEBUG_LIB_PATH
      );
      nob_da_append_many(linker_cmd, raylib_object_files, NOB_ARRAY_LEN(raylib_object_files));
      nob_da_append_many(linker_cmd, raylib_ldflags_linux, NOB_ARRAY_LEN(raylib_ldflags_linux));

    } else if(build_type == RAYLIB_BUILD_SHARED) {
      nob_cmd_append(linker_cmd,
        raylib_compiler_linux,
        shared_build_cflag_linux,
        "-o",
        RAYLIB_SHARED_LIB_PATH
      );
      nob_da_append_many(linker_cmd, raylib_object_files, NOB_ARRAY_LEN(raylib_object_files));
      nob_da_append_many(linker_cmd, raylib_ldflags_linux, NOB_ARRAY_LEN(raylib_ldflags_linux));

    } else if(build_type == RAYLIB_BUILD_STATIC) {
      nob_cmd_append(linker_cmd,
        raylib_linker_linux,
        "rcs",
        RAYLIB_STATIC_LIB_PATH
      );
      nob_da_append_many(linker_cmd, raylib_object_files, NOB_ARRAY_LEN(raylib_object_files));

    } else if(build_type == RAYLIB_BUILD_WEB) {
      NOB_UNREACHABLE("web builds unimplemented");
    } else {
      NOB_UNREACHABLE("unsupported build type");
    }

  }

  nob_log(NOB_INFO, "compiling all builds of raylib");
  Nob_Procs procs = {0};

  for(int i = 0; i < compile_cmds_count; i++) {
    nob_procs_append_with_flush(&procs, nob_cmd_run_async(compile_cmds[i]), 512);
  }

  NOB_ASSERT(nob_procs_wait_and_reset(&procs));

  nob_log(NOB_INFO, "linking all builds of raylib");

  for(int i = 0; i < linker_cmds_count; i++) {
    nob_procs_append_with_flush(&procs, nob_cmd_run_async(linker_cmds[i]), 512);
  }

  NOB_ASSERT(nob_procs_wait_and_reset(&procs));

  return 1;
}
