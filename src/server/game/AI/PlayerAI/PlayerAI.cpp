/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"

#include "PlayerAI.h"

enum class Specs : uint8
{
    WARRIOR_ARMS           = 0,
    WARRIOR_FURY           = 1,
    WARRIOR_PROTECTION     = 2,

    PALADIN_HOLY           = 0,
    PALADIN_PROTECTION     = 1,
    PALADIN_RETRIBUTION    = 2,

    HUNTER_BEAST_MASTERY   = 0,
    HUNTER_MARKSMANSHIP    = 1,
    HUNTER_SURVIVAL        = 2,

    ROGUE_ASSASSINATION    = 0,
    ROGUE_COMBAT           = 1,
    ROGUE_SUBLETY          = 2,

    PRIEST_DISCIPLINE      = 0,
    PRIEST_HOLY            = 1,
    PRIEST_SHADOW          = 2,

    DEATH_KNIGHT_BLOOD     = 0,
    DEATH_KNIGHT_FROST     = 1,
    DEATH_KNIGHT_UNHOLY    = 2,

    SHAMAN_ELEMENTAL       = 0,
    SHAMAN_ENHANCEMENT     = 1,
    SHAMAN_RESTORATION     = 2,

    MAGE_ARCANE            = 0,
    MAGE_FIRE              = 1,
    MAGE_FROST             = 2,

    WARLOCK_AFFLICTION     = 0,
    WARLOCK_DEMONOLOGY     = 1,
    WARLOCK_DESTRUCTION    = 2,

    DRUID_BALANCE          = 0,
    DRUID_FERAL            = 1,
    DRUID_RESTORATION      = 2
};

enum class Spells : uint32
{
    NONE                  = 0,

    /* Generic */
    AUTO_SHOT             = 75,
    SHOOT                 = 3018,
    THROW                 = 2764,
    SHOOT_WAND            = 5019,

    SPELL_DRUID_CATFORM   = 768,
};

constexpr float CASTER_CHASE_DISTANCE = 28.0f;

PlayerAI::PlayerAI(Player& player) : UnitAI(&player), me(player), _selfSpec(GetPlayerSpec(player))
{
    if (!player.GetCharmerGUID().IsCreature())
        return;

    _isSelfRangedAttacker = IsPlayerRangedAttacker(_selfSpec);

    switch (player.getClass())
    {
        case CLASS_DRUID:
            if (!_isSelfRangedAttacker &&
                player.HasSpell(static_cast<uint32>(Spells::SPELL_DRUID_CATFORM)) &&
                !player.HasAura(static_cast<uint32>(Spells::SPELL_DRUID_CATFORM)))
                player.CastSpell(&player, static_cast<uint32>(Spells::SPELL_DRUID_CATFORM), true);
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            _CancelAllShapeshifts();
        default:
            break;
    }
}

Creature* PlayerAI::GetCharmer() const
{
    if (me.GetCharmerGUID().IsCreature())
        return ObjectAccessor::GetCreature(me, me.GetCharmerGUID());
    return nullptr;
}

Specs PlayerAI::GetPlayerSpec(Player const& who)
{
    const uint8 distributions[3] =
    {
        who.GetTalentPointsInTab(0),
        who.GetTalentPointsInTab(1),
        who.GetTalentPointsInTab(2)
    };

    uint8 max = 0, best = 0;
    for (uint8 i = 0; i < 3; i++)
    {
        if (distributions[i] > max)
        {
            max = distributions[i];
            best = i;
        }
    }
    return static_cast<Specs>(best);
}

bool PlayerAI::IsPlayerRangedAttacker(Specs specc) const
{
    switch (me.getClass())
    {
        case CLASS_PALADIN:
            return specc == Specs::PALADIN_HOLY;
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_PRIEST:
            return true;
        case CLASS_HUNTER:
        {
            // check if we have a ranged weapon equipped
            Item const* rangedSlot = me.GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
            if (ItemTemplate const* rangedTemplate = rangedSlot ? rangedSlot->GetTemplate() : nullptr)
                if ((1 << rangedTemplate->SubClass) & ITEM_SUBCLASS_MASK_WEAPON_RANGED)
                    return true;
            return false;
        }
        case CLASS_SHAMAN:
            return specc != Specs::SHAMAN_ENHANCEMENT;
        case CLASS_DRUID:
            return specc != Specs::DRUID_FERAL;
        default:
            return false;
    }
}

