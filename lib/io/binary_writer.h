/**
 * Copyright (c) 2014 - Jorma Rebane
 */
#pragma once
#include <cstdarg>
#include <cstdlib>
#include <rpp/file_io.h>

/**
 * A simple self-expanding buffer
 */
struct write_buffer
{
    union {
        char*     Buffer;  
        wchar_t*  WBuffer; // alias for previewing UCS-2 strings
    };
    unsigned  Size;     // current size
    unsigned  Capacity; // total buffer capacity

    /** @brief Create a new uninitialized buffer */
    inline write_buffer() : Buffer(0), Size(0), Capacity(0)
    {
    }
    /** @brief Create a new write_buffer initialized to the specified capacity */
    explicit inline write_buffer(unsigned capacity) 
        : Buffer((char*)malloc(capacity)), Size(0), Capacity(capacity)
    {
    }
    inline ~write_buffer()
    {
        if (Buffer) free(Buffer);
    }

    /** @brief Clears the buffer state, but does not free the buffer */
    inline void clear()
    {
        Size = 0;
        Buffer[0] = 0;
    }

    /** @brief Reserve room for N extra bytes */
    void reserve(unsigned numBytes)
    {
        if (numBytes > (Capacity - Size))
        {
            int newCapacity = Capacity + numBytes;
            if (int rem = (newCapacity % 16384))
                newCapacity += 16384 - rem;	// fix alignment (always 16KB aligned size increase)
            Buffer = (char*)realloc(Buffer, Capacity = newCapacity);
        }
    }
    /** @brief Reserve room for 1 extra byte only */
    void reserve()
    {
        if (Size == Capacity)
            Buffer = (char*)realloc(Buffer, Capacity += 16384); // 16KB increase (don't hold back)
    }

    /** @brief Writes raw data into the buffer */
    write_buffer& write(const void* data, unsigned numBytes)
    {
        reserve(numBytes);
        memcpy(Buffer + Size, data, numBytes);
        Size += numBytes;
        return *this;
    }
    /** @brief Writes a formatted string into the buffer (4KB buffer limit) */
    write_buffer& writef(const char* fmt, ...)
    {
        char buffer[4096];
        va_list ap;
        va_start(ap, fmt);
        return write(buffer, vsprintf(buffer, fmt, ap));
    }
    /** @brief Writes a formatted string into the buffer (4KB buffer limit) */
    write_buffer& writevf(const char* fmt, va_list ap)
    {
        char buffer[4096];
        return write(buffer, vsprintf(buffer, fmt, ap));
    }
    /** @brief Writes a regular character into the buffer */
    write_buffer& write(char ch)
    {
        reserve();
        Buffer[Size++] = ch;
        return *this;
    }
    /** @brief Writes a wide character into the buffer */
    write_buffer& write(wchar_t ch)
    {
        reserve(sizeof(wchar_t));
        *(wchar_t*)(Buffer + Size) = ch;
        Size += sizeof(wchar_t);
        return *this;
    }

    /** @brief Appends an existing buffer to this buffer */
    inline write_buffer& write(const write_buffer& other)
    {
        return write(other.Buffer, other.Size);
    }
    /** @brief Writes a regular string into the buffer */
    inline write_buffer& write(const std::string& str)
    {
        return write(str.c_str(), str.length());
    }
    /** @brief Writes a wide string into the buffer */
    inline write_buffer& write(const std::wstring& str)
    {
        return write(str.c_str(), str.length()*sizeof(wchar_t));
    }
    /** @brief Writes a string token into the buffer */
    inline write_buffer& write(const rpp::strview& str)
    {
        return write(str.c_str(), str.length());
    }
    /** @brief Writes a regular string literal into the buffer */
    template<int SIZE> inline write_buffer& write(const char (&str)[SIZE])
    {
        return write(str, SIZE - 1);
    }
    /** @brief Writes a wide string literal into the buffer */
    template<int SIZE> inline write_buffer& write(const wchar_t (&str)[SIZE])
    {
        return write(str, sizeof(str) - sizeof(wchar_t));
    }

