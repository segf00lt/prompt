#ifndef PLATFORM_MACOS_C
#define PLATFORM_MACOS_C

// globals

global Arena *platform_arena;



// functions

int main(void) {

  #if 0
  u64 platform_page_size = platform_macos_get_page_size();

  u64 platform_arena_page_count = 10;
  u64 platform_arena_bytes = platform_arena_page_count * platform_page_size;

  void *platform_memory_block = platform_macos_debug_alloc_memory_block(platform_arena_page_count);

  platform_arena = arena_create_ex(platform_arena_bytes, 1, platform_memory_block);
  #else

  platform_arena = arena_create_ex(MB(5), 0, 0);

  #endif

  App *ap = app_init();

  // HERE
  // TODO jfd:
  // - DONE Basic prompt interface
  // - DONE Send all_messages and all_tools in POST to groq api
  // - DONE Generate array of tools immediate mode style
  // - DONE Process tool calls
  // - return tool call results to model in all_messages


  for(;;) {

    if(ap->flags & APP_FLAG_QUIT) {
      break;
    }

    app_update_and_render(ap);

  }

  return 0;
}


internal u64
func platform_macos_get_page_size(void) {
  vm_size_t page;
  host_page_size(mach_host_self(), &page);

  u64 result = (u64)page;

  return result;
}

internal u8*
func platform_macos_debug_alloc_memory_block(u64 page_count) {
  mach_vm_address_t base = 0x200000000;

  u64 page_size = platform_macos_get_page_size();

  mach_vm_size_t size = page_size * page_count;

  kern_return_t kr = mach_vm_allocate(
    mach_task_self(),
    &base,
    size,
    VM_FLAGS_FIXED
  );

  if(kr != KERN_SUCCESS) {
    fprintf(stderr, "mach_vm_allocate failed: %s (%d)\n",
      mach_error_string(kr), kr);
    return 0;
  }

  kr = mach_vm_protect(
    mach_task_self(),
    base,
    size,
    FALSE,
    VM_PROT_READ | VM_PROT_WRITE
  );

  if(kr != KERN_SUCCESS) {
    fprintf(stderr, "mach_vm_allocate failed: %s (%d)\n",
      mach_error_string(kr), kr);
    return 0;
  }

  return (u8*)(void*)base;
}


internal void*
func platform_alloc(u64 bytes) {
  void *result = malloc((size_t)bytes);
  if(result) {
    memset(result, 0, (size_t)bytes);
  }
  return result;
}

internal void
func platform_free(void *ptr) {
  ASSERT(ptr);
  free(ptr);
}

internal Str8
func platform_read_entire_file(Arena *a, char *path) {
  Str8 result = {0};

  FILE *f = 0;
  f = fopen(path, "rb");

  if(f == 0) {
    goto end;
  }

  if(fseeko(f, 0, SEEK_END) != 0) {
    goto end;
  }

  off_t file_size = ftello(f);
  if(file_size < 0) {
    goto end;
  }

  if(fseeko(f, 0, SEEK_SET) != 0) {
    goto end;
  }

  u8 *p = push_array_no_zero(a, u8, (u64)file_size);
  if(!p) {
    goto end;
  }

  size_t read_count = fread((void*)p, 1, (size_t)file_size, f);
  if(read_count != (size_t)file_size || ferror(f)) {
    goto end;
  }

  result.s = p;
  result.len = (s64)file_size;

  end:
  if(f) {
    fclose(f);
  }
  return result;
}

internal b32
func platform_write_entire_file(Str8 data, char *path) {
  b32 result = 0;

  FILE *f = fopen(path, "wb");
  if(!f) {
    return 0;
  }

  size_t written = fwrite(data.s, 1, (size_t)data.len, f);
  if(written != (size_t)data.len) {
    goto end;
  }

  if(fflush(f) != 0) {
    goto end;
  }

  result = 1;

  end:
  fclose(f);
  return result;
}

internal b32
func platform_file_exists(char *path) {
  b32 result = 0;

  struct stat st;
  if(stat(path, &st) == 0) {
    result = 1;
  } else {
    result = 0;
  }

  return result;
}

internal Str8
func platform_get_current_dir(Arena *a) {
  /* PATH_MAX should be safe here; allocate with arena */
  char *buf = push_array_no_zero(a, char, PATH_MAX);
  if(!getcwd(buf, PATH_MAX)) {
    /* return empty Str8 on failure */
    return (Str8){0};
  }

  size_t len = strlen(buf);
  Str8 result = { .s = (u8*)buf, .len = (s64)len };
  return result;
}

internal b32
func platform_set_current_dir(char *dir_path) {
  b32 result = 0;

  if(chdir(dir_path) == 0) {
    result = 1;
  } else {
    result = 0;
  }

  return result;
}

internal b32
func platform_move_file(char *old_path, char *new_path) {
  b32 result = 0;

  if(rename(old_path, new_path) == 0) {
    result = 1;
  } else {
    /* rename can fail across filesystems (EXDEV) — keep simple for now */
    result = 0;
  }

  return result;
}

internal b32
func platform_remove_file(char *path) {
  b32 result = 1;

  if(unlink(path) != 0) {
    result = 0;
  }

  return result;
}

internal b32
func platform_make_dir(char *dir_path) {
  b32 result = 0;

  if(mkdir(dir_path, 0755) == 0) {
    result = 1;
  } else {
    if(errno == EEXIST) {
      result = 1;
    } else {
      result = 0;
    }
  }

  return result;
}

internal void
func platform_sleep_ms(u32 ms) {
  struct timespec req, rem;
  req.tv_sec = (time_t)(ms / 1000);
  req.tv_nsec = (long)((ms % 1000) * 1000000L);
  while(nanosleep(&req, &rem) == -1 && errno == EINTR) {
    req = rem;
  }
}

internal void*
func platform_library_load(char *path) {
  void *lib = 0;
  lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  return lib;
}

internal void
func platform_library_unload(void* lib) {
  ASSERT(lib);
  dlclose(lib);
}

internal Void_func*
func platform_library_load_function(void* lib, char *name) {
  Void_func *fn = 0;

  if(lib) {
    fn = (Void_func*)dlsym(lib, name);
  } else {
    fn = 0;
  }

  return fn;
}

internal Str8
func platform_file_name_from_path(Str8 path) {
  Str8 result = {0};

  s64 i;

  for(i = path.len - 1; i >= 0; i--) {
    if(path.s[i] == '/') break;
  }

  result = str8_slice(path, i + 1, path.len);

  return result;
}


#endif
