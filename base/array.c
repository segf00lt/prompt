#ifndef ARRAY_C
#define ARRAY_C

internal void
func arr_init_(__Arr_header *arr, Arena *arena, s64 stride, s64 cap) {
  arr->count = 0;
  arr->cap = cap;
  arr->arena = arena;
  arr->d = arena_push(arena, cap * stride, 1);
}

internal void
func slice_init_(__Slice_header *slice, Arena *arena, s64 stride, s64 count) {
  slice->count = count;
  slice->d = arena_push(arena, count * stride, 1);
}

internal void*
func arr_push_no_zero_(__Arr_header *arr, s64 stride, s64 push_count) {
  ASSERT(arr->d && arr->cap && arr->arena);

  if(arr->count + push_count >= arr->cap) {
    s64 new_cap = arr->cap << 1;

    while(new_cap < arr->count + push_count) {
      new_cap <<= 1;
    }

    void *new_d = arena_push(arr->arena, new_cap * stride, 1);
    memory_copy(new_d, arr->d, stride * (arr->count + push_count));
    arr->d = new_d;
    arr->cap = new_cap;
  }

  void *result = (u8*)(arr->d) + arr->count;
  arr->count += push_count;

  return result;
}

internal s64
func arr_dict_put_(__Arr_header *dict_array, u64 stride, u64 key_offset, Str8 new_key) {
  ASSERT(dict_array->d && dict_array->cap && dict_array->arena);
  ASSERT((u64)stride > sizeof(Str8));

  s64 hash = hash_key(new_key);
  s64 new_key_pos = hash % dict_array->cap;
  s64 new_key_pos_byte_offset = new_key_pos * stride;

  if(dict_array->count >= dict_array->cap) {
    s64 new_cap = dict_array->cap << 1;

    while(new_cap < dict_array->count + 1) {
      new_cap <<= 1;
    }

    void *new_d = arena_push(dict_array->arena, new_cap * stride, 1);
    memory_zero(new_d, new_cap);

    void *old_d = dict_array->d;
    s64 old_cap = dict_array->cap;
    dict_array->d = new_d;
    dict_array->cap = new_cap;

    { /* re-hash all elements */
      s64 max_bytes = old_cap * stride;
      for(s64 i = 0; i < max_bytes; i += stride) {
        void *elem_addr = (u8*)old_d + i;
        Str8 *key = (Str8*)elem_addr;

        if(key->s) {
          hash = hash_key(*key);
          s64 pos = hash % dict_array->cap;
          s64 pos_byte_offset = pos * stride;

          Str8 *key_at_pos = (Str8*)((u8*)dict_array->d + pos_byte_offset + key_offset);
          while(key_at_pos->s) {
            pos++;
            pos %= dict_array->cap;
            pos_byte_offset = pos * stride;
            key_at_pos = (Str8*)((u8*)dict_array->d + pos_byte_offset);
          }

          void *new_elem_addr = (u8*)dict_array->d + pos_byte_offset;
          memory_copy(new_elem_addr, elem_addr, stride);
        }

      }

    } /* re-hash all elements */

  }

  {
    Str8 *key_at_pos = (Str8*)((u8*)dict_array->d + new_key_pos_byte_offset + key_offset);
    b32 key_already_exists = false;

    while(key_at_pos->s) {
      if(str8_match(*key_at_pos, new_key)) {
        key_already_exists = true;
        break;
      } else {
        new_key_pos++;
        new_key_pos %= dict_array->cap;
        new_key_pos_byte_offset = new_key_pos * stride;
        key_at_pos = (Str8*)((u8*)dict_array->d + new_key_pos_byte_offset + key_offset);
      }
    }

    if(!key_already_exists) {
      dict_array->count += 1;
    }

    u8 *new_elem_addr = (u8*)dict_array->d + new_key_pos_byte_offset;
    Str8 *key_addr = (Str8*)(new_elem_addr + key_offset);
    memory_copy(key_addr, &new_key, sizeof(Str8));
  }

  return new_key_pos;
}

internal s64
func arr_dict_get_(__Arr_header *dict_array, u64 stride, u64 key_offset, Str8 key) {
  u64 hash = hash_key(key);
  s64 pos = hash % dict_array->cap;
  s64 pos_byte_offset = pos * stride;

  Str8 *key_at_pos = (Str8*)((u8*)dict_array->d + pos_byte_offset + key_offset);
  s64 start_search_pos = pos;
  while(!str8_match(*key_at_pos, key)) {
    pos++;
    pos %= dict_array->cap;
    pos_byte_offset = pos * stride;
    key_at_pos = (Str8*)((u8*)dict_array->d + pos_byte_offset + key_offset);
    if(pos == start_search_pos) {
      pos = -1;
      break;
    }
  }

  return pos;
}

internal u64
func hash_key(Str8 key) {
  u64 hash = 0;
  for(s64 i = 0; i < key.len; i++) {
    hash += (key.s[i] * 37) >> 1;
  }
  return hash;
}

#endif
