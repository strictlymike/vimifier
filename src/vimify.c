#include <windows.h>

#include "queue.h"

#include "vimify.h"
#include "vimstate.h"
#include "vimreg.h"
#include "keyaut.h"
#include "moves.h"
#include "ui.h"
#include "debug.h"

#pragma comment(lib, "user32")

#define CASE_CONTROL \
	case VK_CONTROL: \
	case VK_LCONTROL: \
	case VK_RCONTROL

#define CASE_ALT \
	case VK_MENU: \
	case VK_LMENU: \
	case VK_RMENU

#define CASE_SHIFT \
	case VK_SHIFT: \
	case VK_LSHIFT: \
	case VK_RSHIFT

#define CASE_IGNORE_MODKEYS \
	CASE_CONTROL: \
	CASE_ALT: \
	CASE_SHIFT

struct KeyStroke {
	TAILQ_ENTRY(KeyStroke) entries;
	WPARAM wParam;
	KBDLLHOOKSTRUCT khs;
	int	shift;
	int control;
};

struct VimifyStateStruct g = {0};

LRESULT CALLBACK vimifyLlkbd(int nCode, WPARAM wParam, LPARAM lParam);
void vimifyMessageLoop(void);
int vimifyKeystroke(struct KeyStroke * ks);
DWORD getForegroundPid(void);

int curState(enum VimifyState query);
void setState(enum VimifyState newstate);

void vimifyDot(void);
void startRecordTo(enum VimifyRegister reg);
void startAppendTo(enum VimifyRegister reg);
void delQueue(struct ksq *q);
void delQueueUnsafe(struct ksq *q);
void trimQueue(struct ksq *q);
void macroAppendKeystroke(enum VimifyRegister reg, struct KeyStroke *ks);
void dumpMacro(enum VimifyRegister reg);
void dumpKeyStroke(struct KeyStroke *ks);
void maybeRecordKeystroke(struct KeyStroke *ks);
void replayMacro(enum VimifyRegister reg);
void startReplayMacroThread(enum VimifyRegister reg);
DWORD WINAPI threadproc_replayMacro(LPVOID lpParameter);

void
startReplayMacroThread(enum VimifyRegister reg)
{
	CreateThread(NULL, 0, threadproc_replayMacro, (LPVOID)reg, 0, NULL);
}

DWORD
WINAPI
threadproc_replayMacro(LPVOID lpParameter)
{
	replayMacro((enum VimifyRegister)lpParameter);
	return 0;
}

void
replayMacro(enum VimifyRegister reg)
{
	struct KeyStroke *ks = NULL;
	INPUT input;

	PDEBUG("\n");
	PDEBUG("Replaying macro %c\n", regToChar(reg));

	if (!isValidRegister(reg)) { return; }

	EnterCriticalSection(&g.ksq_cs);
	g.reg_replaying_from = g.reg_last = reg;

	TAILQ_FOREACH(ks, &g.ksq_reg[reg], entries) {
		memset(&input, 0, sizeof(input));
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = ks->khs.vkCode;
		input.ki.wScan = ks->khs.scanCode;
		input.ki.dwFlags = KEYEVENTF_SCANCODE;
		input.ki.dwExtraInfo = kst_macro_replay;
		if ((ks->wParam == WM_KEYUP) || (ks->wParam == WM_SYSKEYUP)) {
			PDEBUG("Replaying keyup %c\n", ks->khs.vkCode);
			input.ki.dwFlags |= KEYEVENTF_KEYUP;
		} else {
			PDEBUG("Replaying keydown %c\n", ks->khs.vkCode);
		}
		SendInput(1, &input, sizeof(INPUT));
		Sleep(10);
	}

	g.reg_replaying_from = reg_none;

	LeaveCriticalSection(&g.ksq_cs);

	PDEBUG("Done replaying macro %c\n", regToChar(reg));
}

