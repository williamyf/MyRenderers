#include "device.h"
#include <windows.h>
#include <assert.h>
#include "matrix.h"
#include <iostream>
#include <fstream>

int CMID(int x, int min, int max) { return (x < min) ? min : ((x > max) ? max : x); }

// ������£����� transform = world * view * projection
void transform_update(transform_t *ts) {
	matrix_t m;
	matrix_mul(&m, &ts->world, &ts->view);
	matrix_mul(&ts->transform, &m, &ts->projection);
}

// ��ʼ����������Ļ����
void transform_init(transform_t *ts, int width, int height) {
	float aspect = (float)width / ((float)height);
	matrix_set_identity(&ts->world);
	matrix_set_identity(&ts->view);
	matrix_set_perspective(&ts->projection, 3.1415926f * 0.5f, aspect, 1.0f, 500.0f);
	ts->w = (float)width;
	ts->h = (float)height;
	transform_update(ts);
}

// ��ʸ�� x ���� project 
void transform_apply(const transform_t *ts, vector_t *y, const vector_t *x) 
{
	matrix_apply(y, x, &ts->transform);
}

// ��Ⱦ�豸��ʼ����fbΪ�ⲿ֡���棬��NULL�������ⲿ֡���棨ÿ�� 4�ֽڶ��룩
void device_init(device_t *device, int width, int height, void *fb) 
{
	int need = sizeof(void*) * (height * 2 + 1024) + width * height * 8;
	char *ptr = (char*)malloc(need + 64);
	assert(ptr);
	device->framebuffer = (unsigned int **)ptr;
	
	device->zbuffer = (float**)(ptr + sizeof(void*) * height);

	std::cout <<"zbuffer �� framebuffer ����ֽ�����"<< ((char*)device->zbuffer - (char*)device->framebuffer) << std::endl;

	ptr += sizeof(void*) * height * 2;
	device->texture = (unsigned int**)ptr;

	ptr += sizeof(void*) * 1024;

	char* framebuf = (char*)ptr;
	if (fb != NULL) framebuf = (char*)fb;
	char* zbuf = (char*)ptr + width * height * 4;
	for (int j = 0; j < height; j++) 
	{
		device->framebuffer[j] = (unsigned int*)(framebuf + width * 4 * j);
		device->zbuffer[j] = (float*)(zbuf + width * 4 * j);
	}

	ptr += width * height * 8;
	device->texture[0] = (unsigned int*)ptr;
	device->texture[1] = (unsigned int*)(ptr + 16);
	memset(device->texture[0], 0, 64);
	device->tex_width = 2;
	device->tex_height = 2;
	device->max_u = 1.0f;
	device->max_v = 1.0f;
	device->width = width;
	device->height = height;
	device->background = 0xf0c0f0;
	device->foreground = 0x0000f0;

	transform_init(&device->transform, width, height);// �˾�ʵ������������һ��ͶӰ����
	device->render_state = RENDER_STATE_WIREFRAME;
}

void camera_at_zero(device_t *device, float x, float y, float z) 
{
	point_t eye = { x, y, z, 1 }, at = { 0, 0, 0, 1 }, up = { 0, 0, 1, 1 };
	matrix_set_lookat(&device->transform.view, &eye, &at, &up);
	transform_update(&device->transform);
}

// ���õ�ǰ����
void device_set_texture(device_t *device, void *bits, long pitch, int w, int h) {
	char *ptr = (char*)bits;
	int j;
	assert(w <= 1024 && h <= 1024);
	for (j = 0; j < h; ptr += pitch, j++) 	// ���¼���ÿ�������ָ��
		device->texture[j] = (unsigned int *)ptr;
	device->tex_width = w;
	device->tex_height = h;
	device->max_u = (float)(w - 1);
	device->max_v = (float)(h - 1);
}

void init_texture(device_t *device) 
{
	//static unsigned int texture[256][256];
	//int i, j;
	//for (j = 0; j < 256; j++) {
	//	for (i = 0; i < 256; i++) {
	//		int x = i / 32, y = j / 32;
	//		texture[j][i] = ((x + y) & 1) ? 0xffffff : 0x3fbcef;
	//	}
	//}
	//device_set_texture(device, texture, 256 * 4, 256, 256);

	int w = 460, h = 613;
	unsigned int *texture = new unsigned int[w*h];
	std::ifstream ifs("ͼ������.txt");
	for (int i=0; i<w*h; i++)
	{
		int r, g, b;
		ifs >> r >> g >> b;
		texture[i] = b << 16 | g << 8 | r;
	}
	ifs.close();
	device_set_texture(device, (void*)texture, w * 4, w, h);
}

