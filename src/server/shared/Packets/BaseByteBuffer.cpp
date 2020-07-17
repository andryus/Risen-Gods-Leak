#include "BaseByteBuffer.h"

#include <sstream>
#include "Log.h"

ByteBufferPositionException::ByteBufferPositionException(bool add, size_t pos,
    size_t size, size_t valueSize)
{
    std::ostringstream ss;

    ss << "Attempted to " << (add ? "put" : "get") << " value with size: "
        << valueSize << " in ByteBuffer (pos: " << pos << " size: " << size
        << ")";

    message().assign(ss.str());
}

ByteBufferSourceException::ByteBufferSourceException(size_t pos, size_t size,
    size_t valueSize)
{
    std::ostringstream ss;

    ss << "Attempted to put a "
        << (valueSize > 0 ? "NULL-pointer" : "zero-sized value")
        << " in ByteBuffer (pos: " << pos << " size: " << size << ")";

    message().assign(ss.str());
}


namespace packet
{
    // constructor

    BaseByteBuffer::BaseByteBuffer()
    {
    }

    BaseByteBuffer::BaseByteBuffer(size_t reserve)
    {
        _storage.reserve(reserve);
    }

    BaseByteBuffer::BaseByteBuffer(std::vector<uint8>&& buf) :
        _storage(std::move(buf))
    {
    }

    BaseByteBuffer::BaseByteBuffer(BaseByteBuffer && buf) : _rpos(buf._rpos), _wpos(buf._wpos),
        _storage(std::move(buf._storage))
    {
        buf._rpos = buf._wpos = 0;
    }

    void BaseByteBuffer::clear()
    {
        _storage.clear();
        _rpos = _wpos = 0;
    }

    void BaseByteBuffer::rfinish()
    {
        _rpos = wpos();
    }

    void BaseByteBuffer::resize(size_t newsize)
    {
        _storage.resize(newsize);
        _rpos = 0;
        _wpos = size();
    }

    void BaseByteBuffer::reserve(size_t ressize)
    {
        _storage.reserve(ressize);
    }

    void BaseByteBuffer::append(const uint8 * src, size_t cnt)
    {
        if (!cnt)
            throw ByteBufferSourceException(_wpos, size(), cnt);

        if (!src)
            throw ByteBufferSourceException(_wpos, size(), cnt);

        ASSERT(size() < 10000000);

        if (_storage.size() < _wpos + cnt)
            _storage.resize(_wpos + cnt);
        std::memcpy(&_storage[_wpos], src, cnt);
        _wpos += cnt;
    }

    void BaseByteBuffer::put(size_t pos, const uint8 * src, size_t cnt)
    {
        if (pos + cnt > size())
            throw ByteBufferPositionException(true, pos, cnt, size());

        if (!src)
            throw ByteBufferSourceException(_wpos, size(), cnt);

        std::memcpy(&_storage[pos], src, cnt);
    }

    void BaseByteBuffer::print_storage() const
    {
        if (!sLog->ShouldLog("network", LOG_LEVEL_TRACE)) // optimize disabled trace output
            return;

        std::ostringstream o;
        o << "STORAGE_SIZE: " << size();
        for (uint32 i = 0; i < size(); ++i)
            o << view_raw<uint8>(i) << " - ";
        o << " ";

        TC_LOG_TRACE("network", "%s", o.str().c_str());
    }

    void BaseByteBuffer::textlike() const
    {
        if (!sLog->ShouldLog("network", LOG_LEVEL_TRACE)) // optimize disabled trace output
            return;

        std::ostringstream o;
        o << "STORAGE_SIZE: " << size();
        for (uint32 i = 0; i < size(); ++i)
        {
            char buf[2];
            snprintf(buf, 2, "%c", view_raw<uint8>(i));
            o << buf;
        }
        o << " ";
        TC_LOG_TRACE("network", "%s", o.str().c_str());
    }

    void BaseByteBuffer::hexlike() const
    {
        if (!sLog->ShouldLog("network", LOG_LEVEL_TRACE)) // optimize disabled trace output
            return;

        uint32 j = 1, k = 1;

        std::ostringstream o;
        o << "STORAGE_SIZE: " << size();

        for (uint32 i = 0; i < size(); ++i)
        {
            char buf[3];
            snprintf(buf, 3, "%2X ", view_raw<uint8>(i));
            if ((i == (j * 8)) && ((i != (k * 16))))
            {
                o << "| ";
                ++j;
            }
            else if (i == (k * 16))
            {
                o << "\n";
                ++k;
                ++j;
            }

            o << buf;
        }
        o << " ";
        TC_LOG_TRACE("network", "%s", o.str().c_str());
    }
}