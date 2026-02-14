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
func normalize_embedding_vector_in_place(App *ap, Embedding_vector v) {
  ASSERT(v.d);
  f32 dot = dot_embedding_vectors(ap, v, v);
  f32 inv_dot = 1.0f/sqrtf(dot);
  for(s64 i = 0; i < v.count; i++) {
    v.d[i] *= inv_dot;
  }
}

internal f32
func dot_embedding_vectors(App *ap, Embedding_vector a, Embedding_vector b) {
  ASSERT(a.d);
  ASSERT(b.d);
  ASSERT(a.count == b.count);

  f32 result = 0;
  s64 count = a.count;

  for(s64 i = 0; i < count; i++) {
    result += a.d[i] * b.d[i];
  }

  return result;
}

internal Embedding_vector_slice
func get_embedding_vectors_for_texts(App *ap, Str8_list texts) {
  int embedding_dimension = 1024;

  Embedding_vector_slice result;
  slice_init(result, texts.count, ap->main_arena);
  for(int i = 0; i < result.count; i++) {
    slice_init(result.d[i], embedding_dimension, ap->main_arena);
  }

  CURL *curl = ap->curl;
  ASSERT(curl);

  Curl_write_buffer write_buffer = {
    .arena = ap->frame_arena,
  };

  struct curl_slist *headers = 0;

  curl_easy_setopt(curl, CURLOPT_URL, "https://api.cohere.com/v2/embed");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_buffer);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  char *request_cstr = cstrf(ap->temp_arena,
    "{"
    "\"model\":\"embed-v4.0\","
    "\"texts\":[\"%S\"],"
    "\"input_type\":\"search_document\","
    "\"embedding_types\":[\"float\"],"
    "\"output_dimension\":%d"
    "}",
    str8_list_join(ap->temp_arena, texts, str8_lit("\",\"")),
    embedding_dimension
  );
  platform_write_entire_file(str8_from_cstr(ap->temp_arena, request_cstr), "tmp/cohere_request.json");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_cstr);

  headers = curl_slist_append(headers, "accept: application/json");
  headers = curl_slist_append(headers, "content-type: application/json");
  headers = curl_slist_append(headers, cstrf(ap->temp_arena, "Authorization: Bearer %S", ap->env[ENV_COHERE_API_KEY]));

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode res = curl_easy_perform(curl);

  if(res) {
    printf("curl command failed with error: %d\n", res);
  }

  curl_slist_free_all(headers);

  Str8 response_json = str8_list_join(ap->frame_arena, write_buffer.chunks, (Str8){0});

  json_parse_result_t parse_result;
  json_value_t *root = json_parse_ex(
    response_json.s,
    response_json.len,
    json_parse_flags_allow_json5,
    json_arena_push,
    (void*)ap->frame_arena,
    &parse_result
  );
  json_object_t *object = json_value_as_object(root);
  ASSERT(object);

  json_object_t *embeddings_object = 0;
  json_array_t *texts_array = 0;

  for(json_object_element_t *elem = object->start; elem; elem = elem->next) {
    Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);

    if(str8_match_lit("embeddings", key)) {
      json_value_t *val = elem->value;
      embeddings_object = json_value_as_object(val);
    } else if(str8_match_lit("texts", key)) {
      json_value_t *val = elem->value;
      texts_array = json_value_as_array(val);
    }

  }

  ASSERT(embeddings_object);
  ASSERT(texts_array);

  json_array_t *embeddings_float = 0;
  for(json_object_element_t *elem = embeddings_object->start; elem; elem = elem->next) {
    Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);

    if(str8_match_lit("float", key)) {
      json_value_t *val = elem->value;
      embeddings_float = json_value_as_array(val);
    }

  }
  ASSERT(embeddings_float);

  ASSERT(embeddings_float->length == (u64)result.count);

  {
    int i = 0;
    for(
      json_array_element_t *elem = embeddings_float->start, *texts_elem = texts_array->start;
      elem && texts_elem;
      elem = elem->next, texts_elem = texts_elem->next, i++
    ) {
      json_array_t *vector = json_value_as_array(elem->value);
      json_string_t *text = json_value_as_string(texts_elem->value);
      ASSERT(vector);
      ASSERT(text);

      ASSERT((int)vector->length == embedding_dimension);
      result.d[i].text = str8_from_cstr(ap->main_arena, (char*)text->string);

      int j = 0;
      for(json_array_element_t *elem = vector->start; elem; elem = elem->next, j++) {
        json_number_t *val = json_value_as_number(elem->value);
        ASSERT(val);
        Str8 number_str = { (u8*)val->number, (s64)val->number_size };
        f64 number = str8_parse_float(number_str);

        result.d[i].d[j] = (f32)number;

      }
      ASSERT(j == embedding_dimension);

    }

    ASSERT(i == result.count);

  }

  arena_clear(ap->temp_arena);

  return result;
}

