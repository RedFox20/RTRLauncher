#pragma once
#include "CoreValue.h"




/**
 * The base EML StringBuffer for use by EmlWriter types
 */
struct EmlStrBuffer
{
	char* Ptr;		// current pointer in the buffer
	char* End;		// end of buffer
	char* Buffer;	// dynamic buffer base

	inline EmlStrBuffer() : Ptr(0), End(0), Buffer(0)
	{
	}
	inline ~EmlStrBuffer()
	{
		if (Buffer) free(Buffer);
	}

	/**
	 * Clear the current position in the buffer, without deallocating memory
	 */
	inline void clear()
	{
		Ptr = Buffer; // reset ptr
	}

	/**
	 * Get the contents of the buffer as an std::string copy
	 */
	inline std::string as_string() const
	{
		return std::string(Buffer, Ptr);
	}

protected:

	/**
	 * Make the internal buffer grow by the specified amount
	 * @note The buffer will always grow in 4KB alignments
	 */
	inline void grow(const int amount)
	{
		// always align amount to upper 4KB boundary
		int growth = amount;
		if (int rem = growth % 4096) 
			growth += (4096 - rem);
		
		int capacity = int(End - Buffer) + growth;
		int position = int(Ptr - Buffer);
		Buffer = (char*)realloc(Buffer, capacity); // generous growth
		
		Ptr = Buffer + position; // restore Ptr / End
		End = Buffer + capacity;
	}

public:

	/**
	 * Ensure that the specified number of bytes is available to be written to *Ptr.
	 */
	void reserve(int len)
	{
		if (int(End - Ptr) < len) grow(len);
	}

	/**
	 * Write a larger block of memory
	 */
	void put(const char* str, int len)
	{
		if (int(End - Ptr) < len) grow(len);
		memcpy(Ptr, str, len); // copy data
		Ptr += len;
	}

	/**
	 * Write a single char to the buffer
	 */
	void put(char ch)
	{
		if (Ptr == End) grow(4096);
		*Ptr++ = ch;
	}

	/**
	 * Output a token without any eosChar's to mark start/end of string
	 */
	inline void put(const token& str)
	{
		put(str.str, str.end - str.str);
	}

	/**
	 * Output a token, using the specified eosChar for start/end of "string"
	 */
	void put(const token& str, char eosChar)
	{
		const int len = str.length();
		reserve(len + 2);

		*Ptr++ = eosChar; // "
		memcpy(Ptr, str.str, len); // string
		Ptr += len;
		*Ptr++ = eosChar; // "
	}

	/**
	 * Output a float using default C-locale '.' separator
	 */
	void put(float number)
	{
		reserve(32); // reserve +32 bytes
		Ptr += _tostring(Ptr, number); // straight into destination buffer!
	}

	/**
	 * Output a boolean as true/false
	 */
	void put(bool boolean)
	{
		if (boolean)
		{
			reserve(4);
			memcpy(Ptr, "true", 4);
			Ptr += 4;
		}
		else
		{
			reserve(5);
			memcpy(Ptr, "false", 5);
			Ptr += 5;
		}
	}
};




/**
 * A very compact version of the EmlWriter that uses only the bare minimum
 * amount of separators and helps conserve size of the serialized EML string.
 */
struct CompactEmlWriter : public EmlStrBuffer
{
	/**
	 * Get a token reference to the contents of the buffer
	 */
	inline token str() const
	{
		return token(Buffer, Ptr);
	}

	/**
	 * Calls write(value) and gets a token reference to the contents of the buffer
	 */
	inline token str(const EmlValue& value)
	{
		write(value);
		return token(Buffer, Ptr);
	}

	/**
	 * Write an EmlValue into the underlying EmlStrBuffer
	 */
	void write(const EmlValue& value)
	{
		switch (value.Type)
		{
			case TEmlNull:		/* don't output null values */		break;
			case TEmlObject:	write(value.Object);				break;
			case TEmlArray:		write(value.Array);					break;
			case TEmlFunc:		write(value.Func);					break;
			case TEmlString:	put(value.String, value.EosChar);	break;
			case TEmlNumber:	put(value.Number);					break;
			case TEmlBool:		put(value.Bool);					break;
		}
	}

	void write(const EmlObject* object)
	{
		put('{');
		if (int count = object->Length)
		{
			for (auto* ptr = object->data(); count; --count, ++ptr)
			{
				put(ptr->Name); // write member name
				put(':');		// separate with :
				write(*ptr);	// write EmlValue
				if (count != 1)	// if not last ...
					put(',');	// delim with commas
			}
		}
		put('}');
	}
	void write_elements(const EmlValue* ptr, int count)
	{
		for (ptr; count; --count, ++ptr) {
			write(*ptr);	// write EmlValue
			if (count != 1)	// if not last ...
				put(',');	// delim with commas
		}
	}
	void write(const EmlArray* array)
	{
		put('[');
		if (int count = array->Length)
			write_elements(array->data(), count);
		put(']');
	}

	void write(const EmlFunc* func)
	{
		put('(');
		if (int count = func->Length)
			write_elements(func->data(), count);
		put(')');
	}
};




/**
 * Writes a formatted "pretty" EML, something that a human would write.
 * Extra effort is spent with object [width] prediction to properly fold
 * trivial arrays or objects.
 */
struct PrettyEmlWriter : public EmlStrBuffer
{
	/**
	 * Get a token reference to the contents of the buffer
	 */
	inline token str() const
	{
		return token(Buffer, Ptr);
	}

	/**
	 * Calls write(value) and gets a token reference to the contents of the buffer
	 */
	inline token str(const EmlValue& value)
	{
		write(value);
		return token(Buffer, Ptr);
	}

