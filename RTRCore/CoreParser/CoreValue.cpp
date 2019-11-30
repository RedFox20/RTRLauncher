#include "CoreValue.h"


	// VALUE

	EmlValue::~EmlValue()
	{
		switch (Type)
		{
			case TEmlNull:							break;
			case TEmlObject:	Object->release();	break;
			case TEmlArray:		Array->release();	break;
			case TEmlFunc:		Func->release();	break;
			case TEmlString:						break;
			case TEmlNumber:						break;
			case TEmlBool:							break;
		}
	}

	EmlValue::EmlValue(const EmlValue& value)
	{
		switch (Type = value.Type)
		{
			case TEmlNull:		Null   = nullptr;					break;
			case TEmlObject:	(Object = value.Object)->addref();	break;
			case TEmlArray:		(Array  = value.Array)->addref();	break;
			case TEmlFunc:		(Func   = value.Func)->addref();	break;
			case TEmlString:	String = value.String, 
								EosChar = value.EosChar;			break;
			case TEmlNumber:	Number = value.Number;				break;
			case TEmlBool:		Bool   = value.Bool;				break;
		}
	}
	EmlValue& EmlValue::operator=(const EmlValue& value)
	{
		if (this != &value)
		{
			this->~EmlValue(); // destroy old
			switch (Type = value.Type)
			{
				case TEmlNull:		Null   = nullptr;					break;
				case TEmlObject:	(Object = value.Object)->addref();	break;
				case TEmlArray:		(Array  = value.Array)->addref();	break;
				case TEmlFunc:		(Func   = value.Func)->addref();	break;
				case TEmlString:	String = value.String, 
									EosChar = value.EosChar;			break;
				case TEmlNumber:	Number = value.Number;				break;
				case TEmlBool:		Bool   = value.Bool;				break;
			}
		}
		return *this;
	}

	EmlValue::EmlValue(EmlValue&& value)
	{
		switch (Type = value.Type)
		{
			case TEmlNull:		Null   = nullptr;		break;
			case TEmlObject:	Object = value.Object;	break;
			case TEmlArray:		Array  = value.Array;	break;
			case TEmlFunc:		Func   = value.Func;	break;
			case TEmlString:	String = value.String, 
								EosChar = value.EosChar;break;
			case TEmlNumber:	Number = value.Number;	break;
			case TEmlBool:		Bool   = value.Bool;	break;
		}
		value.Type = TEmlNull;
	}
	EmlValue& EmlValue::operator=(EmlValue&& value)
	{
		switch (Type = value.Type)
		{
			case TEmlNull:		Null   = nullptr;		break;
			case TEmlObject:	Object = value.Object;	break;
			case TEmlArray:		Array  = value.Array;	break;
			case TEmlFunc:		Func   = value.Func;	break;
			case TEmlString:	String = value.String, 
								EosChar = value.EosChar;break;
			case TEmlNumber:	Number = value.Number;	break;
			case TEmlBool:		Bool   = value.Bool;	break;
		}
		value.Type = TEmlNull;
		return *this;
	}


	void EmlValue::destroy()
	{
		this->~EmlValue();
		Type = TEmlNull;
	}



	// OBJECT IMPLEMENTATION

	EmlObject::~EmlObject()
	{
		if (Capacity) free(Ptr);
	}

	void EmlObject::push_back(const EmlNamedValue& value)
	{
		if (Capacity) // currently using dynamic buffer
		{
			if (Length == Capacity) // need to resize?
			{
				Capacity += Capacity / 2; // always 150% growth
				if (int rem = Capacity % 8) Capacity += (8 - rem); // upper boundary aligned to 8
				Ptr = (EmlNamedValue*)realloc(Ptr, sizeof(EmlNamedValue) * Capacity);
			}
			Ptr[Length++] = value;
		}
		else // currently using static buffer
		{
			if (Length == BUF_SIZE) // limit reached, should go dynamic
			{
				Capacity = BUF_SIZE * 2;
				EmlNamedValue* ptr = (EmlNamedValue*)malloc(sizeof(EmlNamedValue) * BUF_SIZE * 2);
				memcpy(ptr, Buf, sizeof(EmlNamedValue) * BUF_SIZE);
				
				ptr[Length++] = value;
				Ptr = ptr; // writeout
			}
			else
			{
				((EmlNamedValue*)Buf)[Length++] = value; // not yet reached static buffer limit
			}
		}
	}

	void EmlObject::erase_at(int index)
	{
		EmlNamedValue* ptr = Capacity ? Ptr : (EmlNamedValue*)Buf;
		EmlNamedValue* dst = ptr + index;
		EmlNamedValue* src = dst + 1;
		EmlNamedValue* end = ptr + Length;
		memmove(dst, src, (char*)end - (char*)src);
		--Length;
	}



	// ARRAY IMPLEMENTATION

	EmlArray::~EmlArray()
	{
		if (Capacity) free(Ptr);
	}

	void EmlArray::push_back(const EmlValue& value)
	{
		if (Capacity) // currently using dynamic buffer
		{
			if (Length == Capacity) // need to resize?
			{
				Capacity += Capacity / 2; // always 150% growth
				if (int rem = Capacity % 8) Capacity += (8 - rem); // upper boundary aligned to 8
				Ptr = (EmlValue*)realloc(Ptr, sizeof(EmlValue) * Capacity);
			}
			Ptr[Length++] = value;
		}
		else // currently using static buffer
		{
			if (Length == BUF_SIZE) // limit reached, should go dynamic
			{
				Capacity = BUF_SIZE * 2;
				EmlValue* ptr = (EmlValue*)malloc(sizeof(EmlValue) * BUF_SIZE * 2);
				memcpy(ptr, Buf, sizeof(EmlValue) * BUF_SIZE);
				
				ptr[Length++] = value;
				Ptr = ptr; // writeout
			}
			else
			{
				((EmlValue*)Buf)[Length++] = value; // not yet reached static buffer limit
			}
		}
	}

	void EmlArray::erase_at(int index)
	{
		EmlValue* ptr = Capacity ? Ptr : (EmlValue*)Buf;
		EmlValue* dst = ptr + index;
		EmlValue* src = dst + 1;
		EmlValue* end = ptr + Length;
		memmove(dst, src, (char*)end - (char*)src);
		--Length;
	}