    /** @brief Writes a newline('\n') into the buffer */
    inline write_buffer& writeln()
    {
        return write('\n');
    }
    /** @brief Writes a regular string into the buffer and appends a newline('\n') */
    inline write_buffer& writeln(const std::string& str)
    {
        return write(str.c_str(), str.length()).write('\n');
    }
    /** @brief Writes a wide string into the buffer and appends a newline('\n') */
    inline write_buffer& writeln(const std::wstring& str)
    {
        return write(str.c_str(), str.length()*sizeof(wchar_t)).write('\n');
    }
    /** @brief Writes a string token into the buffer and appends a newline('\n') */
    inline write_buffer& writeln(const rpp::strview& str)
    {
        return write(str.c_str(), str.length()).write('\n');
    }
    /** @brief Writes a regular string literal into the buffer and appends a newline('\n') */
    template<int SIZE> write_buffer& writeln(const char (&str)[SIZE])
    {
        return write(str, SIZE - 1).write('\n');
    }
    /** @brief Writes a wide string literal into the buffer and appends a newline('\n') */
    template<int SIZE> write_buffer& writeln(const wchar_t (&str)[SIZE])
    {
        return write(str, sizeof(str) - sizeof(wchar_t)).write(L'\n');
    }
    /** @brief Writes raw data into the buffer and appends a newline('\n') */
    write_buffer& writeln(const void* str, int len)
    {
        return write(str, len).write('\n');
    }


    /** @brief Fill the buffer with the specified number of the specified char (memset) */
    write_buffer& fill(char ch, unsigned fillCount)
    {
        reserve(fillCount);
        memset(Buffer + Size, ch, fillCount);
        Size += fillCount;
        return *this;
    }

    /** @brief Write a padded string token into the buffer. (tabs, each tab == 4 spaces) */
    write_buffer& write_padded(int minWidth, const rpp::strview& str)
    {
        int strLen = str.length();
        int padding = minWidth - strLen;

        write(str.str, strLen);

        if (padding > 0)
        {
            int numTabs = 0;
            for (; padding >= 4; padding -= 4) ++numTabs;
            if (padding) ++numTabs;
            fill('\t', numTabs);
        }
        return *this;
    }
};











/**
 * binary_writer - A basic write buffer for writing raw binary data instead of string based conversions
 */
struct binary_buffer : public write_buffer
{
    /** @brief Writes a single unsigned byte into the buffer */
    binary_buffer& write_byte(unsigned char value)
    {
        reserve();
        *(unsigned char*)&Buffer[Size++] = value;
        return *this;
    }

    /** @brief Writes a 16-bit signed short into the buffer */
    binary_buffer& write_short(short value)
    {
        reserve(2);
        *(short*)&Buffer[Size] = value;
        Size += 2;
        return *this;
    }

    /** @brief Writes a 32-bit signed integer into the buffer */
    binary_buffer& write_int(int value)
    {
        reserve(4);
        *(int*)&Buffer[Size] = value;
        Size += 4;
        return *this;
    }

    /** @brief Writes a 64-bit signed integer into the buffer */
    binary_buffer& write_int64(__int64 value)
    {
        reserve(sizeof(__int64));
        *(__int64*)&Buffer[Size] = value;
        Size += sizeof(__int64);
        return *this;
    }

    /** @brief Writes generic POD type data into the buffer */
    template<class T> inline binary_buffer& write_data(const T& data)
    {
        write((const void*)&data, sizeof(T));
        return *this;
    }

    /**
     * @brief Writes all the binary data inside the vector as [len] [ sizeof(T) * len ]
     * @warning Only POD types are supported. No serialization is performed.
     */
    template<class T, class U> inline binary_buffer& write_vector(const std::vector<T, U>& vec)
    {
        int count = vec.size();
        write_int(count);
        write(vec.data(), sizeof(T) * count);
        return *this;
    }


    template<class T, class U, class SerializeFunc> 
    /**
     * @brief Writes all the binary data inside the vector as [len] [ len * serializeFunc() ]
     * @note  Binary data layout depends on the user passed function serializeFunc() implementation
     * @note  SerializeFunc signature:  <void(binary_buffer& os, const T& item)>
     */
    inline binary_buffer& write_vector(const std::vector<T, U>& vec, SerializeFunc& serializeFunc)
    {
        int count = vec.size();
        write_int(count);
        const T* data = vec.data();
        for (int i = 0; i < count; ++i)
            serializeFunc(*this, data[i]);
        return *this;
    }


