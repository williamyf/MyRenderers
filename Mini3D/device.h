#pragma once
#include "matrix.h"

#define RENDER_STATE_WIREFRAME      1		// 渲染线框
#define RENDER_STATE_TEXTURE        2		// 渲染纹理
#define RENDER_STATE_COLOR          4		// 渲染颜色

//=====================================================================
// 坐标变换
//=====================================================================
typedef struct {
	matrix_t world;         // 世界坐标变换
	matrix_t view;          // 摄影机坐标变换
	matrix_t projection;    // 投影变换
	matrix_t transform;     // transform = world * view * projection
	float w, h;             // 屏幕大小
} transform_t;

//=====================================================================
// 渲染设备
//=====================================================================
typedef struct {
	transform_t transform;      // 坐标变换器
	int width;                  // 窗口宽度
	int height;                 // 窗口高度
	unsigned int **framebuffer; // 像素缓存：framebuffer[y] 代表第 y行
	float **zbuffer;            // 深度缓存：zbuffer[y] 为第 y行指针
	
	unsigned int **texture;     // 纹理：同样是每行索引
	int tex_width;              // 纹理宽度
	int tex_height;             // 纹理高度	
	float max_u;                // 纹理最大宽度：tex_width - 1
	float max_v;                // 纹理最大高度：tex_height - 1

	int render_state;           // 渲染状态
	unsigned int background;    // 背景颜色
	unsigned int foreground;    // 线框颜色

} device_t;

// 渲染设备初始化，fb为外部帧缓存，非NULL将引用外部帧缓存（每行 4字节对齐）
void device_init(device_t *device, int width, int height, void *fb);

void camera_at_zero(device_t *device, float x, float y, float z);

void init_texture(device_t *device);

// 清空 framebuffer 和 zbuffer
void device_clear(device_t *device, int mode);




void draw_box(device_t *device, vector_t axis, float theta);