#pragma once
#include "Position.h"
#include "ObjectGuid.h"

enum MovementFlags
{
    MOVEMENTFLAG_NONE = 0x00000000,
    MOVEMENTFLAG_FORWARD = 0x00000001,
    MOVEMENTFLAG_BACKWARD = 0x00000002,
    MOVEMENTFLAG_STRAFE_LEFT = 0x00000004,
    MOVEMENTFLAG_STRAFE_RIGHT = 0x00000008,
    MOVEMENTFLAG_LEFT = 0x00000010,
    MOVEMENTFLAG_RIGHT = 0x00000020,
    MOVEMENTFLAG_PITCH_UP = 0x00000040,
    MOVEMENTFLAG_PITCH_DOWN = 0x00000080,
    MOVEMENTFLAG_WALKING = 0x00000100,               // Walking
    MOVEMENTFLAG_ONTRANSPORT = 0x00000200,               // Used for flying on some creatures
    MOVEMENTFLAG_DISABLE_GRAVITY = 0x00000400,               // Former MOVEMENTFLAG_LEVITATING. This is used when walking is not possible.
    MOVEMENTFLAG_ROOT = 0x00000800,               // Must not be set along with MOVEMENTFLAG_MASK_MOVING
    MOVEMENTFLAG_FALLING = 0x00001000,               // damage dealt on that type of falling
    MOVEMENTFLAG_FALLING_FAR = 0x00002000,
    MOVEMENTFLAG_PENDING_STOP = 0x00004000,
    MOVEMENTFLAG_PENDING_STRAFE_STOP = 0x00008000,
    MOVEMENTFLAG_PENDING_FORWARD = 0x00010000,
    MOVEMENTFLAG_PENDING_BACKWARD = 0x00020000,
    MOVEMENTFLAG_PENDING_STRAFE_LEFT = 0x00040000,
    MOVEMENTFLAG_PENDING_STRAFE_RIGHT = 0x00080000,
    MOVEMENTFLAG_PENDING_ROOT = 0x00100000,
    MOVEMENTFLAG_SWIMMING = 0x00200000,               // appears with fly flag also
    MOVEMENTFLAG_ASCENDING = 0x00400000,               // press "space" when flying
    MOVEMENTFLAG_DESCENDING = 0x00800000,
    MOVEMENTFLAG_CAN_FLY = 0x01000000,               // Appears when unit can fly AND also walk
    MOVEMENTFLAG_FLYING = 0x02000000,               // unit is actually flying. pretty sure this is only used for players. creatures use disable_gravity
    MOVEMENTFLAG_SPLINE_ELEVATION = 0x04000000,               // used for flight paths
    MOVEMENTFLAG_SPLINE_ENABLED = 0x08000000,               // used for flight paths
    MOVEMENTFLAG_WATERWALKING = 0x10000000,               // prevent unit from falling through water
    MOVEMENTFLAG_FALLING_SLOW = 0x20000000,               // active rogue safe fall spell (passive)
    MOVEMENTFLAG_HOVER = 0x40000000,               // hover, cannot jump

                                                   /// @todo Check if PITCH_UP and PITCH_DOWN really belong here..
                                                   MOVEMENTFLAG_MASK_MOVING =
                                                   MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD | MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT |
    MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN | MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING |
    MOVEMENTFLAG_SPLINE_ELEVATION,

    MOVEMENTFLAG_MASK_TURNING =
    MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT,

    MOVEMENTFLAG_MASK_MOVING_FLY =
    MOVEMENTFLAG_FLYING | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING,

    /// @todo if needed: add more flags to this masks that are exclusive to players
    MOVEMENTFLAG_MASK_PLAYER_ONLY =
    MOVEMENTFLAG_FLYING,

    /// Movement flags that have change status opcodes associated for players
    MOVEMENTFLAG_MASK_HAS_PLAYER_STATUS_OPCODE = MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_ROOT |
    MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_WATERWALKING | MOVEMENTFLAG_FALLING_SLOW | MOVEMENTFLAG_HOVER
};

enum MovementFlags2
{
    MOVEMENTFLAG2_NONE = 0x00000000,
    MOVEMENTFLAG2_NO_STRAFE = 0x00000001,
    MOVEMENTFLAG2_NO_JUMPING = 0x00000002,
    MOVEMENTFLAG2_UNK3 = 0x00000004,        // Overrides various clientside checks
    MOVEMENTFLAG2_FULL_SPEED_TURNING = 0x00000008,
    MOVEMENTFLAG2_FULL_SPEED_PITCHING = 0x00000010,
    MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING = 0x00000020,
    MOVEMENTFLAG2_UNK7 = 0x00000040,
    MOVEMENTFLAG2_UNK8 = 0x00000080,
    MOVEMENTFLAG2_UNK9 = 0x00000100,
    MOVEMENTFLAG2_UNK10 = 0x00000200,
    MOVEMENTFLAG2_INTERPOLATED_MOVEMENT = 0x00000400,
    MOVEMENTFLAG2_INTERPOLATED_TURNING = 0x00000800,
    MOVEMENTFLAG2_INTERPOLATED_PITCHING = 0x00001000,
    MOVEMENTFLAG2_UNK14 = 0x00002000,
    MOVEMENTFLAG2_UNK15 = 0x00004000,
    MOVEMENTFLAG2_UNK16 = 0x00008000
};



struct MovementInfo
{
    // common
    ObjectGuid guid;
    uint32 flags;
    uint16 flags2;
    Position pos;
    uint32 time;

    // transport
    struct TransportInfo
    {
        void Reset()
        {
            guid.Clear();
            pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
            seat = -1;
            time = 0;
            time2 = 0;
        }

        ObjectGuid guid;
        Position pos;
        int8 seat;
        uint32 time;
        uint32 time2;
    } transport;

    // swimming/flying
    float pitch;

    // falling
    uint32 fallTime;

    // jumping
    struct JumpInfo
    {
        void Reset()
        {
            zspeed = sinAngle = cosAngle = xyspeed = 0.0f;
        }

        float zspeed, sinAngle, cosAngle, xyspeed;

    } jump;

    // spline
    float splineElevation;

    MovementInfo() :
        guid(), flags(0), flags2(0), time(0), pitch(0.0f), fallTime(0), splineElevation(0.0f)
    {
        pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
        transport.Reset();
        jump.Reset();
    }

    uint32 GetMovementFlags() const { return flags; }
    void SetMovementFlags(uint32 flag) { flags = flag; }
    void AddMovementFlag(uint32 flag) { flags |= flag; }
    void RemoveMovementFlag(uint32 flag) { flags &= ~flag; }
    bool HasMovementFlag(uint32 flag) const { return (flags & flag) != 0; }

    uint16 GetExtraMovementFlags() const { return flags2; }
    void AddExtraMovementFlag(uint16 flag) { flags2 |= flag; }
    bool HasExtraMovementFlag(uint16 flag) const { return (flags2 & flag) != 0; }

    void SetFallTime(uint32 time) { fallTime = time; }

    void OutDebug();
};