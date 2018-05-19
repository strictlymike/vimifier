#include <windows.h>
#include <TlHelp32.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xgetopt.h"

#include "config.h"
#include "vimify.h"
#include "clip.h"

#if VIMIFY_CUI
#define ARGC argc
#define ARGV argv
#else
#define ARGC __argc
#define ARGV __argv
#endif

struct VimifyOpts {
	union {
		struct {
			unsigned int pid:1,
				start:1,
				name:1;
		};
		int all;
	} flags;

	union {
		int pid;
		char *name;
		char *start;
	} parm;
};

int Usage(int is_err, char *argv0);
int findFirstPidByName(char *progname);
int startAndReturnPid(char *cmdline);
char *filename(char *exename);
int exeNameMatchesProgname(char *progname, char *exename);

#if VIMIFY_CUI
int
main(int argc, char **argv)
#else
int
WINAPI
WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
   )
#endif
{
	struct VimifyOpts opts = {0};
	int pid = 0;
	char c;
	HANDLE hProc = NULL;
#if VIMIFY_CUI
	HINSTANCE hInstance = NULL;
#endif

	while ((c = getopt(ARGC, ARGV, "p:s:n:h")) != -1) {
		switch (c)
		{
			case 'p':
				opts.flags.pid = 1;
				opts.parm.pid = atoi(optarg);
				break;

			case 's':
				opts.flags.start = 1;
				opts.parm.name = optarg;
				break;

			case 'n':
				opts.flags.name = 1;
				opts.parm.start = optarg;
				break;

			case 'h':
				return Usage(0, ARGV[0]);

			default:
				return Usage(1, ARGV[0]);
		}
	}

	if ((opts.flags.pid + opts.flags.name + opts.flags.start) > 1) {
		fprintf(stderr, "Only one of -p, -n, -s can be specified\n");
		return Usage(1, ARGV[0]);
	}

	if (opts.flags.pid) {
		hProc = OpenProcess(SYNCHRONIZE, FALSE, opts.parm.pid);
		if (!hProc) {
			fprintf(
				stderr,
				"Can't access pid %d, error %d\n",
				opts.parm.pid,
				GetLastError()
			   );
		} else {
			pid = opts.parm.pid;
			CloseHandle(hProc);
		}
	} else if (opts.flags.name) {
		pid = findFirstPidByName(opts.parm.name);
		if (!pid) {
			fprintf(stderr, "Can't find process named %s\n", opts.parm.name);
		}
	} else if (opts.flags.start) {
		pid = startAndReturnPid(opts.parm.start);
	}

	if (vimifyInit()) {
		fprintf(stderr, "Error initializing vimify\n");
		return 1;
	}

	if (vimify(hInstance, pid)) {
		fprintf(stderr, "Failure\n");
		return 1;
	}

	return 0;
}

int
Usage(int is_err, char *argv0)
{
	FILE *out = is_err? stderr: stdout;
	fprintf(out, "Usage: %s [-p <pid> | -s <prog> | -n <name>]\n", argv0);
	fprintf(out, "  -p <pid>  Attach to running program with PID <pid>\n");
	fprintf(out, "  -s <prog> Start program <prog> Vimified\n");
	fprintf(out, "  -n <name> Attach to running program named <name>\n");
	return is_err;
}

char *
filename(char *exename)
{
	char *fn = NULL;

	fn = strrchr(exename, '\\');
	if (fn) {
		fn++;
	}

	if (!fn) {
		fn = exename;
	}

	return fn;
}

int
exeNameMatchesProgname(char *progname, char *exename)
{
	char *loc = NULL;
	size_t strlen_progname = 0;
	size_t strlen_exename = 0;

	exename = filename(exename);

	strlen_progname = strlen(progname);
	strlen_exename = strlen(exename);

	if (strlen_exename < strlen_progname) {
		return 0;
	}

	loc = strstr(exename, progname);

	if ((loc == exename)) {
		if (strlen_progname == strlen_exename) {
			return 1;
		} else if (strlen_exename = (strlen_exename + 4)) {
			if (!strcmp(".exe", &exename[strlen_progname])) {
				return 1;
			}
		}
	}

	return 0;
}

int
findFirstPidByName(char *progname)
{
	int pid = 0;
	HANDLE hSnap = INVALID_HANDLE_VALUE;
	PROCESSENTRY32 pe = {0};
	char *exe_file_name = NULL;

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnap == INVALID_HANDLE_VALUE) {
		fprintf(
			stderr,
			"CreateToolhelp32Snapshot failed, %d\n",
			GetLastError()
		   );
		goto exit_findFirstPidByName;
	}

	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnap, &pe)) {
		fprintf(stderr, "Process32First failed, %d\n", GetLastError());
		goto exit_findFirstPidByName;
	}

	do {
		if (exeNameMatchesProgname(progname, pe.szExeFile)) {
			pid = pe.th32ProcessID;
			break;
		}
	} while (Process32Next(hSnap, &pe));

exit_findFirstPidByName:
	if (INVALID_HANDLE_VALUE != hSnap) {
		CloseHandle(hSnap);
	}

	return pid;
}

int
startAndReturnPid(char *cmdline)
{
	int pid = 0;
	BOOL ok = FALSE;
	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};

	ok = CreateProcessA(
		NULL,
		cmdline,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi
	   );
	if (!ok) {
		fprintf(stderr, "CreateProcessA failed, %d\n", GetLastError());
	} else {
		pid = pi.dwProcessId;
	}

	return pid;
}