void PlayerAI::DoAutoAttackIfReady()
{
    if (_isSelfRangedAttacker)
        DoRangedAttackIfReady();
    else
        DoMeleeAttackIfReady();
}

void PlayerAI::DoRangedAttackIfReady()
{
    if (me.HasUnitState(UNIT_STATE_CASTING) || !me.isAttackReady(RANGED_ATTACK))
        return;

    Unit* victim = me.GetVictim();
    if (!victim)
        return;

    Spells rangedAttackSpell;
    Item const* rangedItem = me.GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
    if (ItemTemplate const* rangedTemplate = rangedItem ? rangedItem->GetTemplate() : nullptr)
    {
        switch (rangedTemplate->SubClass)
        {
            case ITEM_SUBCLASS_WEAPON_BOW:
            case ITEM_SUBCLASS_WEAPON_GUN:
            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                rangedAttackSpell = Spells::SHOOT;
                break;
            case ITEM_SUBCLASS_WEAPON_THROWN:
                rangedAttackSpell = Spells::THROW;
                break;
            case ITEM_SUBCLASS_WEAPON_WAND:
                rangedAttackSpell = Spells::SHOOT_WAND;
                break;
            default:
                return;
        }

        me.CastSpell(victim, static_cast<uint32>(rangedAttackSpell), TRIGGERED_CAST_DIRECTLY);
        me.resetAttackTimer(RANGED_ATTACK);
    }
}

PlayerAI::TargetedSpell PlayerAI::SelectSpell()
{
    if (_usableSpells.empty())
        ShuffleRandomSpells();

    for (uint8 i = 0; i < std::min<uint32>(_usableSpells.size(), 10U); ++i)
    {
        auto itr = Trinity::Containers::SelectRandomWeightedContainerElement(_usableSpells, _usableSpellsWeights);
        const uint32 spellId = *itr;
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);

        if (me.GetSpellHistory()->HasGlobalCooldown(spellInfo))
            return {};

        if (me.GetSpellHistory()->HasCooldown(spellId))
        {
            TC_LOG_INFO("playerai", "PlayerAI: HasSpellCooldown() - failed for spell: %u. class: %u specc: %u", spellId, me.getClass(), static_cast<uint8>(_selfSpec));
            const auto index = itr - _usableSpells.begin();
            _usableSpellsWeights.erase(_usableSpellsWeights.begin() + index);
            _usableSpells.erase(itr);
            return {};
        }

        if (Unit* target = _SelectTargetForSpell(spellInfo))
        {
            // there is no other way afaik...
            Spell* spell = new Spell(&me, spellInfo, TRIGGERED_NONE);
            // most spells can be check by this simple check. Only perform CheckCast if CanAutoCast fails (mostly applies for AOE spells with not explicit target)
            if (spell->CanAutoCast(target))
            {
                if (spellInfo->RecoveryTime)
                {
                    const auto index = itr - _usableSpells.begin();
                    _usableSpellsWeights.erase(_usableSpellsWeights.begin() + index);
                    _usableSpells.erase(itr);
                }
                return{ target, spell };
            }
            else if (spell->CheckCast(true) == SPELL_CAST_OK)
            {
                if (spellInfo->RecoveryTime)
                {
                    const auto index = itr - _usableSpells.begin();
                    _usableSpellsWeights.erase(_usableSpellsWeights.begin() + index);
                    _usableSpells.erase(itr);
                }
                return{ target, spell };
            }

            TC_LOG_INFO("playerai", "PlayerAI: CheckCast() - failed for spell: %u. class: %u specc: %u", spellId, me.getClass(), static_cast<uint8>(_selfSpec));
            const auto index = itr - _usableSpells.begin();
            _usableSpellsWeights.erase(_usableSpellsWeights.begin() + index);
            _usableSpells.erase(itr);
            delete spell;
            return {};
        }
        else
        {
            TC_LOG_WARN("playerai", "PlayerAI: SelectSpell() - unsupported target for spell: %u", spellId);
            const auto index = itr - _usableSpells.begin();
            _usableSpellsWeights.erase(_usableSpellsWeights.begin() + index);
            _usableSpells.erase(itr);
            return {};
        }
    }
    return {};
}

void PlayerAI::DoCastAtTarget(TargetedSpell spell)
{
    me.SetTarget(spell.target->GetGUID());

    SpellCastTargets targets;
    targets.SetUnitTarget(spell.target);
    spell.spell->prepare(&targets);
}

