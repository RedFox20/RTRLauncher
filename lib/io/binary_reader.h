/**
 * Copyright (c) 2014 - Jorma Rebane
 */
#pragma once
#include <rpp/strview.h>
#include <rpp/file_io.h>

/**
 * binary_reader - A basic binary reader with multiple useful methods to simplify parsing binary data files
 */
struct read_buffer
{
    union {
        char*     Buffer;  
        wchar_t*  WBuffer; // alias for previewing UCS-2 strings
    };
    unsigned  SeekPos;
    unsigned  Size;

    /**
     * Initializes an empty binary_reader object (no overhead involved)
     */
    inline read_buffer() : Buffer(NULL), Size(0), SeekPos(0) 
    {
    }
    /**
     * Initializes the binary_reader from the specified buffer
     * @note The buffer is owned by the caller and is never freed by binary_reader!
     */
    inline read_buffer(void* buffer, unsigned bufferSize) 
        : Buffer((char*)buffer), Size(bufferSize), SeekPos(0)
    {
    }

    /** @return true if Buffer is initialized (and thus reader state is good) */
    inline bool good() const { return Buffer != NULL; }
    /** @return true if Buffer is not initialized (and thus reader state is bad) */
    inline bool bad() const { return Buffer == NULL; }
    /** @return Current seekposition in the read buffer */
    inline unsigned tell() const { return SeekPos; }
    /** @return Size of the read buffer */
    inline unsigned size() const { return Size; }
    /** 
     * @brief Look at current seek position through a pointer cast.
     *        Useful for directly accessing a POD type members from the buffer
     */
    template<class T> inline T* tell_ptr() { return (T*)&Buffer[SeekPos]; }

    /**
     * Gets the pointer to data block of size (sizeof(T) * count)
     */
    template<class T> inline T* get_block_ptr(unsigned count)
    {
        T* data = (T*)&Buffer[SeekPos];
        SeekPos += sizeof(T) * count;
        return data;
    }


    /** @brief Set the buffer seek position to the specified position and return the new seek value */
    inline unsigned seek_set(unsigned pos) { return SeekPos = pos; }
    /** @brief Seek buffer from the current seek position by the given offset and return the new seek value */
    inline unsigned seek_cur(int offset)   { return SeekPos += offset; }
    /** @brief Seek from the end of the buffer by the given offset and return the new seek value */
    inline unsigned seek_end(int offset)   { return SeekPos = Size + offset; }


    /** @brief Reads a single item of type T into destination buffer*/
    template<class T> inline void read(T& out)
    {
        out = *(T*)&Buffer[SeekPos];
        SeekPos += sizeof(T);
    }
    /** @brief Reads an entire static T array[SIZE] into destination buffer */
    template<class T, int SIZE> inline void read(T (&out)[SIZE])
    {
        memcpy(out, &Buffer[SeekPos], sizeof(out));
        SeekPos += sizeof(out);
    }
    /** @brief Reads a single item of type T */
    template<class T> inline T read()
    {
        T* value = (T*)&Buffer[SeekPos];
        SeekPos += sizeof(T);
        return *value;
    }
    /** @brief Reads [numBytes] of bytes into destination buffer */
    inline void read(void* dst, unsigned numBytes)
    {
        memcpy(dst, &Buffer[SeekPos], numBytes);
        SeekPos += numBytes;
    }
    /** @brief Reads [count] raw items of type T into destination buffer */
    template<class T> inline void read(T* dst, unsigned count)
    {
        int numBytes = sizeof(T) * count;
        memcpy(dst, &Buffer[SeekPos], numBytes);
        SeekPos += numBytes;
    }

    /**
     * Reads vector as [count] [ count * sizeof(T) ].
     * The count is passed as a parameter
     */
    template<class T, class U> void read_vector(std::vector<T, U>& out, unsigned count)
    {
        out.resize(count);
        size_t size = sizeof(T) * count;
        memcpy((void*)out.data(), &Buffer[SeekPos], size);
        SeekPos += size;
    }

