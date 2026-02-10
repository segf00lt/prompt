#ifndef PLATFORM_MACOS_C
#define PLATFORM_MACOS_C


typedef struct Curl_write_buffer Curl_write_buffer;
struct Curl_write_buffer {
  Arena *arena;
  Str8_list chunks;
};


typedef struct Groq_tool_parameter Groq_tool_parameter;
struct Groq_tool_parameter {
  Str8 name;
  Str8 type; // NOTE jfd: change this to an enum
  Str8 description;
};

TYPEDEF_SLICE(Groq_tool_parameter, Groq_tool_parameter_slice);

typedef struct Groq_tool Groq_tool;
struct Groq_tool {
  Str8 name;
  Str8 description;
  Groq_tool_parameter_slice parameters;
};

TYPEDEF_SLICE(Groq_tool, Groq_tool_array);

typedef struct Groq_tool_call Groq_tool_call;
struct Groq_tool_call {
  Str8 id;
  Str8 tool_name; // NOTE jfd: corresponds to function.name in the json
  json_value_t *arguments_json;
};

TYPEDEF_SLICE(Groq_tool_call, Groq_tool_call_slice);

typedef struct Groq_message Groq_message;
struct Groq_message {
  Str8 name;
  Str8 role;
  Str8 content;
  Str8 reasoning;
  Groq_tool_call_slice tool_calls;
};

TYPEDEF_ARRAY(Groq_message, Groq_message_array);



///////////////////////////
// globals

global Str8 env_file;
global Str8 groq_api_key;
global Str8 prompt_json;
global Groq_message_array all_messages;
global Groq_tool_array all_tools;
global Arena *platform_arena;


///////////////////////////
// headers

internal void* json_arena_push(void *user_data, size_t size);

internal size_t my_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp);

internal Groq_message create_groq_user_message(Str8 content);

internal void send_all_messages_to_groq(void);

internal Groq_message parse_groq_response_message(Arena *scratch, Str8 groq_response_json, Arena *output_arena);

internal void prompt_loop(void);





///////////////////////////
// functions

internal void*
func json_arena_push(void *user_data, size_t size) {
  Arena *arena = (Arena*)user_data;
  u64 bytes = size;
  return arena_push(arena, bytes, 1);
}

internal size_t
func my_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {

  Curl_write_buffer *write_buffer = (Curl_write_buffer*)userp;

  size_t bytes = size * nmemb;

  Str8 chunk = { .s = (u8*)contents, .len = bytes };

  chunk = str8_copy(write_buffer->arena, chunk);
  str8_list_append_str(write_buffer->arena, &write_buffer->chunks, chunk);

  return bytes;
}

internal Groq_message
func create_groq_user_message(Str8 content) {
  Groq_message message = {
    .role = str8_lit("user"),
    .content = content,
  };

  return message;
}

