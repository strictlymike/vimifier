#pragma once

#include <windows.h>

#define VIMIFY_WINDOW_CLASS_A "VimifySelector"
#define VIMIFY_WINDOW_TITLE_A "Vimifier"

HWND vimifyUI(HINSTANCE hInstance, DWORD *ppid);
void vimifyMinimize(HWND hWnd);
void vimifyNormalize(HWND hWnd);
