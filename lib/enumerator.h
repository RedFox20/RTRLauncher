#pragma once
/**
 * Automatic enum Enumerator, Copyright (c) 2014 - Jorma Rebane
 */
#include <rpp/strview.h>


template<class T> struct _enum
{
    static const rpp::strview data[];
    static const int maxValue;
};

template<class T> struct _enum_unknown
{
    static const rpp::strview value; // unknown
};

template<> const rpp::strview _enum_unknown<void>::value = "unknown";



template<class T> T enum_parse(const rpp::strview& tok)
{

    const rpp::strview* start = _enum<T>::data;
    const rpp::strview* end = start + sizeof(_enum<T>::data) / sizeof(rpp::strview);

    const char* str  = tok.c_str();
    const int strlen = tok.length();

    //// @todo Perhaps something better than linear search would be nice?
    for (const rpp::strview* p = start; p != end; ++p)
        if (p->equals(str, strlen))
            return T(p - start);
    return T(-1); // unknown value (-1)
}

template<class T> void enum_parse(rpp::strview token, T& out)
{
    out = enum_parse<T>(token);
}



template<class T> rpp::strview enum_str(T value)
{
    int index = static_cast<int>(value);
    return (0 <= index && index <= _enum<T>::maxValue) 
        ? _enum<T>::data[index] 
        : _enum_unknown<void>::value;
}
template<class T> const char* enum_cstr(T value)
{
    int index = static_cast<int>(value);
    return (0 <= index && index <= _enum<T>::maxValue) 
        ? _enum<T>::data[index].c_str()
        : _enum_unknown<void>::value.c_str();
}



#define ENUM_EXPAND(x) x
//#define ENUM_EXPAND(...) __VA_ARGS__
#define ENUM_ARG_N( \
          _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
         _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
         _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
         _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
         _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
         _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
         _61, _62, _63, N, ...) N
#define ENUM_NARG(...)	ENUM_EXPAND(ENUM_ARG_N(_, ##__VA_ARGS__, 62, 61, 60, \
         59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
         49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
         39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
         29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
         19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
         9, 8, 7, 6, 5, 4, 3, 2, 1, 0))


#define ENUM_STR__(func, argc, ...) func ## argc(ENUM_EXPAND(__VA_ARGS__))
#define ENUM_STR_(func, argc, ...)  ENUM_STR__(func, argc, __VA_ARGS__)
#define ENUM_STR(...)               ENUM_STR_(ENUM_STR, ENUM_NARG(__VA_ARGS__), __VA_ARGS__)
#define ENUM_STR0()
#define ENUM_STR1(a)			#a
#define ENUM_STR2(a,b)			#a, #b
#define ENUM_STR3(a,b,c)		#a, #b, #c
#define ENUM_STR4(a,b,c,d)		#a, #b, #c, #d
#define ENUM_STR5(a,b,c,d,e)	#a, #b, #c, #d, #e
#define ENUM_STR6(a,b,c,d,e,f)	#a, #b, #c, #d, #e, #f
#define ENUM_STR7(a,b,c,d,e,f,g)		#a, ENUM_STR6(b,c,d,e,f,g)
#define ENUM_STR8(a,b,c,d,e,f,g,h)		#a, ENUM_STR7(b,c,d,e,f,g,h)
#define ENUM_STR9(a,b,c,d,e,f,g,h,i)	#a, ENUM_STR8(b,c,d,e,f,g,h,i)
#define ENUM_STR10(a,b,c,d,e,f,g,h,i,j)	#a, ENUM_STR9(b,c,d,e,f,g,h,i,j)
#define ENUM_STR11(a,b,c,d,e,f,g,h,i,j,k)		#a, ENUM_STR10(b,c,d,e,f,g,h,i,j,k)
#define ENUM_STR12(a,b,c,d,e,f,g,h,i,j,k,l)		#a, ENUM_STR11(b,c,d,e,f,g,h,i,j,k,l)
#define ENUM_STR13(a,b,c,d,e,f,g,h,i,j,k,l,m)	#a, ENUM_STR12(b,c,d,e,f,g,h,i,j,k,l,m)
#define ENUM_STR14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)	#a, ENUM_STR13(b,c,d,e,f,g,h,i,j,k,l,m,n)
#define ENUM_STR15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)		#a, ENUM_STR14(b,c,d,e,f,g,h,i,j,k,l,m,n,o)
#define ENUM_STR16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)		#a, ENUM_STR15(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
#define ENUM_STR17(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)	#a, ENUM_STR16(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)
#define ENUM_STR18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)	#a, ENUM_STR17(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)
#define ENUM_STR19(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)		#a, ENUM_STR18(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)
#define ENUM_STR20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)		#a, ENUM_STR19(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)
#define ENUM_STR21(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u)	#a, ENUM_STR20(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u)
#define ENUM_STR22(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)	#a, ENUM_STR21(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)
#define ENUM_STR23(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w)		#a, ENUM_STR22(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w)
#define ENUM_STR24(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x)		#a, ENUM_STR23(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x)
#define ENUM_STR25(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y)	#a, ENUM_STR24(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y)
#define ENUM_STR26(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z) #a, ENUM_STR25(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)
#define ENUM_STR27(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A)		#a, ENUM_STR26(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A)
#define ENUM_STR28(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B)		#a, ENUM_STR27(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B)
#define ENUM_STR29(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C)	#a, ENUM_STR28(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C)
#define ENUM_STR30(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D)	#a, ENUM_STR29(b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D)
//// @note Add more if needed ^