    /** @brief Write a null-terminated string to the buffer in the form of [data][0] */
    binary_buffer& write_cstr(const rpp::strview& str)
    {
        write(str).write('\0'); return *this;
    }
    /** @brief Write a length specified string to the buffer in the form of [len][data] */
    binary_buffer& write_nstr(const rpp::strview& str)
    {
        write_int(str.length()).write(str); return *this;
    }
    /** @brief Write a bastardized length specified and null-terminated string to the buffer in the form of [len][data][0] */
    binary_buffer& write_ncstr(const rpp::strview& str)
    {
        write_int(str.length()).write(str).write('\0'); return *this;
    }


    /** @brief Write a null-terminated string to the buffer in the form of [data][0] */
    binary_buffer& write_cstr(const std::string& str)
    {
        write(str.c_str(), str.length()).write('\0'); return *this;
    }
    /** @brief Write a length specified string to the buffer in the form of [len][data] */
    binary_buffer& write_nstr(const std::string& str)
    {
        int len = str.length();
        write_int(len).write(str.c_str(), len); return *this;
    }
    /** @brief Write a bastardized length specified and null-terminated string to the buffer in the form of [len][data][0] */
    binary_buffer& write_ncstr(const std::string& str)
    {
        int len = str.length();
        write_int(len).write(str.c_str(), len).write('\0'); return *this;
    }


    /** @brief Write a null-terminated string to the buffer in the form of [data][0] */
    binary_buffer& write_cstr(const std::wstring& str)
    {
        write(str.c_str(), str.length() * 2).write(L'\0'); return *this;
    }
    /** @brief Write a length specified string to the buffer in the form of [len][data] */
    binary_buffer& write_nstr(const std::wstring& str)
    {
        int len = str.length();
        write_int(len).write(str.c_str(), len * 2); return *this;
    }
    /** @brief Write a bastardized length specified and null-terminated string to the buffer in the form of [len][data][0] */
    binary_buffer& write_ncstr(const std::wstring& str)
    {
        int len = str.length();
        write_int(len).write(str.c_str(), len * 2).write(L'\0'); return *this;
    }
};







/**
 * @brief This class writes elements in a human-readable way instead of binary data like binary_buffer
 */
struct string_buffer : public write_buffer
{
    /** @brief Writes a bool as "true" or "false" */
    string_buffer& write_bool(bool value)
    {
        if (value) write_buffer::write("true", 4);
        else       write_buffer::write("false", 5);
        return *this;
    }
    /** @brief Writes the string representation of an int (%d) */
    string_buffer& write_int(int value)
    {
        write_buffer::writef("%d", value);
        return *this;
    }
    /** @brief Writes the shortest string representation of a float (%g) */
    string_buffer& write_float(float value)
    {
        write_buffer::writef("%g", value);
        return *this;
    }
    /** @brief Writes the shortest string representation of a double (%g) */
    string_buffer& write_double(double value)
    {
        write_buffer::writef("%g", value);
        return *this;
    }


    //
    // wrappers for write_buffer:
    //

    
    /** @brief Write raw binary data into the buffer */
    string_buffer& write(const void* data, unsigned numBytes)
    {
        write_buffer::write(data, numBytes); return *this;
    }
    /** @brief Write formatted string into the buffer (4KB buffer limit) */
    string_buffer& writef(const char* fmt, ...)
    {
        va_list ap; va_start(ap, fmt);
        write_buffer::writevf(fmt, ap);
        return *this;
    }
    /** @brief Write formatted string into the buffer (4KB buffer limit) */
    string_buffer& writevf(const char* fmt, va_list ap)
    {
        write_buffer::writevf(fmt, ap);
        return *this;
    }
    /** @brief Write a single character into the buffer */
    string_buffer& write(char ch)
    {
        write_buffer::write(ch); return *this;
    }
    /** @brief Write a wide character into the buffer */
    string_buffer& write(wchar_t ch)
    {
        write_buffer::write(ch); return *this;
    }


