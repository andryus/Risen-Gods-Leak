/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackwing_lair.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum Events
{
    // Victor Nefarius
    EVENT_SPAWN_ADD = 1,
    EVENT_SHADOW_BOLT,
    EVENT_FEAR,
    EVENT_MIND_CONTROL,

    // Nefarian
    EVENT_SHADOWFLAME,
    //EVENT_FEAR,
    EVENT_VEILOFSHADOW,
    EVENT_CLEAVE,
    EVENT_TAILLASH,
    EVENT_CLASSCALL,

    // UBRS
    EVENT_CHAOS_1,
    EVENT_CHAOS_2,
    EVENT_PATH_2,
    EVENT_PATH_3,
    EVENT_SUCCESS_1,
    EVENT_SUCCESS_2,
    EVENT_SUCCESS_3
};

enum Says
{
    // Nefarius
    // UBRS
    SAY_CHAOS_SPELL         = 9,
    SAY_SUCCESS             = 10,
    SAY_FAILURE             = 11,
    // BWL
    SAY_GAMESBEGIN_1        = 12,
    SAY_GAMESBEGIN_2        = 13,

    // Nefarian
    SAY_RANDOM              = 0,
    SAY_RAISE_SKELETONS     = 1,
    SAY_SLAY                = 2,
    SAY_DEATH               = 3,

    SAY_MAGE                = 4,
    SAY_WARRIOR             = 5,
    SAY_DRUID               = 6,
    SAY_PRIEST              = 7,
    SAY_PALADIN             = 8,
    SAY_SHAMAN              = 9,
    SAY_WARLOCK             = 10,
    SAY_HUNTER              = 11,
    SAY_ROGUE               = 12,
    SAY_DEATH_KNIGHT        = 13
};

enum Creatures
{
    NPC_BRONZE_DRAKANOID       = 14263,
    NPC_BLUE_DRAKANOID         = 14261,
    NPC_RED_DRAKANOID          = 14264,
    NPC_GREEN_DRAKANOID        = 14262,
    NPC_BLACK_DRAKANOID        = 14265,
    NPC_CHROMATIC_DRAKANOID    = 14302,

    BONE_CONSTRUCT             = 14605,

    // UBRS
    NPC_GYTH                   = 10339
};

enum Spells
{
    // Victor Nefarius
    // UBRS Spells
    SPELL_VAELASTRASZZ_SPAWN     = 16354, // Self Cast Depawn one sec after
    SPELL_CHROMATIC_CHAOS        = 16337, // Self Cast hits 10339
    // Victor Nefarius
    // BWL Spells
    SPELL_SHADOWBOLT             = 22677,
    SPELL_SHADOWBOLT_VOLLEY      = 22665,
    SPELL_SHADOW_COMMAND         = 22667,
    SPELL_FEAR                   = 22678,

    SPELL_NEFARIANS_BARRIER      = 22663,

    // Nefarian
    SPELL_SHADOWFLAME_INITIAL    = 22992,
    SPELL_SHADOWFLAME            = 22539,
    SPELL_BELLOWINGROAR          = 22686,
    SPELL_VEILOFSHADOW           = 7068,
    SPELL_CLEAVE                 = 20691,
    SPELL_TAILLASH               = 23364,

    SPELL_WARRIOR                = 23397,
    SPELL_PALADIN                = 23418,
    SPELL_HUNTER                 = 23436,
    SPELL_ROGUE                  = 23414,
    SPELL_PRIEST                 = 23401,
    SPELL_DEATH_KNIGHT           = 53276,
    SPELL_SHAMAN                 = 23425,
    SPELL_MAGE                   = 23410,
    SPELL_WARLOCK                = 23427,
    SPELL_DRUID                  = 23398,

    SPELL_PRIEST_TRIGGERED       = 23402,
    SPELL_DEATH_KNIGHT_TRIGGERED = 64431, // not sure about this
    SPELL_SHAMAN_TRIGGERED       = 23424, // not in client dbc
    SPELL_MAGE_TRIGGERED         = 23603,
    SPELL_WARLOCK_TRIGGERED      = 23426,