    /**
     * Reads vector as [len][ len * sizeof(T) ].
     * The first len is read as a 32-bit integer.
     */
    template<class T, class U> void read_vector(std::vector<T, U>& out)
    {
        int len = read_int();
        read_vector(out, len);
    }


    // We could repeat these with read<int>() etc.., 
    // but we want to avoid an extra func call and
    // have the function properly inline itself


    /** @brief Read a single 64-bit integer from the buffer */
    inline __int64 read_int64()
    {
        __int64 value = *(__int64*)&Buffer[SeekPos];
        SeekPos += sizeof(__int64);
        return value;
    }
    /** @brief Read a single 32-bit integer from the buffer */
    inline int read_int() 
    {
        int value = *(int*)&Buffer[SeekPos];
        SeekPos += sizeof(int);
        return value; 
    }
    /** @brief Read a single byte from the buffer */
    inline unsigned char read_byte()
    {
        unsigned char value = *(unsigned char*)&Buffer[SeekPos];
        SeekPos += sizeof(unsigned char);
        return value;
    }
    /** @brief Read a single 16-bit integer from the buffer */
    inline short read_word()
    {
        short value = *(short*)&Buffer[SeekPos];
        SeekPos += sizeof(short);
        return value;
    }
    /** @brief Read a single 32-bit float from the buffer */
    inline float read_float() 
    {
        float value = *(float*)&Buffer[SeekPos];
        SeekPos += sizeof(float);
        return value;
    }
    /** @brief Read a single 64-bit double from the buffer */
    inline double read_double()
    {
        double value = *(double*)&Buffer[SeekPos];
        SeekPos += sizeof(double);
        return value;
    }


    

    /** @brief Reads a null-terminated C-string [data][0]. No size data is given. */
    void read_cstr(rpp::strview& out)
    {
        char* str = &Buffer[SeekPos]; //// @note Using Start/End pointer validation to avoid buffer overflow
        char* end = strchr(str, '\0');
        if (end > &Buffer[Size]) end = &Buffer[Size]; // validate end pointer

        out = rpp::strview{str, end};
        SeekPos += int(end - str) + 1;
    }
    /** @brief Reads a length specified binary string in the form of [len][data].  */
    void read_nstr(rpp::strview& out)
    {
        char* ptr = &Buffer[SeekPos]; //// @note Using Start/End pointer validation to avoid buffer overflow
        char* str = ptr + sizeof(int);
        char* end = str + *(int*)ptr;
        if (end > &Buffer[Size]) end = &Buffer[Size]; // validate end pointer
        
        out = rpp::strview{str, end};
        SeekPos += sizeof(int) + int(end - str);
    }
    /** @brief Reads a bastardized length specified and null-terminated binary string in the form of [len][data][0].  */
    void read_ncstr(rpp::strview& out)
    {
        char* ptr = &Buffer[SeekPos]; //// @note Using Start/End pointer validation to avoid buffer overflow
        char* str = ptr + sizeof(int);
        char* end = str + *(int*)ptr;
        if (end > &Buffer[Size]) end = &Buffer[Size]; // validate end pointer
        
        out = rpp::strview{str, end};
        SeekPos += sizeof(int) + int(end - str) + 1;
    }




    /** @brief Reads a null-terminated C-string [data][0]. No size data is given. */
    void read_cstr(std::string& out)
    {
        char* str = &Buffer[SeekPos]; //// @note Using Start/End pointer validation to avoid buffer overflow
        char* end = strchr(str, '\0');
        if (end > &Buffer[Size]) end = &Buffer[Size]; // validate end pointer

        out.assign(str, end);
        SeekPos += int(end - str) + 1;
    }
    /** @brief Reads a length specified binary string in the form of [len][data].  */
    void read_nstr(std::string& out)
    {
        char* ptr = &Buffer[SeekPos]; //// @note Using Start/End pointer validation to avoid buffer overflow
        char* str = ptr + sizeof(int);
        char* end = str + *(int*)ptr;
        if (end > &Buffer[Size]) end = &Buffer[Size]; // validate end pointer

        out.assign(str, end);
        SeekPos += sizeof(int) + int(end - str);
    }
    /** @brief Reads a bastardized length specified and null-terminated binary string in the form of [len][data][0].  */
    void read_ncstr(std::string& out)
    {
        char* ptr = &Buffer[SeekPos]; //// @note Using Start/End pointer validation to avoid buffer overflow
        char* str = ptr + sizeof(int);
        char* end = str + *(int*)ptr;
        if (end > &Buffer[Size]) end = &Buffer[Size]; // validate end pointer

        out.assign(str, end);
        SeekPos += sizeof(int) + int(end - str) + 1;
    }




