// VMR9D3D.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "VMR9D3D.h"


// 全局变量: 
HINSTANCE hInst;								// 当前实例
TCHAR szWindowClass[] = _T("VMR9D3D");			// 主窗口类名
HWND g_hWnd = NULL;

IDirect3DDevice9* g_device = NULL;
IDirect3DVertexBuffer9* g_vertexBuf = NULL;
IDirect3DTexture9* g_texture = NULL;
IDirect3DSurface9* g_surface = NULL;

CDecoder* g_decoder = NULL;
SIZE g_videoSize;

struct Vertex
{
	static const DWORD FVF = D3DFVF_XYZ | D3DFVF_TEX1;
	
	float x, y, z;
	float u, v;

	void Set(float _x, float _y, float _z, float _u, float _v)
	{
		x = _x; y = _y; z = _z;
		u = _u; v = _v;
	}
};


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
 	// TODO:  在此放置代码。
	MSG msg = {};

	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	// 主消息循环: 
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			Render();
	}

	Clear();

	return (int) msg.wParam;
}



//
//  函数:  MyRegisterClass()
//
//  目的:  注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VMR9D3D));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数:  InitInstance(HINSTANCE, int)
//
//   目的:  保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   g_hWnd = CreateWindow(szWindowClass, szWindowClass, WS_OVERLAPPEDWINDOW,
	   CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

   if (!g_hWnd)
   {
      return FALSE;
   }

   if (!Init())
   {
	   return FALSE;
   }

   ShowWindow(g_hWnd, nCmdShow);
   UpdateWindow(g_hWnd);

   return TRUE;
}

//
//  函数:  WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

BOOL Init()
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	// D3D
	if (!InitD3D())
	{
		MessageBox(0, _T("InitD3D() - FAILED"), 0, 0);
		return FALSE;
	}

	// 解码器
	g_decoder = new CDecoder(L"E:\\pump it.avi", g_hWnd, g_device); // 指定播放的文件名
	g_decoder->SetOnPresent(std::function<void(VMR9PresentationInfo*)>(OnPresent));

	// 顶点
	g_device->CreateVertexBuffer(4 * sizeof(Vertex), D3DUSAGE_WRITEONLY, Vertex::FVF, D3DPOOL_DEFAULT, &g_vertexBuf, NULL);
	Vertex* vertices;
	g_vertexBuf->Lock(0, 0, (void**)&vertices, 0);
	vertices[0].Set(-1, -1, 0, 0, 1);
	vertices[1].Set(-1, 1, 0, 0, 0);
	vertices[2].Set(1, -1, 0, 1, 1);
	vertices[3].Set(1, 1, 0, 1, 0);
	g_vertexBuf->Unlock();

	// 纹理
	g_decoder->GetVideoSize(g_videoSize);
	D3DDISPLAYMODE dm;
	g_device->GetDisplayMode(NULL, &dm);
	g_device->CreateTexture(g_videoSize.cx, g_videoSize.cy, 1, D3DUSAGE_RENDERTARGET, dm.Format, D3DPOOL_DEFAULT, &g_texture, NULL);
	g_texture->GetSurfaceLevel(0, &g_surface);

	g_decoder->Run();

	return TRUE;
}

BOOL InitD3D()
{
	HRESULT hr = 0;

	// Step 1: Create the IDirect3D9 object.

	IDirect3D9* d3d9 = 0;
	d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d9)
	{
		::MessageBox(0, _T("Direct3DCreate9() - FAILED"), 0, 0);
		return FALSE;
	}

	// Step 2: Check for hardware vp.

	D3DCAPS9 caps;
	d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

	int vp = 0;
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	vp |= D3DCREATE_MULTITHREADED; // OnPresent是在另一个线程调用的

	// Step 3: Fill out the D3DPRESENT_PARAMETERS structure.

	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth = 800;
	d3dpp.BackBufferHeight = 600;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
	d3dpp.MultiSampleQuality = 0;
	//d3d9->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, deviceType, d3dpp.BackBufferFormat, windowed, d3dpp.MultiSampleType, &d3dpp.MultiSampleQuality);
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = g_hWnd;
	d3dpp.Windowed = TRUE;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.Flags = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	// Step 4: Create the device.

	hr = d3d9->CreateDevice(
		D3DADAPTER_DEFAULT, // primary adapter
		D3DDEVTYPE_HAL,         // device type
		g_hWnd,               // window associated with device
		vp,                 // vertex processing
		&d3dpp,             // present parameters
		&g_device);            // return created device

	if (FAILED(hr))
	{
		// try again using a 16-bit depth buffer
		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

		hr = d3d9->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			g_hWnd,
			vp,
			&d3dpp,
			&g_device);

		if (FAILED(hr))
		{
			d3d9->Release(); // done with d3d9 object
			::MessageBox(0, _T("CreateDevice() - FAILED"), 0, 0);
			return FALSE;
		}
	}

	d3d9->Release(); // done with d3d9 object

	return TRUE;
}

void Clear()
{
	if (g_decoder != NULL)
		delete g_decoder;

	if (g_surface != NULL)
		g_surface->Release();
	if (g_texture != NULL)
		g_texture->Release();
	if (g_vertexBuf != NULL)
		g_vertexBuf->Release();
	if (g_device != NULL)
		g_device->Release();

	CoUninitialize();
}

void Render()
{
	if (g_device == NULL)
		return;
	
	g_device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF000000, 1.0f, 0);
	g_device->BeginScene();

	g_device->SetTexture(0, g_texture);
	g_device->SetRenderState(D3DRS_LIGHTING, FALSE);

	g_device->SetStreamSource(0, g_vertexBuf, 0, sizeof(Vertex));
	g_device->SetFVF(Vertex::FVF);
	g_device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	g_device->EndScene();
	g_device->Present(0, 0, 0, 0);
}

void OnPresent(VMR9PresentationInfo* info)
{
	// 复制到纹理表面，自动转换颜色格式
	g_device->StretchRect(info->lpSurf, NULL, g_surface, NULL, D3DTEXF_NONE);
}
