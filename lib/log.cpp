#include "log.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "io/console.h"
logstream logs;

static HANDLE STDOUT  = NULL;
static HANDLE LogFile = NULL;
static char Buffer[8192]; // temporary buffer


static void close_log()
{
	if (LogFile)
	{
		CloseHandle(LogFile);
		LogFile = NULL;
	}
}

void logger_init(const char* logfile, const char* consoleTitle)
{
	console_initialize(consoleTitle);
	STDOUT = GetStdHandle(STD_OUTPUT_HANDLE);

	close_log();
	if (!logfile)
		return;

	LogFile = CreateFileA(logfile, FILE_WRITE_DATA|FILE_APPEND_DATA, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if (LogFile == INVALID_HANDLE_VALUE)
		LogFile = 0;
	else
		atexit(close_log);
}





logstream& log(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logstr(Buffer, vsprintf(Buffer, fmt, ap));
	return logs;
}
logstream& logstr(const char* str, int len)
{
	DWORD written;
	WriteConsoleA(STDOUT, str, len, &written, 0);
	if (LogFile)
		WriteFile(LogFile, str, len, &written, 0);
	return logs;
}

logstream& logsec(const char* secName, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	// write [section]' '
	int seclen = strlen(secName);
	memcpy(Buffer, secName, seclen);
	Buffer[seclen++] = ' ';

	logstr(Buffer, seclen + vsprintf(Buffer + seclen, fmt, ap));
	return logs;
}

logstream& logput(const char* s)
{
	logstr(s, strlen(s));
	return logs;
}