void
maybeRecordKeystroke(struct KeyStroke *ks)
{
	int recording = 0;
	int injected = 0;
	int should_record_this = 0;
	struct KeyStroke *ks_cpy = NULL;

	recording = (g.reg_recording_to != reg_none);
	injected = keyInjected(&ks->khs);
	should_record_this = !injected && recording;

	/* If record_this a macro to a register... */
	if (should_record_this) {
		ks_cpy = malloc(sizeof(struct KeyStroke));
		if (!ks_cpy) {
			MessageBox(
				0,
				"Failed to allocate memory, recording terminated",
				"Recording failed",
				MB_OK
			   );
			g.reg_recording_to = reg_none;
		} else {
			*ks_cpy = *ks;
			macroAppendKeystroke(g.reg_recording_to, ks_cpy);
		}
	}
}

void
dumpKeyStroke(struct KeyStroke *ks)
{
	char *wParamDesc = "Unknown";

	switch (ks->wParam) {
	case WM_KEYUP: wParamDesc = "WM_KEYUP"; break;
	case WM_KEYDOWN: wParamDesc = "WM_KEYDOWN"; break;
	case WM_SYSKEYUP: wParamDesc = "WM_SYSKEYUP"; break;
	case WM_SYSKEYDOWN: wParamDesc = "WM_SYSKEYDOWN"; break;
	}

	PDEBUG("KeyStroke:\n");
	PDEBUG("\twParam = 0x%x (%s)\n", ks->wParam, wParamDesc);
	PDEBUG("\tvkCode = 0x%x (%c)\n", ks->khs.vkCode, ks->khs.vkCode);
	PDEBUG("\tscanCode = 0x%x (%c)\n", ks->khs.scanCode, ks->khs.scanCode);
	PDEBUG("\tflags = 0x%x (%c)\n", ks->khs.flags, ks->khs.flags);
	PDEBUG("\tdwExtraInfo = 0x%x (%d)\n", ks->khs.dwExtraInfo, ks->khs.dwExtraInfo);
}

void
dumpMacro(enum VimifyRegister reg)
{
	struct KeyStroke *ks = NULL;

	if (!isValidRegister(reg)) { return; }

	EnterCriticalSection(&g.ksq_cs);
	TAILQ_FOREACH(ks, &g.ksq_reg[reg], entries) {
		dumpKeyStroke(ks);
	}
	LeaveCriticalSection(&g.ksq_cs);
}

void
macroAppendKeystroke(enum VimifyRegister reg, struct KeyStroke *ks)
{
	if (!isValidRegister(reg)) { return; }

	EnterCriticalSection(&g.ksq_cs);
	TAILQ_INSERT_TAIL(&g.ksq_reg[reg], ks, entries);
	LeaveCriticalSection(&g.ksq_cs);
}

void
delQueue(struct ksq *q)
{
	EnterCriticalSection(&g.ksq_cs);
	delQueueUnsafe(q);
	LeaveCriticalSection(&g.ksq_cs);
}

/* Caller is responsible for mutual exclusion */
void
delQueueUnsafe(struct ksq *q)
{
	struct KeyStroke *ks;

	while ((ks = q->tqh_first) != NULL) {
		TAILQ_REMOVE(q, ks, entries);
		free(ks);
	}
}

/* Remove head and tail, for taking out the register name keyup and the keydown
 * for the 'q' command to terminate recording. */
void
trimQueue(struct ksq *q)
{
	struct KeyStroke *ks;

	EnterCriticalSection(&g.ksq_cs);

	ks = TAILQ_FIRST(q);
	TAILQ_REMOVE(q, ks, entries);
	ks = TAILQ_LAST(q, ksq);
	TAILQ_REMOVE(q, ks, entries);
	LeaveCriticalSection(&g.ksq_cs);
}

void
startRecordTo(enum VimifyRegister reg)
{
	if (!isValidRegister(reg)) { return; }

	EnterCriticalSection(&g.ksq_cs);
	g.reg_recording_to = reg;
	delQueueUnsafe(&g.ksq_reg[g.reg_recording_to]);
	LeaveCriticalSection(&g.ksq_cs);
}

void
startAppendTo(enum VimifyRegister reg)
{
	PDEBUG("q<REGISTER> not implemented\n");
	EnterCriticalSection(&g.ksq_cs);
	g.reg_recording_to = reg;
	LeaveCriticalSection(&g.ksq_cs);
}

