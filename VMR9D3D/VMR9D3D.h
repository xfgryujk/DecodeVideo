#pragma once
#include "resource.h"
#include "Decoder.h"


// 此代码模块中包含的函数的前向声明: 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL				Init();
BOOL				InitD3D();
void				Clear();
void				Render();
void				OnPresent(VMR9PresentationInfo* info);
