#pragma once


struct map_view
{
    char* data;
    int   size;

    void* get() const { return data; }
    template<class T> T* get() { return reinterpret_cast<T*>(data); }

    operator void*() const { return data; }

    map_view(void* data, int size) : data{static_cast<char*>(data)}, size{size} {}
    map_view(map_view&& mv);
    map_view& operator=(map_view&& mv);
    ~map_view();

    void write_bytes(const void* inBytes, int size);

    template<class T> void write_struct(const T& podStruct)
    {
        write_bytes(&podStruct, sizeof(T));
    }

    void read_bytes(void* outBytes, int size);

    template<class T> void read_struct(T& outPodStruct)
    {
        read_bytes(&outPodStruct, sizeof(T));
    }

    template<class T> T read_struct()
    {
        T podStruct;
        read_bytes(&podStruct, sizeof(T));
        return podStruct;
    }

private:

    map_view(const map_view& lval) = default;
    map_view& operator=(const map_view& lvl) = default;
};


class memory_map
{
    memory_map(void* map) : map(map) {}
    memory_map(const memory_map& lval) = default;
    memory_map& operator=(const memory_map& lvl) = default;

public:

    void* map;

    memory_map(memory_map&& mv);
    memory_map& operator=(memory_map&& mv);
    ~memory_map();
    explicit operator bool() const { return map != nullptr; }


    /**
     * Initializes a new view of the named memory map
     * @param size Number of bytes to map for viewing. If size=0, entire file is mapped
     */
    map_view create_view(int size = 0);

    /**
     * @brief Creates a new named memory map of specified size
     * @param name Unique name of the memory map
     * @param size Size of the memory map
     */
    static memory_map create(const char* name, int size);

    ///**
    // * @brief Creates a new named 
    // */
    //static memory_map create_tempfile(const char* name, int size, const char* tempfile);

    /**
     * @brief Opens an existing named memory map
     * @param name Unique name of the memory map
     */
    static memory_map open(const char* name);
};