void
vimifyDot(void)
{
	PDEBUG(". not implemented\n");
}

int
curState(enum VimifyState query)
{
	return g.state == query;
}

void
setState(enum VimifyState newstate)
{
	g.state = newstate;
}

DWORD
toPunc(DWORD vkCode)
{
	DWORD ret = vkCode;
	int n = vkCode - '0';
	char c = '\0';

	if ((0 <= n) && (n <= 9)) {
		c = ")!@#$%^&*("[n];
		PDEBUG("Returning 0x%02x (%c)\n", c, c);
		ret = c;
	}

	return ret;
}

/** Return nonzero if the keystroke must be deleted */
int
vimifyKeystroke(struct KeyStroke *ks)
{
	int swallow = 0;

	WPARAM wParam = ks->wParam;
	PKBDLLHOOKSTRUCT pkhs = &ks->khs;
	int shift = ks->shift;
	int control = ks->control;

	/* For convenient evaluation */
	int is_key_down = (wParam == WM_KEYDOWN);
	int is_key_up = (wParam == WM_KEYUP);
	int is_syskey_down = (wParam == WM_SYSKEYDOWN);
	int is_syskey_up = (wParam == WM_SYSKEYUP);

	enum VimifyRegister reg = reg_none;
	int valid_reg = 0;
	int is_ucase = 0;

	DWORD translated;

	// PDEBUG("\n");

	translated = translateKey(pkhs->vkCode);
	PDEBUG(
		"[%s mode] [key%s] %s %s vk 0x%02x ('%c') => 0x%02x\n",
		g.insert_mode? "Insert": "Command",
		wParam == WM_KEYDOWN? "down": "up",
		g.shift?  "SHIFT": "",
		g.control?  "CTRL": "",
		pkhs->vkCode,
		pkhs->vkCode,
		translated
	);

	if (
		(translated == VK_SHIFT) || 
		(translated == VK_LSHIFT) || 
		(translated == VK_RSHIFT)
	   )
	{
		PDEBUG("VK_SHIFT\n");
	}

	suspendMods(shift, control);

	// NOTE: If we accidentally trap WM_SYSKEY[DOWN|UP], we'll mess with
	// Alt+Tab and so on
	if ((g.swallow_keyup_vkcode == ks->khs.vkCode) && (is_key_up || is_syskey_up)) {
		PDEBUG("Swallowing vkCode 0x%02x (%c)\n", g.swallow_keyup_vkcode, g.swallow_keyup_vkcode);
		g.swallow_keyup_vkcode = 0;
		swallow = 1;
	} else if (is_key_up) {
		if (!g.insert_mode) {
			switch (translated) {
				/* I track SHIFT and CONTROL keystrokes (both keyup and
				 * keydown) because I think they'll become useful, but as of
				 * this writing, they are unused. */
				CASE_SHIFT:
					PDEBUG("Shift keyup, g.shift = 0\n");
					g.shift = 0;
					break;

				CASE_CONTROL:
					PDEBUG("Control keyup, g.control = 0\n");
					g.control = 0;
					break;
			}
		}
	} else if (is_key_down) {
		if (g.insert_mode) {
			switch (translated) {
			case VK_ESCAPE:
				PDEBUG("Escape from insert mode, g.insert_mode = 0\n");
				g.insert_mode = 0;
				swallow = 1;
				break;

			case 'w':
				if (control) {
					PDEBUG("^W, cutting previous word\n");
					depressKey(VK_SHIFT);
					sendKey(VK_CONTROL, VK_LEFT);
					releaseKey(VK_SHIFT);
					cut();
					swallow = 1;
				}
				break;
			}
		} else { // Command mode
			if (curState(vs_colon)) {
				switch (translated) {
					case 'q':
						// PDEBUG(":q, g.exit = 1\n");
						// g.exit = 1;
						PDEBUG(":q, normalizing\n");
						vimifyNormalize(g.hwnd_ui);
						g.vimify_pid = 0;
						break;

					case 'w':
						PDEBUG(":w, saving\n");
						sendKey(VK_CONTROL, 'S');
						break;
				}

				swallow = 1;
				setState(vs_none);

			} else if (curState(vs_search)) {
				switch (translated) {
				/* Pass everything through, but try to notice when the search
				 * dialog is closed. This is pretty dodgy, eh? */
				case VK_ESCAPE:
					setState(vs_none);
				}

			} else if (curState(vs_r)) {
				switch (translated) {
				CASE_IGNORE_MODKEYS:
					PDEBUG("r..., ignoring mod key\n");
					break;

				default:
					PDEBUG("r%c\n", translated);
					sendKey(VK_SHIFT, VK_RIGHT);
					setState(vs_none);
				}

			} else if (curState(vs_R)) {
				if (translated == VK_ESCAPE) {
					setState(vs_none);
				} else {
					sendKey(VK_SHIFT, VK_RIGHT);
				}

			} else if (curState(vs_g)) {
				switch (translated) {
				case 'g':
					PDEBUG("gg, issuing Ctrl-Home\n");
					startMove();
					sendKey(VK_CONTROL, VK_HOME);
					endMove();
					swallow = 1;
					break;
				}
				setState(vs_none);

			} else if (curState(vs_z)) {
				switch (translated) {
				case 't':
					PDEBUG("zt, doing zt\n");
					do_zt();
					break;

				case 'b':
					PDEBUG("zb, doing zb\n");
					do_zb();
					break;
				}

				swallow = 1;
				setState(vs_none);

			} else if (curState(vs_q)) {
				reg = charToReg(translated);
				valid_reg = (reg != reg_none);

				if (valid_reg) {
					is_ucase = isUcaseReg(translated);

					PDEBUG(
						"Leaving vs_q (reg %c%s)\n",
						translated,
						is_ucase? " - append": ""
					   );
					setState(vs_none);

					/* Suppress the keyup from the register name */
					g.swallow_keyup_vkcode = ks->khs.vkCode;

					if (is_ucase) {
						startAppendTo(reg);
					} else {
						startRecordTo(reg);
					}
				} else {
					switch (translated) {
					CASE_IGNORE_MODKEYS:
						break;

					default:
						PDEBUG("Leaving vs_q, no valid register\n");
						setState(vs_none);
					}
				}
				swallow = 1;

			} else if (curState(vs_at)) {
				if (translated == '@') {
					/* TODO: This suffers from a bug wherein the SHIFT key
					 * remains depressed long enough to affect macro replay
					 * before the user's finger and the operating system
					 * release the SHIFT key. */
					PDEBUG("@@, replaying from %c\n", regToChar(g.reg_last));
					reg = g.reg_last;
				} else {
					reg = charToReg(translated);
				}

				valid_reg = (reg != reg_none);
				setState(vs_none);

				if (valid_reg) {
					PDEBUG("@%c, replaying macro %c\n", translated, regToChar(reg));
					startReplayMacroThread(reg);

					/* Suppress the keyup from the register name */
					g.swallow_keyup_vkcode = pkhs->vkCode;
				}
				PDEBUG("\n");

				swallow = 1;

			} else if (curState(vs_none) || curState(vs_d) || curState(vs_c) || curState(vs_y)) {
				switch (translated) {

				case VK_ESCAPE:
					/* Clear modes */
					g.selecting = 0;
					swallow = 1;
					break;

				case VK_OEM_1: /* :; of which : begins colon command */
					/* TODO: IMPLEMENT */
					if (shift) {
						PDEBUG(":, setState(vs_colon)\n");
						setState(vs_colon);
					}
					swallow = 1;
					break;

				/* Misc */
				case VK_OEM_2: /* /? Search */
					g.selecting = 0;
					PDEBUG("/, issuing Ctrl-F\n");
					sendKey(VK_CONTROL, 'F');
					setState(vs_search);
					swallow = 1;
					break;

				case VK_OEM_PERIOD: /* Repeat (.) command */
					if (!g.selecting) {
						PDEBUG("., repeating last command\n");
						vimifyDot();
					} else {
						PDEBUG(". during g.selecting, ignored\n");
					}
					swallow = 1;
					break;

				case 'q': /* Record to macro */
					if (g.reg_recording_to == reg_none) {
						PDEBUG("q (opening), setState(vs_q)\n");
						setState(vs_q);
					} else {
						PDEBUG("q (closing), g.reg_recording_to = reg_none\n");
						if (isValidRegister(g.reg_recording_to)) {
							trimQueue(&g.ksq_reg[g.reg_recording_to]);
						}
						dumpMacro(g.reg_recording_to);
						g.reg_recording_to = reg_none;
					}
					swallow = 1;
					break;

				case '@':
					g.selecting = 0;
					PDEBUG("@, setState(vs_at)\n");
					setState(vs_at);
					swallow = 1;
					break;

				case 'u': /* Undo */
					g.selecting = 0;
					PDEBUG("u, issuing Ctrl-Z\n");
					sendKey(VK_CONTROL, 'Z');
					swallow = 1;
					break;

				case 'v': /* Visual mode selection */
					g.selecting = !g.selecting;
					PDEBUG("v, toggling g.selecting to %d\n", g.selecting);
					swallow = 1;
					break;

				case 'z':
					PDEBUG("z, setState(vs_z)\n");
					setState(vs_z);
					g.selecting = 0;
					swallow = 1;
					break;

				/* Line operations */
				case 'J':
					g.selecting = 0; // Alas, not going to support multi-line
					PDEBUG("J, issuing end + Del + space");
					sendKey(0, VK_END);
					sendKey(0, VK_DELETE);
					sendKey(0, VK_SPACE);
					swallow = 1;
					break;

				/* Ways to replace characters */
				case '~':
					if (g.selecting) {
						
					} else {
					}
					break;
				case 'r':
					g.selecting = 0;
					if (control) { /* Redo */
						PDEBUG("^R, issuing Ctrl-Y\n");
						sendKey(VK_CONTROL, 'Y');
					} else { /* Replace character */
						PDEBUG("r, setState(vs_r)\n");
						setState(vs_r);
					}
					swallow = 1;
					break;

				case 'R': /* REPLACE mode */
					g.selecting = 0;
					setState(vs_R);
					swallow = 1;
					break;

				/* Ways to delete */
				case 'X':
					PDEBUG("x, issuing Left + Del\n");
					cutPrev();
					swallow = 1;
					break;

				case 'x':
					if (g.selecting) {
						PDEBUG("x (selecting), ending select with cut\n");
						cut();
						g.selecting = 0;
					} else {
						PDEBUG("x, issuing cutNext\n");
						cutNext();
					}

					swallow = 1;
					break;

				case 'C':
					g.selecting = 0;
					PDEBUG("C, issuing Shift-End + cut + g.insert_mode = 1\n");
					sendKey(VK_SHIFT, VK_END);
					cut();
					g.insert_mode = 1;
					swallow = 1;
					break;

				case 'c':
					if (curState(vs_c)) {
						PDEBUG("cc, changing whole line\n");
						sendKey(0, VK_HOME);
						sendKey(VK_SHIFT, VK_END);
						cut();
						g.insert_mode = 1;
					} else if (g.selecting) {
						PDEBUG("d (selecting), ending select with cut\n");
						cut();
						g.selecting = 0;
						g.insert_mode = 1;
					} else {
						PDEBUG("c, entering vs_c mode\n");
						setState(vs_c);
						g.selecting = 1;
					}

					swallow = 1;
					break;

				case 'D':
					g.selecting = 0;
					PDEBUG("D, issuing Shift-End + cut\n");
					sendKey(VK_SHIFT, VK_END);
					cut();
					swallow = 1;
					break;

				case 'd': 
					if (curState(vs_d)) {
						PDEBUG("dd, cutting whole line\n");
						sendKey(0, VK_HOME);
						sendKey(VK_SHIFT, VK_END);
						cut();
						sendKey(0, VK_DELETE);
						g.selecting = 0;
						setState(vs_none);
					} else if (g.selecting) {
						PDEBUG("d (selecting), ending select with cut\n");
						cut();
						g.selecting = 0;
					} else {
						PDEBUG("d, entering vs_d mode\n");
						setState(vs_d);
						g.selecting = 1;
					}

					swallow = 1;
					break;

				/* Ways to copy */
				case 'y':
					if (curState(vs_y)) {
						PDEBUG("yy, copying whole line\n");
						sendKey(0, VK_HOME);
						sendKey(VK_SHIFT, VK_END);
						copy();
					} else if (g.selecting) {
						PDEBUG("y (selecting), ending select with copy\n");
						copy();
						g.selecting = 0;
					} else {
						PDEBUG("y, entering vs_y mode\n");
						setState(vs_y);
						g.selecting = 1;
					}

					swallow = 1;
					break;

				/* Ways to paste */
				case 'P':
					g.selecting = 0;
					PDEBUG("P, issuing Right + Ctrl-V\n");
					paste();
					swallow = 1;
					break;

				case 'p':
					g.selecting = 0;
					PDEBUG("p, issuing Ctrl-V\n");
					moveRight();
					paste();
					swallow = 1;
					break;

				/* Ways to enter insert mode */
				case 'O':
					g.selecting = 0;
					PDEBUG("0, releasing Shift and issuing home/return/up\n");
					sendKey(0, VK_HOME);
					sendKey(0, VK_RETURN);
					sendKey(0, VK_UP);
					g.insert_mode = 1;
					swallow = 1;
					break;

				case 'o':
					g.selecting = 0;
					PDEBUG("o, issuing End/Return\n");
					sendKey(0, VK_END);
					sendKey(0, VK_RETURN);
					g.insert_mode = 1;
					swallow = 1;
					break;

				case 's':
					PDEBUG("s, issuing Del, g.insert_mode = 1\n");
					cutNext();
					g.selecting = 0;
					g.insert_mode = 1;
					swallow = 1;
					break;

				case 'i':
					g.selecting = 0;
					PDEBUG("i, g.insert_mode = 1\n");
					g.insert_mode = 1;
					swallow = 1;
					break;

				case 'a':
					g.selecting = 0;
					PDEBUG("a, appending after char\n");
					sendKey(0, VK_RIGHT);
					g.insert_mode = 1;
					swallow = 1;
					break;

				case 'A':
					g.selecting = 0;
					PDEBUG("A, appending at end of line\n");
					sendKey(0, VK_END);
					g.insert_mode = 1;
					swallow = 1;
					break;

				case 'I':
					g.selecting = 0;
					PDEBUG("I, issuing Home + g.insert_mode = 1\n");
					sendKey(0, VK_HOME);
					swallow = 1;
					g.insert_mode = 1;
					break;

				/* Motions */
				case 'h':
					PDEBUG("h, issuing Left arrow\n");
					moveLeft();
					checkEndCopyDel();
					swallow = 1;
					break;

				case 'j':
					PDEBUG("j, issuing Down arrow\n");
					moveDown();
					checkEndCopyDel();
					swallow = 1;
					break;

				case 'k':
					PDEBUG("k, issuing Up arrow\n");
					moveUp();
					checkEndCopyDel();
					swallow = 1;
					break;

				case 'l':
					PDEBUG("l, issuing Right arrow\n");
					moveRight();
					checkEndCopyDel();
					swallow = 1;
					break;

				case 'B': /* BACK */
				case 'b': /* Back */
					PDEBUG("b, issuing Ctrl-Left\n");
					moveBack();
					checkEndCopyDel();
					swallow = 1;
					break;

				case 'e': // Not the same, but as close as I can get
				case 'w':
				case 'W':
					PDEBUG("e/w, issuing Ctrl-Right\n");
					moveWord();
					checkEndCopyDel();
					swallow = 1;
					break;

				case '0':
					PDEBUG("0, issuing Home\n");
					moveHome();
					checkEndCopyDel();
					swallow = 1;
					break;

				case 'g':
					PDEBUG("g, state = vs_g\n");
					setState(vs_g);
					swallow = 1;
					break;

				case 'G':
					PDEBUG("G, issuing Ctrl-End + Home\n");
					startMove();
					sendKey(VK_CONTROL, VK_END);
					sendKey(0, VK_HOME);
					endMove();
					checkEndCopyDel();
					swallow = 1;
					break;

				case '$':
					PDEBUG("$, issuing End\n");
					moveEnd();
					checkEndCopyDel();
					swallow = 1;
					break;

				CASE_SHIFT:
					PDEBUG("Shift, g.shift = 1\n");
					g.shift = 1;
					break;

				CASE_CONTROL:
					PDEBUG("Control, g.control = 1\n");
					g.control = 1;
					break;

				CASE_ALT:

				case VK_UP:
				case VK_DOWN:
				case VK_LEFT:
				case VK_RIGHT:
					PDEBUG("Alt or Up/Down/Left/Right, passing through\n");
					break;

				default:
					swallow = 1;
				}
			} // per g.state

		}
	}

	resumeMods(shift, control);

	return swallow;
}

