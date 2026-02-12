#ifndef PROMPT_C
#define PROMPT_C


///////////////////////////
// globals



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

internal void
func push_groq_user_message(App *ap, Str8 content) {
  Groq_message message = {
    .role = str8_lit("user"),
    .content = str8_copy(ap->main_arena, content),
  };

  arr_push(ap->all_messages, message);
}

internal void
func send_all_messages_to_groq(App *ap) {
  CURL *curl = curl_easy_init();
  ASSERT(curl);

  Curl_write_buffer write_buffer = {
    .arena = ap->frame_arena,
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
      "\"temperature\": 0,"
      "\"max_completion_tokens\": 8192,"
      "\"top_p\": 1,"
      "\"stream\": false,"
      "\"reasoning_effort\": \"medium\","
      "\"stop\": null,"
    );

    Str8 end = str8_lit("}");

    Str8 messages_begin = str8_lit("\"messages\": [");
    Str8 messages_end = str8_lit("],");

    for(s64 i = 0; i < ap->all_messages.count; i++) {
      Str8 message_str = str8f(ap->frame_arena,
        "{ \"role\": \"%S\", \"content\": \"%S\" }",
        ap->all_messages.d[i].role, str8_escaped(ap->frame_arena, ap->all_messages.d[i].content)
      );
      str8_list_append_str(ap->frame_arena, &messages_str_list, message_str);
    }

    Str8 all_messages_json_str = str8_list_join(ap->frame_arena, messages_str_list, str8_lit(","));

    #if 0
    arena_scope(ap->frame_arena) {
      printf("all messages: %s\n", cstr_from_str8(ap->frame_arena, all_messages_json_str));
    }
    #endif


    Str8 tools_begin = str8_lit("\"tools\": [");
    Str8 tools_end = str8_lit("]"); // NOTE jfd: tools come last

    for(s64 i = 0; i < ap->all_tools.count; i++) {

      Groq_tool tool = ap->all_tools.d[i];

      Str8_list tool_param_list = {0};
      Str8 tool_params_json = {0};
      Str8 required_params_json = {0};
      Str8_list required_param_list = {0};
      Str8 tool_param_list_sep = str8_lit(",");
      Str8 required_param_list_sep = str8_lit("\",\"");

      for(s64 i = 0; i < tool.parameters.count; i++) {
        Groq_tool_parameter param = tool.parameters.d[i];

        switch(param.param_type) {
          default:
          UNREACHABLE;
          break;
          case GROQ_TOOL_PARAM_NUMBER: {
            UNIMPLEMENTED;
          } break;
          case GROQ_TOOL_PARAM_INTEGER: {
            UNIMPLEMENTED;
          } break;
          case GROQ_TOOL_PARAM_BOOLEAN: {
            UNIMPLEMENTED;
          } break;
          case GROQ_TOOL_PARAM_STRING: {

            if(param.accepted_string_values.count > 0) {
              UNIMPLEMENTED;
            } else {
              str8_list_append_str(ap->frame_arena, &tool_param_list,
                str8f(ap->frame_arena, "\"%S\": { \"type\": \"string\", \"description\": \"%S\" }", param.name, str8_escaped(ap->frame_arena, param.description))
              );
            }

          } break;
          case GROQ_TOOL_PARAM_ARRAY: {
            UNIMPLEMENTED;
          } break;
          case GROQ_TOOL_PARAM_OBJECT: {
            UNIMPLEMENTED;
          } break;
        }

        if(!param.is_optional) {
          str8_list_append_str(ap->frame_arena, &required_param_list, param.name);
        }

      }

      tool_params_json = str8_list_join(ap->frame_arena, tool_param_list, tool_param_list_sep);
      required_params_json = str8_list_join(ap->frame_arena, required_param_list, required_param_list_sep);

      Str8 tool_str = str8f(ap->frame_arena,
        "{ \"type\": \"function\", \"function\": {"
        "\"name\": \"%S\", \"description\": \"%S\","
        "\"parameters\": { \"type\": \"object\", \"properties\": { %S }, \"required\": [ \"%S\" ] }"
        "} }",
        tool.name, str8_escaped(ap->frame_arena, tool.description), tool_params_json, required_params_json
      );
      str8_list_append_str(ap->frame_arena, &tools_str_list, tool_str);

    }
    Str8 all_tools_json_str = str8_list_join(ap->frame_arena, tools_str_list, str8_lit(","));


    str8_list_append_str(ap->frame_arena, &prompt_str_list, preamble);
    str8_list_append_str(ap->frame_arena, &prompt_str_list, messages_begin);
    str8_list_append_str(ap->frame_arena, &prompt_str_list, all_messages_json_str);
    str8_list_append_str(ap->frame_arena, &prompt_str_list, messages_end);
    str8_list_append_str(ap->frame_arena, &prompt_str_list, tools_begin);
    str8_list_append_str(ap->frame_arena, &prompt_str_list, all_tools_json_str);
    str8_list_append_str(ap->frame_arena, &prompt_str_list, tools_end);
    str8_list_append_str(ap->frame_arena, &prompt_str_list, end);

    prompt_json = str8_list_join(ap->frame_arena, prompt_str_list, (Str8){0});
    platform_write_entire_file(prompt_json, "prompt_log.json");


    #if 0
    json_parse_result_t parse_result;
    json_value_t *root = json_parse_ex(
      prompt_json.s,
      prompt_json.len,
      json_parse_flags_allow_json5,
      json_arena_push,
      (void*)ap->frame_arena,
      &parse_result
    );

    printf("prompt_json.len = %li\n", prompt_json.len);
    ASSERT(prompt_json.len > 0);
    ASSERT(root);

    char *indent = "  ";
    char *newline = "\n";
    size_t bytes;
    char *pretty = json_write_pretty(root, indent, newline, &bytes);
    printf("bytes = %zu\n", bytes);

    platform_write_entire_file((Str8){ .s = (u8*)pretty, .len = bytes - 1}, "prompt_log.json");
    // printf("prompt_json:\n%s\n", pretty);
    #endif

  }

  arena_scope(ap->frame_arena) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.groq.com/openai/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_buffer);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
      cstr_from_str8(ap->frame_arena, prompt_json)
    );

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, cstrf(ap->frame_arena, "Authorization: Bearer %S", ap->groq_api_key));

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  CURLcode res = curl_easy_perform(curl);
  if(res) {
    printf("curl command failed with error: %d\n", res);
  }

  Str8 curl_response = str8_list_join(ap->frame_arena, write_buffer.chunks, str8_lit(""));
  platform_write_entire_file(curl_response, "curl_response.json");

  // Groq_message response_message = parse_groq_response_message(ap->frame_arena, curl_response, platform_arena);
  push_groq_model_response_message(ap, curl_response);

  #if 0
  json_value_t *root = json_parse(curl_response.s, curl_response.len);

  Str8 pretty;
  char *indent = "  ";
  char *newline = "\n";
  pretty.s = json_write_pretty(root, indent, newline, (size_t*)&pretty.len);
  pretty.len--;

  platform_write_entire_file(pretty, "output.json");
  #endif



  // curl_slist_free_all(headers);
  // curl_easy_cleanup(curl);

  // printf("curl result = %d\n", res == CURLE_OK);
}