	/**
	 * Write an EmlValue into the underlying EmlStrBuffer
	 */
	void write(const EmlValue& value, int indent = 0)
	{
		switch (value.Type)
		{
			case TEmlNull:		/* don't output null values */		break;
			case TEmlObject:	write(value.Object, indent);		break;
			case TEmlArray:		write(value.Array, indent);			break;
			case TEmlFunc:		write(value.Func);					break;
			case TEmlString:	put(value.String, value.EosChar);	break;
			case TEmlNumber:	put(value.Number);					break;
			case TEmlBool:		put(value.Bool);					break;
		}
	}

	void write(const EmlObject* object, int indent)
	{
		put('{');
		if (int count = object->Length)
		{
			int n = count;
			int maxWidth = 0;
			int approxWidth = 0;
			for (auto* ptr = object->data(); n; --n, ++ptr)
			{
				const int nameLen = ptr->Name.length();
				if (maxWidth < nameLen)
					maxWidth = nameLen;

				if (approxWidth < 100)
				{
					approxWidth += nameLen;
					approxWidth += approximate_width(ptr);
				}
			}

			if (approxWidth <= 100) // use folded formatting
			{
				put(' ');
				for (auto* ptr = object->data(); count; --count, ++ptr)
				{
					put(ptr->Name);					// write member name
					put(": ", 2);					// separate with ": "
					write(*ptr);					// write EmlValue
					if (count != 1) folded_separator();	// write separator
				}
				put(' ');
			}
			else // use multi-line formatting
			{
				++maxWidth; // one extra space for ": {" case
				if (int rem = maxWidth % 4)
					maxWidth += 4 - rem; // also, align it to 4, otherwise it will look ugly

				const int childIndent = indent + 1;

				indented_newline(childIndent);
				for (auto* ptr = object->data(); count; --count, ++ptr)
				{
					int nameLen = ptr->Name.length();
					put(ptr->Name.str, nameLen);
					put(':');

					// now we use maxWidth to align all values under this object
					++nameLen; // include ':' in name for proper alignment
					if (int pad = maxWidth - nameLen) 
					{
						reserve(pad);
						if ((nameLen & 3) != 0)	   *Ptr++ = '\t'; // align first
						//for (; pad >= 4; pad -= 4) *Ptr++ = '\t'; // push rest as tabs
					}

					write(*ptr, childIndent);
					if (count != 1) multiline_separator(childIndent);	// write separator
				}
				indented_newline(indent);
			}
		}
		put('}');
	}

	inline int approximate_width(const EmlValue* ptr)
	{
		switch (ptr->Type)
		{
			case TEmlObject: 
			case TEmlArray: 
			case TEmlFunc:		return 100;	// subobjects force expand
			case TEmlString:	return ptr->getString().length();
			case TEmlNumber:	return 8;
			case TEmlBool:		return 6;
			default:			return 0;
		}
	}

	void write(const EmlArray* array, int indent)
	{
		put('[');
		if (int count = array->Length)
		{
			const int newIndent = indent + 1;
			auto* ptr = array->data();
			// arrays are expected to only contain same type of elements
			// in the case of trivial data, we should use complex folding

			bool complexFolding = ptr->Type == TEmlBool || ptr->Type == TEmlNumber;
			if (complexFolding)
			{
				put(' ');
				// this is a special case of formatting - newline after every 8 elements
				// [ 0, 1, 2, 3, 4, 5, 6, 7,
				//   8, 9, 10, 11, 12, 13, 14, 15,
				//   16, 17 ]
				//
				// distance to prev line for identation depth
				int indentDepth = 0;
				const char* p = Ptr - 3;
				for (char c = *p; c != '\n'; c = *--p) {
					if (c == '\t')	indentDepth += 4; // tabs are worth 4 chars
					else			++indentDepth;
				}

				int newLineCounter = 9;
				for (; count; --count, ++ptr)
				{
					if (!--newLineCounter) {
						put('\n');

						reserve(64);
						for (int i = indentDepth; i >= 4; i -= 4) *Ptr++ = '\t';
						*Ptr++ = ' '; // "  " to shadow "[ "
						*Ptr++ = ' ';
						newLineCounter = 9; // start over
					}
					write(*ptr);						// write EmlValue
					if (count != 1) folded_separator();	// write separator
				}
				put(' ');
			}
			else // use multi-line folding
			{
				indented_newline(newIndent);
				for (; count; --count, ++ptr) 
				{
					write(*ptr, newIndent);							// write EmlValue
					if (count != 1) multiline_separator(newIndent);	// write separator
				}
				indented_newline(indent);
			}
		}
		put(']');
	}

	void write(const EmlFunc* func)
	{
		put('(');
		if (int count = func->Length)
		{
			for (auto* ptr = func->data(); count; --count, ++ptr) 
			{
				write(*ptr);		// write EmlValue
				if (count != 1)		// if not last ...
					put(", ", 2);	// delim with ", "
			}
		}
		put(')');
	}

	inline void indented_newline(int indent)
	{
		*Ptr++ = '\n';
		for (int i = 0; i < indent; ++i) *Ptr++ = '\t'; // indentation
	}

	inline void multiline_separator(int indent)
	{
		reserve(64);
		*Ptr++ = ',';
		*Ptr++ = '\n';
		for (int i = 0; i < indent; ++i) *Ptr++ = '\t'; // indentation
	}

	inline void folded_separator()
	{
		reserve(2);
		*Ptr++ = ',';
		*Ptr++ = ' ';
	}
};