// ��� framebuffer �� zbuffer
void device_clear(device_t *device, int mode) {
	int y, x, height = device->height;
	for (y = 0; y < device->height; y++) {
		unsigned int *dst = device->framebuffer[y];
		unsigned int cc = (height - 1 - y) * 230 / (height - 1);
		cc = (cc << 16) | (cc << 8) | cc;
		if (mode == 0) cc = device->background;
		for (x = device->width; x > 0; dst++, x--) dst[0] = cc;
	}
	for (y = 0; y < device->height; y++) {
		float *dst = device->zbuffer[y];
		for (x = device->width; x > 0; dst++, x--) dst[0] = 0.0f;
	}
}













// ����������ͬ cvv �ı߽�������׶�ü�
int transform_check_cvv(const vector_t *v) 
{
	float w = v->w;
	int check = 0;
	if (v->z < 0.0f) check |= 1;
	if (v->z > w) check |= 2;
	if (v->x < -w) check |= 4;
	if (v->x > w) check |= 8;
	if (v->y < -w) check |= 16;
	if (v->y > w) check |= 32;
	return check;
}

// ��һ�����õ���Ļ����
void transform_homogenize(const transform_t *ts, vector_t *y, const vector_t *x) {
	float rhw = 1.0f / x->w;
	y->x = (x->x * rhw + 1.0f) * ts->w * 0.5f;
	y->y = (1.0f - x->y * rhw) * ts->h * 0.5f;
	y->z = x->z * rhw;
	y->w = 1.0f;
}


//=====================================================================
// ���μ��㣺���㡢ɨ���ߡ���Ե�����Ρ���������
//=====================================================================
typedef struct { float r, g, b; } color_t;
typedef struct { float u, v; } texcoord_t;
typedef struct { point_t pos; texcoord_t tc; color_t color; float rhw; } vertex_t;

typedef struct { vertex_t v, v1, v2; } edge_t;
typedef struct { float top, bottom; edge_t left, right; } trapezoid_t;
typedef struct { vertex_t v, step; int x, y, w; } scanline_t;

void vertex_rhw_init(vertex_t *v) 
{
	float rhw = 1.0f / v->pos.w;
	v->rhw = rhw;
	v->tc.u *= rhw;
	v->tc.v *= rhw;
	v->color.r *= rhw;
	v->color.g *= rhw;
	v->color.b *= rhw;
}

void vertex_interp(vertex_t *y, const vertex_t *x1, const vertex_t *x2, float t) {
	vector_interp(&y->pos, &x1->pos, &x2->pos, t);
	y->tc.u = interp(x1->tc.u, x2->tc.u, t);
	y->tc.v = interp(x1->tc.v, x2->tc.v, t);
	y->color.r = interp(x1->color.r, x2->color.r, t);
	y->color.g = interp(x1->color.g, x2->color.g, t);
	y->color.b = interp(x1->color.b, x2->color.b, t);
	y->rhw = interp(x1->rhw, x2->rhw, t);
}

void vertex_division(vertex_t *y, const vertex_t *x1, const vertex_t *x2, float w) {
	float inv = 1.0f / w;
	y->pos.x = (x2->pos.x - x1->pos.x) * inv;
	y->pos.y = (x2->pos.y - x1->pos.y) * inv;
	y->pos.z = (x2->pos.z - x1->pos.z) * inv;
	y->pos.w = (x2->pos.w - x1->pos.w) * inv;
	y->tc.u = (x2->tc.u - x1->tc.u) * inv;
	y->tc.v = (x2->tc.v - x1->tc.v) * inv;
	y->color.r = (x2->color.r - x1->color.r) * inv;
	y->color.g = (x2->color.g - x1->color.g) * inv;
	y->color.b = (x2->color.b - x1->color.b) * inv;
	y->rhw = (x2->rhw - x1->rhw) * inv;
}

void vertex_add(vertex_t *y, const vertex_t *x) 
{
	y->pos.x += x->pos.x;
	y->pos.y += x->pos.y;
	y->pos.z += x->pos.z;
	y->pos.w += x->pos.w;
	y->rhw += x->rhw;
	y->tc.u += x->tc.u;
	y->tc.v += x->tc.v;
	y->color.r += x->color.r;
	y->color.g += x->color.g;
	y->color.b += x->color.b;
}

