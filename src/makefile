TARGETS = vimify.exe cliptest.exe

CC = cl.exe
CFLAGS = $(CFLAGS) /DDEBUG=1 /Zi
LIBS =
RM = DEL /Q
CLEANFILES = $(TARGETS) *.exp *.obj *.pdb *.ilk

all: $(TARGETS)


vimify.exe: main.c vimify.c vimreg.c keyaut.c moves.c clip.c ui.c
	$(CC) $(CFLAGS) /Fe$@ $**

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
