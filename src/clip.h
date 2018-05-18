#pragma once

#define VIMIFY_CLIP_WINDOW_CLASS_A "VimifyClipWnd"
#define VIMIFY_CLIP_WINDOW_TITLE_A "VimifyClip"

int initClip(HINSTANCE hInstance);
int getClip(char *out_data, SIZE_T *psize);
int setClip(char *data, SIZE_T size);
int stashClipboard(void);
int restoreClipboard(void);
