#pragma once
#include <rpp/strview.h>
using rpp::strview;

// all we need is some forward declarations:
struct EmlValue;


enum ParseFlags {
	None = 0,				//
	NullTerminate	= 1,	// modifies the buffer to null-terminate strings
	KeepComments	= 2,	// parses and creates comment nodes
	UnknownFlag		= 4,	// ??
};

struct EmlParser
{
	const char* Start;		// parse buffer start
	const char* End;		// parse buffer end
	int ParseFlags;			// parse flags
	int Line;				// error line
	int Column;				// error column
	char Error[232];		// error message

	EmlParser() : Line(0), Column(0)
	{
		Error[0] = 0;
	}


	/**
	 * The actual parse method.
	 * @note The resulting EmlValue hierarchy will reference
	 *
	 * @param flags ParseFlags to control parsing options such as NullTerminate strings
	 * @param out Parsed EML root node
	 * @param buffer Input buffer which is not modified unless NullTerminate flag is used
	 */
	bool parse(int flags, EmlValue& out, strview& buffer);


	bool parse(int flags, EmlValue& out, const strview& buffer)
	{
		strview s(buffer);
		return parse(flags, out, s);
	}
	template<int SIZE> bool parse(int flags, EmlValue& out, const char(&string)[SIZE])
	{
		return parse(flags, out, token(string, SIZE - 1));
	}
	bool parse(int flags, EmlValue& out, const char* buffer, int length)
	{
		return parse(flags, out, strview(buffer, length));
	}
	bool parse(int flags, EmlValue& out, const char* buffer, const char* end)
	{
		return parse(flags, out, strview(buffer, end));
	}


	template<int flags = 0> bool parse(EmlValue& out, strview& buffer)
	{
		return parse(flags, out, buffer);
	}
	template<int flags = 0> bool parse(EmlValue& out, const strview& buffer)
	{
		token s(buffer);
		return parse(flags, out, s);
	}
	template<int flags = 0, int SIZE> bool parse(EmlValue& out, const char (&string)[SIZE])
	{
		return parse(flags, out, token(string, SIZE - 1));
	}
	template<int flags = 0> bool parse(EmlValue& out, const char* buffer, int length)
	{
		return parse(flags, out, token(buffer, length));
	}
	template<int flags = 0> bool parse(EmlValue& out, const char* buffer, const char* end)
	{
		return parse(flags, out, token(buffer, end));
	}


private:


	bool parse_comments	(EmlValue& out, strview& buffer);
	bool parse_value	(EmlValue& out, strview& buffer);
	bool parse_object	(EmlValue& out, strview& buffer);
	bool parse_array	(EmlValue& out, strview& buffer);
	bool parse_func		(EmlValue& out, strview& buffer);
	bool parse_str		(EmlValue& out, strview& buffer);
	bool parse_number	(EmlValue& out, strview& buffer);
	bool parse_bool		(EmlValue& out, strview& buffer);



	// error format, with the current buffer state, returns true to signal Error:true
	bool errorf(strview pos, const char* fmt, ...);

	// takes a small code snapshot of the target position (for errors)
	const char* snapshot(strview pos, int width);


	bool bracket_mismatch			(const strview& pos);
	bool unrecognized_token			(const strview& pos);
	bool unexpected_token			(const strview& pos);
	bool unexpected_literal			(const strview& got, const char* expected);
	bool missing_eos				(const strview& pos, char eosChar);
	bool invalid_comment			(const strview& pos);
	bool expected_named_identifier	(const strview& pos, char ch);
	bool expected_named_identifier	(const strview& pos, const strview& str);
};