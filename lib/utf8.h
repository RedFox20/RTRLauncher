#pragma once
/**
 * UTF8 <> UCS-2 (WCHAR) conversion methods
 * @author	Jorma Rebane
 * @date	30.11.2014
 */
#include <string>

/**
 * Encodes a WCHAR into an UTF8 sequence
 * @param wch Wide Char (UCS-2) to encode
 * @param dst Destination buffer at least [3] chars long
 * @return Number of bytes in the UTF8 sequence [1-3]
 */
int utf_encode(unsigned int wch, char* dst);


/**
 * Decodes an UTF8 sequence into a WCHAR
 * @param src Source UTF8 sequence
 * @param outch Destination WCHAR character
 * @return Number of bytes decoded from src: [1-4]
 */
int utf_decode(const char* src, wchar_t& outch);


/**
 * Decodes an UTF8 sequence into a WCHAR by first scanning backwards for
 * an appropriate UTF8 sequence start byte
 * @param src Source UTF8 sequence
 * @param outch Destination WCHAR character
 * @return Number of bytes decoded from src: [1-4]
 */
int utf_decode_reverse(const char* src, wchar_t& outch);


/**
 * Converts a WCHAR string into an UTF8 string. The DST is assumed to be large enough.
 * @param dst Destination buffer (assumed to be large enough)
 * @param str WCHAR string
 * @param len Length of WCHAR string
 * @return Size of the resulting UTF8 string in bytes.
 */
int utf8_convert(char* dst, const wchar_t* str, int len);


/**
 * Converts an UTF8 string into a WCHAR string. The DST is assumed to be large enough.
 * @param dst Destination buffer (assumed to be large enough)
 * @param str UTF8 string
 * @param len Length of UTF8 string
 * @return Length of the resulting WCHAR string.
 */
int utf8_convert(wchar_t* dst, const char* str, int len);


/**
 * Converts a WCHAR string into an UTF8 std::string.
 * @param str WCHAR string
 * @param len Length of WCHAR string
 * @return An UTF8 std::string
 */
std::string utf8_convert(const wchar_t* str, int len);


/**
 * Converts an UTF8 string into a WCHAR std::wstring.
 * @param str UTF8 string
 * @param len Length of UTF8 string
 * @return An UTF8 std::string
 */
std::wstring utf8_convert(const char* str, int len);