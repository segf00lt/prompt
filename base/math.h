#ifndef BASE_MATH_H
#define BASE_MATH_H

#define TYPEDEF_VEC2(T) typedef struct v2_##T { T x; T y; } v2_##T;
#define TYPEDEF_VEC3(T) typedef struct v3_##T { T x; T y; T z; } v3_##T;


TYPEDEF_VEC2(s8);
TYPEDEF_VEC2(s16);
TYPEDEF_VEC2(s32);
TYPEDEF_VEC2(s64);

TYPEDEF_VEC2(u8);
TYPEDEF_VEC2(u16);
TYPEDEF_VEC2(u32);
TYPEDEF_VEC2(u64);

TYPEDEF_VEC2(f32);
TYPEDEF_VEC2(f64);

typedef v2_f32 v2;

TYPEDEF_VEC3(s8);
TYPEDEF_VEC3(s16);
TYPEDEF_VEC3(s32);
TYPEDEF_VEC3(s64);

TYPEDEF_VEC3(u8);
TYPEDEF_VEC3(u16);
TYPEDEF_VEC3(u32);
TYPEDEF_VEC3(u64);

TYPEDEF_VEC3(f32);
TYPEDEF_VEC3(f64);

typedef v3_f32 v3;

force_inline s32 round_f32_to_s32(f32 value);
force_inline f32 max_f32(f32 a, f32 b);
force_inline f32 abs_f32(f32 x);
force_inline f32 lerp_f32(f32 t, f32 begin, f32 end);
force_inline f32 wrap_f32(f32 value, f32 min, f32 max);

force_inline v2 truncate_v2(v2 v);
force_inline v2 wrap_v2(v2 v, v2 min, v2 max);
force_inline v2 max_v2(v2 a, v2 b);
force_inline v2 abs_v2(v2 v);

force_inline v2 add_v2(v2 a, v2 b);
force_inline v2 sub_v2(v2 a, v2 b);
force_inline v2 mul_v2(v2 a, v2 b);
force_inline v2 div_v2(v2 a, v2 b);
force_inline v2 scale_v2(v2 v, f32 a);
force_inline v2 lt_v2(v2 a, v2 b);
force_inline v2 gte_v2(v2 a, v2 b);
force_inline v2 lte_v2(v2 a, v2 b);
force_inline v2 gt_v2(v2 a, v2 b);
force_inline v2 add_value_v2(v2 v, f32 a);

force_inline v2_s16 add_v2_s16(v2_s16 a, v2_s16 b);
force_inline v2_s16 sub_v2_s16(v2_s16 a, v2_s16 b);
force_inline v2_s16 mul_v2_s16(v2_s16 a, v2_s16 b);
force_inline v2_s16 div_v2_s16(v2_s16 a, v2_s16 b);
force_inline v2_s16 scale_v2_s16(v2_s16 v, s16 a);
force_inline v2_s16 lt_v2_s16(v2_s16 a, v2_s16 b);
force_inline v2_s16 gte_v2_s16(v2_s16 a, v2_s16 b);
force_inline v2_s16 lte_v2_s16(v2_s16 a, v2_s16 b);
force_inline v2_s16 gt_v2_s16(v2_s16 a, v2_s16 b);
force_inline v2_s16 add_value_v2_s16(v2_s16 v, s16 a);

force_inline v2_s32 add_v2_s32(v2_s32 a, v2_s32 b);
force_inline v2_s32 sub_v2_s32(v2_s32 a, v2_s32 b);
force_inline v2_s32 mul_v2_s32(v2_s32 a, v2_s32 b);
force_inline v2_s32 div_v2_s32(v2_s32 a, v2_s32 b);
force_inline v2_s32 scale_v2_s32(v2_s32 v, s32 a);
force_inline v2_s32 lt_v2_s32(v2_s32 a, v2_s32 b);
force_inline v2_s32 gte_v2_s32(v2_s32 a, v2_s32 b);
force_inline v2_s32 lte_v2_s32(v2_s32 a, v2_s32 b);
force_inline v2_s32 gt_v2_s32(v2_s32 a, v2_s32 b);
force_inline v2_s32 add_value_v2_s32(v2_s32 v, s32 a);

force_inline v2_u32 add_v2_u32(v2_u32 a, v2_u32 b);
force_inline v2_u32 sub_v2_u32(v2_u32 a, v2_u32 b);
force_inline v2_u32 mul_v2_u32(v2_u32 a, v2_u32 b);
force_inline v2_u32 div_v2_u32(v2_u32 a, v2_u32 b);
force_inline v2_u32 scale_v2_u32(v2_u32 v, u32 a);
force_inline v2_u32 lt_v2_u32(v2_u32 a, v2_u32 b);
force_inline v2_u32 gte_v2_u32(v2_u32 a, v2_u32 b);
force_inline v2_u32 lte_v2_u32(v2_u32 a, v2_u32 b);
force_inline v2_u32 gt_v2_u32(v2_u32 a, v2_u32 b);
force_inline v2_u32 add_value_v2_u32(v2_u32 v, u32 a);

force_inline v2_s32 cast_v2_f32_to_s32(v2 v);
force_inline v2_u32 cast_v2_f32_to_u32(v2 v);
force_inline v2     cast_v2_u32_to_f32(v2_u32 v);
force_inline v2     cast_v2_s32_to_f32(v2_s32 v);

#endif
