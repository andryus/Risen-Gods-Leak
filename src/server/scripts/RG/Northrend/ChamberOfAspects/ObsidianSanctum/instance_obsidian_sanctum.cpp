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

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "obsidian_sanctum.h"
#include "ScriptedCreature.h"

/* Obsidian Sanctum encounters:
0 - Sartharion
*/

class instance_obsidian_sanctum : public InstanceMapScript
{
public:
    instance_obsidian_sanctum() : InstanceMapScript("instance_obsidian_sanctum", 615) { }

    struct instance_obsidian_sanctumInstanceMapScript : public InstanceScript
    {
        instance_obsidian_sanctumInstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);

            SartharionGUID.Clear();
            TenebronGUID.Clear();
            ShadronGUID.Clear();
            VesperonGUID.Clear();

            TwilightPortalGUID.Clear();

            trashList.clear();

            TenebronPreKilled = false;
            ShadronPreKilled = false;
            VesperonPreKilled = false;
            isLesserIsMoreGroup = true;
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_SARTHARION:
                    SartharionGUID = creature->GetGUID();
                    break;
                case NPC_TENEBRON:
                    TenebronGUID = creature->GetGUID();
                    creature->setActive(true);
                    break;
                case NPC_SHADRON:
                    ShadronGUID = creature->GetGUID();
                    creature->setActive(true);
                    break;
                case NPC_VESPERON:
                    VesperonGUID = creature->GetGUID();
                    creature->setActive(true);
                    break;
                case NPC_ONYX_SANCTUM_GUARDIEN:
                case NPC_ONYX_BROOD_GENERAL:
                case NPC_ONYX_BLAZE_MISTRESS:
                case ONYX_FLY_CAPTAIN:
                    trashList.push_back(creature->GetGUID());
                    break;
                case NPC_ACOLYTE_OF_VESPERON:
                    if (Creature* vesperon = instance->GetCreature(VesperonGUID))
                        vesperon->AI()->JustSummoned(creature);
                    break;
                //case NPC_LAVA_BLAZE: // servercrash
                //    if (creature && creature->IsAlive())
                //        if (Creature* sartharion = instance->GetCreature(SartharionGUID))
                //            sartharion->AI()->JustSummoned(creature);
                //    break;
            }
        }

        void OnCreatureRemove(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_ONYX_SANCTUM_GUARDIEN:
                case NPC_ONYX_BROOD_GENERAL:
                case NPC_ONYX_BLAZE_MISTRESS:
                case ONYX_FLY_CAPTAIN:
                    trashList.remove(creature->GetGUID());
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_TWILIGHT_PORTAL:
                    go->SetUInt32Value(GAMEOBJECT_FACTION, 35); // friendly
                    if (go->GetPositionZ() < 60.0f)
                        TwilightPortalGUID = go->GetGUID();
                    break;
            }
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            if (type == BOSS_SARTHARION)
            {
                switch (state)
                {
                    case NOT_STARTED:
                        if (GameObject* twilightPortal = instance->GetGameObject(TwilightPortalGUID))
                            twilightPortal->Delete();

                        // drake respawn system
                        if (Creature* shadron = instance->GetCreature(ShadronGUID))
                            if (shadron->isDead() && !ShadronPreKilled)
                                shadron->Respawn();

                        if (Creature* tenebron = instance->GetCreature(TenebronGUID))
                            if (tenebron->isDead() && !TenebronPreKilled)
                                tenebron->Respawn();

                        if (Creature* vesperon = instance->GetCreature(VesperonGUID))
                            if (vesperon->isDead() && !VesperonPreKilled)
                                vesperon->Respawn();

                        for (std::list<ObjectGuid>::const_iterator itr = trashList.begin(); itr != trashList.end(); ++itr)
                        {
                            if (Creature* temp = instance->GetCreature((*itr)))
                                if (temp && temp->IsAlive())
                                    temp->AI()->EnterEvadeMode();
                        }

                            break;
                    case IN_PROGRESS:
                        if (GameObject* twilightPortal = instance->GetGameObject(TwilightPortalGUID))
                            twilightPortal->Delete();

                        if (instance)
                            isLesserIsMoreGroup = instance->GetPlayersCountExceptGMs() < (uint32)(instance->GetDifficulty() ? 21 : 9);

                        for (std::list<ObjectGuid>::const_iterator itr = trashList.begin(); itr != trashList.end(); ++itr)
                        {
                            if (Creature* temp = instance->GetCreature((*itr)))
                                if (temp->IsAlive())
                                    if (Creature* sartharion = instance->GetCreature(SartharionGUID))
                                    {
                                        if (auto victim = sartharion->GetVictim())
                                            temp->EngageWithTarget(victim);
                                    }
                        }
                        break;
                    case DONE:
                    {
                        if (GameObject* twilightPortal = instance->GetGameObject(TwilightPortalGUID))
                            twilightPortal->Delete();

                        ObjectGuid DragonGUID[] = { ShadronGUID, TenebronGUID, VesperonGUID };
                        for (uint8 i = 0; i < 3; ++i)
                        {
                            if (Creature* tmpDragon = instance->GetCreature(DragonGUID[i]))
                                if (tmpDragon->IsAlive())
                                    tmpDragon->Kill(tmpDragon, false);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
            return true;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_TENEBRON_EVENT:
                    if (data == DONE)
                    {
                        if (GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
                        {
                            if (Creature* sartharion = instance->GetCreature(SartharionGUID))
                                sartharion->CastSpell(sartharion, SPELL_TWILIGHT_REVENGE);
                        }
                        else
                            TenebronPreKilled = true;
                    }
                    break;
                case DATA_SHADRON_EVENT:
                    if (data == DONE)
                    {
                        if (GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
                        {
                            if (Creature* sartharion = instance->GetCreature(SartharionGUID))
                                sartharion->CastSpell(sartharion, SPELL_TWILIGHT_REVENGE);
                        }
                        else
                            ShadronPreKilled = true;
                    }
                    break;
                case DATA_VESPERON_EVENT:
                    if (data == DONE)
                    {
                        if (GetBossState(BOSS_SARTHARION) == IN_PROGRESS)
                        {
                            if (Creature* sartharion = instance->GetCreature(SartharionGUID))
                                sartharion->CastSpell(sartharion, SPELL_TWILIGHT_REVENGE);
                        }
                        else
                            VesperonPreKilled = true;
                    }
                    break;

            }
            if (instance)
                isLesserIsMoreGroup = instance->GetPlayersCountExceptGMs() < (uint32)(instance->GetDifficulty() ? 21 : 9);
        }

        uint32 GetData(uint32 type) const
        {
            switch (type)
            {

                case DATA_TENEBRON_PREFIGHT:
                    return (uint32) TenebronPreKilled;
                case DATA_SHADRON_PREFIGHT:
                    return (uint32) ShadronPreKilled;
                case DATA_VESPERON_PREFIGHT:
                    return (uint32) VesperonPreKilled;
            }
            return 0;
        }

        ObjectGuid GetGuidData(uint32 data) const override
        {
            switch (data)
            {
                case DATA_SARTHARION:
                    return SartharionGUID;
                case DATA_TENEBRON:
                    return TenebronGUID;
                case DATA_SHADRON:
                    return ShadronGUID;
                case DATA_VESPERON:
                    return VesperonGUID;
            }
            return ObjectGuid::Empty;
        }

        bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target = NULL*/, uint32 /*miscvalue1 = 0*/)
        {
            switch(criteria_id)
            {
                // Criteria for achievement 624: Less Is More (10 player)
                case 522:
                case 7189:
                case 7190:
                case 7191:
                // Criteria for achievement 1877: Less Is More (25 player)
                case 7185:
                case 7186:
                case 7187:
                case 7188:
                    return isLesserIsMoreGroup;
            }
            return false;
        }

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            data << (uint32)TenebronPreKilled << ' ' 
                 << (uint32)ShadronPreKilled << ' '
                 << (uint32)VesperonPreKilled;
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            data >> TenebronPreKilled;
            data >> ShadronPreKilled;
            data >> VesperonPreKilled;

            bool DrakeBool[] = { TenebronPreKilled, ShadronPreKilled, VesperonPreKilled };
            ObjectGuid DragonGUID[] = { ShadronGUID, TenebronGUID, VesperonGUID };
            for (uint8 i = 0; i<3; ++i)
            {
                if (DrakeBool[i])
                {
                    if (Creature* tmpDragon = instance->GetCreature(DragonGUID[i]))
                        if (tmpDragon->IsAlive())
                            tmpDragon->DespawnOrUnsummon();
                }
            }
        }

    private:
        ObjectGuid SartharionGUID;
        ObjectGuid TenebronGUID;
        ObjectGuid ShadronGUID;
        ObjectGuid VesperonGUID;

        ObjectGuid TwilightPortalGUID;

        std::list<ObjectGuid> trashList;

        bool TenebronPreKilled;
        bool ShadronPreKilled;
        bool VesperonPreKilled;

        bool isLesserIsMoreGroup;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_obsidian_sanctumInstanceMapScript(map);
    }
};

