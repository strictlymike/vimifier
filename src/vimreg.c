#include "vimreg.h"

enum VimifyRegister
charToReg(char c)
{
	enum VimifyRegister retval = reg_none;

	if (('a' <= c) && (c <= 'z')) {
		retval = (c - VIMREG_ALPHA_DELTA);
	} else if (('A' <= c) && (c <= 'Z')) {
		retval = (c - VIMREG_ALPHA_DELTA);
	} else if (('0' <= c) && (c <= '9')) {
		retval = (c - VIMREG_NUM_DELTA);
	}

	return retval;
}

char
regToChar(enum VimifyRegister reg)
{
	char retval = 0xff;

	if ((reg_0 <= reg) && (reg <= reg_9)) {
		retval = reg + VIMREG_NUM_DELTA;
	} else if ((reg_a <= reg) && (reg <= reg_z)) {
		retval = reg + VIMREG_ALPHA_DELTA;
	}

	return retval;
}

int
isUcaseReg(char c)
{
	return ('A' <= c) && (c <= 'Z');
}

int
isValidRegister(enum VimifyRegister reg)
{
	return ((reg_0 <= reg) && (reg < reg_max));
}