    /** @brief Reads a null-terminated WIDE C-string [data][0]. No size data is given. */
    void read_cstr(std::wstring& out)
    {
        wchar_t* str = &WBuffer[SeekPos]; //// @note Using Start/End pointer validation to avoid buffer overflow
        wchar_t* end = wcschr(str, L'\0');
        if (end > (wchar_t*)&Buffer[Size]) end = (wchar_t*)&Buffer[Size]; // validate end pointer

        out.assign(str, end);
        SeekPos += int(end - str) + 2;
    }
    /** @brief Reads a length specified binary WIDE string in the form of [len][data].  */
    void read_nstr(std::wstring& out)
    {
        wchar_t* ptr = &WBuffer[SeekPos]; //// @note Using Start/End pointer validation to avoid buffer overflow
        wchar_t* str = ptr + sizeof(int);
        wchar_t* end = str + *(int*)ptr;
        if (end > (wchar_t*)&Buffer[Size]) end = (wchar_t*)&Buffer[Size]; // validate end pointer

        out.assign(str, end);
        SeekPos += sizeof(int) + int(end - str);
    }
    /** @brief Reads a bastardized length specified and null-terminated WIDE binary string in the form of [len][data][0].  */
    void read_ncstr(std::wstring& out)
    {
        wchar_t* ptr = &WBuffer[SeekPos]; //// @note Using Start/End pointer validation to avoid buffer overflow
        wchar_t* str = ptr + sizeof(int);
        wchar_t* end = str + *(int*)ptr;
        if (end > (wchar_t*)&Buffer[Size]) end = (wchar_t*)&Buffer[Size]; // validate end pointer

        out.assign(str, end);
        SeekPos += sizeof(int) + int(end - str) + 2;
    }
};


/**
 * Extension to the binary_reader - manages the buffer and auto-loads the file
 * @note The entire file is loaded, so avoid this for large files!
 */
template<class ReadBufferType> struct basic_filereader : public ReadBufferType
{
    /**
     * Creates an empty uninitialized reader. Call ::open(filename) to initialize.
     */
    inline basic_filereader()
    {
    }

    /**
     * @param filename Opens the specified file and reads all the data from it
     */
    inline basic_filereader(const char* filename)
    {
        open(filename);
    }

    /**
     * @param filename Opens the specified file and reads all the data from it
     */
    inline basic_filereader(const std::string& filename)
    {
        open(filename);
    }

    inline ~basic_filereader()
    {
        destroy();
    }
    
    /**
     * Opens file and reads all the data from it, initializing binary_reader state.
     */
    bool open(const char* filename)
    {
        destroy(); // destroy buffer if it's open

        unbuffered_file file(filename, IOFlags::READONLY);
        if (file.good())
        {
            int alignedSize = file.size_aligned();
            Buffer = (char*)malloc(alignedSize);
            Size = file.read(Buffer, alignedSize);

            return true;
        }
        return false;
    }

    /**
     * Opens file and reads all the data from it
     */
    inline bool open(const std::string& filename)
    {
        return open(filename.c_str());
    }

    /**
     * Destroys the allocated buffer and clears reader state
     */
    void destroy()
    {
        if (Buffer)
        {
            free(Buffer);
            Buffer      = 0;
            SeekPos     = 0;
            Size        = 0;
        }
    }
};


/**
 * @todo Implement this - a streaming binaryreader...
 *       
 */
template<class ReadBufferType> struct basic_streamreader : public ReadBufferType
{

};

typedef basic_filereader<read_buffer> binary_filereader;
typedef basic_streamreader<read_buffer> binary_streamreader;