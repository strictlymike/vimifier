#pragma once

#define LBL ""
#if DEBUG
#define PDEBUG(_fmt, ...) printf(LBL _fmt, __VA_ARGS__)
#else
#define PDEBUG(_fmt, ...)
#endif

