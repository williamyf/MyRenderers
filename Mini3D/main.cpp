#include <windows.h>
#include <tchar.h>
#include <iostream>

//=====================================================================
// Win32 窗口及图形绘制：为 device 提供一个 DibSection 的 FB
//=====================================================================
static HWND		g_hWnd		= NULL;		// 主窗口 HWND
static HDC		g_hDc		= NULL;		// 配套的 HDC
static HBITMAP	g_hBmp		= NULL;		// DIB
static HBITMAP	g_hOldBmp	= NULL;		// 老的 BITMAP

int				g_wndWidth	= 0;
int				g_wndHeight = 0;
unsigned char*	g_wndFrameBuffer = NULL;// frame buffer
long			g_wndPitch	= 0;

// 全局状态控制
BOOL			g_exit = FALSE;
int				g_keys[512]; // 当前键盘按下状态

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

// 初始化窗口并设置标题
int window_init(int w, int h, const TCHAR *title) 
{
	window_close();

	WNDCLASS wc = { CS_BYTEALIGNCLIENT, (WNDPROC)window_events, 0, 0, 0,
		NULL, NULL, NULL, NULL, _T("SCREEN3.1415926") };	
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
	//g_hBmp = CreateDIBSection(g_hDc, &bi, DIB_RGB_COLORS, &ptr, 0, 0);
	g_hBmp = (HBITMAP)LoadImage(NULL, _T("1.bmp"), IMAGE_BITMAP, w, h,
		LR_DEFAULTCOLOR | LR_CREATEDIBSECTION | LR_LOADFROMFILE);

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
	TCHAR *title = _T("Mini3D (software render tutorial) - ")
		_T("Left/Right: rotation, Up/Down: forward/backward, Space: switch state");

	if (window_init(800, 600, title))
		return -1;

	while (!g_exit && g_keys[VK_ESCAPE] == 0)
	{
		dispatch_msg();

		if (g_keys[VK_UP]) {
			std::cout << "VK_UP down" << std::endl;
		}
		if (g_keys[VK_DOWN]) {
			std::cout << "VK_DOWN down" << std::endl;
		}
		if (g_keys[VK_LEFT]) {
			std::cout << "VK_LEFT down" << std::endl;
		}
		if (g_keys[VK_RIGHT]) {
			std::cout << "VK_RIGHT down" << std::endl;
		}
		if (g_keys[VK_SPACE]) {
			std::cout << "VK_SPACE down" << std::endl;
		}
		else {
		}

		window_update();

		Sleep(1);
	}
	return 0;
}