enum MistressSpells
{
    SPELL_CONJURE_FLAME_ORB                       = 57753,
    SPELL_FLAME_SHOCK                             = 39529,
    SPELL_RAIN_OF_FIRE                            = 57757,
    SPELL_FLAME_ORB_PERIODIC                      = 57750,
    SPELL_FLAME_ORB_VISUAL                        = 55928
};

enum MistressEvents
{
    EVENT_CONJURE_FLAME_ORB                       = 1,
    EVENT_FLAME_SHOCK                             = 2,
    EVENT_RAIN_OF_FIRE                            = 3
};

class npc_onyx_blaze_mistress : public CreatureScript
{
    public:
        npc_onyx_blaze_mistress() : CreatureScript("npc_onyx_blaze_mistress") { }

        struct npc_onyx_blaze_mistressAI : public ScriptedAI
        {
            npc_onyx_blaze_mistressAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_CONJURE_FLAME_ORB, urand(11, 22) * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_FLAME_SHOCK, urand(3, 5) * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_RAIN_OF_FIRE, urand(7, 9) * IN_MILLISECONDS);
                me->RemoveAllAuras();
            }

            void UpdateAI(uint32 diff)
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CONJURE_FLAME_ORB:
                            DoCast(SPELL_CONJURE_FLAME_ORB);
                            _events.ScheduleEvent(EVENT_CONJURE_FLAME_ORB, urand(22, 31) * IN_MILLISECONDS);
                            break;
                        case EVENT_FLAME_SHOCK:
                            DoCastVictim(SPELL_FLAME_SHOCK);
                            _events.ScheduleEvent(EVENT_FLAME_SHOCK, urand(15, 21) * IN_MILLISECONDS);
                            break;
                        case EVENT_RAIN_OF_FIRE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40, true))
                                DoCast(target, SPELL_RAIN_OF_FIRE);
                            _events.ScheduleEvent(EVENT_RAIN_OF_FIRE, urand(11, 14) * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_onyx_blaze_mistressAI(creature);
        }
};

