#ifndef PROMPT_H
#define PROMPT_H

typedef struct App App;

typedef struct Curl_write_buffer Curl_write_buffer;
struct Curl_write_buffer {
  Arena *arena;
  Str8_list chunks;
};

TYPEDEF_SLICE(Str8, Str8_slice);
TYPEDEF_ARRAY(Str8, Str8_array);

typedef enum Groq_tool_parameter_type {
  GROQ_TOOL_PARAM_NONE = 0,
  GROQ_TOOL_PARAM_NUMBER,
  GROQ_TOOL_PARAM_INTEGER,
  GROQ_TOOL_PARAM_BOOLEAN,
  GROQ_TOOL_PARAM_STRING,
  GROQ_TOOL_PARAM_ARRAY,
  GROQ_TOOL_PARAM_OBJECT,
} Groq_tool_parameter_type;

typedef struct Groq_tool_parameter Groq_tool_parameter;
struct Groq_tool_parameter {
  Groq_tool_parameter_type param_type;
  Str8 name;
  Str8 description;
  Str8_slice accepted_string_values;
  s64 minimum_value;
  s64 maximum_value;
  b32 is_optional;
};

TYPEDEF_SLICE(Groq_tool_parameter, Groq_tool_parameter_slice);

typedef void (*Tool_callback)(App *, json_value_t *);

typedef struct Groq_tool Groq_tool;
struct Groq_tool {
  Str8 name;
  Str8 description;
  Groq_tool_parameter_slice parameters;
  Tool_callback callback;
};
STATIC_ASSERT(sizeof(Groq_tool) <= 64, groq_tool_is_less_than_64_bytes);
#define GROQ_TOOL_SIZE ((u64)96)

TYPEDEF_SLICE(Groq_tool, Groq_tool_array);

typedef struct Groq_tool_call Groq_tool_call;
struct Groq_tool_call {
  Str8 id;
  Str8 tool_name; // NOTE jfd: corresponds to function.name in the json
  json_value_t *parameters;
};

TYPEDEF_SLICE(Groq_tool_call, Groq_tool_call_slice);

typedef u64 Groq_message_flags;
#define GROQ_MESSAGE_FLAG_ERROR ((Groq_message_flags)(1ul << 0))

typedef struct Groq_message Groq_message;
struct Groq_message {
  Groq_message_flags flags;
  Str8 name;
  Str8 role;
  Str8 content;
  Str8 reasoning;
  Groq_tool_call_slice tool_calls;
};
STATIC_ASSERT(sizeof(Groq_message) <= 96, groq_message_is_less_than_96_bytes);
#define GROQ_MESSAGE_SIZE ((u64)96)


TYPEDEF_ARRAY(Groq_message, Groq_message_array);


typedef u64 App_flags;
#define APP_FLAG_QUIT ((App_flags)(1ul << 0))

struct App {
  App_flags flags;

  Arena *main_arena;
  Arena *frame_arena;
  Arena *temp_arena;

  Str8 env_file;
  Str8 groq_api_key;
  Groq_message_array all_messages;
  Groq_tool_array all_tools;

};
STATIC_ASSERT(sizeof(App) <= MB(1), app_state_is_less_than_a_megabyte);
#define APP_STATE_SIZE ((u64)MB(1))


internal void* json_arena_push(void *user_data, size_t size);

internal size_t my_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp);

internal void push_groq_user_message(App *ap, Str8 content);
internal void push_groq_model_response_message(App *ap, Str8 groq_response_json);

internal void send_all_messages_to_groq(App *ap);


internal void app_update_and_render(App *ap);


internal void create_tool_hello(App *ap);


#endif
