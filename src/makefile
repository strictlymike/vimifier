TARGETS = vimify.exe

CC = cl.exe
CFLAGS = $(CFLAGS) /DDEBUG=1 /Zi
LIBS =
RM = DEL /Q
CLEANFILES = $(TARGETS) *.exp *.obj *.pdb *.ilk

all: $(TARGETS)

VIMIFY_SOURCES = \
	xgetopt.c \
	vimify.c \
	vimreg.c \
	keyaut.c \
	moves.c \
	clip.c \
	ui.c \
	resource.res \
	main.c

vimify.exe: $(VIMIFY_SOURCES)
	$(CC) $(CFLAGS) /Fe$@ $**

resource.res: resource.rc
	rc.exe /fo $@ $**

cliptest.exe: cliptest.c clip.c

.cpp.exe:
	$(CC) $** $(LIBS) $(CFLAGS)

.c.exe:
	$(CC) $** $(LIBS) $(CFLAGS)

.cpp.dll:
	$(CC) /LD $** $(LIBS) $(CFLAGS)

.c.dll:
	$(CC) /LD $** $(LIBS) $(CFLAGS)

clean:
	$(RM) $(CLEANFILES) > NUL 2>&1