class npc_onyx_blaze_mistress_flame_orb : public CreatureScript
{
    public:
        npc_onyx_blaze_mistress_flame_orb() : CreatureScript("npc_onyx_blaze_mistress_flame_orb") { }

        struct npc_onyx_blaze_mistress_flame_orbAI : public ScriptedAI
        {
            npc_onyx_blaze_mistress_flame_orbAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                me->DespawnOrUnsummon(15000);

                me->SetSpeedRate(MOVE_WALK, 0.3f);
                me->SetSpeedRate(MOVE_RUN, 0.3f);
                me->SetSpeedRate(MOVE_SWIM, 0.3f);
                me->SetSpeedRate(MOVE_FLIGHT, 0.3f);

                DoCastSelf(SPELL_FLAME_ORB_PERIODIC);
                DoCastSelf(SPELL_FLAME_ORB_VISUAL);

                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 40, true))
                    me->AI()->AttackStart(target);

                me->SetHover(true);
            }

            void UpdateAI(uint32 /*diff*/) override { }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_onyx_blaze_mistress_flame_orbAI(creature);
        }
};

void AddSC_instance_obsidian_sanctum()
{
    new instance_obsidian_sanctum();
}

//! Prevents linker complains. Trinity moved all non boss scripts to separate file
void AddSC_obsidian_sanctum()
{
    new npc_onyx_blaze_mistress();
    new npc_onyx_blaze_mistress_flame_orb();
}