    /** @brief Appends all data from an existing buffer into this buffer */
    string_buffer& write(const write_buffer& other)
    {
        write_buffer::write(other); return *this;
    }
    /** @brief Writes a regular string into the buffer */
    string_buffer& write(const std::string& str)
    {
        write_buffer::write(str); return *this;
    }
    /** @brief Writes a wide string into the buffer */
    string_buffer& write(const std::wstring& str)
    {
        write_buffer::write(str); return *this;
    }
    /** @brief Writes a token string into the buffer */
    string_buffer& write(const rpp::strview& str)
    {
        write_buffer::write(str); return *this;
    }
    /** @brief Writes a regular string literal into the buffer */
    template<int SIZE> string_buffer& write(const char (&str)[SIZE])
    {
        write_buffer::write(str, SIZE - 1); return *this;
    }
    /** @brief Writes a wide string literal into the buffer */
    template<int SIZE> string_buffer& write(const wchar_t (&str)[SIZE])
    {
        write_buffer::write(str, sizeof(str) - sizeof(wchar_t)); return *this;
    }


    /** @brief Writes a newline('\n') into the buffer */
    string_buffer& writeln()
    {
        write_buffer::writeln(); return *this;
    }
    /** @brief Writes a regular string and appends a newline('\n') */
    string_buffer& writeln(const std::string& str)
    {
        write_buffer::writeln(str); return *this;
    }
    /** @brief Writes a wide string and appends a newline(L'\n') */
    string_buffer& writeln(const std::wstring& str)
    {
        write_buffer::writeln(str); return *this;
    }
    /** @brief Writes a string token and appends a newline('\n') */
    string_buffer& writeln(const rpp::strview& str)
    {
        write_buffer::writeln(str); return *this;
    }
    /** @brief Writes a regular string literal and appends a newline('\n') */
    template<int SIZE> string_buffer& writeln(const char (&str)[SIZE])
    {
        write_buffer::writeln(str, SIZE - 1); return *this;
    }
    /** @brief Writes a wide string literal and appends a newline(L'\n') */
    template<int SIZE> string_buffer& writeln(const wchar_t (&str)[SIZE])
    {
        write_buffer::writeln(str, sizeof(str) - sizeof(wchar_t)); return *this;
    }
    /** @brief Writes data into the buffer and appends a newline('\n') */
    string_buffer& writeln(const void* str, int len)
    {
        write_buffer::writeln(str, len); return *this;
    }


    /** @brief fill the buffer with the specified number of the specified char (memset) */
    string_buffer& fill(char ch, unsigned fillCount)
    {
        write_buffer::fill(ch, fillCount); return *this;
    }

    /** @brief padded write (tabs, each tab == 4 spaces) */
    string_buffer& write_padded(int minWidth, const rpp::strview& str)
    {
        write_buffer::write_padded(minWidth, str); return *this;
    }
};








inline write_buffer& _strmwritebuf(write_buffer& os, char value)
{
    return os.write(value);
}
inline write_buffer& _strmwritebuf(write_buffer& os, const std::string& s)
{
    return os.write(s.c_str(), s.length());
}
inline write_buffer& _strmwritebuf(write_buffer& os, const std::wstring& s)
{
    return os.write(s.c_str(), s.length() * sizeof(wchar_t));
}
inline write_buffer& _strmwritebuf(write_buffer& os, const char* str)
{
    return os.write(str, strlen(str));
}
inline write_buffer& _strmwritebuf(write_buffer& os, const wchar_t* str)
{
    return os.write(str, wcslen(str) * sizeof(wchar_t));
}
inline write_buffer& _strmwritebuf(write_buffer& os, const write_buffer& str)
{
    return os.write(str.Buffer, str.Size);
}
inline write_buffer& _strmwritebuf(write_buffer& os, const rpp::load_buffer& lb)
{
    return os.write(lb.c_str(), lb.size());
}
inline write_buffer& _strmwritebuf(write_buffer& os, const rpp::string_buffer& sb)
{
    return os.write(sb.c_str(), sb.size());
}
template<class T> inline write_buffer& operator<<(write_buffer& os, const T& value)
{
    return _strmwritebuf(os, value);
}


