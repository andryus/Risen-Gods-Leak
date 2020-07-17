/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2015 Rising Gods <http://www.rising-gods.de/>
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

/* ScriptData
SDName: Urom
SD%Complete: 80
SDComment: Is not working SPELL_ARCANE_SHIELD. SPELL_FROSTBOMB has some issues, the damage aura should not stack.
SDCategory: Instance Script
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "oculus.h"
#include "SpellInfo.h"
#include "SpellScript.h"

enum Spells
{

    SPELL_ARCANE_SHIELD                           = 53813, //Dummy --> Channeled, shields the caster from damage.
    SPELL_EMPOWERED_ARCANE_EXPLOSION              = 51110,
    SPELL_EMPOWERED_ARCANE_EXPLOSION_2            = 59377,
    SPELL_FROSTBOMB                               = 51103, //Urom throws a bomb, hitting its target with the highest aggro which inflict directly 650 frost damage and drops a frost zone on the ground. This zone deals 650 frost damage per second and reduce the movement speed by 35%. Lasts 1 minute.
    SPELL_SUMMON_MENAGERIE                        = 50476, //Summons an assortment of creatures and teleports the caster to safety.
    SPELL_SUMMON_MENAGERIE_2                      = 50495,
    SPELL_SUMMON_MENAGERIE_3                      = 50496,
    SPELL_TELEPORT                                = 51112, //Teleports to the center of Oculus
    SPELL_TIME_BOMB                               = 51121, //Deals arcane damage to a random player, and after 6 seconds, deals zone damage to nearby equal to the health missing of the target afflicted by the debuff.
    SPELL_TIME_BOMB_2                             = 59376,
    SPELL_EVOCATE                                 = 51602, // He always cast it on reset or after teleportation
    SPELL_FROST_BUFFET                            = 58025,
};

enum Yells
{
    SAY_SUMMON_1                                  = 0,
    SAY_SUMMON_2                                  = 1,
    SAY_SUMMON_3                                  = 2,
    SAY_AGGRO                                     = 3,
    EMOTE_ARCANE_EXPLOSION                        = 4,
    SAY_ARCANE_EXPLOSION                          = 5,
    SAY_DEATH                                     = 6,
    SAY_PLAYER_KILL                               = 7
};

enum eCreature
{
    NPC_PHANTASMAL_CLOUDSCRAPER                   = 27645,
    NPC_PHANTASMAL_MAMMOTH                        = 27642,
    NPC_PHANTASMAL_WOLF                           = 27644,

    NPC_PHANTASMAL_AIR                            = 27650,
    NPC_PHANTASMAL_FIRE                           = 27651,
    NPC_PHANTASMAL_WATER                          = 27653,

    NPC_PHANTASMAL_MURLOC                         = 27649,
    NPC_PHANTASMAL_NAGAL                          = 27648,
    NPC_PHANTASMAL_OGRE                           = 27647
};

struct Summons
{
    uint32 entry[4];
};

static Summons Group[]=
{
    { {NPC_PHANTASMAL_CLOUDSCRAPER, NPC_PHANTASMAL_CLOUDSCRAPER, NPC_PHANTASMAL_MAMMOTH, NPC_PHANTASMAL_WOLF} },
    { {NPC_PHANTASMAL_AIR, NPC_PHANTASMAL_AIR, NPC_PHANTASMAL_WATER, NPC_PHANTASMAL_FIRE} },
    { {NPC_PHANTASMAL_OGRE, NPC_PHANTASMAL_OGRE, NPC_PHANTASMAL_NAGAL, NPC_PHANTASMAL_MURLOC} }
};

static uint32 TeleportSpells[]=
{
    SPELL_SUMMON_MENAGERIE, SPELL_SUMMON_MENAGERIE_2, SPELL_SUMMON_MENAGERIE_3
};

constexpr Position startPosition = { 1177.47f, 937.72f, 527.41f, 2.216f };
constexpr Position deathPortPosition = { 1118.31f, 1080.377f, 508.361f, 4.25f };

class boss_urom : public CreatureScript
{
public:
    boss_urom() : CreatureScript("boss_urom") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_uromAI (creature);
    }

    struct boss_uromAI : public BossAI
    {
        boss_uromAI(Creature* creature) : BossAI(creature, DATA_UROM_EVENT)
        {
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); 
        }

        void Reset() override
        {
            me->ForbidInhabit(INHABIT_AIR);
            DoCastSelf(SPELL_EVOCATE);

            BossAI::Reset();

            summonCounter = 0;
            teleportInProgress = false;
            delayedActionAfterTeleport = false;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetImmuneToAll(false);

            if (instance->GetData(DATA_UROM_PLATAFORM) < 3)
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetHomePosition(startPosition);
                me->NearTeleportTo(startPosition);
                instance->SetData(DATA_UROM_PLATAFORM, 0);
            }

            if (instance->GetData(DATA_UROM_PLATAFORM) == 0)
            {
                for (uint8 i = 0; i < 3; i++)
                    group[i] = 0;
            }

            x = 0.0f;
            y = 0.0f;
            canCast = false;
            canGoBack = false;

            teleportBackTimer = 2000;
            teleportTimer = urand(30000, 35000);
            frostBombTimer = urand(5000, 8000);
            timeBombTimer = urand(20000, 25000);
        }

        void EnterCombat(Unit* who) override
        {
            if (teleportInProgress)
                return;

            if (instance->GetData(DATA_UROM_PLATAFORM) < 3)
                teleportInProgress = true;
            else
            {
                Talk(SAY_AGGRO);
                me->InterruptNonMeleeSpells(true);
            }

            BossAI::EnterCombat(who);

            SetGroups();
            SummonGroups();
            CastTeleport();

            if (instance->GetData(DATA_UROM_PLATAFORM) != 3)
                instance->SetData(DATA_UROM_PLATAFORM, instance->GetData(DATA_UROM_PLATAFORM) + 1);
        }

        void DamageTaken(Unit* /*attacker*/, uint32 &damage) override
        {
            if (instance->GetData(DATA_UROM_PLATAFORM) < 3)
                damage = 0;
        }

        bool CanAIAttack(Unit const* target) const override
        {
            return target->GetPositionZ() > 507.0f;
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            ++summonCounter;

            if (summonCounter < 4)
                return;

            summonCounter = 0;

            if (teleportInProgress)
            {
                delayedActionAfterTeleport = true;
                return;
            }

            me->RemoveAurasDueToSpell(SPELL_ARCANE_SHIELD);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetImmuneToAll(false);
            DoCastSelf(SPELL_EVOCATE, true);
        }

        void AttackStart(Unit* who) override
        {
            if (!who || instance->GetData(DATA_UROM_PLATAFORM) < 3)
                DoStartNoMovement(who);
            else
                BossAI::AttackStart(who);
        }

        void SetGroups()
        {
            if (!instance || instance->GetData(DATA_UROM_PLATAFORM) != 0)
                return;

            while (group[0] == group[1] || group[0] == group[2] || group[1] == group[2])
            {
                for (uint8 i = 0; i < 3; i++)
                    group[i] = urand(0, 2);
            }
        }

        void SetPosition(uint8 i)
        {
            switch (i)
            {
                case 0:
                    x = me->GetPositionX() + 6.0f;
                    y = me->GetPositionY() - 6.0f;
                    break;
                case 1:
                    x = me->GetPositionX() + 6.0f;
                    y = me->GetPositionY() + 6.0f;
                    break;
                case 2:
                    x = me->GetPositionX() - 6.0f;
                    y = me->GetPositionY() + 6.0f;
                    break;
                case 3:
                    x = me->GetPositionX() - 6.0f;
                    y = me->GetPositionY() - 6.0f;
                    break;
                default:
                    break;
            }
        }

        void SummonGroups()
        {
            if (!instance || instance->GetData(DATA_UROM_PLATAFORM) > 2)
                return;

            for (uint8 i = 0; i < 4; i++)
            {
                SetPosition(i);
                me->SummonCreature(Group[group[instance->GetData(DATA_UROM_PLATAFORM)]].entry[i], x, y, me->GetPositionZ(), me->GetOrientation());
            }

            switch (instance->GetData(DATA_UROM_PLATAFORM))
            {
                case 0:
                    Talk(SAY_SUMMON_1);
                    break;
                case 1:
                    Talk(SAY_SUMMON_2);
                    break;
                case 2:
                    Talk(SAY_SUMMON_3);
                    break;
                default:
                    break;
            }
        }

        void CastTeleport()
        {
            if (!instance || instance->GetData(DATA_UROM_PLATAFORM) > 2)
                return;

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetImmuneToAll(true);
            LeaveCombat();

            DoCast(TeleportSpells[instance->GetData(DATA_UROM_PLATAFORM)]);
        }

        void KilledUnit(Unit* /*victim*/) override
        {
            Talk(SAY_PLAYER_KILL);
        }

        void UpdateAI(uint32 uiDiff) override
        {
            if (teleportInProgress)
                return;

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (teleportTimer <= uiDiff)
            {
                teleportPosition = (*me);
                me->InterruptNonMeleeSpells(false);
                me->GetMotionMaster()->Clear();
                SetCombatMovement(false);
                me->AllowInhabit(INHABIT_AIR);
                me->SetCanFly(true);
                DoCast(SPELL_TELEPORT);
                teleportTimer = urand(30000, 35000);
            } else teleportTimer -= uiDiff;

            if (canCast)
            {
                canCast = false;
                canGoBack = true;
                Talk(SAY_ARCANE_EXPLOSION);
                Talk(EMOTE_ARCANE_EXPLOSION);
                DoCastAOE(DUNGEON_MODE(SPELL_EMPOWERED_ARCANE_EXPLOSION, SPELL_EMPOWERED_ARCANE_EXPLOSION_2));
            }

            if (canGoBack && !me->IsNonMeleeSpellCast(false))
            {
                if (teleportBackTimer <= uiDiff)
                {
                    me->SetCanFly(false);
                    me->ForbidInhabit(INHABIT_AIR);
                    SetCombatMovement(true);
                    if (Unit* victim = me->GetVictim())
                        me->GetMotionMaster()->MoveChase(victim, 0, 0);

                    Position& pPos = teleportPosition;
                    me->NearTeleportTo(pPos.GetPositionX(), pPos.GetPositionY(), pPos.GetPositionZ(), pPos.GetOrientation());

                    canCast = false;
                    canGoBack = false;
                    teleportBackTimer = 2000;
                }
                else 
                    teleportBackTimer -= uiDiff;
            }

            if (!canGoBack && !me->IsNonMeleeSpellCast(false, true, true))
            {
                if (frostBombTimer <= uiDiff)
                {
                    DoCastVictim(SPELL_FROSTBOMB);
                    frostBombTimer = urand(5000, 8000);
                } else frostBombTimer -= uiDiff;

                if (timeBombTimer <= uiDiff)
                {
                    if (Unit* unit = SelectTarget(SELECT_TARGET_RANDOM))
                        DoCast(unit, DUNGEON_MODE(SPELL_TIME_BOMB, SPELL_TIME_BOMB_2));

                    timeBombTimer = urand(20000, 25000);
                } else timeBombTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer) override
        {
            if (canCast || canGoBack)
                me->NearTeleportTo(deathPortPosition);

            BossAI::JustDied(killer);
            Talk(SAY_DEATH);
            DoCastSelf(SPELL_DEATH_SPELL, true); // we cast the spell as triggered or the summon effect does not occur
        }

        void LeaveCombat()
        {
            instance->SetBossState(DATA_UROM_EVENT, SPECIAL);
            me->RemoveAllAuras();
            me->CombatStop(false);
            me->GetThreatManager().ClearAllThreat();
        }

        void SpellHit(Unit* /*pCaster*/, const SpellInfo* pSpell) override
        {
            switch (pSpell->Id)
            {
                case SPELL_SUMMON_MENAGERIE:
                case SPELL_SUMMON_MENAGERIE_2:
                case SPELL_SUMMON_MENAGERIE_3:
                {
                    Position pos;
                    switch (pSpell->Id)
                    {
                        case SPELL_SUMMON_MENAGERIE:
                            pos.Relocate(968.66f, 1042.53f, 527.32f, 0.077f);
                            break;
                        case SPELL_SUMMON_MENAGERIE_2:
                            pos.Relocate(1164.02f, 1170.85f, 527.321f, 3.66f);
                            break;
                        case SPELL_SUMMON_MENAGERIE_3:
                            pos.Relocate(1118.31f, 1080.377f, 508.361f, 4.25f);
                            break;
                    }
                    me->SetHomePosition(pos);
                    LeaveCombat();
                    teleportInProgress = false;

                    if (pSpell->Id == SPELL_SUMMON_MENAGERIE_3)
                        me->SetReactState(REACT_AGGRESSIVE);

                    if (delayedActionAfterTeleport)
                    {
                        delayedActionAfterTeleport = false;
                        me->RemoveAurasDueToSpell(SPELL_ARCANE_SHIELD);
                        me->CastSpell(me, SPELL_EVOCATE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->SetImmuneToAll(false);
                    }
                    else
                        DoCastSelf(SPELL_ARCANE_SHIELD, true);
                    break;
                }
                case SPELL_TELEPORT:
                    canCast = true;
                    me->SetDisableGravity(true, true);
                    break;
                default:
                    break;
            }
        }
        private:
            float x, y;

            bool canCast;
            bool canGoBack;

            uint8 group[3];
            Position teleportPosition;

            uint8 summonCounter;
            bool teleportInProgress;
            bool delayedActionAfterTeleport;

            uint32 teleportBackTimer;
            uint32 teleportTimer;
            uint32 frostBombTimer;
            uint32 timeBombTimer;
    };
};

