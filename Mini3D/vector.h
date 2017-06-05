#pragma once

typedef struct { float x, y, z, w; } vector_t;
typedef vector_t point_t;

// 计算插值：t 为 [0, 1] 之间的数值
inline float interp(float x1, float x2, float t) { return x1 + (x2 - x1) * t; }

// | v |
float vector_length(const vector_t *v);

// z = x + y
void vector_add(vector_t *z, const vector_t *x, const vector_t *y);

// z = x - y
void vector_sub(vector_t *z, const vector_t *x, const vector_t *y);

// 矢量点乘
float vector_dotproduct(const vector_t *x, const vector_t *y);

// 矢量叉乘
void vector_crossproduct(vector_t *z, const vector_t *x, const vector_t *y);

// 矢量插值，t取值 [0, 1]
void vector_interp(vector_t *z, const vector_t *x1, const vector_t *x2, float t);

// 矢量归一化
void vector_normalize(vector_t *v);