    // shaman class call. casted by triggered spell
    SPELL_SHAMAN_TOTEM_1         = 23419,
    SPELL_SHAMAN_TOTEM_2         = 23423,
    SPELL_SHAMAN_TOTEM_3         = 23420,
    SPELL_SHAMAN_TOTEM_4         = 23422,
};

enum Faction
{
    FACTION_NEFARIAN    = 103
};

enum MovePoints
{
    MOVE_POINT_COMBAT   = 1
};

enum Paths
{
    NEFARIUS_PATH_2             = 1379671,
    NEFARIUS_PATH_3             = 1379672
};

enum GameObjects
{
    GO_PORTCULLIS_ACTIVE        = 164726,
    GO_PORTCULLIS_TOBOSSROOMS   = 175186
};

#define GOSSIP_ITEM_1           "I've made no mistakes."
#define GOSSIP_ITEM_2           "You have lost your mind, Nefarius. You speak in riddles."
#define GOSSIP_ITEM_3           "Please do."

Position const DrakeSpawnLoc[2] = // drakonid
{
    {-7591.151855f, -1204.051880f, 476.800476f, 3.0f},
    {-7514.598633f, -1150.448853f, 476.796570f, 3.0f}
};

Position const NefarianLoc[2] =
{
    { -7495.615723f, -1337.156738f, 507.922516f, 3.0f }, // nefarian spawn
    { -7535.456543f, -1279.562500f, 476.798706f, 3.0f }  // nefarian move
};

uint32 const Entry[5] = {NPC_BRONZE_DRAKANOID, NPC_BLUE_DRAKANOID, NPC_RED_DRAKANOID, NPC_GREEN_DRAKANOID, NPC_BLACK_DRAKANOID};

