#pragma once
#include "matrix.h"

#define RENDER_STATE_WIREFRAME      1		// ��Ⱦ�߿�
#define RENDER_STATE_TEXTURE        2		// ��Ⱦ����
#define RENDER_STATE_COLOR          4		// ��Ⱦ��ɫ

//=====================================================================
// ����任
//=====================================================================
typedef struct {
	matrix_t world;         // ��������任
	matrix_t view;          // ��Ӱ������任
	matrix_t projection;    // ͶӰ�任
	matrix_t transform;     // transform = world * view * projection
	float w, h;             // ��Ļ��С
} transform_t;

//=====================================================================
// ��Ⱦ�豸
//=====================================================================
typedef struct {
	transform_t transform;      // ����任��
	int width;                  // ���ڿ��
	int height;                 // ���ڸ߶�
	unsigned int **framebuffer; // ���ػ��棺framebuffer[y] ����� y��
	float **zbuffer;            // ��Ȼ��棺zbuffer[y] Ϊ�� y��ָ��
	
	unsigned int **texture;     // ����ͬ����ÿ������
	int tex_width;              // ������
	int tex_height;             // ����߶�	
	float max_u;                // ��������ȣ�tex_width - 1
	float max_v;                // �������߶ȣ�tex_height - 1

	int render_state;           // ��Ⱦ״̬
	unsigned int background;    // ������ɫ
	unsigned int foreground;    // �߿���ɫ

} device_t;

// ��Ⱦ�豸��ʼ����fbΪ�ⲿ֡���棬��NULL�������ⲿ֡���棨ÿ�� 4�ֽڶ��룩
void device_init(device_t *device, int width, int height, void *fb);

void camera_at_zero(device_t *device, float x, float y, float z);

void init_texture(device_t *device);

// ��� framebuffer �� zbuffer
void device_clear(device_t *device, int mode);




void draw_box(device_t *device, vector_t axis, float theta);