internal void
func send_all_messages_to_groq(App *ap) {
  CURL *curl = ap->curl;
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
      Groq_message message = ap->all_messages.d[i];
      Str8 message_str = {0};

      if(str8_match_lit("tool", message.role)) {
        message_str = str8f(ap->frame_arena,
          "{ \"role\": \"%S\", \"content\": \"%S\", \"name\": \"%S\", \"tool_call_id\": \"%S\" }",
          message.role, str8_escaped(ap->frame_arena, message.content), message.name, message.tool_call_id
        );
      } else {
        message_str = str8f(ap->frame_arena,
          "{ \"role\": \"%S\", \"content\": \"%S\" }",
          message.role, str8_escaped(ap->frame_arena, message.content)
        );
      }

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
        TOOL_NAME_STR[tool.name], str8_escaped(ap->frame_arena, tool.description), tool_params_json, required_params_json
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
    platform_write_entire_file(prompt_json, "tmp/prompt_log.json");


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

    platform_write_entire_file((Str8){ .s = (u8*)pretty, .len = bytes - 1}, "tmp/prompt_log.json");
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
    headers = curl_slist_append(headers, cstrf(ap->frame_arena, "Authorization: Bearer %S", ap->env[ENV_GROQ_API_KEY]));

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  CURLcode res = curl_easy_perform(curl);
  if(res) {
    printf("curl command failed with error: %d\n", res);
  }

  Str8 curl_response = str8_list_join(ap->frame_arena, write_buffer.chunks, str8_lit(""));
  platform_write_entire_file(curl_response, "tmp/curl_response.json");

  push_groq_model_response_message(ap, curl_response);

  #if 0
  json_value_t *root = json_parse(curl_response.s, curl_response.len);

  Str8 pretty;
  char *indent = "  ";
  char *newline = "\n";
  pretty.s = json_write_pretty(root, indent, newline, (size_t*)&pretty.len);
  pretty.len--;

  platform_write_entire_file(pretty, "tmp/output.json");
  #endif



  curl_slist_free_all(headers);

}

