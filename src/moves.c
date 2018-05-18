#include "vimify.h"
#include "vimstate.h"
#include "moves.h"
#include "debug.h"

/* If selecting, moves should apply SHIFT key to retain selection */
void
startMove(void) { if (g.selecting) { depressKey(VK_SHIFT); } }

void
endMove(void) { if (g.selecting) { releaseKey(VK_SHIFT); } }

void wrapMoveWithSelect(DWORD vkMod, DWORD vkCode)
{
	startMove();
	sendKey(vkMod, vkCode);
	endMove();
}

void
moveLeft(void) { wrapMoveWithSelect(0, VK_LEFT); }
void
moveRight(void) { wrapMoveWithSelect(0, VK_RIGHT); }
void
moveUp(void) { wrapMoveWithSelect(0, VK_UP); }
void
moveDown(void) { wrapMoveWithSelect(0, VK_DOWN); }
void
moveHome(void) { wrapMoveWithSelect(0, VK_HOME); }
void
moveEnd(void) { wrapMoveWithSelect(0, VK_END); }
void
moveBack(void) { wrapMoveWithSelect(VK_CONTROL, VK_LEFT); }
void
moveWord(void) { wrapMoveWithSelect(VK_CONTROL, VK_RIGHT); }

void
cut(void) { sendKey(VK_CONTROL, 'X'); }

void
copy(void) { sendKey(VK_CONTROL, 'C'); }

void
cutPrev(void)
{
	depressKey(VK_SHIFT);
	sendKey(0, VK_LEFT);
	releaseKey(VK_SHIFT);
	cut();
}

void
cutNext(void)
{
	sendKey(VK_SHIFT, VK_RIGHT);
	cut();
}

/* Doesn't seem to work */
void
checkEndCopyDel(void)
{
	if (curState(vs_y)) {
		PDEBUG("Ending copy\n");
		copy();
		setState(vs_none);
		g.selecting = 0;
	} else if (curState(vs_d) || curState(vs_c)) {
		PDEBUG("Ending delete\n");
		cut();
		if (curState(vs_c)) {
			g.insert_mode = 1;
		}
		setState(vs_none);
		g.selecting = 0;
	}
}

void
paste(void)
{
	sendKey(VK_CONTROL, 'V');
}

/* Ctrl-Shift-End, Home */
void
do_zt(void)
{
	INPUT input[8] = {0};

	input[0].type = INPUT_KEYBOARD;
	input[0].ki.wVk = VK_CONTROL;
	input[0].ki.dwFlags = 0;

	input[1].type = INPUT_KEYBOARD;
	input[1].ki.wVk = VK_SHIFT;
	input[1].ki.dwFlags = 0;

	input[2].type = INPUT_KEYBOARD;
	input[2].ki.wVk = VK_END;
	input[2].ki.dwFlags = 0;

	input[3].type = INPUT_KEYBOARD;
	input[3].ki.wVk = VK_END;
	input[3].ki.dwFlags = KEYEVENTF_KEYUP;

	input[4].type = INPUT_KEYBOARD;
	input[4].ki.wVk = VK_SHIFT;
	input[4].ki.dwFlags = KEYEVENTF_KEYUP;

	input[5].type = INPUT_KEYBOARD;
	input[5].ki.wVk = VK_CONTROL;
	input[5].ki.dwFlags = KEYEVENTF_KEYUP;

	input[6].type = INPUT_KEYBOARD;
	input[6].ki.wVk = VK_HOME;
	input[6].ki.dwFlags = 0;

	input[7].type = INPUT_KEYBOARD;
	input[7].ki.wVk = VK_HOME;
	input[7].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(sizeof(input) / sizeof(INPUT), input, sizeof(INPUT));
}

/* Ctrl-Shift-Home, End, Home */
void
do_zb(void)
{
	INPUT input[10] = {0};

	input[0].type = INPUT_KEYBOARD;
	input[0].ki.wVk = VK_CONTROL;
	input[0].ki.dwFlags = 0;

	input[1].type = INPUT_KEYBOARD;
	input[1].ki.wVk = VK_SHIFT;
	input[1].ki.dwFlags = 0;

	input[2].type = INPUT_KEYBOARD;
	input[2].ki.wVk = VK_HOME;
	input[2].ki.dwFlags = 0;

	input[3].type = INPUT_KEYBOARD;
	input[3].ki.wVk = VK_HOME;
	input[3].ki.dwFlags = KEYEVENTF_KEYUP;

	input[4].type = INPUT_KEYBOARD;
	input[4].ki.wVk = VK_SHIFT;
	input[4].ki.dwFlags = KEYEVENTF_KEYUP;

	input[5].type = INPUT_KEYBOARD;
	input[5].ki.wVk = VK_CONTROL;
	input[5].ki.dwFlags = KEYEVENTF_KEYUP;

	input[6].type = INPUT_KEYBOARD;
	input[6].ki.wVk = VK_END;
	input[6].ki.dwFlags = 0;

	input[7].type = INPUT_KEYBOARD;
	input[7].ki.wVk = VK_END;
	input[7].ki.dwFlags = KEYEVENTF_KEYUP;

	input[8].type = INPUT_KEYBOARD;
	input[8].ki.wVk = VK_HOME;
	input[8].ki.dwFlags = 0;

	input[9].type = INPUT_KEYBOARD;
	input[9].ki.wVk = VK_HOME;
	input[9].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(sizeof(input) / sizeof(INPUT), input, sizeof(INPUT));
}
