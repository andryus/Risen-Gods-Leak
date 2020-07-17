#pragma once

#include <string>

#include "Common.h"
#include "MathHelper.h"
#include "Packets/BufferTraits.h"

class ByteBuffer;

struct Position
{
    constexpr Position() = default;
    constexpr Position(float x, float y, float z, float o = 0)
        : m_positionX(x), m_positionY(y), m_positionZ(z), m_orientation(NormalizeOrientation(o)) { }

    constexpr Position(const Position &loc) { Relocate(loc); }
    struct PositionXYZStreamer
    {
        explicit PositionXYZStreamer(Position& pos) : m_pos(&pos) { }
        Position* m_pos;
    };

    struct PositionXYZOStreamer
    {
        explicit PositionXYZOStreamer(Position& pos) : m_pos(&pos) { }
        Position* m_pos;
    };

    float m_positionX = 0.0f;
    float m_positionY = 0.0f;
    float m_positionZ = 0.0f;
    // Better to limit access to m_orientation field, but this will be hard to achieve with many scripts using array initialization for this structure
    //private:
    float m_orientation = 0.0f;
    //public:

    bool operator==(Position const &a);

    inline bool operator!=(Position const &a)
    {
        return !(operator==(a));
    }

    constexpr void Relocate(float x, float y)
    {
        m_positionX = x; m_positionY = y;
    }
    constexpr void Relocate(float x, float y, float z)
    {
        m_positionX = x; m_positionY = y; m_positionZ = z;
    }
    constexpr void Relocate(float x, float y, float z, float orientation)
    {
        m_positionX = x; m_positionY = y; m_positionZ = z; SetOrientation(orientation);
    }
    constexpr void Relocate(Position const &pos)
    {
        m_positionX = pos.m_positionX; m_positionY = pos.m_positionY; m_positionZ = pos.m_positionZ; SetOrientation(pos.m_orientation);
    }
    constexpr void Relocate(Position const* pos)
    {
        m_positionX = pos->m_positionX; m_positionY = pos->m_positionY; m_positionZ = pos->m_positionZ; SetOrientation(pos->m_orientation);
    }
    void RelocateOffset(Position const &offset);
    constexpr void SetOrientation(float orientation)
    {
        m_orientation = NormalizeOrientation(orientation);
    }

    float GetPositionX() const { return m_positionX; }
    float GetPositionY() const { return m_positionY; }
    float GetPositionZ() const { return m_positionZ; }
    float GetOrientation() const { return m_orientation; }

    void GetPosition(float &x, float &y) const
    {
        x = m_positionX; y = m_positionY;
    }
    void GetPosition(float &x, float &y, float &z) const
    {
        x = m_positionX; y = m_positionY; z = m_positionZ;
    }
    void GetPosition(float &x, float &y, float &z, float &o) const
    {
        x = m_positionX; y = m_positionY; z = m_positionZ; o = m_orientation;
    }
    void GetPosition(Position* pos) const
    {
        if (pos)
            pos->Relocate(m_positionX, m_positionY, m_positionZ, m_orientation);
    }
    Position GetPosition() const { return *this; }

    Position::PositionXYZStreamer PositionXYZStream()
    {
        return PositionXYZStreamer(*this);
    }
    Position::PositionXYZOStreamer PositionXYZOStream()
    {
        return PositionXYZOStreamer(*this);
    }