class spell_urom_frost_bomb : public SpellScriptLoader
{
    public:
        spell_urom_frost_bomb() : SpellScriptLoader("spell_urom_frost_bomb") { }

        class spell_urom_frost_bomb_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_urom_frost_bomb_AuraScript);

        public:
            spell_urom_frost_bomb_AuraScript() : AuraScript() { }


            void HandlePeriodic(AuraEffect const* /*aurEff*/)
            {
                if (Unit* caster = GetCaster())
                    if (caster->GetMap()->GetDifficulty() == DUNGEON_DIFFICULTY_HEROIC)
                        caster->CastSpell(GetTarget(), SPELL_FROST_BUFFET, true);
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_urom_frost_bomb_AuraScript::HandlePeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_urom_frost_bomb_AuraScript();
        }
};

class spell_urom_time_bomb_damage : public SpellScriptLoader
{
    public:
        spell_urom_time_bomb_damage() : SpellScriptLoader("spell_urom_time_bomb_damage") { }

        class spell_urom_time_bomb_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_urom_time_bomb_damage_SpellScript);

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if([&](WorldObject* tar) { return tar == GetCaster(); });
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_urom_time_bomb_damage_SpellScript::RemoveInvalidTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_urom_time_bomb_damage_SpellScript();
        }
};

void AddSC_boss_urom()
{
    new boss_urom();
    new spell_urom_frost_bomb();
    new spell_urom_time_bomb_damage();
}
