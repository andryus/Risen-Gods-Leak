#include "UnitScript.h"
#include "UnitHooks.h"
#include "Unit.h"
#include "CreatureTextMgr.h"

/************************************
* ACTIONS
************************************/

SCRIPT_ACTION_IMPL(Talk)
{
    if (text.id != 255)
    {
        if (frand(0.0f, 1.0f) > text.chance)
            return SetSuccess(false);

        if (Creature* creature = me.To<Creature>())
            sCreatureTextMgr->SendChat(creature, text.id);
    }
    else
        SetSuccess(false);
}


SCRIPT_ACTION_IMPL(TalkTo)
{    
    if (text.id != 255)
    {
        if (frand(0.0f, 1.0f) > text.chance)
            return SetSuccess(false);

        if (Creature* creature = me.To<Creature>())
            sCreatureTextMgr->SendChat(creature, text.id, &target);
        else if (Player* player = me.To<Player>())
            if (Creature* creature = target.To<Creature>()) // players needs a creature to talk to
                sCreatureTextMgr->SendChat(creature, text.id, nullptr, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_NORMAL, 0, TEAM_OTHER, false, player);
    }
    else
        SetSuccess(false);
}

SCRIPT_ACTION_IMPL(FaceTo)
{
    me.SetFacingToObject(&target);
}

SCRIPT_ACTION_IMPL(FaceDirection)
{
    me.SetFacingTo(orientation);
}

SCRIPT_ACTION_IMPL(Show)
{
    me.SetVisible(true);

    me <<= Fire::Visible;

    AddUndoable(Do::Hide);
}

SCRIPT_ACTION_IMPL(Hide)
{
    me.SetVisible(false);

    me <<= Fire::Invisible;

    AddUndoable(Do::Show);
}

SCRIPT_ACTION_IMPL(HideModel)
{
    me <<= Do::Morph(11686);
}

SCRIPT_ACTION_IMPL(Morph)
{
    const uint32 old = me.GetDisplayId();
    me.SetDisplayId(modelId);

    AddUndoable(Do::Morph(old));
}

SCRIPT_ACTION_IMPL(ChangeFaction)
{
    const uint32 previousFaction = me.getFaction();
    me.setFaction(faction.id);

    AddUndoable(Do::ChangeFaction((FactionType)previousFaction));
}

SCRIPT_ACTION_IMPL(Die)
{
    me.Kill(&me);
}

// flags

struct FlagPack
{
    uint16 type;
    uint32 flags;
};


SCRIPT_ACTION_EX(RemoveFlags, Unit, FlagPack, flags)

SCRIPT_ACTION_EX_INLINE(SetFlags, Unit, FlagPack, flags)
{
    auto activeFlags = me.GetUInt32Value(flags.type);
    activeFlags ^= flags.flags;
    activeFlags &= flags.flags;

    me.SetFlag(flags.type, flags.flags);

    AddUndoable(Do::RemoveFlags({flags.type, activeFlags }));
}

SCRIPT_ACTION_IMPL(RemoveFlags)
{
    auto activeFlags = me.GetUInt32Value(flags.type);
    activeFlags &= flags.flags;

    me.RemoveFlag(flags.type, activeFlags);

    AddUndoable(Do::SetFlags({flags.type, activeFlags}));
}



SCRIPT_ACTION_IMPL(MakeSelectable)
{
    me <<= Do::RemoveFlags({UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE});
}

SCRIPT_ACTION_IMPL(MakeUnselectable)
{
    me <<= Do::SetFlags({ UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE });
}

SCRIPT_ACTION_INLINE(UndoEvade, Unit)
{
    me.ClearUnitState(UNIT_STATE_EVADE);
}

SCRIPT_ACTION_IMPL(MakeEvade)
{
    me.AddUnitState(UNIT_STATE_EVADE);

    AddUndoable(Do::UndoEvade);
}

SCRIPT_ACTION_IMPL(DisablePlayerInteraction)
{
    me <<= Do::RemoveFlags({ UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_SPELLCLICK});
}

SCRIPT_ACTION_IMPL(DisableGossip)
{
    me <<= Do::RemoveFlags({ UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP });
}

SCRIPT_ACTION_IMPL(MakeQuestGiver)
{
    me <<= Do::SetFlags({UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER});
}

