#ifndef ARRAY_H
#define ARRAY_H

typedef struct __Arr_header __Arr_header;
struct __Arr_header {
  void *d;
  s64 count;
  s64 cap;

  Arena *arena;
};

typedef struct __Slice_header __Slice_header;
struct __Slice_header {
  void *d;
  s64 count;
};

#define TYPEDEF_ARRAY(T, name) \
typedef struct name name; \
struct name {                 \
  T *d;                           \
  s64 count;                      \
  s64 cap;                        \
  Arena *arena;                   \
};                                \

#define TYPEDEF_SLICE(T, name) \
typedef struct name name; \
struct name {                 \
  T *d;                           \
  s64 count;                      \
};                                \

#define ARRAY_DEFAULT_CAP 64

#define Arr(T)   struct { T *d; s64 count; s64 cap; Arena *arena; }
#define Slice(T) struct { T *d; s64 count; }

#define header_from_arr(arr)         (*(__Arr_header*)(void*)(&(arr)))
#define header_from_arr_ptr(arr)     (*(__Arr_header*)(void*)(arr))
#define header_ptr_from_arr(arr)     ((__Arr_header*)(void*)(&(arr)))
#define header_ptr_from_arr_ptr(arr) ((__Arr_header*)(void*)(arr))

#define header_from_slice(slice)         (*(__Slice_header*)(void*)(&(slice)))
#define header_from_slice_ptr(slice)     (*(__Slice_header*)(void*)(slice))
#define header_ptr_from_slice(slice)     ((__Slice_header*)(void*)(&(slice)))
#define header_ptr_from_slice_ptr(slice) ((__Slice_header*)(void*)(slice))

#define arr_stride(array) ((s64)sizeof(*((array).d)))
#define arr_ptr_stride(array) ((s64)sizeof(*((array)->d)))

#define arr_init(array, arena) arr_init_(header_ptr_from_arr((array)), arena, arr_stride(array), ARRAY_DEFAULT_CAP)
#define arr_init_ex(array, arena, cap) arr_init_(header_ptr_from_arr((array)), arena, arr_stride(array), (cap))

#define dict_init(dict, arena) dict_init_ex(dict, arena, DICT_DEFAULT_CAP)
#define dict_init_ex(dict, arena, cap) (memory_zero(&((dict).nil_item), sizeof(*&(dict).nil_item)), arr_init_(header_ptr_from_arr((dict)), arena, dict_stride(dict), cap))

#define arr_push(array, elem) ((arr_push_no_zero_(header_ptr_from_arr((array)), arr_stride(array), 1)), (array).d[(array).count-1] = (elem))
#define arr_push_n_ptr(array, n) ((arr_push_no_zero_(header_ptr_from_arr((array)), arr_stride(array), (n))), &((array).d[(array).count - (n)]))
#define arr_push_n_index(array, n) ((arr_push_no_zero_(header_ptr_from_arr((array)), arr_stride(array), (n))), (s64)((array).count - (n)))

#define arr_clear(array) ((array).count = 0, memory_zero((array).d, arr_stride((array)) * (array).cap))

#define arr_pop(array)        ( ( ((array).count > 0) ? ((array).count--) : (0) ), ( ((array).count <= 0) ? (arr_clear((array))) : (0) ), (array).d[(array).count] )

#define arr_del(array, i) ( (array).d[i] = arr_last(array), --(array).count )
// #define arr_del_ordered(array, i)

#define arr_first(array) ((array).d[0])

#define arr_last_index(array) ( ( ((array).count > 0) ? ((array).count - 1) : (0) ) )
#define arr_last(array) ( (array).d[ arr_last_index(array) ] )
#define arr_last_ptr(array) ( &((array).d[ arr_last_index(array) ]) )

// NOTE ~jfd 23/12/2025: unordered insert
#define arr_insert_unordered(array, i, elem) \
( \
  memory_copy(arr_push_no_zero_(header_ptr_from_arr((array)), arr_stride((array)), 1), (void*)&((array).d[i]), arr_stride((array))), \
  (array).d[i] = elem \
)

// NOTE ~jfd 23/12/2025: ordered insert
#define arr_insert(array, i, elem) \
( \
  arr_push_no_zero_(header_ptr_from_array((array)), arr_stride((array)), 1), \
  memory_copy((void*)&(array).d[i + 1], (void*)&(array).d[i], arr_stride((array)) * ((array).count - i)), \
  (array).d[i] = elem \
)

#define arr_to_slice(T, array) (*(Slice(T)*)(&(array)))
#define carray_to_slice(T, carray) ((Slice(T)){ .d = (T*)carray, .count = (sizeof(carray)/sizeof(T)) })

#define slice_last arr_last
#define slice_stride arr_stride

#define slice_init(slice, n, arena) (slice_init_(header_ptr_from_slice(slice), arena, slice_stride(slice), (n)), (slice).count = (s64)(n))

internal void  arr_init_(__Arr_header *arr, Arena *arena, s64 stride, s64 cap);
internal void* arr_push_no_zero_(__Arr_header *arr, s64 stride, s64 push_count);

internal void  slice_init_(__Slice_header *slice, Arena *arena, s64 stride, s64 count);

internal u64 hash_key(Str8 key);
internal s64 arr_dict_put_(__Arr_header *dict_array, u64 stride, u64 key_offset, Str8 new_key);
internal s64 arr_dict_get_(__Arr_header *dict_array, u64 stride, u64 key_offset, Str8 key);

#endif