class boss_victor_nefarius : public CreatureScript
{
    public:
        boss_victor_nefarius() : CreatureScript("boss_victor_nefarius") { }
    
        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            player->PlayerTalkClass->ClearMenus();
            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF+1:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                    player->SEND_GOSSIP_MENU(7198, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF+2:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                    player->SEND_GOSSIP_MENU(7199, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF+3:
                    player->CLOSE_GOSSIP_MENU();
                    creature->AI()->Talk(SAY_GAMESBEGIN_1);
                    CAST_AI(boss_victor_nefarius::boss_victor_nefariusAI, creature->AI())->BeginEvent(player);
                    break;
            }
            return true;
        }
    
        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (InstanceScript* instance = player->GetInstanceScript())
                if (!instance->CheckRequiredBosses(BOSS_NEFARIAN, player) || instance->GetBossState(BOSS_NEFARIAN) == DONE)
                    return false;
    
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            player->SEND_GOSSIP_MENU(7134, creature->GetGUID());
            return true;
        }
    
        struct boss_victor_nefariusAI : public ScriptedAI
        {
            boss_victor_nefariusAI(Creature* creature) : ScriptedAI(creature), summons(creature) { }
    
            void Reset() override
            {
                instance = me->GetInstanceScript();
                events.Reset();
                SpawnedAdds = 0;
                me->setFaction(FACTION_FRIENDLY_TO_ALL);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->SetStandState(UNIT_STAND_STATE_SIT_LOW_CHAIR);
            }

            void JustReachedHome()
            {
                instance->SetBossState(BOSS_NEFARIAN, NOT_STARTED);
                summons.DespawnAll();
            }
    
            void BeginEvent(Player* target)
            {
                Talk(SAY_GAMESBEGIN_2);
    
                me->setFaction(FACTION_NEFARIAN);
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->SetStandState(UNIT_STAND_STATE_STAND);
                DoCastSelf(SPELL_NEFARIANS_BARRIER);

                instance->SetBossState(BOSS_NEFARIAN, IN_PROGRESS);
                DoZoneInCombat();
                AttackStart(target);

                events.ScheduleEvent(EVENT_SHADOW_BOLT,     urand(3, 10) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FEAR,           urand(10, 20) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MIND_CONTROL,   urand(30, 35) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SPAWN_ADD,                 10 * IN_MILLISECONDS);
            }

            void JustSummoned(Creature* summon) override
            {
                summon->setFaction(FACTION_NEFARIAN);

                summon->SetCorpseDelay(DAY);
                summons.Summon(summon);
                if (me->IsInCombat())
                    DoZoneInCombat(summon);
            }
    
            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
            {
                if (summon->GetEntry() != NPC_NEFARIAN)
                {
                    summon->UpdateEntry(BONE_CONSTRUCT);
                    summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    summon->SetReactState(REACT_PASSIVE);
                    summon->SetStandState(UNIT_STAND_STATE_DEAD);
                }
            }
    
            void SetData(uint32 type, uint32 data) override
            {
                if (instance && type == 1 && data == 1)
                {
                    me->StopMoving();
                    events.ScheduleEvent(EVENT_PATH_2, 9 * IN_MILLISECONDS);
                }
    
                if (instance && type == 1 && data == 2)
                {
                    events.ScheduleEvent(EVENT_SUCCESS_1, 5 * IN_MILLISECONDS);
                }
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                {
                    events.Update(diff);
    
                    while (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_PATH_2:
                                me->GetMotionMaster()->MovePath(NEFARIUS_PATH_2, false);
                                events.ScheduleEvent(EVENT_CHAOS_1, 7 * IN_MILLISECONDS);
                                break;
                            case EVENT_CHAOS_1:
                                if (Creature* gyth = me->FindNearestCreature(NPC_GYTH, 75.0f, true))
                                {
                                    me->SetFacingToObject(gyth);
                                    Talk(SAY_CHAOS_SPELL);
                                }
                                events.ScheduleEvent(EVENT_CHAOS_2, 2 * IN_MILLISECONDS);
                                break;
                            case EVENT_CHAOS_2:
                                DoCast(SPELL_CHROMATIC_CHAOS);
                                me->SetFacingTo(1.570796f);
                                break;
                            case EVENT_SUCCESS_1:
                                if (Unit* player = me->FindNearestPlayer(60.0f))
                                {
                                    me->SetInFront(player);
                                    me->SendMovementFlagUpdate();
                                    Talk(SAY_SUCCESS);
                                    if (GameObject* portcullis1 = me->FindNearestGameObject(GO_PORTCULLIS_ACTIVE, 65.0f))
                                        portcullis1->SetGoState(GO_STATE_ACTIVE);
                                    if (GameObject* portcullis2 = me->FindNearestGameObject(GO_PORTCULLIS_TOBOSSROOMS, 80.0f))
                                        portcullis2->SetGoState(GO_STATE_ACTIVE);
                                }
                                events.ScheduleEvent(EVENT_SUCCESS_2, 4 * IN_MILLISECONDS);
                                break;
                            case EVENT_SUCCESS_2:
                                DoCastSelf(SPELL_VAELASTRASZZ_SPAWN);
                                me->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                                break;
                            case EVENT_PATH_3:
                                me->GetMotionMaster()->MovePath(NEFARIUS_PATH_3, false);
                                break;
                            default:
                                break;
                        }
                    }
                    return;
                }
    
                // Only do this if we haven't spawned nefarian yet
                if (UpdateVictim() && SpawnedAdds < 40)
                {
                    events.Update(diff);
    
                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;
    
                    while (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_SHADOW_BOLT:
                                switch (urand(0, 1))
                                {
                                    case 0:
                                        DoCastVictim(SPELL_SHADOWBOLT_VOLLEY);
                                        break;
                                    case 1:
                                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40, true))
                                            DoCast(target, SPELL_SHADOWBOLT);
                                        break;
                                }
                                ResetThreatList();
                                events.ScheduleEvent(EVENT_SHADOW_BOLT, urand(3, 10) * IN_MILLISECONDS);
                                break;
                            case EVENT_FEAR:
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40, true))
                                    DoCast(target, SPELL_FEAR);
                                events.ScheduleEvent(EVENT_FEAR, urand(10, 20) * IN_MILLISECONDS);
                                break;
                            case EVENT_MIND_CONTROL:
                                DoZoneInCombat();
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40, true))
                                    DoCast(target, SPELL_SHADOW_COMMAND);
                                events.ScheduleEvent(EVENT_MIND_CONTROL, urand(30, 35) * IN_MILLISECONDS);
                                break;
                            case EVENT_SPAWN_ADD:
                                for (uint8 i = 0; i < 2; ++i)
                                {
                                    uint32 CreatureID;
                                    if (urand(0, 2) == 0)
                                        CreatureID = NPC_CHROMATIC_DRAKANOID;
                                    else
                                        CreatureID = Entry[urand(0, 4)];
                                    me->SummonCreature(CreatureID, DrakeSpawnLoc[i]);

                                    if (++SpawnedAdds >= 40)
                                    {
                                        if (Creature* nefarian = me->SummonCreature(NPC_NEFARIAN, NefarianLoc[0]))
                                        {
                                            nefarian->setActive(true);
                                            nefarian->SetCanFly(true);
                                            nefarian->AI()->DoCastAOE(SPELL_SHADOWFLAME_INITIAL);
                                            nefarian->SetHomePosition(NefarianLoc[1]);
                                            nefarian->GetMotionMaster()->MovePoint(MOVE_POINT_COMBAT, NefarianLoc[1]);
                                        }
                                        events.CancelEvent(EVENT_MIND_CONTROL);
                                        events.CancelEvent(EVENT_FEAR);
                                        events.CancelEvent(EVENT_SHADOW_BOLT);
                                        me->DespawnOrUnsummon();
                                        return;
                                    }
                                }
                                events.ScheduleEvent(EVENT_SPAWN_ADD, 6 * IN_MILLISECONDS);
                                break;
                        }
                    }
                }
            }
    
        private:
            InstanceScript* instance;
            EventMap events;
            SummonList summons;
            uint32 SpawnedAdds;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_victor_nefariusAI>(creature);
        }
};

