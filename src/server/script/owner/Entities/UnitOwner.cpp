#include "UnitOwner.h"
#include "Unit.h"

SCRIPT_OWNER_IMPL(Unit, WorldObject)
{
    return base.To<Unit>();
}

SCRIPT_PRINTER_IMPL(TextLine)
{
    if (value.id != 255)
    {
        std::string out = "TextId: " + script::print(value.id);

        if(value.chance != 1.0f)
            out += ", Chance: " + script::print(value.chance);

        return out;
    }
    else
        return "<NO_TEXT>";
}