// �������������� 0-2 �����Σ����ҷ��غϷ����ε�����
int trapezoid_init_triangle(trapezoid_t *trap, const vertex_t *p1, const vertex_t *p2, const vertex_t *p3) 
{
	const vertex_t *p;
	float k, x;

	if (p1->pos.y > p2->pos.y) p = p1, p1 = p2, p2 = p;
	if (p1->pos.y > p3->pos.y) p = p1, p1 = p3, p3 = p;
	if (p2->pos.y > p3->pos.y) p = p2, p2 = p3, p3 = p;
	if (p1->pos.y == p2->pos.y && p1->pos.y == p3->pos.y) return 0;
	if (p1->pos.x == p2->pos.x && p1->pos.x == p3->pos.x) return 0;

	if (p1->pos.y == p2->pos.y) {	// triangle down
		if (p1->pos.x > p2->pos.x) p = p1, p1 = p2, p2 = p;
		trap[0].top = p1->pos.y;
		trap[0].bottom = p3->pos.y;
		trap[0].left.v1 = *p1;
		trap[0].left.v2 = *p3;
		trap[0].right.v1 = *p2;
		trap[0].right.v2 = *p3;
		return (trap[0].top < trap[0].bottom) ? 1 : 0;
	}

	if (p2->pos.y == p3->pos.y) {	// triangle up
		if (p2->pos.x > p3->pos.x) p = p2, p2 = p3, p3 = p;
		trap[0].top = p1->pos.y;
		trap[0].bottom = p3->pos.y;
		trap[0].left.v1 = *p1;
		trap[0].left.v2 = *p2;
		trap[0].right.v1 = *p1;
		trap[0].right.v2 = *p3;
		return (trap[0].top < trap[0].bottom) ? 1 : 0;
	}

	trap[0].top = p1->pos.y;
	trap[0].bottom = p2->pos.y;
	trap[1].top = p2->pos.y;
	trap[1].bottom = p3->pos.y;

	k = (p3->pos.y - p1->pos.y) / (p2->pos.y - p1->pos.y);
	x = p1->pos.x + (p2->pos.x - p1->pos.x) * k;

	if (x <= p3->pos.x) {		// triangle left
		trap[0].left.v1 = *p1;
		trap[0].left.v2 = *p2;
		trap[0].right.v1 = *p1;
		trap[0].right.v2 = *p3;
		trap[1].left.v1 = *p2;
		trap[1].left.v2 = *p3;
		trap[1].right.v1 = *p1;
		trap[1].right.v2 = *p3;
	}
	else {					// triangle right
		trap[0].left.v1 = *p1;
		trap[0].left.v2 = *p3;
		trap[0].right.v1 = *p1;
		trap[0].right.v2 = *p2;
		trap[1].left.v1 = *p1;
		trap[1].left.v2 = *p3;
		trap[1].right.v1 = *p2;
		trap[1].right.v2 = *p3;
	}

	return 2;
}

// ���� Y ��������������������������� Y �Ķ���
void trapezoid_edge_interp(trapezoid_t *trap, float y) {
	float s1 = trap->left.v2.pos.y - trap->left.v1.pos.y;
	float s2 = trap->right.v2.pos.y - trap->right.v1.pos.y;
	float t1 = (y - trap->left.v1.pos.y) / s1;
	float t2 = (y - trap->right.v1.pos.y) / s2;
	vertex_interp(&trap->left.v, &trap->left.v1, &trap->left.v2, t1);
	vertex_interp(&trap->right.v, &trap->right.v1, &trap->right.v2, t2);
}

// �����������ߵĶ˵㣬��ʼ�������ɨ���ߵ����Ͳ���
void trapezoid_init_scan_line(const trapezoid_t *trap, scanline_t *scanline, int y) {
	float width = trap->right.v.pos.x - trap->left.v.pos.x;
	scanline->x = (int)(trap->left.v.pos.x + 0.5f);
	scanline->w = (int)(trap->right.v.pos.x + 0.5f) - scanline->x;
	scanline->y = y;
	scanline->v = trap->left.v;
	if (trap->left.v.pos.x >= trap->right.v.pos.x) scanline->w = 0;
	vertex_division(&scanline->step, &trap->left.v, &trap->right.v, width);
}

// ����
void device_pixel(device_t *device, int x, int y, unsigned int color) {
	if (((unsigned int)x) < (unsigned int)device->width && ((unsigned int)y) < (unsigned int)device->height) {
		device->framebuffer[y][x] = color;
	}
}

