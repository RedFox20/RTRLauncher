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
    char buffer[8192];
    va_list ap;
    va_start(ap, fmt);
    logstr(buffer, vsnprintf(buffer, sizeof(buffer), fmt, ap));
    return logs;
}
logstream& logstr(const char* str, int len)
{
    DWORD written;
    WriteConsoleA(STDOUT, str, len, &written, nullptr);
    if (LogFile)
        WriteFile(LogFile, str, len, &written, nullptr);
    return logs;
}

logstream& logsec(const char* secName, const char* fmt, ...)
{
    char buffer[8192];
    va_list ap;
    va_start(ap, fmt);

    // write [section]' '
    int seclen = strlen(secName);
    memcpy(buffer, secName, seclen);
    buffer[seclen++] = ' ';

    logstr(buffer, seclen + vsnprintf(buffer + seclen, sizeof(buffer), fmt, ap));
    return logs;
}


logstream& logsec(const char* secName, rpp::strview text)
{
    char buffer[8192];
    // write [section]' '
    int seclen = strlen(secName);
    memcpy(buffer, secName, seclen);
    buffer[seclen++] = ' ';

    memcpy(buffer+seclen, text.c_str(), text.length());
    seclen += text.length();
    buffer[seclen] = '\0';

    logstr(buffer, seclen);
    return logs;
}

logstream& logput(const char* s)
{
    logstr(s, strlen(s));
    return logs;
}

void logflush()
{
    if (LogFile)
        FlushFileBuffers(LogFile);
}

void show_message_box(rpp::strview message, rpp::strview title)
{
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK);
}
