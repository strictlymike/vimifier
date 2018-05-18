#include <windows.h>
#include <stdio.h>

#include "clip.h"
#include "debug.h"

CRITICAL_SECTION g_clip_cs;
char *g_clipboard_stash = NULL;
HANDLE g_clip_hmem = NULL;
HWND g_clip_hwnd = NULL;

int
initClip(HINSTANCE hInstance)
{
    WNDCLASSEX wc = {0};
	HWND hWnd = NULL;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = DefWindowProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = VIMIFY_CLIP_WINDOW_CLASS_A;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBoxA(
			NULL,
			"Failed to register window class",
			VIMIFY_CLIP_WINDOW_CLASS_A,
			MB_OK
		   );
		return 1;
	}

	hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		VIMIFY_CLIP_WINDOW_CLASS_A,
		VIMIFY_CLIP_WINDOW_TITLE_A,
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		1,
		1,
		NULL,
		NULL,
		hInstance,
		NULL
	   );

	if (hWnd == NULL) {
		MessageBoxA(
			NULL,
			"Failed to create window",
			VIMIFY_CLIP_WINDOW_TITLE_A,
			MB_OK
		   );
		return 1;
	}

	if (!InitializeCriticalSectionAndSpinCount(&g_clip_cs, 0x500)) {
		return 1;
	}

	g_clip_hwnd = hWnd;

	return 0;
}

int
setClip(char *data, SIZE_T size)
{
	int ret = 1;
	BOOL opened = FALSE;
	HANDLE hClip = NULL;
	HANDLE hMem = NULL;
	char *data_intermediate = NULL;

	EnterCriticalSection(&g_clip_cs);

	if (!data || !size) {
		goto exit_setClip;
	}

	hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!hMem) { goto exit_setClip; }

	data_intermediate = GlobalLock(hMem);
	if (!data_intermediate) { goto exit_setClip; }

	memcpy(data_intermediate, data, size);
	data_intermediate[size-1] = '\0';

	opened = OpenClipboard(g_clip_hwnd);
	if (!opened) { goto exit_setClip; }

	if (!EmptyClipboard()) {
		goto exit_setClip;
	}

	hClip = SetClipboardData(CF_TEXT, hMem);
	if (!hClip) { goto exit_setClip; }

	ret = 0;

exit_setClip:
	if (data_intermediate) { GlobalUnlock(hMem); }
	if (hMem) { GlobalFree(hMem); }
	if (opened) { CloseClipboard(); }

	LeaveCriticalSection(&g_clip_cs);
	return ret;
}

int
getClip(char *out_data, SIZE_T *psize)
{
	int ret = 1;
	BOOL opened = FALSE;
	HANDLE hClip = NULL;
	char *data = NULL;

	EnterCriticalSection(&g_clip_cs);

	opened = OpenClipboard(g_clip_hwnd);
	if (!opened) {
		goto exit_getClip;
	}

	hClip = GetClipboardData(CF_TEXT);
	if (NULL == hClip) {
		PDEBUG("GetClipboardData failed, %d\n", GetLastError());
		goto exit_getClip;
	}

	data = GlobalLock(hClip);
	if (!data) {
		PDEBUG("GlobalLock failed, %d\n", GetLastError());
		PDEBUG("hClip = 0x%08x\n", hClip);
		goto exit_getClip;
	}

	if (NULL == out_data) {
		*psize = GlobalSize(hClip);
		ret = 0;
		goto exit_getClip;
	}

	if (GlobalSize(hClip) != *psize) {
		PDEBUG("getClip: GlobalSize/size mismatch\n");
		goto exit_getClip;
	}

	memcpy(out_data, data, *psize);

	ret = 0;

exit_getClip:
	if (hClip && data) {
		GlobalUnlock(hClip);
	}

	if (opened) {
		CloseClipboard();
	}

	LeaveCriticalSection(&g_clip_cs);
	return ret;
}

int
stashClipboard(void)
{
	HANDLE hClip = NULL;
	int ret = 1;
	SIZE_T size = 0;
	char *data = NULL;
	BOOL opened = FALSE;

	EnterCriticalSection(&g_clip_cs);

	/* If the clipboard has already been stashed, fail the operation */
	if (g_clipboard_stash) {
		goto exit_stashClipboard;
	}

	opened = OpenClipboard(g_clip_hwnd);
	if (!opened) {
		goto exit_stashClipboard;
	}

	hClip = GetClipboardData(CF_TEXT);
	if (!hClip) {
		goto exit_stashClipboard;
	}

	data = GlobalLock(hClip);
	if (!data) {
		goto exit_stashClipboard;
	}

	size = GlobalSize(hClip);

	g_clip_hmem = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!g_clip_hmem) {
		goto exit_stashClipboard;
	}

	g_clipboard_stash = (char *)GlobalLock(g_clip_hmem);
	if (!g_clipboard_stash) {
		goto exit_stashClipboard;
	}

	memcpy(g_clipboard_stash, data, size);

	ret = 0;
exit_stashClipboard:
	if (!ret) {
		if (g_clip_hmem && g_clipboard_stash) {
			GlobalUnlock(g_clip_hmem);
		}
		GlobalFree(g_clip_hmem);
		g_clip_hmem = NULL;
		g_clipboard_stash = NULL;
	}

	if (hClip && data) {
		GlobalUnlock(hClip);
		hClip = NULL;
	}

	if (opened) {
		CloseClipboard();
	}

	LeaveCriticalSection(&g_clip_cs);

	return ret;
}

int
restoreClipboard(void)
{
	HANDLE hClip = NULL;
	int ret = 1;
	SIZE_T size = 0;
	char *data = NULL;
	BOOL opened = FALSE;

	EnterCriticalSection(&g_clip_cs);

	if (!g_clip_hmem || !g_clipboard_stash) {
		goto exit_restoreClipboard;
	}

	opened = OpenClipboard(g_clip_hwnd);
	if (!opened) { goto exit_restoreClipboard; }

	if (!EmptyClipboard()) {
		goto exit_restoreClipboard;
	}

	hClip = SetClipboardData(CF_TEXT, g_clip_hmem);
	if (!hClip) { goto exit_restoreClipboard; }

	ret = 0;

exit_restoreClipboard:

	if (g_clip_hmem) {
		GlobalUnlock(g_clip_hmem);
		GlobalFree(g_clip_hmem);
	}
	if (opened) {
		CloseClipboard();
	}

	LeaveCriticalSection(&g_clip_cs);

	return ret;
}
