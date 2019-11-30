#include "CoreValue.h"
#include <assert.h>
#include <stdarg.h>






      /////                                    ////
     //         PARSER IMPLEMENTATION           //
    ////                                  ///////

    enum ParserStates
    {
        PNull	= 0,
        PObj	= 1,	
        PArr	= 2,
        PFun	= 3,
        PStr	= 4,
        PNum	= 5,
        PBool	= 6,
        PSep	= 7,	// separators   :  ,
        PEnd	= 8,	// bracket endings }])
    };
    static const unsigned char ParserIndex[256] = {
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    //    ! " # $ % & '   ( ) * + , - . /   0 1 2 3 4 5 6 7   8 9 : ; < = > ?
        0,0,4,0,0,0,0,4,  3,8,0,5,7,5,5,0,  5,5,5,5,5,5,5,5,  5,5,7,0,0,0,0,0,
    //  @ A B C D E F G   H I J K L M N O   P Q R S T U V W   X Y Z [ \ ] ^ _
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,2,0,8,0,0,
    //  ` a b c d e f g   h i j k l m n o   p q r s t u v w   x y z { | } ~
        0,0,0,0,0,0,6,0,  0,0,0,0,0,0,0,0,  0,0,0,0,6,0,0,0,  0,0,0,1,0,8,0,0,
    //	upper 128-255 is unused:
    };

    enum IdentifierStates
    {
        INull,
        IOperator	= 1,	// operators:  &&  ||  >  >=  <=  <  ==
        IIdentifier	= 2,	// start of identifier:  _  A-Z  a-z
    };
    static const unsigned char IdentifierIndex[256] = {
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    //    ! " # $ % & '   ( ) * + , - . /   0 1 2 3 4 5 6 7   8 9 : ; < = > ?
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    //  @ A B C D E F G   H I J K L M N O   P Q R S T U V W   X Y Z [ \ ] ^ _
        0,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,  2,2,2,0,0,0,0,2,
    //  ` a b c d e f g   h i j k l m n o   p q r s t u v w   x y z { | } ~
        0,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,  2,2,2,0,0,0,0,0,
    //	upper 128-255 is unused:
    };


    // run this test to validate the states if needed
    static void TestParserStates()
    {
        assert(PObj == ParserIndex['{']);
        assert(PArr == ParserIndex['[']);
        assert(PFun == ParserIndex['(']);
        assert(PStr == ParserIndex['\'']);
        assert(PStr == ParserIndex['"']);

        assert(PNum == ParserIndex['+']);
        assert(PNum == ParserIndex['-']);
        assert(PNum == ParserIndex['.']);
        assert(PNum == ParserIndex['0']);
        assert(PNum == ParserIndex['1']);
        assert(PNum == ParserIndex['2']);
        assert(PNum == ParserIndex['3']);
        assert(PNum == ParserIndex['4']);
        assert(PNum == ParserIndex['5']);
        assert(PNum == ParserIndex['6']);
        assert(PNum == ParserIndex['7']);
        assert(PNum == ParserIndex['8']);
        assert(PNum == ParserIndex['9']);

        assert(PBool == ParserIndex['t']);
        assert(PBool == ParserIndex['f']);
        assert(PSep == ParserIndex[':']);
        assert(PSep == ParserIndex[',']);

        assert(PEnd == ParserIndex['}']);
        assert(PEnd == ParserIndex[']']);
        assert(PEnd == ParserIndex[')']);
    }


    bool EmlParser::parse(int flags, EmlValue& out, strview& buffer)
    {
        Start      = buffer.str;
        End        = buffer.end();
        ParseFlags = flags;		 //// @todo Make <actual> use of ParseFlags

        // let's be kind to trim any leading whitespace, otherwise we'll fail
        // if someone passes "  { ... }"
        buffer.trim_start(" \t\r\n", 4);

        // if someone passes "  /*comment*/ { ... }" we should also catch that
        if (parse_comments(out, buffer))
            return false; // FAILED

        //// Internally we use FALSE:OK  TRUE:ERR for performance reasons
        //// On the API surface, we use TRUE:OK  FALSE:ERROR instead
        //// So in this case, flip the value:
        return !parse_value(out, buffer);
    }


    bool EmlParser::parse_value(EmlValue& out, strview& buffer)
    {
        // what kind of object is it?
        switch (ParserIndex[*buffer.str])
        {
            case PNull:	return unrecognized_token(buffer); // some stray unrecognized character
            case PObj:	return parse_object(out, buffer);
            case PArr:	return parse_array(out, buffer);
            case PFun:	return parse_func(out, buffer);
            case PStr:	return parse_str(out, buffer);
            case PNum:	return parse_number(out, buffer);
            case PBool:	return parse_bool(out, buffer);
            case PSep:	return unexpected_token(buffer); // a double ,, or :: probably?
            case PEnd:	return bracket_mismatch(buffer); // shouldn't happen in this context
        }
        return false;
    }



    // skip the specified comment, buffer state should be "/ comment" or "* comment"
    inline bool skip_comment(strview& buffer)
    {
        char c = *buffer.chomp_first().str; // chomp and get the next char
        if (c == '/')		buffer.chomp_first().skip_after('\n');		// LINE comment: "// comment\n"
        else if (c == '*')	buffer.chomp_first().skip_after("*/", 2);	// block comment: "/* comment */"
        else return true; // error!
        return false; // ok
    }

    bool EmlParser::parse_comments(EmlValue& out, strview& buffer)
    {
        char c = *buffer.str;
        if (c == '/') // potential comment?
        {
            if (ParseFlags & KeepComments)
            {
                const char* start = buffer.str; // this is where the comments started
                const char* end;
                do
                {
                    if (skip_comment(buffer)) return invalid_comment(buffer);
                    end = buffer.str;
                    buffer.trim_start();	// trim any whitespace after the comment
                    c = *buffer.str;		// recheck char
                } while (c == '/');

                end -= *end == '\n' ? 1 : 2;	// chomp_last '\n' or "*/"
                if (ParseFlags & NullTerminate) // force NullTerminate?
                    *(char*)end = '\0';			// hope it isn't a read-only buffer...
                strview comment(start, end);	//// @todo use the comment value
                return false;
            }
            else do // truncate comments
            {
                if (skip_comment(buffer)) return invalid_comment(buffer);
                buffer.trim_start();	// trim any whitespace after the comment
                c = *buffer.str;		// recheck char
            } while (c == '/');
        }
        return false; // no error
    }




    bool EmlParser::parse_object(EmlValue& out, strview& buffer)
    {
        EmlObject* object = new EmlObject();
        out.Type   = TEmlObject;
        out.Object = object;

        EmlNamedValue value;
        for (++buffer.str; ; ) // skip leading char
        {
            buffer.trim_start(", \t\r\n", 5); // trim any leading whitespace, including ','

            if (parse_comments(value, buffer)) // skip any commented out lines
                return true; // error!

            char ch = *buffer.str;
            if (ch == '}') // check for end of object
            {
                buffer.chomp_first();
                return false; // no error
            }

            // check for unnamed objects??
            if (int i = ParserIndex[ch]) // anyof {}[]():,"'+-.0123456789tf
            {
                if (i == PBool) // "true" or "false" identifier perhaps?
                {
                    strview(buffer).next(value.Name, ": \t{[(", 6);
                    if (value.Name == "true" || value.Name == "false")
                        return expected_named_identifier(buffer, value.Name);
                }
                else
                {
                    return expected_named_identifier(buffer, ch);
                }
            }

            buffer.next_notrim(value.Name, ": \t{[(", 6);	// split token name
            buffer.trim_start(": \t\r\n", 5);				// trim superfluous characters

            if (parse_value(value, buffer)) // parse the value
                return true; // error! 
            object->push_back(value);
        }
    }



    bool EmlParser::parse_array(EmlValue& out, strview& buffer)
    {
        EmlArray* array = new EmlArray();
        out.Type  = TEmlArray;
        out.Array = array;

        EmlValue value;
        for (++buffer.str; ; ) // skip leading char
        {
            buffer.trim_start(", \t\r\n", 5); // trim any leading whitespace, including ','

            if (parse_comments(value, buffer)) // skip any commented out lines
                return true; // error!

            // check for end of array:
            if (buffer.str[0] == ']') 
            {
                buffer.chomp_first();
                return false; // no error
            }

            if (parse_value(value, buffer)) // parse the value
                return true; // error! 
            array->push_back(value);
        }
    }



    bool EmlParser::parse_func(EmlValue& out, strview& buffer)
    {
        EmlFunc* func = new EmlFunc();
        out.Type = TEmlFunc;
        out.Func = func;

        EmlValue value;
        for (++buffer.str; ; ) // skip leading char
        {
            buffer.trim_start(", \t\r\n", 5); // trim any leading whitespace, including ','

            if (parse_comments(value, buffer)) // skip any commented out lines
                return true; // error!

            // check for end of function:
            if (*buffer.str == ')')
            {
                buffer.chomp_first();
                return false; // no error
            }

            if (parse_value(value, buffer)) // parse the value
                return true; // error! 
            func->push_back(value);
        }
    }
    
    

    bool EmlParser::parse_str(EmlValue& out, strview& buffer)
    {
        const char* str = buffer.begin();
        const char* end = buffer.end();
        if (str == end)
        {
            out.Type       = TEmlString;
            out.String.str = str;
            out.String.len = int(end-str);
            out.EosChar    = '"';

            if (ParseFlags & NullTerminate) // nullterminate if requested
                *(char*)str = 0;			// you better pray this wasn't a read-only string literal
            return false; // done, no errors, it's an empty string: ""
        }

        char eosChar = *str++;			// EOS character
        for (const char* start = str; str < end; ++str)
        {
            char ch = *str;
            if (ch == '\\') // escape sequence
            {
                ++str;		// skip '\\'
                continue;	// skip the escaped char
            }
            if (ch == eosChar) // end of string reached!
            {
                out.Type       = TEmlString;
                out.String.str = start;
                out.String.len = int(str-start);
                out.EosChar	   = eosChar;

                if (ParseFlags & NullTerminate) // nullterminate if requested
                    *(char*)str = 0;			// you better pray this wasn't a read-only string literal

                buffer.str     = ++str;		// now truncate eosChar
                return false;				// done, no errors
            }
        }

        return missing_eos(buffer, eosChar); // error!!
    }



    bool EmlParser::parse_number(EmlValue& out, strview& buffer)
    {
        out.Type   = TEmlNumber;
        out.Number = buffer.next_float();
        return false; // no error state! why? because we already know it's the start of a number
    }



    bool EmlParser::parse_bool(EmlValue& out, strview& buffer)
    {
        if (*buffer.str == 't')
        {
            strview s;
            buffer.next_notrim(s, ", \t\r\n", 5); // skip the literal until separators
            if (s != "true")
                return unexpected_literal(s, "true");
            out.Bool = true;
        }
        else // must be 'f'
        {
            strview s;
            buffer.next_notrim(s, ", \t\r\n", 5); // skip the literal until separators
            if (s != "false")
                return unexpected_literal(s, "false");
            out.Bool = false;
        }
        out.Type = TEmlBool;
        return false; // no error state
    }







    bool EmlParser::errorf(strview pos, const char* fmt, ...)
    {
        Line = 1;
        const char* lineStart = Start;
        for (const char* p = Start; p < pos.str; ++p) {
            if (*p == '\n') {
                lineStart = p + 1;
                ++Line;
            }
        }
        Column = int(pos.str - lineStart);

        // first sprint line and column
        int len = sprintf(Error, "line %d col %d: ", Line, Column);

        // then append rest of the format string
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(Error + len, sizeof(Error) - len, fmt, ap);
        return false;
    }

    // takes a little snapshot of the target position
    const char* EmlParser::snapshot(strview pos, int width)
    {
        static char snapshot[256];
        int w2 = width > 254 ? 127 : width / 2;
        const char* start = pos.str - w2;
        const char* end   = pos.str + w2;
        if (start < Start)	start = Start;
        if (End < end)		end = End;
        memcpy(snapshot, start, end - start);
        snapshot[end - start] = '\0';
        return snapshot;
    }

    bool EmlParser::bracket_mismatch(const strview& pos)
    {
        return errorf(pos, "Bracket mismatch! Token '%c' is unexpected at this point:\n%s", pos[0], snapshot(pos, 20));
    }
    bool EmlParser::unrecognized_token(const strview& pos)
    {
        return errorf(pos, "Syntax Error! Unrecognized token %c:\n%s", pos[0], snapshot(pos, 20));
    }
    bool EmlParser::unexpected_token(const strview& pos)
    {
        return errorf(pos, "Syntax Error! Unexpected token %c:\n%s", pos[0], snapshot(pos, 20));
    }
    bool EmlParser::unexpected_literal(const strview& got, const char* expected)
    {
        return errorf(got, "Syntax Error! Unexpected literal '%.*s' while expecting '%s':\n%s",
                                    got.length(), got.c_str(), expected, snapshot(got, 20));
    }
    bool EmlParser::missing_eos(const strview& pos, char eosChar)
    {
        return errorf(pos, "Failed to find End of String marker %c for the specified string.\n"
                      "Make sure the string value is properly terminated.\n", eosChar);
    }
    bool EmlParser::invalid_comment(const strview& pos)
    {
        return errorf(pos, "Expected start of COMMENT, found '/%c' instead.\n"
                      "Comments should start with // or /*.", pos[0]);
    }
    bool EmlParser::expected_named_identifier(const strview& pos, char ch)
    {
        return errorf(pos, "Expected start of member NAME, found token '%c' instead.\n"
                      "EmlObject Syntax '{ member1:\"string\", member2:1235, ... }'", ch);
    }
    bool EmlParser::expected_named_identifier(const strview& pos, const strview& str)
    {
        return errorf(pos, "Expected start of member NAME, found keyword '%.*s' instead.\n"
                      "EmlObject Syntax '{ member1:\"string\", member2:1235, ... }'", str.length(), str.c_str());
    }