DWORD
getForegroundPid(void)
{
	DWORD pid = 0;
	HWND hWndForeground = NULL;

	hWndForeground = GetForegroundWindow();
	if (NULL != hWndForeground) {
		GetWindowThreadProcessId(hWndForeground, &pid);
	}
	return pid;
}

DWORD
foregroundTargeted(void)
{
	return g.vimify_pid && (getForegroundPid() == g.vimify_pid);
}

int
keyInjected(PKBDLLHOOKSTRUCT pkhs)
{
	return (pkhs->flags & LLKHF_INJECTED);
}

LRESULT
CALLBACK
vimifyLlkbd(
	int nCode,
	WPARAM wParam,
	LPARAM lParam
)
{
	int swallow = 0;
	PKBDLLHOOKSTRUCT pkhs = NULL;
	struct KeyStroke ks = {0};
	int replaying = 0;
	int injected = 0;
	int was_macro_inj = 0;
	int should_vimify = 0;
	struct KeyStroke *ks_cpy = NULL;

	if (g.exit) {
		/* Unhooking due to :q command while still inside vimifyKeystroke
		 * causes the 'q' keydown event to pass through, so the exit command is
		 * processed on the next keystroke, which is the 'q' keyup event. */
		PDEBUG("Terminating due to g.exit != 0\n");
		ExitProcess(0);
	}

	if (nCode == HC_ACTION) {
		pkhs = (PKBDLLHOOKSTRUCT)lParam;

		injected = keyInjected(pkhs);
		replaying = (g.reg_replaying_from != reg_none);
		was_macro_inj = kst_macro_replay == pkhs->dwExtraInfo;
		should_vimify = !injected || (replaying && was_macro_inj);

		ks.wParam = wParam;
		ks.khs = *pkhs;
		ks.shift = shiftIsDepressed();
		ks.control = controlIsDepressed();

		/* If recording a macro to a register... */
		maybeRecordKeystroke(&ks);

		if (should_vimify && foregroundTargeted()) {
			swallow = vimifyKeystroke(&ks);
		}
	}

	return swallow || CallNextHookEx(g.hhk, nCode, wParam, lParam);
}