class boss_nefarian : public CreatureScript
{
    public:
        boss_nefarian() : CreatureScript("boss_nefarian") { }
    
        struct boss_nefarianAI : public BossAI
        {
            boss_nefarianAI(Creature* creature) : BossAI(creature, BOSS_NEFARIAN) { }
    
            void Reset() override
            {
                Phase3 = false;

                //BossAI::Reset();
                me->SetCombatPulseDelay(0);
                me->ResetLootMode();
                events.Reset();
                summons.DespawnAll();
            }
    
            void JustReachedHome() override
            {
                instance->SetBossState(BOSS_NEFARIAN, FAIL);
                std::list<Creature*> constructList;
                me->GetCreatureListWithEntryInGrid(constructList, BONE_CONSTRUCT, 500.0f);
                for (std::list<Creature*>::const_iterator itr = constructList.begin(); itr != constructList.end(); ++itr)
                    (*itr)->DespawnOrUnsummon();
                me->DespawnOrUnsummon();
            }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(EVENT_SHADOWFLAME,             12 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FEAR,         urand(25, 35) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_VEILOFSHADOW, urand(25, 35) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CLEAVE,                   7 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_TAILLASH,                10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CLASSCALL,    urand(25, 35) * IN_MILLISECONDS);
                Talk(SAY_RANDOM);
            }
    
            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);
                Talk(SAY_DEATH);
            }
    
            void KilledUnit(Unit* victim) override
            {
                if (rand32() %5)
                    return;
    
                Talk(SAY_SLAY, victim);
            }
    
            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;
    
                if (id == MOVE_POINT_COMBAT)
                {
                    me->SetInCombatWithZone();
                    if (me->GetVictim())
                        AttackStart(me->GetVictim());
                }
            }
    
            void UpdateAI(uint32 diff) override
            {    
                if (!UpdateVictim())
                    return;
    
                events.Update(diff);
    
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
    
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHADOWFLAME:
                            DoCastVictim(SPELL_SHADOWFLAME);
                            events.ScheduleEvent(EVENT_SHADOWFLAME, 12 * IN_MILLISECONDS);
                            break;
                        case EVENT_FEAR:
                            DoCastAOE(SPELL_BELLOWINGROAR);
                            events.ScheduleEvent(EVENT_FEAR, urand(25, 35) * IN_MILLISECONDS);
                            break;
                        case EVENT_VEILOFSHADOW:
                            DoCastVictim(SPELL_VEILOFSHADOW);
                            events.ScheduleEvent(EVENT_VEILOFSHADOW, urand(25, 35) * IN_MILLISECONDS);
                            break;
                        case EVENT_CLEAVE:
                            DoCastVictim(SPELL_CLEAVE);
                            events.ScheduleEvent(EVENT_CLEAVE, 7 * IN_MILLISECONDS);
                            break;
                        case EVENT_TAILLASH:
                            DoCast(SPELL_TAILLASH);
                            events.ScheduleEvent(EVENT_TAILLASH, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_CLASSCALL:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                switch (target->getClass())
                                {
                                    case CLASS_MAGE:
                                        Talk(SAY_MAGE);
                                        DoCastSelf(SPELL_MAGE);
                                        break;
                                    case CLASS_WARRIOR:
                                        Talk(SAY_WARRIOR);
                                        DoCastSelf(SPELL_WARRIOR);
                                        break;
                                    case CLASS_DRUID:
                                        Talk(SAY_DRUID);
                                        DoCastSelf(SPELL_DRUID);
                                        break;
                                    case CLASS_PRIEST:
                                        Talk(SAY_PRIEST);
                                        DoCastSelf(SPELL_PRIEST);
                                        break;
                                    case CLASS_PALADIN:
                                        Talk(SAY_PALADIN);
                                        DoCastSelf(SPELL_PALADIN);
                                        break;
                                    case CLASS_SHAMAN:
                                        Talk(SAY_SHAMAN);
                                        DoCastSelf(SPELL_SHAMAN);
                                        break;
                                    case CLASS_WARLOCK:
                                        Talk(SAY_WARLOCK);
                                        DoCastSelf(SPELL_WARLOCK);
                                        break;
                                    case CLASS_HUNTER:
                                        Talk(SAY_HUNTER);
                                        DoCastSelf(SPELL_HUNTER);
                                        break;
                                    case CLASS_ROGUE:
                                        Talk(SAY_ROGUE);
                                        DoCastSelf(SPELL_ROGUE);
                                        break;
                                    case CLASS_DEATH_KNIGHT:
                                        Talk(SAY_DEATH_KNIGHT);
                                        DoCastSelf(SPELL_DEATH_KNIGHT);
                                    default:
                                        break;
                                }
                            events.ScheduleEvent(EVENT_CLASSCALL, urand(25, 35) * IN_MILLISECONDS);
                            break;
                    }
                }
    
                if (!Phase3 && HealthBelowPct(20))
                {
                    std::list<Creature*> constructList;
                    me->GetCreatureListWithEntryInGrid(constructList, BONE_CONSTRUCT, 500.0f);
                    for (auto itr = constructList.begin(); itr != constructList.end(); ++itr)
                    {
                        (*itr)->Respawn();
                        (*itr)->AI()->DoZoneInCombat();
                        (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        (*itr)->SetReactState(REACT_AGGRESSIVE);
                        (*itr)->SetStandState(UNIT_STAND_STATE_STAND);
                        (*itr)->UpdateEntry(BONE_CONSTRUCT);
                        (*itr)->setFaction(FACTION_NEFARIAN);
                    }
    
                    Phase3 = true;
                    Talk(SAY_RAISE_SKELETONS);
                }
    
                DoMeleeAttackIfReady();
            }
    
        private:
            bool Phase3;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_nefarianAI>(creature);
        }
};

