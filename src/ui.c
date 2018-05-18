#include <windowsx.h>

#include "ui.h"
#include "debug.h"

#pragma comment(lib, "gdi32")

DWORD *g_ppid = NULL;

LRESULT
CALLBACK
vimifyWndProc(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam
   );

void
vimifyMinimize(HWND hWnd)
{
	if (hWnd) {
		ShowWindow(hWnd, SW_MINIMIZE);
	}
}

void
vimifyNormalize(HWND hWnd)
{
	if (hWnd) {
		ShowWindow(hWnd, SW_RESTORE);
	}
}

HWND
vimifyUI(HINSTANCE hInstance, DWORD *ppid)
{
    WNDCLASSEX wc = {0};
	HWND hWnd = NULL;
	BOOL Ok = FALSE;

	if (NULL == ppid) {
		return NULL;
	}

	g_ppid = ppid;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = vimifyWndProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = VIMIFY_WINDOW_CLASS_A;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
    {
        MessageBoxA(
			NULL,
			"Failed to register window class",
			"Failwhale",
			MB_OK
		   );
		return NULL;
    }

    hWnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        VIMIFY_WINDOW_CLASS_A,
        VIMIFY_WINDOW_TITLE_A,
        WS_OVERLAPPEDWINDOW,
        0,
		0,
		250,
		145,
        NULL,
		NULL,
		hInstance,
		NULL
	   );

    if(hWnd == NULL)
    {
        MessageBoxA(
			NULL,
			"Failed to create window",
			"Failwhale",
			MB_OK
		   );
		return NULL;
    }

    ShowWindow(hWnd, SW_SHOW);

	return hWnd;
}

LRESULT
CALLBACK
vimifyWndProc(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam
   )
{
	HWND Tgt;
	DWORD Pid;
	POINT pWhere;
	BOOL Ok;
	static BOOL FirstShowWindow = TRUE;

	HDC hDc;
	RECT Rect;
	char * Label = "\n\n\nClick here and drag to the window you want\n"
		"to Vimify (or press Esc to quit)";
	PAINTSTRUCT ps;
	HFONT hFont, hOldFont; 

    switch(Msg)
    {
		case WM_CREATE:
			/* Position this window in the topmost position so that the user
			 * can see this interface over the windows they may be trying to
			 * target. */
			SetWindowPos(
				hWnd,
				HWND_TOPMOST,
				0,
				0,
				0,
				0,
				SWP_NOMOVE|SWP_NOSIZE
			   );
			break;

		case WM_SHOWWINDOW:
			if (FirstShowWindow && (0 != *g_ppid)) {
				ShowWindow(hWnd, SW_MINIMIZE);
				FirstShowWindow = FALSE;
			}

		case WM_LBUTTONDOWN:
			SetCapture(hWnd);
			SetCursor(LoadCursor(NULL, IDC_CROSS));
			break;

		case WM_PAINT:
			GetClientRect(hWnd, &Rect);
			hDc = BeginPaint(hWnd, &ps);
			hFont = (HFONT)GetStockObject(ANSI_VAR_FONT); 
			hOldFont = (HFONT)SelectObject(hDc, hFont);
			DrawTextA(hDc, Label, strlen(Label), &Rect, DT_CENTER | DT_VCENTER);
			SelectObject(hDc, hOldFont); 
			EndPaint(hWnd, &ps);
			break;

		// When the user releases the left mouse button, get the point where
		// the mouse button was released, convert it to screen coordinates, get
		// the window residing there, and get the PID.
		case WM_LBUTTONUP:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			pWhere.x = GET_X_LPARAM(lParam);
			pWhere.y = GET_Y_LPARAM(lParam);

			PDEBUG("x, y = %d, %d\n", pWhere.x, pWhere.y);

			// Convert to screen coordinates
			ClientToScreen(hWnd, &pWhere);

			PDEBUG("x, y = %d, %d\n", pWhere.x, pWhere.y);

			// What window falls under the cursor
			Tgt = WindowFromPoint(pWhere);

			if (!Tgt) {
				PDEBUG("!Tgt\n");
			}

			// Get top-level window (as opposed to a static text or other
			// control that the user may have clicked on) against which to call
			// SetWindowPos() and set HWND_NOTOPMOST.
			if (Tgt)
			{
				GetWindowThreadProcessId(Tgt, &Pid);
				if (Pid != GetCurrentProcessId()) {
					PDEBUG("Assigning pid %d\n", Pid);
					*g_ppid = Pid;
					ShowWindow(hWnd, SW_MINIMIZE);
				} else {
					PDEBUG("Not assigning own pid %d\n", Pid);
				}
			}

			break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
			break;

		// All DestroyWindow() paths lead here:
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ff381396%28
		// v=vs.85%29.aspx
        case WM_DESTROY:
            PostQuitMessage(0);
			break;

        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    return 0;
}