internal void
func push_groq_model_response_message(App *ap, Str8 groq_response_json) {

  Groq_message groq_response_message = {0};

  platform_write_entire_file(groq_response_json, "tmp/output_test.json");

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

  json_array_t *choices_array = 0;
  json_object_t *error_object = 0;

  for(json_object_element_t *elem = object->start; elem; elem = elem->next) {
    json_value_t *val  = elem->value;

    Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);

    if(str8_match_lit("choices", key)) {
      choices_array = json_value_as_array(val);
      break; // NOTE jfd: once we've parsed choices[]

    } else if(str8_match_lit("error", key)) {
      error_object = json_value_as_object(val);

    } else if(str8_match_lit("model", key)) {
      // json_string_t *val_str = json_value_as_string(val);
      // printf("model: %s\n", val_str->string);

    } else if(str8_match_lit("id", key)) {
      // json_string_t *val_str = json_value_as_string(val);
      // printf("id: %s\n", val_str->string);

    }

  }

  if(error_object) {

    for(json_object_element_t *elem = error_object->start; elem; elem = elem->next) {
      json_value_t *val = elem->value;
      Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);
      if(str8_match_lit("code", key)) {
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

  } else {

    json_array_element_t *last_choice = 0;
    for(json_array_element_t *array_elem = choices_array->start; array_elem; array_elem = array_elem->next) {
      if(array_elem->next == 0) {
        last_choice = array_elem;
      }
    }

    json_value_t *last_choice_val = last_choice->value;
    json_object_t *last_choice_object = json_value_as_object(last_choice_val);

    json_object_t *message_object = 0;

    for(json_object_element_t *elem = last_choice_object->start; elem; elem = elem->next) {
      Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);
      if(str8_match_lit("message", key)) {
        message_object = json_value_as_object(elem->value);
        break;
      }

    }

    json_array_t *tool_call_array = 0;

    if(message_object) {

      for(json_object_element_t *elem = message_object->start; elem; elem = elem->next) {
        json_value_t *val  = elem->value;

        Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);
        if(str8_match_lit("content", key)) {
          json_string_t *val_str = json_value_as_string(val);
          groq_response_message.content = str8_from_cstr(ap->main_arena, (char*)val_str->string);

        } else if(str8_match_lit("role", key)) {
          json_string_t *val_str = json_value_as_string(val);
          groq_response_message.role = str8_from_cstr(ap->main_arena, (char*)val_str->string);

        } else if(str8_match_lit("reasoning", key)) {
          json_string_t *val_str = json_value_as_string(val);
          groq_response_message.reasoning = str8_from_cstr(ap->main_arena, (char*)val_str->string);

        } else if(str8_match_lit("tool_calls", key)) {
          tool_call_array = json_value_as_array(val);
        }

      }

    }


    if(tool_call_array) {

      s64 count = tool_call_array->length;
      Groq_tool_call *tool_calls = push_array(ap->main_arena, Groq_tool_call, count);
      groq_response_message.tool_calls.count = count;
      groq_response_message.tool_calls.d = tool_calls;
      s64 i = 0;

      for(json_array_element_t *tool_call_elem = tool_call_array->start; tool_call_elem; tool_call_elem = tool_call_elem->next) {
        json_object_t *tool_call_object = json_value_as_object(tool_call_elem->value);

        b32 parsed_id_field = false;
        b32 parsed_function_field = false;

        for(json_object_element_t *elem = tool_call_object->start; elem; elem = elem->next) {

          Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);
          if(str8_match_lit("function", key)) {
            json_object_t *function_object = json_value_as_object(elem->value);

            for(json_object_element_t *elem = function_object->start; elem; elem = elem->next) {
              json_value_t *val  = elem->value;

              Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);
              if(str8_match_lit("name", key)) {

                json_string_t *name_string = json_value_as_string(val);
                ASSERT(name_string && name_string->string);
                tool_calls[i].tool_name = str8_from_cstr(ap->main_arena, (char*)name_string->string);

              } else if(str8_match_lit("arguments", key)) {

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

          } else if(str8_match_lit("id", key)) {

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

    } /* if(tool_call_array) */


  } /* else */

  arena_clear(ap->temp_arena);
  arr_push(ap->all_messages, groq_response_message);

}


internal void
func load_tools(App *ap) {
  arr_init_ex(ap->all_tools, ap->main_arena, TOOL_COUNT);
  ap->all_tools.count = TOOL_COUNT;
  Groq_tool *all_tools = ap->all_tools.d;

  for(Tool_name tool_name = 0; tool_name < TOOL_COUNT; tool_name++) {
    switch(tool_name) {
      default:
      UNREACHABLE;
      break;

      case TOOL_SAY_HELLO: {
        Groq_tool *tool = &all_tools[tool_name];

        tool->name = tool_name;
        tool->description = str8_lit("This tool prints \"hello\" to the user.");
        Groq_tool_parameter parameters[] = {
          {
            .param_type = GROQ_TOOL_PARAM_STRING,
            .name = str8_lit("user_name"),
            .description = str8_lit("The name of the user to say \"hello\" to. Ask if you don't know."),
          },
        };

        slice_init(tool->parameters, ARRLEN(parameters), ap->main_arena);
        memory_copy(tool->parameters.d, parameters, sizeof(parameters));

      } break;

      case TOOL_SAY_GOODBYE: {

        Groq_tool *tool = &all_tools[tool_name];

        tool->name = tool_name;
        tool->description = str8_lit("This tool prints \"goodbye\" to the user.");
        Groq_tool_parameter parameters[] = {
          {
            .param_type = GROQ_TOOL_PARAM_STRING,
            .name = str8_lit("user_name"),
            .description = str8_lit("The name of the user to say \"goodbye\" to. Ask if you don't know."),
          },
        };

        slice_init(tool->parameters, ARRLEN(parameters), ap->main_arena);
        memory_copy(tool->parameters.d, parameters, sizeof(parameters));

      } break;

    }
  }
}

internal void
func exec_tool_calls(App *ap, Groq_message message) {
  Groq_tool_call_slice tool_calls = message.tool_calls;

  for(s64 i = 0; i < tool_calls.count; i++) {
    Groq_tool_call tool_call = tool_calls.d[i];

    b32 called = false;
    for(Tool_name tool_name = 0; tool_name < TOOL_COUNT && !called; tool_name++) {
      if(str8_match(TOOL_NAME_STR[tool_name], tool_call.tool_name)) {
        called = true;

        Groq_message result = {
          .role = str8_lit("tool"),
          .tool_call_id = tool_call.id,
          .name = TOOL_NAME_STR[tool_name],
        };

        switch(tool_name) {
          default:
          UNREACHABLE;
          break;

          case TOOL_SAY_HELLO: {

            json_object_t *args_object = json_value_as_object(tool_call.parameters);

            for(json_object_element_t *elem = args_object->start; elem; elem = elem->next) {
              Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);
              if(str8_match_lit("user_name", key)) {
                json_string_t *val = json_value_as_string(elem->value);
                if(val) {
                  result.content = str8f(ap->main_arena, "Hello there, %s!", val->string);
                } else {
                  result.content = str8_lit("bad 'user_name' parameter\n");
                }
              }
            }


          } break;

          case TOOL_SAY_GOODBYE: {


            json_object_t *args_object = json_value_as_object(tool_call.parameters);

            for(json_object_element_t *elem = args_object->start; elem; elem = elem->next) {
              Str8 key = str8_from_cstr(ap->temp_arena, (char*)elem->name->string);
              if(str8_match_lit("user_name", key)) {
                json_string_t *val = json_value_as_string(elem->value);
                if(val) {
                  result.content = str8f(ap->main_arena, "Goodbye, %s!", val->string);
                } else {
                  result.content = str8_lit("bad 'user_name' parameter\n");
                }
              }
            }

          } break;

        }

        arr_push(ap->all_messages, result);

      }
    }

    if(!called) {
      Groq_message result = {
        .role = str8_lit("assistant"),
        .content = str8f(ap->main_arena, "error: the requested tool '%S' is not available", tool_call.tool_name),
      };
      arr_push(ap->all_messages, result);
    }

    #if 0
    if(called) {
      printf("model called tool '%s'.\n", cstr_from_str8(ap->temp_arena, tool_call.tool_name));
      arena_clear(ap->temp_arena);
    } else {
      printf("model called tool '%s', but it doesn't exist\n", cstr_from_str8(ap->temp_arena, tool_call.tool_name));
      arena_clear(ap->temp_arena);
    }
    #endif

  }

}