class PlayerClassTargetSelector
{
public:
    PlayerClassTargetSelector(Classes classes) : _classes(classes) { }

    bool operator() (WorldObject* target)
    {
        if (!target->ToPlayer())
            return true;
        if (target->ToPlayer()->getClass() != _classes)
            return true;
        return false;
    }
private:
    Classes _classes;
};

class spell_nefarian_class_call : public SpellScriptLoader
{
public:
    spell_nefarian_class_call(const char* name, uint32 triggeredSpellId, Classes classes) : SpellScriptLoader(name),
        _triggeredSpellId(triggeredSpellId), _classes(classes) { }

    class spell_nefarian_class_call_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_nefarian_class_call_SpellScript);

    public:
        spell_nefarian_class_call_SpellScript(uint32 triggeredSpellId, Classes classes) : SpellScript(),
            _triggeredSpellId(triggeredSpellId), _classes(classes) { }

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (_triggeredSpellId == 0)
                return true;
            return sSpellMgr->GetSpellInfo(_triggeredSpellId);
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if(PlayerClassTargetSelector(_classes));
        }

        void CalculateRougeTeleportPos(SpellEffIndex /*effIndex*/)
        {
            GetCaster()->GetRandomNearPosition(_pos, 5.0f);
        }

        void HandleRougleClassCall(SpellEffIndex /*effIndex*/)
        {
            if (Unit* victim = GetCaster()->GetVictim())
                GetHitPlayer()->NearTeleportTo(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), _pos.GetOrientation());
        }

        void HandleDeathKnightClassCall(SpellEffIndex /*effIndex*/)
        {
            for (Player& player : GetCaster()->GetMap()->GetPlayers())
            {
                if (GetHitPlayer()->GetGUID() == player.GetGUID() || player.isDead())
                    continue;
                player.CastSpell(GetHitPlayer(), _triggeredSpellId, true, nullptr, nullptr, GetHitPlayer()->GetGUID());
            }
        }

        void HandleWarlockClassCall(SpellEffIndex /*effIndex*/)
        {
            GetHitPlayer()->CastSpell(GetHitPlayer(), _triggeredSpellId, true);
        }

        void Register() override
        {
            switch (_classes)
            {
                case CLASS_WARRIOR:
                    OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_nefarian_class_call_SpellScript::FilterTargets,
                        EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                    OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_nefarian_class_call_SpellScript::FilterTargets,
                        EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                    OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_nefarian_class_call_SpellScript::FilterTargets,
                        EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
                    break;
                case CLASS_DRUID:
                    OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_nefarian_class_call_SpellScript::FilterTargets,
                        EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                    OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_nefarian_class_call_SpellScript::FilterTargets,
                        EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                    break;
                default:
                    OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_nefarian_class_call_SpellScript::FilterTargets,
                        EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                    break;
            }

            switch (_classes)
            {
                case CLASS_ROGUE:
                    OnEffectLaunchTarget += SpellEffectFn(spell_nefarian_class_call_SpellScript::CalculateRougeTeleportPos, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
                    OnEffectHitTarget += SpellEffectFn(spell_nefarian_class_call_SpellScript::HandleRougleClassCall, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
                    break;
                case CLASS_PRIEST:
                case CLASS_DEATH_KNIGHT:
                    OnEffectHitTarget += SpellEffectFn(spell_nefarian_class_call_SpellScript::HandleDeathKnightClassCall, EFFECT_0, SPELL_EFFECT_DUMMY);
                    break;
                case CLASS_WARLOCK:
                    OnEffectHitTarget += SpellEffectFn(spell_nefarian_class_call_SpellScript::HandleWarlockClassCall, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
                    break;
                default: break;
            }
        }
    private:
        uint32 _triggeredSpellId;
        Classes _classes;
        Position _pos;
    };

    class spell_nefarian_class_call_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_nefarian_class_call_AuraScript);

    public:
        spell_nefarian_class_call_AuraScript(uint32 triggeredSpellId, Classes classes) : AuraScript(),
            _triggeredSpellId(triggeredSpellId), _classes(classes) { }

        void HandlePriestClassCall(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            if (!GetUnitOwner())
                return;

            PreventDefaultAction();
            GetUnitOwner()->CastSpell(eventInfo.GetProcTarget(), _triggeredSpellId, true, nullptr, aurEff, GetUnitOwner()->GetGUID());
        }

        void HandleMageClassCall(AuraEffect const* /*aurEff*/)
        {
            if (!GetUnitOwner() || !GetCaster())
                return;

            std::list<ObjectGuid> playerWithoutTiggerEffect;
            for (Player& player : GetCaster()->GetMap()->GetPlayers())
            {
                if (GetUnitOwner()->GetGUID() == player.GetGUID() || player.HasAura(_triggeredSpellId) || player.isDead() || player.IsGameMaster())
                    continue;
                playerWithoutTiggerEffect.push_back(player.GetGUID());
            }

            if (playerWithoutTiggerEffect.empty())
                return;

            if (Player* player = ObjectAccessor::GetPlayer(*GetCaster(), Trinity::Containers::SelectRandomContainerElement(playerWithoutTiggerEffect)))
                GetUnitOwner()->CastSpell(player, _triggeredSpellId, true, nullptr, nullptr, GetUnitOwner()->GetGUID());
        }

        void HandleShamanClassCall(AuraEffect const* aurEff)
        {
            if (Unit* owner = GetUnitOwner())
                owner->CastSpell(owner, _triggeredSpellId, true, nullptr, aurEff, GetCasterGUID());
        }

        void Register() override
        {
            switch (_classes)
            {
            case CLASS_PRIEST:
                OnEffectProc += AuraEffectProcFn(spell_nefarian_class_call_AuraScript::HandlePriestClassCall, EFFECT_0,
                    SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                break;
            case CLASS_SHAMAN:
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_nefarian_class_call_AuraScript::HandleShamanClassCall, EFFECT_0,
                    SPELL_AURA_PERIODIC_TRIGGER_SPELL);
                break;
            case CLASS_MAGE:
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_nefarian_class_call_AuraScript::HandleMageClassCall, EFFECT_0,
                    SPELL_AURA_PERIODIC_TRIGGER_SPELL);
                break;
            default: break;
            }
        }
    private:
        uint32 _triggeredSpellId;
        Classes _classes;
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_nefarian_class_call_SpellScript(_triggeredSpellId, _classes);
    }

    AuraScript* GetAuraScript() const override
    {
        return new spell_nefarian_class_call_AuraScript(_triggeredSpellId, _classes);
    }

