#include "memory_map.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



map_view::map_view(map_view&& mv) : data(mv.data), size(mv.size)
{
	mv.data = 0;
	mv.size = 0;
}
map_view& map_view::operator=(map_view&& mv)
{
	if (this != &mv)
	{
		this->~map_view();
		data    = mv.data;
		size    = mv.size;
		mv.data = 0;
		mv.size = 0;
	}
	return *this;
}
map_view::~map_view()
{
	if (data) UnmapViewOfFile(data);
}



memory_map::memory_map(memory_map&& mv) : map(mv.map)
{
	mv.map  = 0;
}
memory_map& memory_map::operator=(memory_map&& mv)
{
	if (this != &mv)
	{
		this->~memory_map();
		map    = mv.map;
		mv.map = 0;
	}
	return *this;
}
memory_map::~memory_map()
{
	if (map) CloseHandle(map);
}



map_view memory_map::create_view(int size)
{
	void* view = MapViewOfFile(HANDLE(map), FILE_MAP_ALL_ACCESS, 0, 0, size);
	MEMORY_BASIC_INFORMATION bi = { 0 };
	VirtualQuery(view, &bi, sizeof bi);
	return map_view(view, bi.RegionSize);
}
memory_map memory_map::create(const char* name, int size)
{
	HANDLE hmap = CreateFileMappingA(
		INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, name);
	if (hmap == INVALID_HANDLE_VALUE)
		hmap = NULL;
	return memory_map(hmap);
}
memory_map memory_map::open(const char* name)
{
	HANDLE hmap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
	if (hmap == INVALID_HANDLE_VALUE)
		hmap = NULL;
	return memory_map(hmap);
}