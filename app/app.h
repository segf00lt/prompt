#ifndef APP_H
#define APP_H



#define ENV_VARS \
X(GROQ_API_KEY) \
X(COHERE_API_KEY) \

typedef enum Env_var {
  ENV_NONE = -1,
  #define X(x) ENV_##x,
  ENV_VARS
  #undef X
  ENV_COUNT,
} Env_var;

global read_only Str8 ENV_VAR_STR[ENV_COUNT] = {
  #define X(x) str8_lit(#x),
  ENV_VARS
  #undef X
};

global read_only char *ENV_VAR_CSTR[ENV_COUNT] = {
  #define X(x) #x,
  ENV_VARS
  #undef X
};


#define TOOLS \
X(SAY_HELLO) \
X(SAY_GOODBYE) \

typedef enum Tool_name {
  TOOL_NONE = -1,
  #define X(x) TOOL_##x,
  TOOLS
  #undef X
  TOOL_COUNT,
} Tool_name;

global read_only Str8 TOOL_NAME_STR[TOOL_COUNT] = {
  #define X(x) str8_lit(#x),
  TOOLS
  #undef X
};

global read_only char *TOOL_NAME_CSTR[TOOL_COUNT] = {
  #define X(x) #x,
  TOOLS
  #undef X
};


typedef struct Curl_write_buffer Curl_write_buffer;
struct Curl_write_buffer {
  Arena *arena;
  Str8_list chunks;
};

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

TYPEDEF_SLICE(Groq_tool_parameter);

typedef struct Groq_tool Groq_tool;
struct Groq_tool {
  Tool_name name;
  Str8 description;
  Groq_tool_parameter_slice parameters;
};
STATIC_ASSERT(sizeof(Groq_tool) <= 64, groq_tool_is_less_than_64_bytes);
#define GROQ_TOOL_SIZE ((u64)96)

TYPEDEF_ARRAY(Groq_tool);

typedef struct Groq_tool_call Groq_tool_call;
struct Groq_tool_call {
  Str8 id;
  Str8 tool_name;
  json_value_t *parameters;
};

TYPEDEF_SLICE(Groq_tool_call);

typedef u64 Groq_message_flags;
#define GROQ_MESSAGE_FLAG_ERROR ((Groq_message_flags)(1ul << 0))

typedef struct Groq_message Groq_message;
struct Groq_message {
  Groq_message_flags flags;
  Str8 name;
  Str8 role;
  Str8 content;
  Str8 reasoning;
  Str8 tool_call_id;
  Groq_tool_call_slice tool_calls;
};
STATIC_ASSERT(sizeof(Groq_message) <= 128, groq_message_is_less_than_96_bytes);
#define GROQ_MESSAGE_SIZE ((u64)128)

TYPEDEF_ARRAY(Groq_message);

typedef struct Embedding_vector Embedding_vector;
struct Embedding_vector {
  f32 *d;
  s64 count;
  Str8 text;
};
STATIC_ASSERT(IS_SLICE(Embedding_vector), embedding_vector_is_slice);
TYPEDEF_SLICE(Embedding_vector);


typedef u64 App_flags;
#define APP_QUIT              ((App_flags)(1ul << 0))


typedef struct App App;
struct App {
  App_flags flags;
  b32 did_reload;

  Arena *main_arena;
  Arena *frame_arena;
  Arena *temp_arena;

  CURL *curl;

  Str8 env_file;
  Groq_message_array all_messages;
  Groq_tool_array all_tools;

  Str8 env[ENV_COUNT];

  Str8 user_role;
  Str8 tool_role;
  Str8 system_role;
  Str8 assistant_role;

};
STATIC_ASSERT(sizeof(App) <= MB(1), app_state_is_less_than_a_megabyte);
#define APP_STATE_SIZE ((u64)MB(1))



internal void* json_arena_push(void *user_data, size_t size);

internal size_t my_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp);

internal void push_groq_user_message(App *ap, Str8 content);

internal void normalize_embedding_vector_in_place(App *ap, Embedding_vector v);

internal f32 dot_embedding_vectors(App *ap, Embedding_vector a, Embedding_vector b);

internal Embedding_vector_slice get_embedding_vectors_for_texts(App *ap, Str8_list texts);

internal void send_all_messages_to_groq(App *ap);

internal void push_groq_model_response_message(App *ap, Str8 groq_response_json);

internal void load_tools(App *ap);

internal void exec_tool_calls(App *ap, Groq_message message);

internal void parse_env_file(Str8 env_file, Str8 *env_dest, Str8 *env_var_str, int env_count);

internal App* app_init(void);

internal void app_update(App *ap);



#endif