void
vimifyMessageLoop(void)
{
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0) != 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

int
vimifyInit(void)
{
	int ret = 1;
	int i = 0;

	g.state = vs_none;
	g.hhk = 0;

	g.reg_recording_to = reg_none;
	g.reg_replaying_from = reg_none;
	g.reg_last = reg_none;

	TAILQ_INIT(&g.ksq_dot);

	for (i=0; i<reg_max; i++) {
		if (isValidRegister(i)) {
			TAILQ_INIT(&g.ksq_reg[i]);
		}
	}

	if (!InitializeCriticalSectionAndSpinCount(&g.ksq_cs, 0x500)) {
		goto exit_vimifyInit;
	}

	initClip();

	ret = 0;
exit_vimifyInit:
	return ret;
}

int
vimify(HINSTANCE hInstance, DWORD pid)
{
	int ret = 1;
	HWND hWnd = NULL;

	g.vimify_pid = pid;

	hWnd = vimifyUI(hInstance, &g.vimify_pid);
	if (!hWnd) {
		goto exit_vimify;
	}

	g.hwnd_ui = hWnd;

	g.hhk = SetWindowsHookExA(WH_KEYBOARD_LL, vimifyLlkbd, NULL, 0);
	if (!g.hhk) {
		goto exit_vimify;
	}

	vimifyMessageLoop();

	ret = 0;

exit_vimify:
	return ret;
}
