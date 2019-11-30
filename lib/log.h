#pragma once
#include "io\console.h"


struct logstream {};
extern logstream logs;
inline logstream& log() { return logs; }


/**
 * Attaches a console to this process and hooks all IO buffers.
 * All output is mirrored to the specified file. If no file mirror
 * is required, pass NULL as an argument.
 */
void logger_init(const char* logfile = "program.log", const char* consoleTitle = 0);
logstream& log(const char* fmt, ...);
logstream& logstr(const char* str, int len);
// log with section name
logstream& logsec(const char* secName, const char* fmt, ...);
logstream& logsec(const char* secName, rpp::strview text);
logstream& logput(const char* s);



inline logstream& _strmlog(char value)
{
    return logstr(&value, 1);
}
inline logstream& _strmlog(bool value)
{
    return value ? logstr("true", 4) : logstr("false", 5);
}
inline logstream& _strmlog(int value)
{
    return log("%d", value);
}
inline logstream& _strmlog(float value)
{
    return log("%g", value);
}
inline logstream& _strmlog(double value)
{
    return log("%g", value);
}
inline logstream& _strmlog(const std::string& s)
{
    return logstr(s.c_str(), s.length());
}
inline logstream& _strmlog(const char* str)
{
    return logstr(str, strlen(str));
}
template<class T> inline logstream& operator<<(logstream& os, const T& value)
{
    return _strmlog(value);
}


inline logstream& operator<<(logstream& os, const rpp::strview& s)
{
    return logstr(s.c_str(), s.length());
}
template<int SIZE> logstream& operator<<(logstream& os, char (&str)[SIZE])
{
    return logstr(str, strlen(str));
}
template<int SIZE> logstream& operator<<(logstream& os, const char (&str)[SIZE])
{
    return logstr(str, SIZE - 1);
}


inline logstream& endl(logstream& os)
{
    return logstr("\n", 1); // insert newline
}


void show_message_box(rpp::strview message, rpp::strview title);