inline write_buffer& operator<<(write_buffer& os, const rpp::strview& s)
{
    return os.write(s);
}
template<int SIZE> write_buffer& operator<<(write_buffer& os, char (&str)[SIZE])
{
    return os.write(str, strlen(str));
}
template<int SIZE> write_buffer& operator<<(write_buffer& os, wchar_t (&str)[SIZE])
{
    return os.write(str, wcslen(str) * sizeof(wchar_t));
}
template<int SIZE> write_buffer& operator<<(write_buffer& os, const char (&str)[SIZE])
{
    return os.write(str, SIZE - 1);
}
template<int SIZE> write_buffer& operator<<(write_buffer& os, const wchar_t (&str)[SIZE])
{
    return os.write(str, sizeof(str)-sizeof(wchar_t));
}


inline write_buffer& endl(write_buffer& os)
{
    return os.writeln();
}








inline string_buffer& _strmwritebuf(string_buffer& os, char value)
{
    return os.write(value);
}
inline string_buffer& _strmwritebuf(string_buffer& os, bool value)
{
    return os.write_bool(value);
}
inline string_buffer& _strmwritebuf(string_buffer& os, int value)
{
    return os.write_int(value);
}
inline string_buffer& _strmwritebuf(string_buffer& os, float value)
{
    return os.write_float(value);
}
inline string_buffer& _strmwritebuf(string_buffer& os, double value)
{
    return os.write_double(value);
}


inline string_buffer& _strmwritebuf(string_buffer& os, const std::string& s)
{
    return os.write(s.c_str(), s.length());
}
inline string_buffer& _strmwritebuf(string_buffer& os, const std::wstring& s)
{
    return os.write(s.c_str(), s.length() * sizeof(wchar_t));
}
inline string_buffer& _strmwritebuf(string_buffer& os, const char* str)
{
    return os.write(str, strlen(str));
}
inline string_buffer& _strmwritebuf(string_buffer& os, const wchar_t* str)
{
    return os.write(str, wcslen(str) * sizeof(wchar_t));
}
inline string_buffer& _strmwritebuf(string_buffer& os, const write_buffer& str)
{
    return os.write(str.Buffer, str.Size);
}
inline string_buffer& _strmwritebuf(string_buffer& os, const rpp::load_buffer& lb)
{
    return os.write(lb.c_str(), lb.size());
}
template<class T> inline string_buffer& operator<<(string_buffer& os, const T& value)
{
    return _strmwritebuf(os, value);
}


inline string_buffer& operator<<(string_buffer& os, const rpp::strview& s)
{
    return os.write(s);
}
template<int SIZE> string_buffer& operator<<(string_buffer& os, char (&str)[SIZE])
{
    return os.write(str, strlen(str));
}
template<int SIZE> string_buffer& operator<<(string_buffer& os, wchar_t (&str)[SIZE])
{
    return os.write(str, wcslen(str) * sizeof(wchar_t));
}
template<int SIZE> string_buffer& operator<<(string_buffer& os, const char (&str)[SIZE])
{
    return os.write(str, SIZE - 1);
}
template<int SIZE> string_buffer& operator<<(string_buffer& os, const wchar_t (&str)[SIZE])
{
    return os.write(str, sizeof(str)-sizeof(wchar_t));
}








/**
 * @brief Extension to the binary_writer, flushes all data to the specified file
 */
template<class WriteBufferType> struct basic_filewriter : public WriteBufferType
{
    rpp::file File;			// the destination file

    /** @brief Creates an uninitialized filewriter object. Call open() to initialize. */
    inline basic_filewriter() {}
    /** @brief Creates a new filewriter object from an already initialized file */
    inline basic_filewriter(rpp::file&& file) : File(std::move(file)) {}
    /** @brief Creates a new filewriter object by creating/opening a file */
    inline basic_filewriter(const char* filename, rpp::file::mode mode = rpp::file::CREATENEW) : File(filename, mode) {}
    /** @brief Creates a new filewriter object by creating/opening a file */
    inline basic_filewriter(const std::string& filename, rpp::file::mode mode = rpp::file::CREATENEW) : File(filename, mode) {}
    inline ~basic_filewriter()
    {
        flush(); // flush on destruction
    }

    /** 
     * @brief Opens the destination file for file flush operation
     * @return TRUE if file was opened successfully, FALSE if failed
     */
    inline bool open(const char* filename, rpp::file::mode mode = rpp::file::CREATENEW)
    {
        return File.open(filename, mode);
    }
    /** 
     * @brief Opens the destination file for file flush operation
     * @return TRUE if file was opened successfully, FALSE if failed
     */
    inline bool open(const std::string& filename, rpp::file::mode mode = rpp::file::CREATENEW)
    {
        return File.open(filename, mode);
    }
    /** @return TRUE if the destination file is initialized and thus good for flushing */
    inline bool good() const
    {
        return File.good();
    }
    /** @return TRUE if the destination file is uninitialized and thus bad for flushing */
    inline bool bad() const
    {
        return File.bad();
    }
    
    /**
     * Flushes all data to file
     * @note This writes all the data and closes the file so only call this once!!!
     *       Use binary_streamwriter for multiple flushes
     * @warning This also !clears! the binary_writer Buffer
     * @return TRUE if all bytes were written to the file, 
     *         FALSE if some or none of the bytes were written
     */
    bool flush()
    {
        if (int size = Size)
        {
            int written = File.write(Buffer, size);
            File.close();
            clear();
            return size == written;
        }
        return false;
    }
};




