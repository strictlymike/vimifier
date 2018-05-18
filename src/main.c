#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "vimify.h"
#include "clip.h"

int Usage(int is_err, char *argv0);

int
main(int argc, char **argv)
{
	int pid = 0;

	if (argc > 2) {
		return Usage(1, argv[0]);
	}

	if (argc == 2) {
		pid = atoi(argv[1]);
	}

	if (vimifyInit()) {
		fprintf(stderr, "Error initializing vimify\n");
		return 1;
	}

	if (vimify(pid)) {
		fprintf(stderr, "Failure\n");
		return 1;
	}

	return 0;
}

int
Usage(int is_err, char *argv0)
{
	FILE *out = is_err? stderr: stdout;
	fprintf(out, "Usage: %s [pid]", argv0);
	return is_err;
}