#define ENUM_PREFIX__(p, func, argc, ...)	func ## argc(p, ENUM_EXPAND(__VA_ARGS__))
#define ENUM_PREFIX_(p, func, argc, ...)	ENUM_PREFIX__(p, func, argc, __VA_ARGS__)
#define ENUM_PREFIX(p, ...)					ENUM_PREFIX_(p, ENUM_PREFIX, ENUM_NARG(__VA_ARGS__), __VA_ARGS__)
#define ENUM_PREFIX0()
#define ENUM_PREFIX1(�,a)			� ## a
#define ENUM_PREFIX2(�,a,b)			� ## a, � ## b
#define ENUM_PREFIX3(�,a,b,c)		� ## a, � ## b, � ## c
#define ENUM_PREFIX4(�,a,b,c,d)		� ## a, � ## b, � ## c, � ## d
#define ENUM_PREFIX5(�,a,b,c,d,e)	� ## a, � ## b, � ## c, � ## d, � ## e
#define ENUM_PREFIX6(�,a,b,c,d,e,f)	� ## a, � ## b, � ## c, � ## d, � ## e, � ## f
#define ENUM_PREFIX7(�,a,b,c,d,e,f,g)		� ## a, ENUM_PREFIX6(�,b,c,d,e,f,g)
#define ENUM_PREFIX8(�,a,b,c,d,e,f,g,h)		� ## a, ENUM_PREFIX7(�,b,c,d,e,f,g,h)
#define ENUM_PREFIX9(�,a,b,c,d,e,f,g,h,i)	� ## a, ENUM_PREFIX8(�,b,c,d,e,f,g,h,i)
#define ENUM_PREFIX10(�,a,b,c,d,e,f,g,h,i,j)� ## a, ENUM_PREFIX9(�,b,c,d,e,f,g,h,i,j)
#define ENUM_PREFIX11(�,a,b,c,d,e,f,g,h,i,j,k)		� ## a, ENUM_PREFIX10(�,b,c,d,e,f,g,h,i,j,k)
#define ENUM_PREFIX12(�,a,b,c,d,e,f,g,h,i,j,k,l)	� ## a, ENUM_PREFIX11(�,b,c,d,e,f,g,h,i,j,k,l)
#define ENUM_PREFIX13(�,a,b,c,d,e,f,g,h,i,j,k,l,m)	� ## a, ENUM_PREFIX12(�,b,c,d,e,f,g,h,i,j,k,l,m)
#define ENUM_PREFIX14(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n)� ## a, ENUM_PREFIX13(�,b,c,d,e,f,g,h,i,j,k,l,m,n)
#define ENUM_PREFIX15(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)		� ## a, ENUM_PREFIX14(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o)
#define ENUM_PREFIX16(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)	� ## a, ENUM_PREFIX15(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
#define ENUM_PREFIX17(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)	� ## a, ENUM_PREFIX16(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)
#define ENUM_PREFIX18(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)� ## a, ENUM_PREFIX17(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)
#define ENUM_PREFIX19(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)		� ## a, ENUM_PREFIX18(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)
#define ENUM_PREFIX20(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)	� ## a, ENUM_PREFIX19(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)
#define ENUM_PREFIX21(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u)	� ## a, ENUM_PREFIX20(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u)
#define ENUM_PREFIX22(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)� ## a, ENUM_PREFIX21(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)
#define ENUM_PREFIX23(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w)		� ## a, ENUM_PREFIX22(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w)
#define ENUM_PREFIX24(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x)	� ## a, ENUM_PREFIX23(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x)
#define ENUM_PREFIX25(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y)	� ## a, ENUM_PREFIX24(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y)
#define ENUM_PREFIX26(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)� ## a, ENUM_PREFIX25(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)
#define ENUM_PREFIX27(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A)		� ## a, ENUM_PREFIX26(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A)
#define ENUM_PREFIX28(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B)	� ## a, ENUM_PREFIX27(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B)
#define ENUM_PREFIX29(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C)	� ## a, ENUM_PREFIX28(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C)
#define ENUM_PREFIX30(�,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D)� ## a, ENUM_PREFIX29(�,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D)
//// @note Add more if needed ^


