#include "Position.h"

#include <sstream>
#include <G3D/g3dmath.h>

#include "Packets/ByteBuffer.h"
#include "Random.h"

#include <G3D/LineSegment.h>

bool Position::operator==(Position const &a)
{
    return (G3D::fuzzyEq(a.m_positionX, m_positionX) &&
        G3D::fuzzyEq(a.m_positionY, m_positionY) &&
        G3D::fuzzyEq(a.m_positionZ, m_positionZ) &&
        G3D::fuzzyEq(a.m_orientation, m_orientation));
}

std::string Position::ToString() const
{
    std::stringstream sstr;
    sstr << "X: " << m_positionX << " Y: " << m_positionY << " Z: " << m_positionZ << " O: " << m_orientation;
    return sstr.str();
}

void Position::RelocateOffset(const Position & offset)
{
    m_positionX = GetPositionX() + (offset.GetPositionX() * std::cos(GetOrientation()) + offset.GetPositionY() * std::sin(GetOrientation() + float(M_PI)));
    m_positionY = GetPositionY() + (offset.GetPositionY() * std::cos(GetOrientation()) + offset.GetPositionX() * std::sin(GetOrientation()));
    m_positionZ = GetPositionZ() + offset.GetPositionZ();
    SetOrientation(GetOrientation() + offset.GetOrientation());
}

void Position::GetPositionOffsetTo(const Position & endPos, Position & retOffset) const
{
    float dx = endPos.GetPositionX() - GetPositionX();
    float dy = endPos.GetPositionY() - GetPositionY();

    retOffset.m_positionX = dx * std::cos(GetOrientation()) + dy * std::sin(GetOrientation());
    retOffset.m_positionY = dy * std::cos(GetOrientation()) - dx * std::sin(GetOrientation());
    retOffset.m_positionZ = endPos.GetPositionZ() - GetPositionZ();
    retOffset.SetOrientation(endPos.GetOrientation() - GetOrientation());
}

float Position::GetAngle(const Position* obj) const
{
    if (!obj)
        return 0;

    return GetAngle(obj->GetPositionX(), obj->GetPositionY());
}

// Return angle in range 0..2*pi
float Position::GetAngle(const float x, const float y) const
{
    float dx = x - GetPositionX();
    float dy = y - GetPositionY();

    float ang = std::atan2(dy, dx);
    ang = (ang >= 0) ? ang : 2 * float(M_PI) + ang;
    return ang;
}

void Position::GetSinCos(const float x, const float y, float &vsin, float &vcos) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;

    if (std::fabs(dx) < 0.001f && std::fabs(dy) < 0.001f)
    {
        float angle = (float)rand_norm()*static_cast<float>(2 * M_PI);
        vcos = std::cos(angle);
        vsin = std::sin(angle);
    }
    else
    {
        float dist = std::sqrt((dx*dx) + (dy*dy));
        vcos = dx / dist;
        vsin = dy / dist;
    }
}

bool Position::IsWithinBox(const Position& center, float xradius, float yradius, float zradius) const
{
    // rotate the WorldObject position instead of rotating the whole cube, that way we can make a simplified
    // is-in-cube check and we have to calculate only one point instead of 4

    // 2PI = 360*, keep in mind that ingame orientation is counter-clockwise
    double rotation = 2 * M_PI - center.GetOrientation();
    double sinVal = std::sin(rotation);
    double cosVal = std::cos(rotation);

    float BoxDistX = GetPositionX() - center.GetPositionX();
    float BoxDistY = GetPositionY() - center.GetPositionY();

    float rotX = float(center.GetPositionX() + BoxDistX * cosVal - BoxDistY * sinVal);
    float rotY = float(center.GetPositionY() + BoxDistY * cosVal + BoxDistX * sinVal);

    // box edges are parallel to coordiante axis, so we can treat every dimension independently :D
    float dz = GetPositionZ() - center.GetPositionZ();
    float dx = rotX - center.GetPositionX();
    float dy = rotY - center.GetPositionY();
    if ((std::fabs(dx) > xradius) ||
        (std::fabs(dy) > yradius) ||
        (std::fabs(dz) > zradius))
    {
        return false;
    }
    return true;
}

bool Position::HasInArc(float arc, const Position* obj, float border) const
{
    // always have self in arc
    if (obj == this)
        return true;

    // move arc to range 0.. 2*pi
    arc = NormalizeOrientation(arc);

    float angle = GetAngle(obj);
    angle -= m_orientation;

    // move angle to range -pi ... +pi
    angle = NormalizeOrientation(angle);
    if (angle > float(M_PI))
        angle -= 2.0f * float(M_PI);

    float lborder = -1 * (arc / border);                        // in range -pi..0
    float rborder = (arc / border);                             // in range 0..pi
    return ((angle >= lborder) && (angle <= rborder));
}

bool Position::IsInBetween3d(Position const& pos1, Position const& pos2, const float size) const
{
    G3D::LineSegment line = G3D::LineSegment::fromTwoPoints(G3D::Vector3(pos1.GetPositionX(), pos1.GetPositionY(), pos1.GetPositionZ()),
        G3D::Vector3(pos2.GetPositionX(), pos2.GetPositionY(), pos2.GetPositionZ()));

    if (line.length() == 0.f)
        return (size * size) >= GetExactDistSq(&pos1);

    return (size * size) >= line.distanceSquared(G3D::Point3(m_positionX, m_positionY, m_positionZ));
}

template<bool WithOrientation = true>
void readPos(Position& position, ByteBuffer& buffer)
{
    buffer >> position.m_positionX >> position.m_positionY >> position.m_positionZ;

    if constexpr(WithOrientation)
        buffer >> position.m_orientation;
}
template<bool WithOrientation = true>
void writePos(Position position, ByteBuffer& buffer)
{
    buffer << position.m_positionX << position.m_positionY << position.m_positionZ;

    if constexpr(WithOrientation)
        buffer << position.m_orientation;
}

void Position::AsXYZTag::read(Position& position, ByteBuffer& buffer)
{
    readPos<false>(position, buffer);
}

void Position::AsXYZTag::write(Position position, ByteBuffer& buffer)
{
    writePos<false>(position, buffer);
}

namespace packet
{
    Position BufferIO<Position>::read(ByteBuffer& buf)
    {
        Position pos;
        readPos(pos, buf);
        return pos;
    }

    void BufferIO<Position>::write(ByteBuffer& buf, Position pos)
    {
        writePos(pos, buf);
    }
}