#include <windows.h>
#include <stdio.h>

#include "clip.h"

#pragma comment(lib, "user32")

void clipTest(void);
void clipTest_setClip(void);
void clipTest_getClip(void);

int
main(void)
{
	initClip(NULL);
	clipTest();
	return 0;
}

void
clipTest(void)
{
	printf("get 1\n");
	clipTest_getClip();
	Sleep(5000);

	printf("get 2\n");
	clipTest_getClip();
	Sleep(5000);

	printf("set\n");
	clipTest_setClip();
	Sleep(5000);

	printf("get 3\n");
	clipTest_getClip();
	Sleep(5000);
}

void
clipTest_setClip(void)
{
	int rv = 0;
	char *teststring = "Ace of spades";
	rv = setClip(teststring, 1 + strlen(teststring));
	if (rv) {
		fprintf(stderr, "rv3 %d\n", rv);
		return;
	}
}

void
clipTest_getClip(void)
{
	char *data = NULL;
	SIZE_T size = 0;
	int rv = 0;

	rv = getClip(NULL, &size);
	if (rv) {
		fprintf(stderr, "rv %d\n", rv);
		return;
	}

	printf("size: %d\n", size);

	data = malloc(size);
	if (!data) {
		fprintf(stderr, "malloc failed\n");
		return;
	}

	memset(data, 0, size);

	rv = getClip(data, &size);
	if (rv) {
		fprintf(stderr, "rv2 %d\n", rv);
		return;
	}

	printf("Was this your card: %s\n", data);
}