private:
    uint32 _triggeredSpellId;
    Classes _classes;
};

class spell_nefarian_class_call_shaman_triggered : public SpellScriptLoader
{
public:
    spell_nefarian_class_call_shaman_triggered() : SpellScriptLoader("spell_nefarian_class_call_shaman_triggered") { }

    class spell_nefarian_class_call_shaman_triggered_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_nefarian_class_call_shaman_triggered_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return sSpellMgr->GetSpellInfo(SPELL_SHAMAN_TOTEM_1) && sSpellMgr->GetSpellInfo(SPELL_SHAMAN_TOTEM_2) &&
                sSpellMgr->GetSpellInfo(SPELL_SHAMAN_TOTEM_3) && sSpellMgr->GetSpellInfo(SPELL_SHAMAN_TOTEM_4);
        }

        void SummonCorruptedTotem()
        {
            Unit* originalCaster = GetOriginalCaster();
            GetCaster()->CastSpell(GetCaster(), RAND(SPELL_SHAMAN_TOTEM_1, SPELL_SHAMAN_TOTEM_2, SPELL_SHAMAN_TOTEM_3, SPELL_SHAMAN_TOTEM_4),
                true, nullptr, nullptr, originalCaster ? originalCaster->GetGUID() : ObjectGuid::Empty);
        }

        void Register() override
        {
            OnCast += SpellCastFn(spell_nefarian_class_call_shaman_triggered_SpellScript::SummonCorruptedTotem);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_nefarian_class_call_shaman_triggered_SpellScript();
    }
};

