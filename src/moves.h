#pragma once

#include "keyaut.h"

void startMove(void);
void endMove(void);
void moveLeft(void);
void moveRight(void);
void moveUp(void);
void moveDown(void);
void moveHome(void);
void moveEnd(void);
void moveBack(void);
void moveWord(void);
void cut(void);
void copy(void);
void cutPrev(void);
void cutNext(void);
void paste(void);
void wrapMoveWithSelect(DWORD vkMod, DWORD vkCode);
void checkEndCopyDel(void);
void do_zt(void);
void do_zb(void);