internal void
func send_all_messages_to_groq(void) {
  CURL *curl = curl_easy_init();
  ASSERT(curl);

  Arena *arena = arena_create_ex(KB(800), 0, 0);

  Curl_write_buffer write_buffer = {
    .arena = arena,
  };

  struct curl_slist *headers = 0;

  Str8 prompt_json = {0};
  {
    Str8_list prompt_str_list = {0};
    Str8_list messages_str_list = {0};
    Str8_list tools_str_list = {0};
    Str8 preamble = str8_lit(
      "{"
      "\"model\": \"openai/gpt-oss-120b\","
      "\"temperature\": 1,"
      "\"max_completion_tokens\": 8192,"
      "\"top_p\": 1,"
      "\"stream\": false,"
      "\"reasoning_effort\": \"medium\","
      "\"stop\": null,"
    );

    Str8 end = str8_lit("}");

    Str8 messages_begin = str8_lit("\"messages\": [");
    Str8 messages_end = str8_lit("],");

    for(s64 i = 0; i < all_messages.count; i++) {
      Str8 message_str = str8f(arena,
        "{ \"role\": \"%S\", \"content\": \"%S\" }",
        all_messages.d[i].role, str8_escaped(arena, all_messages.d[i].content)
      );
      str8_list_append_str(arena, &messages_str_list, message_str);
    }

    Str8 all_messages_json_str = str8_list_join(arena, messages_str_list, str8_lit(","));

    #if 0
    arena_scope(arena) {
      printf("all messages: %s\n", cstr_from_str8(arena, all_messages_json_str));
    }
    #endif


    Str8 tools_begin = str8_lit("\"tools\": [");
    Str8 tools_end = str8_lit("]"); // NOTE jfd: tools come last

    // for(s64 i = 0; i < all_tools.count; i++) {
    //   Str8 tool_str = str8f(arena,

    //   );
    // }
    Str8 all_tools_json_str = str8_list_join(arena, tools_str_list, str8_lit(","));


    str8_list_append_str(arena, &prompt_str_list, preamble);
    str8_list_append_str(arena, &prompt_str_list, messages_begin);
    str8_list_append_str(arena, &prompt_str_list, all_messages_json_str);
    str8_list_append_str(arena, &prompt_str_list, messages_end);
    str8_list_append_str(arena, &prompt_str_list, tools_begin);
    str8_list_append_str(arena, &prompt_str_list, all_tools_json_str);
    str8_list_append_str(arena, &prompt_str_list, tools_end);
    str8_list_append_str(arena, &prompt_str_list, end);

    prompt_json = str8_list_join(arena, prompt_str_list, (Str8){0});

    #if 0
    json_value_t *root = json_parse(prompt_json.s, prompt_json.len);

    char *indent = "  ";
    char *newline = "\n";
    size_t bytes;
    char *pretty = json_write_pretty(root, indent, newline, &bytes);

    printf("prompt_json:\n%s\n", pretty);
    #endif

  }

  arena_scope(arena) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.groq.com/openai/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_buffer);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
      cstr_from_str8(arena, prompt_json)
    );

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, cstrf(arena, "Authorization: Bearer %S", groq_api_key));

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  CURLcode res = curl_easy_perform(curl);
  if(res) {
    printf("curl command failed with error: %d\n", res);
  }

  Str8 curl_response = str8_list_join(arena, write_buffer.chunks, str8_lit(""));

  Groq_message response_message = parse_groq_response_message(arena, curl_response, platform_arena);

  arr_push(all_messages, response_message);

  #if 0
  json_value_t *root = json_parse(curl_response.s, curl_response.len);

  Str8 pretty;
  char *indent = "  ";
  char *newline = "\n";
  pretty.s = json_write_pretty(root, indent, newline, (size_t*)&pretty.len);
  pretty.len--;

  platform_write_entire_file(pretty, "output.json");
  #endif

  arena_destroy(arena);


  // curl_slist_free_all(headers);
  // curl_easy_cleanup(curl);

  // printf("curl result = %d\n", res == CURLE_OK);
}

