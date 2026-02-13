#ifndef PLATFORM_MACOS_H
#define PLATFORM_MACOS_H

// NOTE jfd: mach.h uses internal somewhere
#undef internal

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dlfcn.h>

#include <mach/mach.h>
#include <mach/mach_vm.h>


#define internal static



internal u64        platform_macos_get_page_size(void);
internal u8*        platform_macos_debug_alloc_memory_block(u64 page_count);
internal void*      platform_alloc(u64 bytes);
internal void       platform_free(void *ptr);
internal Str8       platform_read_entire_file(Arena *a, char *path);
internal b32        platform_write_entire_file(Str8 data, char *path);
internal b32        platform_file_exists(char *path);
internal Str8       platform_get_current_dir(Arena *a);
internal b32        platform_set_current_dir(char *dir_path);
internal b32        platform_move_file(char *old_path, char *new_path);
internal b32        platform_remove_file(char *path);
internal b32        platform_make_dir(char *dir_path);
internal void       platform_sleep_ms(u32 ms);
internal void*      platform_library_load(char *path);
internal void       platform_library_unload(void* lib);
internal Void_func* platform_library_load_function(void* lib, char *name);
internal Str8       platform_file_name_from_path(Str8 path);


#endif