internal void
func push_groq_model_response_message(App *ap, Str8 groq_response_json) {

  Groq_message groq_response_message = {0};

  platform_write_entire_file(groq_response_json, "output_test.json");

  json_parse_result_t parse_result;
  json_value_t *root = json_parse_ex(
    groq_response_json.s,
    groq_response_json.len,
    json_parse_flags_allow_json5,
    json_arena_push,
    (void*)ap->frame_arena,
    &parse_result
  );

  json_object_t *object = json_value_as_object(root);
  ASSERT(object);

  for(json_object_element_t *elem = object->start; elem; elem = elem->next) {
    json_string_t *key = elem->name;
    json_value_t *val  = elem->value;

    Str8 key_str;
    arena_scope(ap->frame_arena) {
      key_str = str8_from_cstr(ap->frame_arena, (char*)key->string);
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
        arena_scope(ap->frame_arena) {
          key_str = str8_from_cstr(ap->frame_arena, (char*)key->string);
        }

        if(str8_match_lit("message", key_str)) {
          json_object_t *message_object = json_value_as_object(val);

          for(json_object_element_t *message_elem = message_object->start; message_elem; message_elem = message_elem->next) {
            json_string_t *key = message_elem->name;
            json_value_t *val  = message_elem->value;

            Str8 key_str;
            arena_scope(ap->frame_arena) {
              key_str = str8_from_cstr(ap->frame_arena, (char*)key->string);
            }

            if(str8_match_lit("content", key_str)) {
              json_string_t *val_str = json_value_as_string(val);

              groq_response_message.content = str8_from_cstr(ap->main_arena, (char*)val_str->string);
            } else if(str8_match_lit("role", key_str)) {
              json_string_t *val_str = json_value_as_string(val);
              groq_response_message.role = str8_from_cstr(ap->main_arena, (char*)val_str->string);

            } else if(str8_match_lit("reasoning", key_str)) {
              json_string_t *val_str = json_value_as_string(val);
              groq_response_message.reasoning = str8_from_cstr(ap->main_arena, (char*)val_str->string);

            } else if(str8_match_lit("tool_calls", key_str)) {
              json_array_t *tool_call_val = json_value_as_array(val);

              s64 count = tool_call_val->length;
              Groq_tool_call *tool_calls = push_array(ap->main_arena, Groq_tool_call, count);
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
                  arena_scope(ap->frame_arena) {
                    key_str = str8_from_cstr(ap->frame_arena, (char*)key->string);
                  }

                  if(str8_match_lit("function", key_str)) {
                    json_object_t *function_object = json_value_as_object(elem->value);

                    for(json_object_element_t *elem = function_object->start; elem; elem = elem->next) {
                      json_string_t *key = elem->name;
                      json_value_t *val  = elem->value;

                      Str8 key_str;
                      arena_scope(ap->frame_arena) {
                        key_str = str8_from_cstr(ap->frame_arena, (char*)key->string);
                      }

                      if(str8_match_lit("name", key_str)) {

                        json_string_t *name_string = json_value_as_string(val);
                        ASSERT(name_string && name_string->string);
                        tool_calls[i].tool_name = str8_from_cstr(ap->main_arena, (char*)name_string->string);

                      } else if(str8_match_lit("arguments", key_str)) {

                        json_string_t *arguments_json_string = json_value_as_string(val);

                        ASSERT(arguments_json_string && arguments_json_string->string);

                        json_value_t *arguments_json = json_parse_ex(
                          arguments_json_string->string,
                          memory_strlen(arguments_json_string->string),
                          0,
                          json_arena_push,
                          (void*)ap->main_arena,
                          0
                        );

                        tool_calls[i].parameters = arguments_json;

                      }

                    }

                    parsed_function_field = true;

                  } else if(str8_match_lit("id", key_str)) {

                    json_string_t *id_string = json_value_as_string(elem->value);
                    ASSERT(id_string && id_string->string);
                    tool_calls[i].id = str8_from_cstr(ap->main_arena, (char*)id_string->string);

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

    } else if(str8_match_lit("error", key_str)) {
      json_object_t *val_object = json_value_as_object(val);

      for(json_object_element_t *elem = val_object->start; elem; elem = elem->next) {
        json_string_t *key = elem->name;
        json_value_t *val = elem->value;
        Str8 key_str;
        arena_scope(ap->frame_arena) {
          key_str = str8_from_cstr(ap->frame_arena, (char*)key->string);
        }

        if(str8_match_lit("code", key_str)) {
          json_string_t *error_code = json_value_as_string(val);
          Str8 error_code_str;
          arena_scope(ap->frame_arena) {
            error_code_str = str8_from_cstr(ap->frame_arena, (char*)error_code->string);
          }

          if(str8_match_lit("rate_limit_exceeded", error_code_str)) {
            groq_response_message.flags |=
              GROQ_MESSAGE_FLAG_ERROR |
              0;
            groq_response_message.role = str8_lit("system");
            groq_response_message.content = str8_lit("model rate limit exceeded");
          }
          break;
        }

      }

    } else if(str8_match_lit("model", key_str)) {
      // json_string_t *val_str = json_value_as_string(val);
      // printf("model: %s\n", val_str->string);

    } else if(str8_match_lit("id", key_str)) {
      // json_string_t *val_str = json_value_as_string(val);
      // printf("id: %s\n", val_str->string);

    }


  }

  arr_push(ap->all_messages, groq_response_message);

}

internal void
func say_hello(App *ap, json_value_t *arguments) {
  json_object_t *args_object = json_value_as_object(arguments);

  for(json_object_element_t *elem = args_object->start; elem; elem = elem->next) {
    Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);
    if(str8_match_lit("user_name", key)) {
      json_string_t *val = json_value_as_string(elem->value);
      if(val) {
        Str8 user_name = str8_from_cstr(ap->temp_arena, (char*)val->string);
        printf("say_hello: Hello there, %s!\n", cstr_from_str8(ap->temp_arena, user_name));
      } else {
        printf("bad parameter to say_hello\n");
      }
    }
  }

  arena_clear(ap->temp_arena);
}

internal void
func create_tool_hello(App *ap) {

  Groq_tool hello_tool;
  hello_tool.name = str8_lit("hello");
  hello_tool.description = str8_lit("This tool prints \"hello\" to the user.");
  hello_tool.callback = say_hello;

  s64 n_params = 1;
  slice_init(hello_tool.parameters, n_params, ap->main_arena);

  s64 i = 0;

  hello_tool.parameters.d[i] = (Groq_tool_parameter) {
    .param_type = GROQ_TOOL_PARAM_STRING,
    .name = str8_lit("user_name"),
    .description = str8_lit("The name of the user to say \"hello\" to. Ask if you don't know."),
  };
  i++;

  arr_push(ap->all_tools, hello_tool);

}

internal void
func exec_tool_calls(App *ap, Groq_message message) {
  Groq_tool_call_slice tool_calls = message.tool_calls;

  for(s64 i = 0; i < tool_calls.count; i++) {
    Groq_tool_call tool_call = tool_calls.d[i];

    b32 called = false;
    for(s64 i = 0; i < ap->all_tools.count; i++) {
      Groq_tool tool = ap->all_tools.d[i];
      if(str8_match(tool_call.tool_name, tool.name)) {
        called = true;

        tool.callback(ap, tool_call.parameters);

      }
    }

    if(called) {
      printf("model called tool '%s'.\n", cstr_from_str8(ap->temp_arena, tool_call.tool_name));
      arena_clear(ap->temp_arena);
    } else {
      printf("model called tool '%s', but it doesn't exist\n", cstr_from_str8(ap->temp_arena, tool_call.tool_name));
      arena_clear(ap->temp_arena);
    }
  }

}

internal App*
func app_init(void) {
  App *ap = platform_alloc(APP_STATE_SIZE);

  ASSERT(ap);

  ap->main_arena  = arena_create_ex(MB(4), 0, 0);
  ap->frame_arena = arena_create_ex(MB(1), 0, 0);
  ap->temp_arena  = arena_create_ex(MB(1), 0, 0);

  arr_init_ex(ap->all_messages, ap->main_arena, 1000);
  arr_init_ex(ap->all_tools, ap->main_arena, 100);

  ap->env_file = platform_read_entire_file(ap->main_arena, "./env");
  ap->groq_api_key = str8_strip_whitespace(str8_slice(ap->env_file, str8_find_char(ap->env_file, '=') + 1, -1));

  create_tool_hello(ap);

  return ap;
}

internal void
func app_update_and_render(App *ap) {

  char *line = linenoise("prompt> ");

  if(!line) {
    ap->flags |= APP_FLAG_QUIT;
  } else {

    Str8 content = str8_from_cstr(ap->frame_arena, line);

    push_groq_user_message(ap, content);

    send_all_messages_to_groq(ap);

    Groq_message response = arr_last(ap->all_messages);

    if(response.tool_calls.count > 0) {
      exec_tool_calls(ap, response);
    }

    if(response.flags & GROQ_MESSAGE_FLAG_ERROR) {
      printf("error:\n\n%s\n\n", cstr_from_str8(ap->frame_arena, response.content));
    } else {
      if(response.content.len > 0) {
        printf("model response:\n\n%s\n\n", cstr_from_str8(ap->frame_arena, response.content));
      } else {
        printf("model didn't reply.\n");
      }
    }


    linenoiseFree(line);

  }

  arena_clear(ap->frame_arena);

}

#endif
