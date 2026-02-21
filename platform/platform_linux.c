#ifndef PLATFORM_LINUX_C
#define PLATFORM_LINUX_C

// globals



// functions


internal Platform_linux*
func platform_linux_init(void) {
  Platform_linux *pp = platform_alloc(sizeof(Platform_linux));

  pp->main_arena = arena_create_ex(MB(5), 0, 0);

  pp->ap = app_init();


  #if 0
  Str8 texts[] = {
    str8_lit("hello"),
    str8_lit("goodbye"),
  };
  Str8_list texts_list = {0};
  for(int i = 0; i < (int)ARRLEN(texts); i++) {
    str8_list_append_str(ap->main_arena, &texts_list, texts[i]);
  }
  Embedding_vector_slice vectors = get_embedding_vectors_for_texts(ap, texts_list);
  ASSERT(vectors.count == texts_list.count);
  #endif

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1000, 800, "prompt");
  SetTargetFPS(60);
  SetTraceLogLevel(LOG_DEBUG);
  SetExitKey(0);

  return pp;
}


internal void
func platform_linux_main(Platform_linux *pp) {

  App *ap = pp->ap;

  for(;;) {

    if(ap->flags & APP_QUIT) {
      CloseWindow();
      break;
    }

    #ifdef BUILD_HOTRELOAD
    if(platform_file_exists(MODULE".so")) {
      ap->did_reload = true;
      pp->do_reload = true;
      return;
    }
    #endif

    {
      if(IsKeyPressed(KEY_ENTER)) {
        ap->say_hello_to_model = true;
      }
    }

    app_update_new(ap);

    { /* render */
      Color color = RED;
      local_persist b32 flip = 1;
      if(flip) {
        color = BLUE;
      }



      BeginDrawing();


      ClearBackground(color);


      EndDrawing();

      if(WindowShouldClose()) {
        CloseWindow();
        break;
      }
      flip = !flip;

    } /* render */


  }

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


#ifndef BUILD_HOTRELOAD

int main(int argc, char **argv) {
  Platform_linux *pp = platform_linux_init();

  platform_linux_main(pp);

  return 0;
}

#endif


#endif