    float GetExactDist2dSq(float x, float y) const
    {
        return math::distance2dSq(m_positionX - x, m_positionY - y);
    }
    float GetExactDist2d(const float x, const float y) const
    {
        return std::sqrt(GetExactDist2dSq(x, y));
    }
    float GetExactDist2dSq(Position const* pos) const
    {
        return GetExactDist2dSq(pos->m_positionX, pos->m_positionY);
    }
    float GetExactDist2d(Position const* pos) const
    {
        return std::sqrt(GetExactDist2dSq(pos));
    }
    float GetExactDistSq(float x, float y, float z) const
    {
        return math::distance3dSq(m_positionX - x, m_positionY - y, m_positionZ - z);
    }
    float GetExactDist(float x, float y, float z) const
    {
        return std::sqrt(GetExactDistSq(x, y, z));
    }
    float GetExactDistSq(Position const* pos) const
    {
        return GetExactDistSq(pos->m_positionX, pos->m_positionY, pos->m_positionZ);
    }
    float GetExactDist(Position const* pos) const
    {
        return std::sqrt(GetExactDistSq(pos));
    }

    void GetPositionOffsetTo(Position const & endPos, Position & retOffset) const;

    float GetAngle(Position const* pos) const;
    float GetAngle(float x, float y) const;
    float GetRelativeAngle(Position const* pos) const
    {
        return GetAngle(pos) - m_orientation;
    }
    float GetRelativeAngle(float x, float y) const { return GetAngle(x, y) - m_orientation; }
    void GetSinCos(float x, float y, float &vsin, float &vcos) const;

    bool IsInDist2d(float x, float y, float dist) const
    {
        return GetExactDist2dSq(x, y) < dist * dist;
    }
    bool IsInDist2d(Position const* pos, float dist) const
    {
        return GetExactDist2dSq(pos) < dist * dist;
    }
    bool IsInDist(float x, float y, float z, float dist) const
    {
        return GetExactDistSq(x, y, z) < dist * dist;
    }
    bool IsInDist(Position const* pos, float dist) const
    {
        return GetExactDistSq(pos) < dist * dist;
    }
    bool IsWithinBox(const Position& center, float xradius, float yradius, float zradius) const;
    bool HasInArc(float arcangle, Position const* pos, float border = 2.0f) const;
    bool IsInBetween3d(Position const& pos1, Position const& pos2, float size) const;

    std::string ToString() const;

private:
    static constexpr float fmodulo(const float x, const float y)
    {
        return (x < 0.0f ? -1.0f : 1.0f) * (
            (x < 0.0f ? -x : x) -
            static_cast<long long int>((x / y < 0.0f ? -x / y : x / y)) * (y < 0.0f ? -y : y)
            );
    }

    struct AsXYZTag
    {
        static void read(Position& position, ByteBuffer& buffer);
        static void write(Position position, ByteBuffer& buffer);
    };

public:
    // modulos a radian orientation to the range of 0..2PI
    static constexpr float NormalizeOrientation(float o)
    {
        // fmod only supports positive numbers. Thus we have
        // to emulate negative numbers
        if (o < 0)
        {
            float mod = o * -1;
            mod = fmodulo(mod, 2.0f * static_cast<float>(M_PI));
            mod = -mod + 2.0f * static_cast<float>(M_PI);
            return mod;
        }
        return fmodulo(o, 2.0f * static_cast<float>(M_PI));
    }

    void Add(Position& a)
    {
        m_positionX += a.GetPositionX();
        m_positionY += a.GetPositionY();
        m_positionZ += a.GetPositionZ();
    }

    void Subtract(Position& a)
    {
        m_positionX -= a.GetPositionX();
        m_positionY -= a.GetPositionY();
        m_positionZ -= a.GetPositionZ();
    }

    void Scale(float s)
    {
        m_positionX *= s;
        m_positionY *= s;
        m_positionZ *= s;
    }

    void RelocatePolar(float angle, float radius, float z = 0.0f)
    {
        m_positionX = radius * std::cos(angle);
        m_positionY = radius * std::sin(angle);
        m_positionZ = z;
    }

    auto AsXYZ() { return packet::with_tag<AsXYZTag>(*this); }

};


namespace packet
{
    template<>
    constexpr bool IsByteBufferType<Position> = true;

    template<>
    struct BufferIO<Position>
    {
        static Position read(ByteBuffer& buffer);
        static void write(ByteBuffer& buffer, Position position);
    };
}