/**
 * @brief Similar to basic_streamwriter, but allows multiple flushes
 * @note The stream must still be manually flushed - no automatic flushing implemented
 */
template<class WriteBufferType> struct basic_streamwriter : public WriteBufferType
{
    rpp::file Stream;			// the destination stream

    /** @brief Creates an uninitialized basic_streamwriter object. Call ::open() to initialize */
    basic_streamwriter() {}
    /** @brief Initialize streamwriter from an already opened file, taking ownership of the file object */
    basic_streamwriter(rpp::file&& file) : Stream(std::move(file)) {}
    /** @brief Initialize streamwriter from the specified filename */
    basic_streamwriter(const char* filename, rpp::file::mode mode = rpp::file::CREATENEW) : Stream(filename, mode) {}
    /** @brief Initialize streamwriter from the specified filename */
    basic_streamwriter(const std::string& filename, rpp::file::mode mode = rpp::file::CREATENEW) : Stream(filename, mode) {}
    ~basic_streamwriter()
    {
        flush(); // flush on destruction
    }

    /** 
     * @brief Opens the destination stream for stream operation
     * @return true if file was opened successfully, false if failed
     */
    bool open(const char* filename, rpp::file::mode mode = rpp::file::CREATENEW)
    {
        return Stream.open(filename, mode);
    }
    /** 
     * @brief Opens the destination stream for stream operation
     * @return true if file was opened successfully, false if failed
     */
    bool open(const std::string& filename, rpp::file::mode mode = rpp::file::CREATENEW)
    {
        return Stream.open(filename, mode);
    }
    /** @return Seek to a new position in the stream (SEEK_SET, SEEK_CUR, SEEK_END) */
    int seek(int seekpos, int seekmode = SEEK_SET)
    {
        return Stream.seek(seekpos, seekmode);
    }
    /** @return Current stream position */
    int tell() const
    {
        return Stream.tell();
    }
    /** @return Current size of the filestream */
    int size() const
    {
        return Stream.size();
    }
    /** @return TRUE if the destination file is initialized and thus good for flushing */
    bool good() const
    {
        return Stream.good();
    }
    /** @return TRUE if the destination file is uninitialized and thus bad for flushing */
    bool bad() const
    {
        return Stream.bad();
    }

    /**
     * Flushes current buffer to the file
     *
     * @warning This also !clears! the binary_writer Buffer
     * @return TRUE if all bytes were written to the file, 
     *         FALSE if some or none of the bytes were written
     */
    bool flush()
    {
        if (int size = Size)
        {
            int written = Stream.write(Buffer, size);
            clear();
            return size == written;
        }
        return false;
    }
};






/** @brief Implements basic_filewriter backed by a binary_buffer */
typedef basic_filewriter  <binary_buffer>  binary_filewriter;

/**  @brief Implements basic_streamwriter backed by a binary_buffer */
typedef basic_streamwriter<binary_buffer>  binary_streamwriter;

/** @brief Implements basic_filewriter backed by a string_buffer */
typedef basic_filewriter  <string_buffer>  string_filewriter;

/** @brief Implements basic_streamwriter backed by a string_buffer */
typedef basic_streamwriter<string_buffer>  string_streamwriter;



