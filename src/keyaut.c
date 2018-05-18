#include "keyaut.h"

int
keystrokeWasMacroReplay(void)
{
	return kst_macro_replay == (DWORD)GetMessageExtraInfo();
}

DWORD
translateKey(DWORD vkCode)
{
	int shiftDepressed = shiftIsDepressed();
	int isletter = isalpha(vkCode);
	int isnumber = isdigit(vkCode);
	// PDEBUG("shiftDepressed: %d\n", shiftDepressed);
	// PDEBUG("isletter: %d\n", isletter);
	// PDEBUG("isnumber: %d\n", isnumber);

	if (isletter && !shiftDepressed) {
		// PDEBUG("Lowercase letter\n");
		return tolower(vkCode);
	} else if (isnumber && shiftDepressed) {
		// PDEBUG("Number row punctuation\n");
		return toPunc(vkCode);
	} else {
		return vkCode;
	}
}

/** Send key down / key up for vkCode, optionally wrapping in vkMod */
void
sendKey(DWORD vkMod, DWORD vkCode)
{
	INPUT input[4] = {0};
	int count = 0;

	if (vkMod != 0) {
		input[count].type = INPUT_KEYBOARD;
		input[count].ki.wVk = vkMod;
		input[count].ki.dwFlags = 0;
		count++;
	}

	input[count].type = INPUT_KEYBOARD;
	input[count].ki.wVk = vkCode;
	input[count].ki.dwFlags = 0;
	count++;

	input[count].type = INPUT_KEYBOARD;
	input[count].ki.wVk = vkCode;
	input[count].ki.dwFlags = KEYEVENTF_KEYUP;
	count++;

	if (vkMod != 0) {
		input[count].type = INPUT_KEYBOARD;
		input[count].ki.wVk = vkMod;
		input[count].ki.dwFlags = KEYEVENTF_KEYUP;
		count++;
	}

	SendInput(count, input, sizeof(INPUT));
}

/* Only key down or key up, not both */
void
sendHalfKey(DWORD vkCode, DWORD dwFlags)
{
	INPUT input;

	input.type = INPUT_KEYBOARD;
	input.ki.wVk = vkCode;
	input.ki.dwFlags = dwFlags;
	SendInput(1, &input, sizeof(INPUT));
}

void
depressKey(DWORD vkCode)
{
	sendHalfKey(vkCode, 0);
}

void
releaseKey(DWORD vkCode)
{
	sendHalfKey(vkCode, KEYEVENTF_KEYUP);
}

int
isDepressed(DWORD vkCode)
{
	BYTE kbs[256] = {0};
	int ret;

	GetKeyboardState(kbs);
	ret = GetKeyState(vkCode) & 0x8000;

#if 0
	printf("\n");
	printf("GetAsyncKeyState(VK_CAPITAL) = 0x%x\n", GetAsyncKeyState(VK_CAPITAL));
	printf("GetKeyState(VK_CAPITAL) = 0x%x\n", GetKeyState(VK_CAPITAL));
	printf("kbs[VK_CAPITAL] = 0x%02x\n", kbs[VK_CAPITAL]);
	printf("GetAsyncKeyState(VK_SHIFT) = 0x%x\n", GetAsyncKeyState(VK_SHIFT));
	printf("GetKeyState(VK_SHIFT) = 0x%x\n", GetKeyState(VK_SHIFT));
	printf("kbs[VK_SHIFT] = 0x%02x\n", kbs[VK_SHIFT]);
#endif

	return ret;
}

int shiftIsDepressed(void) { return isDepressed(VK_SHIFT); }

int controlIsDepressed(void) { return isDepressed(VK_CONTROL); }

/* Conditionally suspend modifier keys */
void
suspendMods(int shiftState, int controlState)
{
	if (shiftState) { releaseKey(VK_SHIFT); }
	if (controlState) { releaseKey(VK_CONTROL); }
}

/* Conditionally resume modifier keys */
void
resumeMods(int origShiftState, int origControlState)
{
	if (origShiftState) { depressKey(VK_SHIFT); }
	if (origControlState) { depressKey(VK_CONTROL); }
}

