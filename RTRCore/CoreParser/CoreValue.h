#pragma once
#include "CoreParser.h"
#include <atomic>

/**
 * EML value type enumeration
 */
enum EmlValueType : unsigned char
{
    TEmlNull,		// null
    TEmlObject,		// object   { id:value, id2:"value", ... }
    TEmlArray,		// array    [   value,    "value",   ... ]
    TEmlFunc,		// function (   value,    "value",   ... )
    TEmlString,		// string   "..."   '...'
    TEmlNumber,		// number   0.05    1248
    TEmlBool,		// bool     true    false
};



/**
 * Forward declarations
 */
struct EmlObject;
struct EmlArray;
struct EmlFunc;


/**
 * Describes a single variant VALUE
 */
struct EmlValue
{
    union {
        const void*   Null;
        EmlObject*    Object;
        EmlArray*     Array;
        EmlFunc*      Func;
        rpp::strview_ String;
        float         Number;
        bool          Bool;
    };
    EmlValueType  Type;
    unsigned char EosChar;	// only applies for string type

    ~EmlValue();
    EmlValue(EmlValue&& value);
    EmlValue(const EmlValue& value);
    EmlValue& operator=(EmlValue&& value);
    EmlValue& operator=(const EmlValue& value);

    EmlValue()							: Type(TEmlNull)						{}
    EmlValue(EmlValueType type)			: Null(0),			Type(type) 			{}
    EmlValue(EmlObject* object)			: Object(object),	Type(TEmlObject)	{}
    EmlValue(EmlArray* array)			: Array(array),		Type(TEmlArray) 	{}
    EmlValue(EmlFunc* func)				: Func(func),		Type(TEmlObject)	{}
    EmlValue(float number)				: Number(number),	Type(TEmlNumber)	{}
    EmlValue(bool boolean)				: Bool(boolean),	Type(TEmlNumber)	{}
    EmlValue(rpp::strview string, char eosChar = '"')		
        : String{string.str, string.len}, Type(TEmlString), EosChar(eosChar)	{}


    /**
     * Destroys this EmlValue by nulling itself and deleting the Object, Array or Function objects
     */
    void destroy();

    bool isNull()	const { return Type == TEmlNull;			}
    bool isObject()	const { return Type == TEmlObject;			}
    bool isArray()	const { return Type == TEmlArray;			}
    bool isFunc()	const { return Type == TEmlFunc;			}	
    bool isString()	const { return Type == TEmlString;			}
    bool isNumber()	const { return Type == TEmlNumber;			}
    bool isBool()	const { return Type == TEmlBool;			}
    bool isTrue()	const { return Type == TEmlBool && Bool;	}
    bool isFalse()	const { return Type == TEmlBool && !Bool;	}

    EmlObject* getObject() const { return Type == TEmlObject ? Object : nullptr; }
    EmlArray*  getArray()  const { return Type == TEmlArray  ? Array  : nullptr; }
    EmlFunc*   getFunc()   const { return Type == TEmlFunc   ? Func   : nullptr; }
    strview    getString() const { return Type == TEmlString ? strview{String.str, String.len} : strview{}; }
    float      getFloat()  const { return Type == TEmlNumber ? Number : 0.0f; }
    int        getInt()    const { return Type == TEmlNumber ? (int)Number : 0; }
    bool       getBool()   const { return Type == TEmlBool   ? Bool   : false; }
};



/**
 * Represents a NAMED VALUE, used in EmlObject
 */
struct EmlNamedValue : EmlValue
{
    strview Name;
};


/**
 * Due to object hierarchy complexities, we must revert
 * to the cheap tactic of reference counted objects...
 */
struct IEmlReference
{
    std::atomic_int RefCount {1};
    IEmlReference() = default;
    virtual ~IEmlReference() = default;
    void addref()
    {
        ++RefCount;
    }
    void release()
    {
        if (--RefCount == 0) // no more refs?
            delete this; // invoke delete on self
    }
};



/**
 * Object holds NAMED VALUES -> { first:"hello", second:1392 }
 */
struct EmlObject : IEmlReference
{
    static const int BUF_SIZE = 4;
    int Length   = 0; // number of elements in the array
    int Capacity = 0; // capacity of Ptr
    union {
        char Buf[sizeof(EmlNamedValue) * BUF_SIZE]; // internal buffer
        EmlNamedValue* Ptr; // array pointer
    };

    EmlObject() = default;
    ~EmlObject();

    EmlNamedValue* data()							{ return Capacity ? Ptr : (EmlNamedValue*)Buf; }
    const EmlNamedValue* data() const				{ return Capacity ? Ptr : (EmlNamedValue*)Buf; }
    EmlNamedValue& operator[](int index)				{ return Capacity ? Ptr[index] : ((EmlNamedValue*)Buf)[index]; }
    const EmlNamedValue& operator[](int index) const	{ return Capacity ? Ptr[index] : ((EmlNamedValue*)Buf)[index]; }


    void push_back(const EmlNamedValue& value);
    void erase_at(int index);
};



/**
 * Array holds a list of unnamed VALUES -> [ "value1", 1234, true ]
 */
struct EmlArray : public IEmlReference
{
    static const int BUF_SIZE = 4;
    int  Length = 0;   // number of elements in the array
    int  Capacity = 0; // capacity of Ptr
    union {
        char       Buf[sizeof(EmlValue) * BUF_SIZE]; // internal buffer
        EmlValue*  Ptr;                              // array pointer
    };

    EmlArray() = default;
    ~EmlArray();

    EmlValue* data()								{ return Capacity ? Ptr : (EmlValue*)Buf; }
    const EmlValue* data() const					{ return Capacity ? Ptr : (EmlValue*)Buf; }
    EmlValue& operator[](int index)				{ return Capacity ? Ptr[index] : ((EmlValue*)Buf)[index]; }
    const EmlValue& operator[](int index) const	{ return Capacity ? Ptr[index] : ((EmlValue*)Buf)[index]; }

    void push_back(const EmlValue& value);
    void erase_at(int index);
};



/**
 * Function call holds an array of function arguments, so 
 * it inherits from EmlArray.
 *
 * Brackets are () and arg1 is parsed as a string literal
 *     VALUES -> ( arg1, "arg2", 1234, true )
 *
 */
struct EmlFunc : public EmlArray
{
};
