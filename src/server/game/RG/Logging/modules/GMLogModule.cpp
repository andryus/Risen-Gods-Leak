#include "GMLogModule.hpp"

#include "Config.h"
#include "Player.h"
#include "Spell.h"
#include "WorldSession.h"

RG::Logging::GMLogModule::GMLogModule() : 
    LogModule("GM")
{ }

void RG::Logging::GMLogModule::LoadConfig()
{
    LogModule::LoadConfig();

    config.gm_level = sConfigMgr->GetIntDefault(GetOption("GMLevel").c_str(), AccountMgr::GetStaffSecurityLevel());
    config.spell_logging_enabled = sConfigMgr->GetBoolDefault(GetOption("Spell.Enabled").c_str(), true);
    config.command_logging_enabled = sConfigMgr->GetBoolDefault(GetOption("Command.Enabled").c_str(), true);
}

void RG::Logging::GMLogModule::Log(Player* player, Spell* spell, Unit* caster, const SpellCastTargets& targets)
{
    if (!IsEnabled() || !config.spell_logging_enabled)
        return;

    if (player->GetSession()->GetSecurity() < config.gm_level)
        return;

    uint32 i = 0;

    PreparedStatement* stmt = RGDatabase.GetPreparedStatement(RG_INS_GM_SPELL_CAST);
    stmt->setUInt32(i++, player->GetSession()->GetAccountId());
    stmt->setUInt32(i++, player->GetGUID().GetCounter());
    stmt->setUInt32(i++, spell->GetSpellInfo()->Id);

    stmt->setUInt64(i++, caster->GetGUID().GetRawValue());

    stmt->setUInt16(i++, static_cast<uint16>(caster->GetMapId()));
    stmt->setFloat(i++,  caster->GetPositionX());
    stmt->setFloat(i++,  caster->GetPositionY());
    stmt->setFloat(i++,  caster->GetPositionZ());

    stmt->setUInt32(i++, targets.GetTargetMask());

    if (uint64 unit = targets.GetUnitTargetGUID().GetRawValue())
        stmt->setUInt64(i++, unit);
    else
        stmt->setNull(i++);

    if (ObjectGuid item = targets.GetItemTargetGUID())
        stmt->setUInt32(i++, item.GetCounter());
    else
        stmt->setNull(i++);
    
    if (targets.HasDst())
    {
        const WorldLocation* dstTarget = targets.GetDstPos();
        stmt->setFloat(i++, dstTarget->GetPositionX());
        stmt->setFloat(i++, dstTarget->GetPositionY());
        stmt->setFloat(i++, dstTarget->GetPositionZ());
    }
    else
    {
        stmt->setNull(i++);
        stmt->setNull(i++);
        stmt->setNull(i++);
    }

    RGDatabase.Execute(stmt);
}

void RG::Logging::GMLogModule::Log(uint32 accountId, Player* player, const std::string& cmd, const std::string& params)
{
    if (!player)
        return;

    if (!IsEnabled() || !config.command_logging_enabled)
        return;

    PreparedStatement* stmt = RGDatabase.GetPreparedStatement(RG_ADD_COMMAND_LOG);

    stmt->setUInt32(0, accountId);
    stmt->setUInt32(1, player->GetGUID().GetCounter());
    stmt->setString(2, cmd);
    stmt->setString(3, params);
    stmt->setFloat(4, player->GetPositionX());
    stmt->setFloat(5, player->GetPositionY());
    stmt->setFloat(6, player->GetPositionZ());
    stmt->setUInt16(7, player->GetMapId());
    stmt->setUInt32(8, player->GetAreaId());
    stmt->setUInt32(9, GetTargetType(player));
    stmt->setInt64(10, GetGuidOrEntry(player));

    RGDatabase.Execute(stmt);
}

void RG::Logging::GMLogModule::Log(uint32 accountId, const std::string& cmd, const std::string& params)
{
    if (!IsEnabled() || !config.command_logging_enabled)
        return;

    PreparedStatement* stmt = RGDatabase.GetPreparedStatement(RG_ADD_COMMAND_LOG);

    stmt->setUInt32(0, accountId);
    stmt->setUInt32(1, 0);
    stmt->setString(2, cmd);
    stmt->setString(3, params);
    stmt->setFloat(4, 0);
    stmt->setFloat(5, 0);
    stmt->setFloat(6, 0);
    stmt->setUInt16(7, 0);
    stmt->setUInt32(8, 0);
    stmt->setUInt32(9, 0);
    stmt->setInt64(10, 0);

    RGDatabase.Execute(stmt);
}

uint32 RG::Logging::GMLogModule::GetTargetType(Player* player)
{
    if (Unit* target = player->GetSelectedUnit())
        if (Player* targetplayer = target->ToPlayer())
        {
            if (targetplayer == player)
            {
                return 1; // custom for self target
            }
            else if (AccountMgr::IsStaffAccount(targetplayer->GetSession()->GetSecurity()))
            {
                return 2; // Game Master
            }
        }

    return static_cast<uint32>(player->GetTarget().GetHigh());
}

uint64 RG::Logging::GMLogModule::GetGuidOrEntry(Player *player)
{
    switch (player->GetTarget().GetHigh())
    {
    case HighGuid::Player:
        return player->GetTarget() ? player->GetTarget().GetRawValue() : 0;
    case HighGuid::Unit:
    case HighGuid::Pet:
        if (Unit* unit = player->GetSelectedUnit())
            if (Creature* creature = unit->ToCreature())
                if (const CreatureTemplate* proto = creature->GetCreatureTemplate())
                    return proto->Entry;
    default:
        return player->GetTarget().GetRawValue();
    }
}
