/**
 * Copyright (c) 2014 - Jorma Rebane
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

	void console_initialize(const char* title
#ifdef __cplusplus
							= 0
#endif
							);

	void console_size(int bufferW, int bufferH, int windowW, int windowH);

	/**
	 * Very fast console IO. The console is initialized
	 * with first call to these console functions
	 */
	int console(const char* buffer, int len);
	int consolef(const char* fmt, ...);

	/**
	 * Digest application commandline into an argv array
	 * @param argv An array of char*[maxCount]
	 * @param maxCount Maximum number of elements that fit into argv
	 * @return Number of items parsed (argc)
	 */
	int process_cmdline(char** argv, int maxCount);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <rpp/strview.h>
template<int SIZE> int console(const char (&str)[SIZE])
{
	return console(str, SIZE - 1);
}
inline int console(const rpp::strview& s)
{
	return console(s.c_str(), s.length());
}
#endif

#ifdef __cplusplus

struct constream {};
extern constream cons;
inline constream& con() { return cons; }


inline constream& _constrm(char value)
{
	console(&value, 1);
	return cons;
}
inline constream& _constrm(bool value)
{
	value ? console("true", 4) : console("false", 5);
	return cons;
}
inline constream& _constrm(int value)
{
	consolef("%d", value);
	return cons;
}
inline constream& _constrm(float value)
{
	consolef("%g", value);
	return cons;
}
inline constream& _constrm(double value)
{
	consolef("%g", value);
	return cons;
}
inline constream& _constrm(const std::string& s)
{
	console(s.c_str(), s.length());
	return cons;
}
inline constream& _constrm(const char* str)
{
	console(str, strlen(str));
	return cons;
}
template<class T> inline constream& operator<<(constream& os, const T& value)
{
	return _constrm(value);
}


inline constream& operator<<(constream& os, const rpp::strview& s)
{
	console(s.c_str(), s.length());
	return cons;
}
template<int SIZE> inline constream& operator<<(constream& os, char (&str)[SIZE])
{
	console(str, strlen(str));
	return cons;
}
template<int SIZE> inline constream& operator<<(constream& os, const char (&str)[SIZE])
{
	console(str, SIZE - 1);
	return cons;
}


inline constream& endl(constream& os)
{
	console("\n", 1); // insert newline
	return cons;
}

#endif