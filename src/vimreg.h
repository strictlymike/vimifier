#pragma once

#define VIMREG_0 0
#define VIMREG_A 10
#define VIMREG_NUM_DELTA ((int)'0')
#define VIMREG_ALPHA_DELTA (((int)'a') - VIMREG_A)
#define VIMREG_UPPERALPHA_DELTA (((int)'A') - VIMREG_A)

/* In practice, subtract 'a' - reg_a to get diff and use that to translate
 * registers by letter */
enum VimifyRegister {
	/* 0-9 */
	reg_0 = VIMREG_0,
	reg_1, reg_2, reg_3, reg_4, reg_5, reg_6, reg_7, reg_8, reg_9,

	/* a-z */
	reg_a = VIMREG_A,
	reg_b, reg_c, reg_d, reg_e, reg_f, reg_g, reg_h, reg_i, reg_j, reg_k,
	reg_l, reg_m, reg_n, reg_o, reg_p, reg_q, reg_r, reg_s, reg_t, reg_u,
	reg_v, reg_w, reg_x, reg_y, reg_z,

	reg_max,
	reg_none = 0x0fffffff
};

enum VimifyRegister charToReg(char c);
char regToChar(enum VimifyRegister reg);
int isUcaseReg(char c);
int isValidRegister(enum VimifyRegister reg);
