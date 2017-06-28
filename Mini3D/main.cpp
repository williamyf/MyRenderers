#include <windows.h>
#include <tchar.h>
#include <iostream>
#include "device.h"
#include "resource1.h"
#include "bmpoper.h"

//=====================================================================
// Win32 ���ڼ�ͼ�λ��ƣ�Ϊ device �ṩһ�� DibSection �� FB
//=====================================================================
static HWND		g_hWnd		= NULL;		// ������ HWND
static HDC		g_hDc		= NULL;		// ���׵� HDC
static HBITMAP	g_hBmp		= NULL;		// DIB
static HBITMAP	g_hOldBmp	= NULL;		// �ϵ� BITMAP

int				g_wndWidth	= 0;
int				g_wndHeight = 0;
unsigned char*	g_wndFrameBuffer = NULL;// frame buffer
long			g_wndPitch	= 0;

// ȫ��״̬����
BOOL			g_exit = FALSE;
int				g_keys[512]; // ��ǰ���̰���״̬

static LRESULT window_events(HWND, UINT, WPARAM, LPARAM);

int window_close(void) 
{
	if (g_hDc) {
		if (g_hOldBmp) {
			SelectObject(g_hDc, g_hOldBmp);
			g_hOldBmp = NULL;
		}
		DeleteDC(g_hDc);
		g_hDc = NULL;
	}
	if (g_hBmp) {
		DeleteObject(g_hBmp);
		g_hBmp = NULL;
	}
	if (g_hWnd) {
		CloseWindow(g_hWnd);
		g_hWnd = NULL;
	}
	return 0;
}

void dispatch_msg(void) 
{
	MSG msg;
	while (1) 
	{
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) 
			break;
		if (!GetMessage(&msg, NULL, 0, 0)) 
			break;
		DispatchMessage(&msg);
	}
}