internal Groq_message
func parse_groq_response_message(Arena *scratch, Str8 groq_response_json, Arena *output_arena) {

  Arena_scope scope = arena_scope_begin(scratch);

  Groq_message groq_response_message = {0};

  platform_write_entire_file(groq_response_json, "output_test.json");

  json_parse_result_t parse_result;
  json_value_t *root = json_parse_ex(
    groq_response_json.s,
    groq_response_json.len,
    json_parse_flags_allow_json5,
    json_arena_push,
    (void*)scratch,
    &parse_result
  );

  json_object_t *object = json_value_as_object(root);
  ASSERT(object);

  for(json_object_element_t *elem = object->start; elem; elem = elem->next) {
    json_string_t *key = elem->name;
    json_value_t *val  = elem->value;

    Str8 key_str;
    arena_scope(scratch) {
      key_str = str8_from_cstr(scratch, (char*)key->string);
    }

    if(str8_match_lit("choices", key_str)) {

      json_array_t *choices_array = json_value_as_array(val);

      json_array_element_t *last_choice = 0;
      for(json_array_element_t *array_elem = choices_array->start; array_elem; array_elem = array_elem->next) {
        if(array_elem->next == 0) {
          last_choice = array_elem;
        }
      }

      json_value_t *last_choice_val = last_choice->value;
      json_object_t *last_choice_object = json_value_as_object(last_choice_val);

      for(json_object_element_t *choice_elem = last_choice_object->start; choice_elem; choice_elem = choice_elem->next) {

        json_string_t *key = choice_elem->name;
        json_value_t *val  = choice_elem->value;

        Str8 key_str;
        arena_scope(scratch) {
          key_str = str8_from_cstr(scratch, (char*)key->string);
        }

        if(str8_match_lit("message", key_str)) {
          json_object_t *message_object = json_value_as_object(val);

          for(json_object_element_t *message_elem = message_object->start; message_elem; message_elem = message_elem->next) {
            json_string_t *key = message_elem->name;
            json_value_t *val  = message_elem->value;

            Str8 key_str;
            arena_scope(scratch) {
              key_str = str8_from_cstr(scratch, (char*)key->string);
            }

            if(str8_match_lit("content", key_str)) {
              json_string_t *val_str = json_value_as_string(val);

              groq_response_message.content = str8_from_cstr(output_arena, (char*)val_str->string);
            } else if(str8_match_lit("role", key_str)) {
              json_string_t *val_str = json_value_as_string(val);
              groq_response_message.role = str8_from_cstr(output_arena, (char*)val_str->string);

            } else if(str8_match_lit("reasoning", key_str)) {
              json_string_t *val_str = json_value_as_string(val);
              groq_response_message.reasoning = str8_from_cstr(output_arena, (char*)val_str->string);

            } else if(str8_match_lit("tool_calls", key_str)) {
              json_array_t *tool_call_val = json_value_as_array(val);

              s64 count = tool_call_val->length;
              Groq_tool_call *tool_calls = push_array(output_arena, Groq_tool_call, count);
              groq_response_message.tool_calls.count = count;
              groq_response_message.tool_calls.d     = tool_calls;
              s64 i = 0;

              for(json_array_element_t *tool_call_elem = tool_call_val->start; tool_call_elem; tool_call_elem = tool_call_elem->next) {
                json_object_t *tool_call_object = json_value_as_object(tool_call_elem->value);

                b32 parsed_id_field = false;
                b32 parsed_function_field = false;

                for(json_object_element_t *elem = tool_call_object->start; elem; elem = elem->next) {
                  json_string_t *key = elem->name;

                  Str8 key_str;
                  arena_scope(scratch) {
                    key_str = str8_from_cstr(scratch, (char*)key->string);
                  }

                  if(str8_match_lit("function", key_str)) {
                    json_object_t *function_object = json_value_as_object(elem->value);

                    for(json_object_element_t *elem = function_object->start; elem; elem = elem->next) {
                      json_string_t *key = elem->name;
                      json_value_t *val  = elem->value;

                      Str8 key_str;
                      arena_scope(scratch) {
                        key_str = str8_from_cstr(scratch, (char*)key->string);
                      }

                      if(str8_match_lit("name", key_str)) {

                        json_string_t *name_string = json_value_as_string(val);
                        ASSERT(name_string && name_string->string);
                        tool_calls[i].tool_name = str8_from_cstr(output_arena, (char*)name_string->string);

                      } else if(str8_match_lit("arguments", key_str)) {

                        json_string_t *arguments_json_string = json_value_as_string(val);

                        ASSERT(arguments_json_string && arguments_json_string->string);

                        json_value_t *arguments_json = json_parse_ex(
                          arguments_json_string->string,
                          memory_strlen(arguments_json_string->string),
                          0,
                          json_arena_push,
                          (void*)output_arena,
                          0
                        );

                        tool_calls[i].arguments_json = arguments_json;

                      }

                    }

                    parsed_function_field = true;

                  } else if(str8_match_lit("id", key_str)) {

                    json_string_t *id_string = json_value_as_string(elem->value);
                    ASSERT(id_string && id_string->string);
                    tool_calls[i].id = str8_from_cstr(output_arena, (char*)id_string->string);

                    parsed_id_field = true;

                  } else {

                    if(parsed_function_field && parsed_id_field) {
                      break;
                    }

                  }

                }

                if(parsed_function_field && parsed_id_field) {
                  i++;
                }

              }

              ASSERT(i == count);

            }

          }

          PASS;
        }

      }

      break; // NOTE jfd: once we've parsed choices[]

    } else if(str8_match_lit("model", key_str)) {
      // json_string_t *val_str = json_value_as_string(val);
      // printf("model: %s\n", val_str->string);

    } else if(str8_match_lit("id", key_str)) {
      // json_string_t *val_str = json_value_as_string(val);
      // printf("id: %s\n", val_str->string);

    }


  }

  arena_scope_end(scope);

  return groq_response_message;
}


internal void
func prompt_loop(void) {
  Arena *prompt_arena = arena_create(LINENOISE_MAX_LINE);

  for(;;) {
    char *line = linenoise("prompt> ");

    if(line) {

      Str8 content = str8_from_cstr(prompt_arena, line);

      Groq_message user_message = create_groq_user_message(content);

      arr_push(all_messages, user_message);

      send_all_messages_to_groq();

      printf("model response:\n\n%s\n\n", cstr_from_str8(prompt_arena, arr_last(all_messages).content));

      linenoiseFree(line);

    } else {
      break;
    }

    arena_clear(prompt_arena);
  }

}


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

  env_file = platform_read_entire_file(platform_arena, "./env");

  groq_api_key = str8_strip_whitespace(str8_slice(env_file, str8_find_char(env_file, '=') + 1, -1));

  {

    arr_init_ex(all_messages, platform_arena, 300);

    // Arena *response_message_output_arena = arena_create_ex(KB(500), 0, 0);
    // Str8 groq_response_message_json = platform_read_entire_file(platform_arena, "output.json");


    // HERE
    // TODO jfd:
    // - DONE Basic prompt interface
    // - DONE Send all_messages and all_tools in POST to groq api
    // - Generate array of tools immediate mode style
    // - Process tool calls and return results in all_messages

    // Groq_message message = parse_groq_response_message(platform_arena, groq_response_message_json, response_message_output_arena);
    // arr_push(all_messages, message);

    // printf("all messages count: %ld\n", all_messages.count);

    prompt_loop();

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
