#pragma once

#include <windows.h>

enum KeyStrokeType {
	kst_normal = 0,

	/* The keystroke must be translated from Vim-style input to input for the
	 * Windows application, despite being an injected keystroke. */
	kst_macro_replay,
};

int keystrokeWasMacroReplay(void);
DWORD translateKey(DWORD vkCode);
void sendKey(DWORD vkMod, DWORD vkCode);
void sendHalfKey(DWORD vkCode, DWORD dwFlags);
void depressKey(DWORD vkCode);
void releaseKey(DWORD vkCode);
int isDepressed(DWORD vkCode);
int shiftIsDepressed(void);
int controlIsDepressed(void);
void suspendMods(int shiftState, int controlState);
void resumeMods(int shiftState, int controlState);

