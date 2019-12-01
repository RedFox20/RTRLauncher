#include "console.h"
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fcntl.h> // _O_TEXT
#include <io.h>    // _open_osfhandle
#include <ios>     // std::ios::sync_with_stdio()
constream cons;

static HANDLE STDOUT = nullptr;
static const char DebugConsole[] = "Debug Console";

static void init_window(const char* title)
{
    if (!GetConsoleWindow())
    {
        AllocConsole(); // allocate a console for this process
        SetConsoleTitleA(title ? title : DebugConsole);
    }
    STDOUT = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof cfi;
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(STDOUT, FALSE, &cfi);
}

extern "C" void console_size(int bufferW, int bufferH, int windowW, int windowH)
{
    if (!STDOUT) init_window(DebugConsole);

    CONSOLE_SCREEN_BUFFER_INFOEX csbi;
    csbi.cbSize = sizeof csbi;
    GetConsoleScreenBufferInfoEx(STDOUT, &csbi);
    csbi.dwSize.X = bufferW;
    if (csbi.dwSize.Y < bufferH) csbi.dwSize.Y = bufferH;
    csbi.srWindow.Right = windowW;
    csbi.srWindow.Bottom = windowH;
    csbi.dwMaximumWindowSize.X = windowW;
    csbi.dwMaximumWindowSize.Y = windowH;
    SetConsoleScreenBufferInfoEx(STDOUT, &csbi);
}

extern "C" void console_initialize(const char* title)
{
    if (STDOUT) return;

    init_window(title);
    console_size(90, 1000, 90, 30);

    #define hook(cstd, WINSTDHANDLE, mode) \
    *cstd = *_fdopen(_open_osfhandle((long)GetStdHandle(WINSTDHANDLE), _O_TEXT), mode); \
    setvbuf(cstd, NULL, _IONBF, 0);

    // redirect unbuffered STD handles to CRT files
    hook(stdout, STD_OUTPUT_HANDLE, "w");
    hook(stdin,  STD_INPUT_HANDLE,  "r");
    hook(stderr, STD_ERROR_HANDLE,  "w");

    // sync cout, wcout, cin, wcin, wcerr, cerr, wclog and clog to CRT files as well
    std::ios::sync_with_stdio();
}

extern "C" int console(const char* buffer, int len)
{
    if (len <= 0)
        return 0;

    if (!STDOUT)
        console_initialize(DebugConsole);

    DWORD written;
    WriteConsoleA(STDOUT, buffer, len, &written, 0);
    return (int)written;
}

extern "C" int consolef(const char* fmt, ...)
{
    char buffer[8192];
    va_list ap;
    va_start(ap, fmt);
    return console(buffer, vsnprintf(buffer, 8192, fmt, ap));
}

int process_cmdline(char** argv, int maxCount)
{
    int argc = 0;
    for (char* ptr = GetCommandLineA(); char ch = *ptr; ++ptr)
    {
        if (ch == ' ' || ch == '\t') continue; // skip whitespace
        else if (ch == '\"') // encountered a \"
        {
            char* start = ++ptr;
            for (; ch = *ptr; ++ptr)
                if (ch == '\"' && ptr[-1] != '\\') // break on ", but not on \"
                    break;
            *ptr = '\0';
            argv[argc++] = start;
        }
        else // regular token
        {
            char* start = ptr;
            for (; ch = *ptr; ++ptr) // skip until whitespace
                if (ch == ' ' || ch == '\t')
                    break;
            *ptr = '\0';
            argv[argc++] = start;
        }
        if (argc >= maxCount) break;
    }
    return argc;
}