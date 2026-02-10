#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H


// NOTE jfd 22/01/2026:
// This file contains type definitions for the game (or application) to interface with the platform.

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



typedef Keyboard_modifier Platform_get_keyboard_modifiers_func(void);
typedef Str8              Platform_read_entire_file_func(char*);
typedef b32               Platform_write_entire_file_func(Str8, char*);

typedef struct Platform_vtable Platform_vtable;
struct Platform_vtable {
  Platform_get_keyboard_modifiers_func  *get_keyboard_modifiers;
  Platform_read_entire_file_func  *read_entire_file;
  Platform_write_entire_file_func *write_entire_file;
};

typedef struct Platform Platform;
struct Platform {
  void *game_memory_backbuffer;
  u64 game_memory_backbuffer_size;
  Platform_vtable vtable;
};

#endif
