#include "WorldObjectScript.h"
#include "WorldObject.h"

/************************************
* ACTION
************************************/

SCRIPT_ACTION_INLINE(UndoActivate, WorldObject)
{
    me.setActive(false);
}

SCRIPT_ACTION_IMPL(MakeAlwaysActive)
{
    me.setActive(true);

    AddUndoable(Do::UndoActivate);
}


/************************************
* MODIFIER
************************************/

SCRIPT_GETTER_IMPL(Location)
{
    return me.GetPosition();
}