// ��ʼ�����ڲ����ñ���
int window_init(int w, int h, const TCHAR *title) 
{
	window_close();

	LPCTSTR menustr = MAKEINTRESOURCE(IDR_MENU_SETING);
	std::cout << "menu: " << menustr << std::endl;

	WNDCLASS wc = { CS_BYTEALIGNCLIENT, (WNDPROC)window_events, 0, 0, 0,
		NULL, NULL, NULL, menustr, _T("SCREEN3.1415926") };	
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	if (!RegisterClass(&wc)) return -1;

	g_hWnd = CreateWindow(_T("SCREEN3.1415926"), title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
	if (g_hWnd == NULL) return -2;

	HDC hDC = GetDC(g_hWnd);
	g_hDc = CreateCompatibleDC(hDC);
	ReleaseDC(g_hWnd, hDC);

	BITMAPINFO bi = { { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB,
		w * h * 4, 0, 0, 0, 0 } };

	LPVOID ptr = NULL;
	g_hBmp = CreateDIBSection(g_hDc, &bi, DIB_RGB_COLORS, &ptr, 0, 0);
	//g_hBmp = (HBITMAP)LoadImage(NULL, _T("1.bmp"), IMAGE_BITMAP, w, h,
	//	LR_DEFAULTCOLOR | LR_CREATEDIBSECTION | LR_LOADFROMFILE);

	if (g_hBmp == NULL) return -3;
	
	g_hOldBmp = (HBITMAP)SelectObject(g_hDc, g_hBmp);
	
	g_wndFrameBuffer = (unsigned char*)ptr;
	if (g_wndFrameBuffer)
	{
		memset(g_wndFrameBuffer, 0, w * h * 4);
	}

	g_wndWidth = w;
	g_wndHeight = h;
	g_wndPitch = w * 4;

	RECT rect = { 0, 0, w, h };
	AdjustWindowRect(&rect, GetWindowLong(g_hWnd, GWL_STYLE), 0);
	int cx = rect.right - rect.left;
	int cy = rect.bottom - rect.top;
	int x = (GetSystemMetrics(SM_CXSCREEN) - cx) / 2;
	int y = (GetSystemMetrics(SM_CYSCREEN) - cy) / 2;
	if (y < 0) y = 0;
	SetWindowPos(g_hWnd, NULL, x, y, cx, cy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
	SetForegroundWindow(g_hWnd);

	ShowWindow(g_hWnd, SW_NORMAL);
	//dispatch_msg(); // is here needed ???

	memset(g_keys, 0, sizeof(int) * 512);

	return 0;
}

static LRESULT window_events(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch (msg) 
	{
	case WM_CLOSE: 
		g_exit = TRUE; 
		break;
	case WM_KEYDOWN: 
		g_keys[wParam & 511] = 1; 
		break;
	case WM_KEYUP: 
		g_keys[wParam & 511] = 0; 
		break;
	default: 
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

void window_update(void) {
	HDC hDC = GetDC(g_hWnd);
	BitBlt(hDC, 0, 0, g_wndWidth, g_wndHeight, g_hDc, 0, 0, SRCCOPY);
	ReleaseDC(g_hWnd, hDC);
	//screen_dispatch(); // is here needed ???
}

int main()
{
	//doIt();
	//return 0;

	TCHAR *title = _T("Mini3D (software render tutorial) - ")
		_T("Left/Right: rotation, Up/Down: forward/backward, Space: switch state");

	if (window_init(800, 600, title))
		return -1;

	device_t device;
	device_init(&device, 800, 600, g_wndFrameBuffer);

	//camera_at_zero(&device, 3, 0, 0);
	init_texture(&device);
	
	
	int states[] = { RENDER_STATE_TEXTURE, RENDER_STATE_COLOR, RENDER_STATE_WIREFRAME };
	std::cout << "sizeof(states)/sizeof(int) = " << sizeof(states)/sizeof(int) << std::endl;

	device.render_state = RENDER_STATE_TEXTURE;

	float pos = 3.5;
	float alpha = 1;
	int bgmode = 0;
	bool kbhit = false; // �����Ƿ���
	bool a_hit = false; // A���Ƿ���
	int  axis_ind = 0;
	vector_t axis[] = { {1,0,0,1},{0,1,0,1},{0,0,1,1} };
	bool z_hit = false; // Z���Ƿ���
	int  lookat_ind = 0;

	auto print_rotate_tip = [](int ind) {
		if (ind == 0) {
			std::cout << "��X����ת" << std::endl;
		} else if (ind == 1) {
			std::cout << "��Y����ת" << std::endl;
		} else {
			std::cout << "��Z����ת" << std::endl;
		}		
	};
	auto print_lookat_tip = [](int ind) {
		if (ind == 0) {
			std::cout << "��X�Ḻ������ԭ��" << std::endl;
		}
		else if (ind == 1) {
			std::cout << "��Y�Ḻ������ԭ��" << std::endl;
		}
		else {
			std::cout << "��Z�Ḻ������ԭ��" << std::endl;
		}
	};
	print_rotate_tip(axis_ind);
	print_lookat_tip(lookat_ind);

	while (!g_exit && g_keys[VK_ESCAPE] == 0)
	{
		dispatch_msg();

		vector_t lookat_pos[3] = { { pos,0,0,1 },{ 0,pos,0,1 },{ 0,0,pos,1 } };

		device_clear(&device, bgmode);
		camera_at_zero(&device, lookat_pos[lookat_ind].x, lookat_pos[lookat_ind].y, lookat_pos[lookat_ind].z);

		if (g_keys[VK_UP]) {
			pos -= 0.01f;
		}
		if (g_keys[VK_DOWN]) {
			pos += 0.01f;
		}
		if (g_keys[VK_LEFT]) {
			alpha += 0.02f;
		}
		if (g_keys[VK_RIGHT]) {
			alpha -= 0.02f;
		}
		if (g_keys[VK_SPACE]) {
			if (!kbhit) {
				kbhit = true;
				static int indicator = 0;
				indicator = (++indicator) % (sizeof(states) / sizeof(int));
				std::cout << "indicator = " << indicator << std::endl;
				device.render_state = states[indicator];
			}
		} else {
			kbhit = false;
		}
		if (g_keys['Q']) {
			bgmode = 1;
		}
		if (g_keys['W']) {
			bgmode = 0;
		}
		if (g_keys['A']) {
			if (!a_hit)
			{
				a_hit = true;
				axis_ind = ++axis_ind % 3;
				print_rotate_tip(axis_ind);
			}
		} else {
			a_hit = false;
		}
		if (g_keys['Z']) {
			if (!z_hit)
			{
				z_hit = true;
				lookat_ind = ++lookat_ind % 3;
				print_lookat_tip(lookat_ind);
			}
		} else {
			z_hit = false;
		}

		draw_box(&device, axis[axis_ind], alpha);

		window_update();

		Sleep(1);
	}
	return 0;
}