#include "fnv.h"

unsigned __int64 fnv_hash(const void* data, int numBytes)
{
	unsigned __int64 hash = 0xcbf29ce484222325;
	for (int i = 0; i < numBytes; ++i)
	{
		hash ^= ((const unsigned char*)data)[i];
		hash *= 0x100000001b3;
	}
	return hash;
}

void fnv_combine(unsigned __int64& inoutSeed, const void* data, int numBytes)
{
	unsigned __int64 hash = inoutSeed;
	for (int i = 0; i < numBytes; ++i)
	{
		hash ^= ((const unsigned char*)data)[i];
		hash *= 0x100000001b3;
	}
	inoutSeed = hash;
}