#define ENUM_BASE_(Type, ...) \
    template<> const int _enum<Type>::maxValue = (int)Type::Type##__last - 1; \
    template<> const rpp::strview _enum<Type>::data[] = {  ENUM_STR(__VA_ARGS__) };

#define ENUM_BASE(Type, DataType, ...) \
    enum Type : DataType { Type##_Unknown=-1, __VA_ARGS__, Type##__last }; ENUM_BASE_(Type, __VA_ARGS__)

#define ENUM_CLASS_BASE(Type, DataType, ...) \
    enum class Type : DataType { Type##_Unknown=-1, ## __VA_ARGS__, Type##__last }; ENUM_BASE_(Type, __VA_ARGS__)

#define ENUM_PREFIXED(Type, Prefix, DataType, ...) \
    enum Type : DataType { Type##_Unknown=-1, ENUM_PREFIX(Prefix, __VA_ARGS__), Type##__last }; ENUM_BASE_(Type, __VA_ARGS__)

/**
 * @brief Creates an enum that can be automatically stringified with enum_str<T> or parsed with enum_parse<T>
 * @usage Enum(MyEnum, Value1, Value2, Value3); 
 * @usage MyEnum value    = enum_parse<MyEnum>("Value1");
 * @usage const char* str = enum_str(Value1);
 */
#define Enum(Type, ...)            ENUM_BASE(Type, int, __VA_ARGS__)
#define EnumByte(Type, ...)        ENUM_BASE(Type, char, __VA_ARGS__)
#define EnumShort(Type, ...)       ENUM_BASE(Type, short, __VA_ARGS__)
#define EnumClass(Type, ...)       ENUM_CLASS_BASE(Type, int, __VA_ARGS__)
#define EnumClassByte(Type, ...)   ENUM_CLASS_BASE(Type, char, __VA_ARGS__)
#define EnumClassShort(Type, ...)  ENUM_CLASS_BASE(Type, short, __VA_ARGS__)

/**
 * @brief Prefixes enum values with the chosen prefix, while leaving strings intact
 * @usage EnumPrefix(MyEnum, ME, Value1, Value2, Value3);
 * @usage MyEnum value = enum_parse<MyEnum>("Value1");
 * @usage const char* str = enum_str(MEValue1);
 */
#define EnumPrefix(Type, Prefix, ...)      ENUM_PREFIXED(Type, Prefix, int, __VA_ARGS__) 
#define EnumPrefixByte(Type, Prefix, ...)  ENUM_PREFIXED(Type, Prefix, char, __VA_ARGS__)
#define EnumPrefixShort(Type, Prefix, ...) ENUM_PREFIXED(Type, Prefix, short, __VA_ARGS__)