#include "log.h"
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "io/console.h"
logstream logs;

static HANDLE STDOUT  = nullptr;
static HANDLE LogFile = nullptr;
static char Buffer[8192]; // temporary buffer


static void close_log()
{
    if (LogFile)
    {
        CloseHandle(LogFile);
        LogFile = nullptr;
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


logstream& logsec(const char* secName, rpp::strview text)
{
    // write [section]' '
    int seclen = strlen(secName);
    memcpy(Buffer, secName, seclen);
    Buffer[seclen++] = ' ';

    memcpy(Buffer+seclen, text.c_str(), text.length());
    seclen += text.length();
    Buffer[seclen] = '\0';

    logstr(Buffer, seclen);
    return logs;
}

logstream& logput(const char* s)
{
    logstr(s, strlen(s));
    return logs;
}

void show_message_box(rpp::strview message, rpp::strview title)
{
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK);
}
