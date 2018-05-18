#pragma once

#include <windows.h>

#include "vimreg.h"
#include "queue.h"

TAILQ_HEAD(ksq, KeyStroke);

enum VimifyState {
	vs_none,
	vs_g,
	vs_r,
	vs_R,
	vs_d,
	vs_c,
	vs_z,
	vs_q,
	vs_at,
	vs_search /* / and ? */,
	vs_colon,
	vs_y,
};

struct VimifyStateStruct {
	int vimify_pid;
	HWND hwnd_ui;
	HHOOK hhk;
	enum VimifyState state;

	CRITICAL_SECTION ksq_cs;
	struct ksq ksq_reg[reg_max];
	struct ksq ksq_dot;

	enum VimifyRegister reg_recording_to;
	enum VimifyRegister reg_replaying_from;
	enum VimifyRegister reg_last;

	int exit;
	int insert_mode;
	int selecting;

	DWORD swallow_keyup_vkcode;

	int shift;
	int control;
};

extern struct VimifyStateStruct g;