Unit* PlayerAI::_SelectTargetForSpell(SpellInfo const* spellInfo) const
{
    Targets target;
    if (spellInfo->Effects[EFFECT_0].IsEffect())
        target = spellInfo->Effects[EFFECT_0].TargetA.GetTarget();
    else
        target = spellInfo->Effects[EFFECT_1].TargetA.GetTarget();

    switch (target)
    {
        case TARGET_UNIT_TARGET_ENEMY:
        case TARGET_UNIT_DEST_AREA_ENEMY:
        case TARGET_DEST_DYNOBJ_ENEMY:
            return me.GetVictim();
        case TARGET_UNIT_TARGET_ALLY:
        case TARGET_UNIT_TARGET_RAID:
            if (me.GetCharmer() && me.IsWithinDist2d(me.GetCharmer(), CASTER_CHASE_DISTANCE))
                return roll_chance_i(50) ? &me : me.GetCharmer();
            else
                return &me;
        default:
            return &me;
    }
}

uint32 PlayerAI::_GetHighestLearedRank(uint32 spellId) const
{
    if (uint32 last = sSpellMgr->GetLastSpellInChain(spellId); me.HasSpell(last))
        return last;

    uint32 nextRank = spellId;
    do
    {
        spellId = nextRank;
        nextRank = sSpellMgr->GetNextSpellInChain(spellId);

    } while (me.HasSpell(nextRank));

    return spellId;
}

void PlayerAI::ShuffleRandomSpells()
{
    _usableSpells.clear();
    _usableSpellsWeights.clear();

    auto& playerSpells = me.GetSpellMap();
    std::set<uint32> uniquePlayerSpells;
    for (auto spell : playerSpells)
    {
        if (!spell.second->active || spell.second->disabled)
            continue;

        if (auto spellInfo = sSpellMgr->GetSpellInfo(spell.first))
        {
            if (!spellInfo->HasAttribute(SPELL_ATTR0_CU_USABLE_FOR_PLAYER_AI))
            {
                TC_LOG_TRACE("playerai", "PlayerAI: ShuffleRandomSpells() - spell: %u. class: %u specc: %u not selected", spellInfo->Id, me.getClass(), static_cast<uint8>(_selfSpec));
                continue;
            }

            uniquePlayerSpells.insert(_GetHighestLearedRank(spell.first));
        }
    }

    for (uint32 spellId : uniquePlayerSpells)
    {
        auto spellInfo = sSpellMgr->GetSpellInfo(spellId);

        if (spellInfo->RecoveryTime)
        {
            _usableSpells.push_back(spellInfo->Id);
            _usableSpellsWeights.push_back(std::max<float>(5.f, spellInfo->RecoveryTime / 10000));
            TC_LOG_TRACE("playerai", "PlayerAI: ShuffleRandomSpells() - spell: %u. class: %u specc: %u is used with chance: %f",
                spellInfo->Id, me.getClass(), static_cast<uint8>(_selfSpec), std::max<float>(5.f, spellInfo->RecoveryTime / 10000));
        }
        else
        {
            _usableSpells.push_back(spellInfo->Id);
            _usableSpellsWeights.push_back(5.f);
            TC_LOG_TRACE("playerai", "PlayerAI: ShuffleRandomSpells() - spell: %u. class: %u specc: %u is used with chance: %f",
                spellInfo->Id, me.getClass(), static_cast<uint8>(_selfSpec), 5.f);
        }
    }
}

void PlayerAI::_CancelAllShapeshifts()
{
    std::list<AuraEffect*> const& shapeshiftAuras = me.GetAuraEffectsByType(SPELL_AURA_MOD_SHAPESHIFT);
    std::set<Aura*> removableShapeshifts;
    for (AuraEffect* auraEff : shapeshiftAuras)
    {
        Aura* aura = auraEff->GetBase();
        if (!aura)
            continue;
        SpellInfo const* auraInfo = aura->GetSpellInfo();
        if (!auraInfo)
            continue;
        if (auraInfo->HasAttribute(SPELL_ATTR0_CANT_CANCEL))
            continue;
        if (!auraInfo->IsPositive() || auraInfo->IsPassive())
            continue;
        removableShapeshifts.insert(aura);

        TC_LOG_TRACE("playerai", "PlayerAI: _CancelAllShapeshifts() - spell: %u. class: %u specc: %u is removed",
            auraInfo->Id, me.getClass(), static_cast<uint8>(_selfSpec));
    }

    for (Aura* aura : removableShapeshifts)
        me.RemoveOwnedAura(aura, AURA_REMOVE_BY_CANCEL);
}

