#pragma once

#include <vector>
#include <exception>
#include <cstring>

#include "Define.h"
#include "Utilities/ByteConverter.h"
#include "Utilities/Span.h"


// Root of ByteBuffer exception hierarchy
class ByteBufferException : public std::exception
{
public:
    ~ByteBufferException() noexcept { }

    char const* what() const throw() override { return msg_.c_str(); }

protected:
    std::string& message() noexcept { return msg_; }

private:
    std::string msg_;
};

class ByteBufferPositionException : public ByteBufferException
{
public:
    ByteBufferPositionException(bool add, size_t pos, size_t size, size_t valueSize);

    ~ByteBufferPositionException() noexcept { }
};

class ByteBufferSourceException : public ByteBufferException
{
public:
    ByteBufferSourceException(size_t pos, size_t size, size_t valueSize);

    ~ByteBufferSourceException() noexcept { }
};

namespace packet
{
    class BaseByteBuffer
    {
    public:
        using Storage = std::vector<uint8>;

        // constructor
        BaseByteBuffer();
        BaseByteBuffer(size_t reserve);
        BaseByteBuffer(std::vector<uint8>&& buf);
        BaseByteBuffer(BaseByteBuffer&& buf);

        BaseByteBuffer(const BaseByteBuffer& right) = default;
        BaseByteBuffer& operator=(const BaseByteBuffer& right) = default;

        void clear();

        uint8* data() { return _storage.data(); }
        const uint8* data() const { return _storage.data();}

        span<uint8> data_span() const { return make_span(_storage); }

        size_t rpos() const { return _rpos; }
        void set_rpos(size_t rpos_) { _rpos = rpos_; }

        void rfinish();

        size_t wpos() const { return _wpos; }
        void set_wpos(size_t wpos_) { _wpos = wpos_; }

        void skip(size_t skip)
        {
            if (_rpos + skip > size())
                throw ByteBufferPositionException(false, _rpos, skip, size());
            _rpos += skip;
        }

        // get memory value w/o endianess-correction
        void extract(uint8 *dest, size_t len)
        {
            if (_rpos + len > size())
                throw ByteBufferPositionException(false, _rpos, len, size());
            std::memcpy(dest, data() + _rpos, len);
            _rpos += len;
        }

        size_t size() const { return _storage.size(); }
        bool empty() const { return _storage.empty(); }

        void resize(size_t newsize);
        void reserve(size_t ressize);

        template<class T>
        void append(const T *src, size_t cnt)
        {
            return append((const uint8 *)src, cnt * sizeof(T));
        }
        void append(const char *src, size_t cnt) { return append((const uint8 *)src, cnt); }
        void append(const uint8 *src, size_t cnt);
        void append(const BaseByteBuffer& buffer)
        {
            if (buffer.size() > buffer.rpos())
                append(buffer.data() + buffer.rpos(), (buffer.size() - buffer.rpos()));
        }

        template<typename Type>
        void put(size_t pos, const Type& value)
        {
            EndianConvert(value);
            put(pos, (const uint8 *)&value, sizeof(value));
        }
        void put(size_t pos, const uint8 *src, size_t cnt);

        template<typename T>
        T read_raw()
        {
            const T val = view_raw<T>(_rpos);
            _rpos += sizeof(T);

            return val;
        }

        template<typename Type>
        span<Type> read_data(size_t length)
        {
            const auto span = data_span();

            const size_t pos = rpos();
            skip(length);

            return make_span((const Type*)span.data() + pos, length);
        }

        template <typename T>
        T view_raw(size_t pos) const
        {
            if (pos + sizeof(T) > size())
                throw ByteBufferPositionException(false, pos, sizeof(T), size());
            T val = *((T const*)&_storage[pos]);
            EndianConvert(val);
            return val;
        }

        template <typename T>
        void append_raw(T value)
        {
            EndianConvert(value);
            append((uint8 *)&value, sizeof(value));
        }

        Storage Move() 
        {
            _wpos = 0;
            _rpos = 0;
            return std::move(_storage);
        }

        // DEBUG
        void print_storage() const;
        void textlike() const;
        void hexlike() const;

    private:
        size_t _rpos = 0;
        size_t _wpos = 0;
        std::vector<uint8> _storage;
    };

}