void AddSC_boss_nefarian()
{
    new boss_victor_nefarius();
    new boss_nefarian();
    new spell_nefarian_class_call("spell_nefarian_class_call_warrior", 0, CLASS_WARRIOR);
    new spell_nefarian_class_call("spell_nefarian_class_call_paladin", 0, CLASS_PALADIN);
    new spell_nefarian_class_call("spell_nefarian_class_call_hunter", 0, CLASS_HUNTER);
    new spell_nefarian_class_call("spell_nefarian_class_call_rogue", 0, CLASS_ROGUE);
    new spell_nefarian_class_call("spell_nefarian_class_call_priest", SPELL_PRIEST_TRIGGERED, CLASS_PRIEST);
    new spell_nefarian_class_call("spell_nefarian_class_call_death_knight", SPELL_DEATH_KNIGHT_TRIGGERED, CLASS_DEATH_KNIGHT);
    new spell_nefarian_class_call("spell_nefarian_class_call_shaman", SPELL_SHAMAN_TRIGGERED, CLASS_SHAMAN);
    new spell_nefarian_class_call("spell_nefarian_class_call_mage", SPELL_MAGE_TRIGGERED, CLASS_MAGE);
    new spell_nefarian_class_call("spell_nefarian_class_call_warlock", SPELL_WARLOCK_TRIGGERED, CLASS_WARLOCK);
    new spell_nefarian_class_call("spell_nefarian_class_call_druid", 0, CLASS_DRUID);
    new spell_nefarian_class_call_shaman_triggered();
}