static uint32 CalculateGCD(Player& player, bool isRange)
{
    constexpr uint32 MIN_GCD = 1000;
    constexpr uint32 MAX_GCD = 1500;
    if (isRange)
    {
        // Apply haste rating
        uint32 gcd = MAX_GCD * player.GetFloatValue(UNIT_MOD_CAST_SPEED);
        if (gcd < MIN_GCD)
            gcd = MIN_GCD;
        else if (gcd > MAX_GCD)
            gcd = MAX_GCD;

        TC_LOG_TRACE("playerai", "PlayerAI: CalculateGCD() - gcd: %u is selected for player with guid: %u", gcd, player.GetGUID().GetCounter());

        return gcd;
    }
    TC_LOG_TRACE("playerai", "PlayerAI: CalculateGCD() - gcd: %u is selected for player with guid: %u", MAX_GCD, player.GetGUID().GetCounter());
    return MAX_GCD;
}

SimpleCharmedPlayerAI::SimpleCharmedPlayerAI(Player& player) :
    PlayerAI(player),
    _castCheckTimer(1500),
    _forceAIChangeTimer(30 * IN_MILLISECONDS),
    _gcd(CalculateGCD(player, IsRangedAttacker())),
    _chaseCloser(false),
    _forceFacing(true),
    _isFollowing(false)
{}


static void InterruptActions(Player& player)
{
    player.AttackStop();
    player.CastStop();
    player.StopMoving();
}

bool SimpleCharmedPlayerAI::CanAIAttack(Unit const* who) const
{
    if (!me.IsValidAttackTarget(who) || who->HasBreakableByDamageCrowdControlAura())
        return false;
    if (Unit const* charmer = me.GetCharmer())
        if (!charmer->IsValidAttackTarget(who))
            return false;
    return UnitAI::CanAIAttack(who);
}

Unit* SimpleCharmedPlayerAI::SelectAttackTarget() const
{
    if (Unit* charmer = me.GetCharmer())
    {
        if (charmer->IsAIEnabled)
        {
            return charmer->GetAI()->SelectTarget(SELECT_TARGET_RANDOM, 0, [ai = this](Unit const* target)
            {
                return ai->CanAIAttack(target);
            });
        }
        else
            return charmer->GetVictim();
    }
    return nullptr;
}