SCRIPT_ACTION_IMPL(RemoveQuestGiver)
{
    me <<= Do::RemoveFlags({ UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER });
}


SCRIPT_ACTION_EX_INLINE(SetStandState, Unit, uint8, state)
{
    const auto old = me.GetStandState();
    me.SetStandState(state);

    AddUndoable(Do::SetStandState(old));
}

SCRIPT_ACTION_IMPL(Kneel)
{
    me <<= Do::SetStandState(UNIT_STAND_STATE_KNEEL);
}

SCRIPT_ACTION_IMPL(StandUp)
{
    me <<= Do::SetStandState(UNIT_STAND_STATE_STAND);
}

SCRIPT_ACTION_IMPL(Pacify)
{
    me <<= Do::SetFlags({UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED});
}

SCRIPT_ACTION_IMPL(TeleportTo)
{
    me.NearTeleportTo(position);
}

SCRIPT_ACTION_EX_INLINE(ChangeEmoteState, Unit, uint32, emote)
{
    const uint32 oldEmote = me.GetUInt32Value(UNIT_NPC_EMOTESTATE);

    if (emote != oldEmote)
    {
        me.SetUInt32Value(UNIT_NPC_EMOTESTATE, emote);

        AddUndoable(Do::ChangeEmoteState(oldEmote));
    }
}

SCRIPT_ACTION_IMPL(StopEmote)
{
    me <<= Do::ChangeEmoteState(EMOTE_ONESHOT_NONE);
}

SCRIPT_ACTION_INLINE(UndoSheath, Unit)
{
    me.SetSheath(SheathState::SHEATH_STATE_UNARMED);
}

SCRIPT_ACTION_IMPL(MakeSheath)
{
    me.SetSheath(ranged ? SheathState::SHEATH_STATE_RANGED : SheathState::SHEATH_STATE_MELEE);

    AddUndoable(Do::UndoSheath);
}

SCRIPT_ACTION_EX_INLINE(Mount, Unit, uint32, id)
{
    me.Mount(id);
}

SCRIPT_ACTION_IMPL(Dismount)
{
    if (me.IsMounted())
    {
        const uint32 id = me.GetMountID();

        me.Dismount();

        AddUndoable(Do::Mount(id));
    }
}


/************************************
* FILTER
************************************/

SCRIPT_FILTER_IMPL(Visible)
{
    return me.IsVisible();
}

SCRIPT_FILTER_IMPL(Sitting)
{
    return me.IsSitState();
}

SCRIPT_FILTER_IMPL(Alive)
{
    return me.IsAlive();
}

SCRIPT_FILTER_IMPL(Pacified)
{
    return me.HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
}

SCRIPT_FILTER_IMPL(IsNPC)
{
    return !me.IsCharmedOwnedByPlayerOrPlayer();
}

SCRIPT_FILTER_IMPL(IsOfFaction)
{
    return me.getFaction() == faction.id;
}

SCRIPT_FILTER_IMPL(PrimaryTargetOf)
{
    return attacker.GetVictim() == &me;
}

SCRIPT_FILTER_IMPL(WithinRange)
{
    return me.IsWithinDist(&target, range);
}

/************************************
* SELECTOR
************************************/

SCRIPT_GETTER_IMPL(DirectionTo)
{
    if (const float distance = me.GetDistance2d(&target))
    {
        Position direction = { me.GetPositionX() - target.GetPositionX(), me.GetPositionY() - target.GetPositionY(), 0.0f, 0.0f };
        direction.m_positionX /= distance;
        direction.m_positionY /= distance;

        return direction;
    }
    else
        return { 1.0f, 0.0f, 0.0f, 0.0f };
}

SCRIPT_GETTER_IMPL(ForwardDirection)
{
    const float orientation = me.GetOrientation();

    Position pos;
    pos.m_positionX = cos(orientation);
    pos.m_positionY = sin(orientation);

    return pos;
}

SCRIPT_GETTER_IMPL(UnitsInCombatWith)
{
    auto& targets = me.GetThreatManager().getThreatList();

    script::List<Unit*> validTargets;
    for (HostileReference* target : targets)
    {
        if (target->IsAvailable())
            validTargets.emplace_back(target->GetVictim());
    }

    return validTargets;
}

SCRIPT_SELECTOR_IMPL(Owner)
{
    return ((TempSummon&)me).GetSummoner();
}