internal void
func parse_env_file(Str8 env_file, Str8 *env_dest, Str8 *env_var_str, int env_count) {

  for(s64 pos = 0; pos < env_file.len;) {
    Str8 cur_line = str8_get_line_no_strip(env_file, pos);
    pos += cur_line.len;
    cur_line = str8_strip_whitespace(cur_line);

    if(cur_line.len == 0) {
      pos++;
      continue;
    }

    if(cur_line.s[0] == '#') {
      continue;
    }

    s64 equals_pos = str8_find_char(cur_line, '=');

    Str8 env_var_name = str8_strip_whitespace(str8_slice(cur_line, 0, equals_pos));
    Str8 env_var_value = str8_strip_whitespace(str8_slice(cur_line, equals_pos + 1, -1));

    for(int var = 0; var < ENV_COUNT; var++) {
      if(str8_match(env_var_name, env_var_str[var])) {
        env_dest[var] = env_var_value;
        break;
      }
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

  ap->curl = curl_easy_init();

  ap->env_file = platform_read_entire_file(ap->main_arena, "./env");
  parse_env_file(ap->env_file, ap->env, ENV_VAR_STR, ENV_COUNT);

  #if 0
  for(int var = 0; var < ENV_COUNT; var++) {
    printf("%s = %s\n", ENV_VAR_CSTR[var], cstr_from_str8(ap->temp_arena, ap->env[var]));
  }
  arena_clear(ap->temp_arena);
  #endif

  load_tools(ap);

  return ap;
}

internal void
func app_update_and_render(App *ap) {

  Groq_message response = arr_last(ap->all_messages);

  if(response.tool_calls.count > 0) {

    exec_tool_calls(ap, response);
    Groq_message tool_result = arr_last(ap->all_messages);
    printf("tool call result:\n\n%s\n\n", cstr_from_str8(ap->frame_arena, tool_result.content));

  } else {

    if(response.content.len > 0) {
      printf("model response:\n\n%s\n\n", cstr_from_str8(ap->frame_arena, response.content));
    }

    if(response.flags & GROQ_MESSAGE_FLAG_ERROR) {
      printf("error:\n\n%s\n\n", cstr_from_str8(ap->frame_arena, response.content));
    }

    char *line = linenoise("prompt> ");

    if(!line) {
      ap->flags |= APP_QUIT;
      return;
    } else {
      Str8 content = str8_from_cstr(ap->frame_arena, line);
      push_groq_user_message(ap, content);

      linenoiseFree(line);
    }

  }

  send_all_messages_to_groq(ap);

  arena_clear(ap->frame_arena);

}

#endif