void SimpleCharmedPlayerAI::UpdateAI(const uint32 diff)
{
    Creature* charmer = me.GetCharmer() ? me.GetCharmer()->ToCreature() : nullptr;
    if (!charmer)
        return;

    // kill self if charm aura has infinite duration
    if (charmer->IsInEvadeMode() || (charmer->IsDungeonBoss() && !charmer->IsInCombat()))
    {
        auto& auras = me.GetAuraEffectsByType(SPELL_AURA_MOD_CHARM);
        for (auto aura : auras)
        {
            if (aura->GetBase()->IsPermanent() && aura->GetCasterGUID() == charmer->GetGUID())
            {
                me.KillSelf();
                return;
            }
        }
    }

    if (charmer->IsInCombat())
    {
        Unit* target = me.GetVictim();
        if (!target || !CanAIAttack(target))
        {
            target = SelectAttackTarget();
            if (!target || !CanAIAttack(target))
            {
                if (!_isFollowing)
                {
                    _isFollowing = true;
                    InterruptActions(me);
                    me.GetMotionMaster()->Clear();
                    me.GetMotionMaster()->MoveFollow(charmer, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                }
                return;
            }
            _isFollowing = false;

            if (IsRangedAttacker())
            {
                _chaseCloser = !me.IsWithinLOSInMap(target);
                if (_chaseCloser)
                    AttackStart(target);
                else
                    AttackStartCaster(target, CASTER_CHASE_DISTANCE);
            }
            else
                AttackStart(target);
            _forceFacing = true;
        }

        if (!me.isMoving() && !me.HasUnitState(UNIT_STATE_CANNOT_TURN))
        {
            float targetAngle = me.GetAngle(target);
            if (_forceFacing || fabs(me.GetOrientation() - targetAngle) > 0.4f)
            {
                me.SetFacingTo(targetAngle);
                _forceFacing = false;
            }
        }

        if (_castCheckTimer.Passed())
        {
            if (me.IsNonMeleeSpellCast(false, false, true))
            {
                _castCheckTimer.Reset(250ms);
            }
            else
            {
                if (IsRangedAttacker())
                {
                    // chase to zero if the target isn't in line of sight
                    bool inLOS = me.IsWithinLOSInMap(target);
                    if (_chaseCloser != !inLOS)
                    {
                        _chaseCloser = !inLOS;
                        if (_chaseCloser)
                            AttackStart(target);
                        else
                            AttackStartCaster(target, CASTER_CHASE_DISTANCE);
                    }
                }
                if (TargetedSpell shouldCast = SelectSpell())
                {
                    if (const uint32 castTime = shouldCast.spell->GetSpellInfo()->CalcCastTime(shouldCast.spell); castTime > 0)
                        _castCheckTimer.Reset(castTime + 50);
                    else
                        _castCheckTimer.Reset(_gcd + 50);

                    DoCastAtTarget(shouldCast);
                }
                else
                    _castCheckTimer.Reset(250ms);
            }
        }
        else
            _castCheckTimer.Update(diff);

        if (!IsRangedAttacker() || me.getClass() == CLASS_HUNTER || (IsRangedAttacker() && me.GetPower(POWER_MANA) <= 100))
            DoAutoAttackIfReady();

        if (_forceAIChangeTimer.Passed())
        {
            ShuffleRandomSpells();
            target = nullptr;
            _forceAIChangeTimer.Reset(20s);
        }
        else
            _forceAIChangeTimer.Update(diff);
    }
    else if (!_isFollowing)
    {
        _isFollowing = true;
        InterruptActions(me);
        me.GetMotionMaster()->Clear();
        me.GetMotionMaster()->MoveFollow(charmer, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
    }
}

void SimpleCharmedPlayerAI::UpdateAINoSpells(uint32 diff)
{
    Creature* charmer = me.GetCharmer() ? me.GetCharmer()->ToCreature() : nullptr;
    if (!charmer)
        return;

    // kill self if charm aura has infinite duration
    if (charmer->IsInEvadeMode() || (charmer->IsDungeonBoss() && !charmer->IsInCombat()))
    {
        auto& auras = me.GetAuraEffectsByType(SPELL_AURA_MOD_CHARM);
        for (auto aura : auras)
        {
            if (aura->GetBase()->IsPermanent() && aura->GetCasterGUID() == charmer->GetGUID())
            {
                me.KillSelf();
                return;
            }
        }
    }

    if (charmer->IsInCombat())
    {
        Unit* target = me.GetVictim();
        if (!target || !CanAIAttack(target))
        {
            target = SelectAttackTarget();
            if (!target || !CanAIAttack(target))
            {
                if (!_isFollowing)
                {
                    _isFollowing = true;
                    InterruptActions(me);
                    me.GetMotionMaster()->Clear();
                    me.GetMotionMaster()->MoveFollow(charmer, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                }
                return;
            }
            _isFollowing = false;

            AttackStart(target);
            _forceFacing = true;
        }

        if (!me.isMoving() && !me.HasUnitState(UNIT_STATE_CANNOT_TURN))
        {
            float targetAngle = me.GetAngle(target);
            if (_forceFacing || fabs(me.GetOrientation() - targetAngle) > 0.4f)
            {
                me.SetFacingTo(targetAngle);
                _forceFacing = false;
            }
        }

        DoMeleeAttackIfReady();

        if (_forceAIChangeTimer.Passed())
        {
            target = nullptr;
            _forceAIChangeTimer.Reset(20s);
        }
        else
            _forceAIChangeTimer.Update(diff);
    }
    else if (!_isFollowing)
    {
        _isFollowing = true;
        InterruptActions(me);
        me.GetMotionMaster()->Clear();
        me.GetMotionMaster()->MoveFollow(charmer, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
    }
}

void SimpleCharmedPlayerAI::OnCharmed(bool apply)
{
    me.ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, apply);
    InterruptActions(me);
    me.GetMotionMaster()->Clear();

    if (apply)
        me.GetMotionMaster()->MovePoint(0, me, false); // force re-sync of current position for all clients
    else
        me.SetTarget(ObjectGuid::Empty);
}
