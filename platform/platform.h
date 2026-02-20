#ifndef PLATFORM_H
#define PLATFORM_H


// NOTE jfd 22/01/2026:
// This file contains function interfaces that the game (or application) can call the platform through.
// It serves as a platform abstraction layer that contains only those things that the game
// absolutely must be able to ask the platform for.


typedef enum Platform_event_kind {
  EVENT_NULL = 0,
  EVENT_KEY_PRESS,
  EVENT_KEY_RELEASE,
  EVENT_MOUSE_MOVE,
  EVENT_MOUSE_CLICK,
  EVENT_MOUSE_SCROLL,
} Platform_event_kind;


typedef struct Platform_event Platform_event;
struct Platform_event {
  Platform_event *next;
  Platform_event *prev;
  Platform_event_kind kind;
  Keyboard_modifier modifier_mask;
  Keyboard_key key;
  Mouse_button mouse_button;
  b16 is_repeat;
  u16 repeat_count;
  u32 character;
  v2 mouse_pos;
  v2 scroll_delta;
};

typedef struct Platform_event_list Platform_event_list;
struct Platform_event_list {
  s64 count;
  Platform_event *first;
  Platform_event *last;
};


typedef struct Platform Platform;
struct Platform {
  void *backbuffer;
  u64   backbuffer_size;
};

internal        Keyboard_modifier platform_get_keyboard_modifiers(void);

internal        void*             platform_alloc(u64 bytes);
internal        void              platform_free(void *ptr);
internal        Str8              platform_read_entire_file(Arena *a, char *path);
internal        b32               platform_write_entire_file(Str8 data, char *path);
internal        b32               platform_file_exists(char *path);
internal        b32               platform_make_dir(char *dir_path);
internal        Str8              platform_get_current_dir(Arena *a);
internal        b32               platform_set_current_dir(char *dir_path);
internal        b32               platform_move_file(char *old_path, char *new_path);
internal        b32               platform_remove_file(char *path);
internal        Str8              platform_file_name_from_path(Str8 path);

internal        void              platform_sleep_ms(u32 ms);

internal        void*             platform_library_load(char *path);
internal        void              platform_library_unload(void *lib);
internal        Void_func*        platform_library_load_function(void *lib, char *name);



#endif