// �����߶�
void device_draw_line(device_t *device, int x1, int y1, int x2, int y2, unsigned int c) {
	int x, y, rem = 0;
	if (x1 == x2 && y1 == y2) {
		device_pixel(device, x1, y1, c);
	}
	else if (x1 == x2) {
		int inc = (y1 <= y2) ? 1 : -1;
		for (y = y1; y != y2; y += inc) device_pixel(device, x1, y, c);
		device_pixel(device, x2, y2, c);
	}
	else if (y1 == y2) {
		int inc = (x1 <= x2) ? 1 : -1;
		for (x = x1; x != x2; x += inc) device_pixel(device, x, y1, c);
		device_pixel(device, x2, y2, c);
	}
	else {
		int dx = (x1 < x2) ? x2 - x1 : x1 - x2;
		int dy = (y1 < y2) ? y2 - y1 : y1 - y2;
		if (dx >= dy) {
			if (x2 < x1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
			for (x = x1, y = y1; x <= x2; x++) {
				device_pixel(device, x, y, c);
				rem += dy;
				if (rem >= dx) {
					rem -= dx;
					y += (y2 >= y1) ? 1 : -1;
					device_pixel(device, x, y, c);
				}
			}
			device_pixel(device, x2, y2, c);
		}
		else {
			if (y2 < y1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
			for (x = x1, y = y1; y <= y2; y++) {
				device_pixel(device, x, y, c);
				rem += dx;
				if (rem >= dy) {
					rem -= dy;
					x += (x2 >= x1) ? 1 : -1;
					device_pixel(device, x, y, c);
				}
			}
			device_pixel(device, x2, y2, c);
		}
	}
}

// ���������ȡ����
unsigned int device_texture_read(const device_t *device, float u, float v) 
{
	int x, y;
	u = u * device->max_u;
	v = v * device->max_v;
	x = (int)(u + 0.5f);
	y = (int)(v + 0.5f);
	x = CMID(x, 0, device->tex_width - 1);
	y = CMID(y, 0, device->tex_height - 1);
	return device->texture[y][x];
}

//=====================================================================
// ��Ⱦʵ��
//=====================================================================

// ����ɨ����
void device_draw_scanline(device_t *device, scanline_t *scanline) 
{
	unsigned int *framebuffer = device->framebuffer[scanline->y];
	float *zbuffer = device->zbuffer[scanline->y];
	int x = scanline->x;
	int w = scanline->w;
	int width = device->width;
	int render_state = device->render_state;
	for (; w > 0; x++, w--) {
		if (x >= 0 && x < width) {
			float rhw = scanline->v.rhw;
			if (rhw >= zbuffer[x]) {
				float w = 1.0f / rhw;
				zbuffer[x] = rhw;
				if (render_state & RENDER_STATE_COLOR) {
					float r = scanline->v.color.r * w;
					float g = scanline->v.color.g * w;
					float b = scanline->v.color.b * w;
					int R = (int)(r * 255.0f);
					int G = (int)(g * 255.0f);
					int B = (int)(b * 255.0f);
					R = CMID(R, 0, 255);
					G = CMID(G, 0, 255);
					B = CMID(B, 0, 255);
					framebuffer[x] = (R << 16) | (G << 8) | (B);
				}
				if (render_state & RENDER_STATE_TEXTURE) {
					float u = scanline->v.tc.u * w;
					float v = scanline->v.tc.v * w;
					unsigned int cc = device_texture_read(device, u, v);
					framebuffer[x] = cc;
				}
			}
		}
		vertex_add(&scanline->v, &scanline->step);
		if (x >= width) break;
	}
}

// ����Ⱦ����
void device_render_trap(device_t *device, trapezoid_t *trap) 
{
	scanline_t scanline;
	int j, top, bottom;
	top = (int)(trap->top + 0.5f);
	bottom = (int)(trap->bottom + 0.5f);
	for (j = top; j < bottom; j++) {
		if (j >= 0 && j < device->height) {
			trapezoid_edge_interp(trap, (float)j + 0.5f);
			trapezoid_init_scan_line(trap, &scanline, j);
			device_draw_scanline(device, &scanline);
		}
		if (j >= device->height) break;
	}
}

// ���� render_state ����ԭʼ������
void device_draw_primitive(device_t *device, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3) 
{
	point_t p1, p2, p3, c1, c2, c3;
	int render_state = device->render_state;

	// ���� Transform �仯
	transform_apply(&device->transform, &c1, &v1->pos);
	transform_apply(&device->transform, &c2, &v2->pos);
	transform_apply(&device->transform, &c3, &v3->pos);

	// �ü���ע��˴���������Ϊ�����жϼ������� cvv���Լ�ͬcvv�ཻƽ����������
	// ���н�һ����ϸ�ü�����һ���ֽ�Ϊ������ȫ���� cvv�ڵ�������
	if (transform_check_cvv(&c1) != 0) return;
	if (transform_check_cvv(&c2) != 0) return;
	if (transform_check_cvv(&c3) != 0) return;

	// ��һ��
	transform_homogenize(&device->transform, &p1, &c1);
	transform_homogenize(&device->transform, &p2, &c2);
	transform_homogenize(&device->transform, &p3, &c3);

	// �������ɫ�ʻ���
	if (render_state & (RENDER_STATE_TEXTURE | RENDER_STATE_COLOR)) 
	{
		vertex_t t1 = *v1, t2 = *v2, t3 = *v3;
		trapezoid_t traps[2];

		t1.pos = p1;
		t2.pos = p2;
		t3.pos = p3;
		t1.pos.w = c1.w;
		t2.pos.w = c2.w;
		t3.pos.w = c3.w;

		vertex_rhw_init(&t1);	// ��ʼ�� w
		vertex_rhw_init(&t2);	// ��ʼ�� w
		vertex_rhw_init(&t3);	// ��ʼ�� w

								// ���������Ϊ0-2�����Σ����ҷ��ؿ�����������
		int n = trapezoid_init_triangle(traps, &t1, &t2, &t3);

		if (n >= 1) device_render_trap(device, &traps[0]);
		if (n >= 2) device_render_trap(device, &traps[1]);
	}

	if (render_state & RENDER_STATE_WIREFRAME) {		// �߿����
		device_draw_line(device, (int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, device->foreground);
		device_draw_line(device, (int)p1.x, (int)p1.y, (int)p3.x, (int)p3.y, device->foreground);
		device_draw_line(device, (int)p3.x, (int)p3.y, (int)p2.x, (int)p2.y, device->foreground);
	}
}

//=====================================================================
// ������
//=====================================================================
vertex_t mesh[8] = {
	{ { 1, -1,  1, 1 },{ 0, 0 },{ 1.0f, 0.2f, 0.2f }, 1 },
	{ { -1, -1,  1, 1 },{ 0, 1 },{ 0.2f, 1.0f, 0.2f }, 1 },
	{ { -1,  1,  1, 1 },{ 1, 1 },{ 0.2f, 0.2f, 1.0f }, 1 },
	{ { 1,  1,  1, 1 },{ 1, 0 },{ 1.0f, 0.2f, 1.0f }, 1 },
	{ { 1, -1, -1, 1 },{ 0, 0 },{ 1.0f, 1.0f, 0.2f }, 1 },
	{ { -1, -1, -1, 1 },{ 0, 1 },{ 0.2f, 1.0f, 1.0f }, 1 },
	{ { -1,  1, -1, 1 },{ 1, 1 },{ 1.0f, 0.3f, 0.3f }, 1 },
	{ { 1,  1, -1, 1 },{ 1, 0 },{ 0.2f, 1.0f, 0.3f }, 1 },
};

void draw_plane(device_t *device, int a, int b, int c, int d) 
{
	vertex_t p1 = mesh[a], p2 = mesh[b], p3 = mesh[c], p4 = mesh[d];
	p1.tc.u = 0, p1.tc.v = 0;
	p2.tc.u = 0, p2.tc.v = 1;
	p3.tc.u = 1, p3.tc.v = 1;
	p4.tc.u = 1, p4.tc.v = 0;
	device_draw_primitive(device, &p1, &p2, &p3);
	device_draw_primitive(device, &p3, &p4, &p1);
}

void draw_box(device_t *device, float theta) 
{
	//matrix_t m;
	//matrix_set_rotate(&m, -1, -0.5, 1, theta);
	//matrix_set_rotate(&m, 1, 0, 0, theta);
	//device->transform.world = m;
	//transform_update(&device->transform);
	draw_plane(device, 0, 1, 2, 3);
	draw_plane(device, 4, 5, 6, 7);
	draw_plane(device, 0, 4, 5, 1);
	draw_plane(device, 1, 5, 6, 2);
	draw_plane(device, 2, 6, 7, 3);
	draw_plane(device, 3, 7, 4, 0);
}