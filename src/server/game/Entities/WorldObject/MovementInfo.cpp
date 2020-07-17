#include "MovementInfo.h"
#include "Log.h"

void MovementInfo::OutDebug()
{
    TC_LOG_DEBUG("misc", "MOVEMENT INFO");
    TC_LOG_DEBUG("misc", "%s", guid.ToString().c_str());
    TC_LOG_DEBUG("misc", "flags %u", flags);
    TC_LOG_DEBUG("misc", "flags2 %u", flags2);
    TC_LOG_DEBUG("misc", "time %u current time " UI64FMTD "", flags2, uint64(::time(NULL)));
    TC_LOG_DEBUG("misc", "position: `%s`", pos.ToString().c_str());
    if (flags & MOVEMENTFLAG_ONTRANSPORT)
    {
        TC_LOG_DEBUG("misc", "TRANSPORT:");
        TC_LOG_DEBUG("misc", "%s", transport.guid.ToString().c_str());
        TC_LOG_DEBUG("misc", "position: `%s`", transport.pos.ToString().c_str());
        TC_LOG_DEBUG("misc", "seat: %i", transport.seat);
        TC_LOG_DEBUG("misc", "time: %u", transport.time);
        if (flags2 & MOVEMENTFLAG2_INTERPOLATED_MOVEMENT)
            TC_LOG_DEBUG("misc", "time2: %u", transport.time2);
    }

    if ((flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING))
        TC_LOG_DEBUG("misc", "pitch: %f", pitch);

    TC_LOG_DEBUG("misc", "fallTime: %u", fallTime);
    if (flags & MOVEMENTFLAG_FALLING)
        TC_LOG_DEBUG("misc", "j_zspeed: %f j_sinAngle: %f j_cosAngle: %f j_xyspeed: %f", jump.zspeed, jump.sinAngle, jump.cosAngle, jump.xyspeed);

    if (flags & MOVEMENTFLAG_SPLINE_ELEVATION)
        TC_LOG_DEBUG("misc", "splineElevation: %f", splineElevation);
}
