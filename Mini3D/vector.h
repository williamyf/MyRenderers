#pragma once

typedef struct { float x, y, z, w; } vector_t;
typedef vector_t point_t;

// �����ֵ��t Ϊ [0, 1] ֮�����ֵ
inline float interp(float x1, float x2, float t) { return x1 + (x2 - x1) * t; }

// | v |
float vector_length(const vector_t *v);

// z = x + y
void vector_add(vector_t *z, const vector_t *x, const vector_t *y);

// z = x - y
void vector_sub(vector_t *z, const vector_t *x, const vector_t *y);

// ʸ�����
float vector_dotproduct(const vector_t *x, const vector_t *y);

// ʸ�����
void vector_crossproduct(vector_t *z, const vector_t *x, const vector_t *y);

// ʸ����ֵ��tȡֵ [0, 1]
void vector_interp(vector_t *z, const vector_t *x1, const vector_t *x2, float t);

// ʸ����һ��
void vector_normalize(vector_t *v);
