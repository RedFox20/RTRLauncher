#pragma once

inline unsigned __int64 fnv_init()
{
	return 0xcbf29ce484222325;
}

unsigned __int64 fnv_hash(const void* data, int numBytes);
template<class T> inline unsigned __int64 fnv_hash(const T& data)
{
	return fnv_hash(&data, sizeof(T));
}

void fnv_combine(unsigned __int64& inoutSeed, const void* data, int numBytes);
template<class T> inline void fnv_combine(unsigned __int64& inoutSeed, const T& data)
{
	fnv_combine(inoutSeed, &data, sizeof(T));
}