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
#include "ScriptedCreature.h"
#include "InstanceScript.h"
#include "drak_tharon_keep.h"

#define MAX_ENCOUNTER     4

/* Drak'Tharon Keep encounters:
0 - Trollgore
1 - Novos
2 - King Dred
3 - Tharon Ja
*/

enum Creatures
{
    NPC_TROLLGORE                               = 26630,
    NPC_NOVOS                                   = 26631,
    NPC_KING_DRED                               = 27483,
    NPC_THARON_JA                               = 26632,
    NPC_CRYSTAL_CHANNEL_TARGET                  = 26712,
    NPC_CRYSTAL_HANDLER                         = 26627
};
enum GameObjects
{
    GO_NOVOS_CRYSTAL_1                          = 189299,
    GO_NOVOS_CRYSTAL_2                          = 189300,
    GO_NOVOS_CRYSTAL_3                          = 189301,
    GO_NOVOS_CRYSTAL_4                          = 189302
};
enum Achievements
{
    ACM_CRITERIA_OH_NOVOS                       = 7361
};

class instance_drak_tharon : public InstanceMapScript
{
public:
    instance_drak_tharon() : InstanceMapScript(DrakTharonKeepScriptName, 600) { }

    struct instance_drak_tharon_InstanceScript : public InstanceScript
    {
        instance_drak_tharon_InstanceScript(Map* map) : InstanceScript(map) 
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);

            for (int i = 0; i < 3; ++i) {
                TrollgoreInvaderSummonerGuids[i].Clear();
            }

            dredAchievCounter = 0;

            trollgoreGUID.Clear();
            novosGUID.Clear();
            dredGUID.Clear();
            tharonJaGUID.Clear();

            novosCrystalGUID1.Clear();
            novosCrystalGUID2.Clear();
            novosCrystalGUID3.Clear();
            novosCrystalGUID4.Clear();

            novosSummonerGUID1.Clear();
            novosSummonerGUID2.Clear();
            novosSummonerGUID3.Clear();
            novosSummonerGUID4.Clear();
            novosSummonerGUID5.Clear();
        }

        uint8 dredAchievCounter;

        ObjectGuid trollgoreGUID;
        ObjectGuid novosGUID;
        ObjectGuid dredGUID;
        ObjectGuid tharonJaGUID;

        ObjectGuid novosCrystalGUID1;
        ObjectGuid novosCrystalGUID2;
        ObjectGuid novosCrystalGUID3;
        ObjectGuid novosCrystalGUID4;

        ObjectGuid novosSummonerGUID1;
        ObjectGuid novosSummonerGUID2;
        ObjectGuid novosSummonerGUID3;
        ObjectGuid novosSummonerGUID4;
        ObjectGuid novosSummonerGUID5;
        ObjectGuid TrollgoreInvaderSummonerGuids[3];

        std::string str_data;

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_NOVOS_CRYSTAL_1:
                    novosCrystalGUID1 = go->GetGUID();
                    go->SetGoState(GO_STATE_READY);
                    break;
                case GO_NOVOS_CRYSTAL_2:
                    novosCrystalGUID2 = go->GetGUID();
                    go->SetGoState(GO_STATE_READY);
                    break;
                case GO_NOVOS_CRYSTAL_3:
                    novosCrystalGUID3 = go->GetGUID();
                    go->SetGoState(GO_STATE_READY);
                    break;
                case GO_NOVOS_CRYSTAL_4:
                    novosCrystalGUID4 = go->GetGUID();
                    go->SetGoState(GO_STATE_READY);
                    break;
            }
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_TROLLGORE:
                    trollgoreGUID = creature->GetGUID();
                    break;
                case NPC_NOVOS:
                    novosGUID = creature->GetGUID();
                    break;
                case NPC_KING_DRED:
                    dredGUID = creature->GetGUID();
                    break;
                case NPC_THARON_JA:
                    tharonJaGUID = creature->GetGUID();
                    break;
                case NPC_WORLD_TRIGGER:
                    InitializeTrollgoreInvaderSummoner(creature);
                    break;
                case NPC_CRYSTAL_CHANNEL_TARGET:
                    InitializeNovosSummoner(creature);
                    break;
            }
        }

        void InitializeTrollgoreInvaderSummoner(Creature* creature)
        {
            float y = creature->GetPositionY();
            float z = creature->GetPositionZ();

            if (z < 50.0f)
                return;

            if (y < -650.0f && y > -660.0f)
                TrollgoreInvaderSummonerGuids[0] = creature->GetGUID();
            else if (y < -660.0f && y > -670.0f)
                TrollgoreInvaderSummonerGuids[1] = creature->GetGUID();
            else if (y < -675.0f && y > -685.0f)
                TrollgoreInvaderSummonerGuids[2] = creature->GetGUID();
        }

        void InitializeNovosSummoner(Creature* creature)
        {
            float x = creature->GetPositionX();
            float y = creature->GetPositionY();
            float z = creature->GetPositionZ();

            if (x < -374.0f && x > -379.0f && y > -820.0f && y < -815.0f && z < 60.0f && z > 58.0f)
                novosSummonerGUID1 = creature->GetGUID();
            else if (x < -379.0f && x > -385.0f && y > -820.0f && y < -815.0f && z < 60.0f && z > 58.0f)
                novosSummonerGUID2 = creature->GetGUID();
            else if (x < -374.0f && x > -385.0f && y > -827.0f && y < -820.0f && z < 60.0f && z > 58.0f)
                novosSummonerGUID3 = creature->GetGUID();
            else if (x < -338.0f && x > -344.0f && y > -727.0f && y < -721.0f && z < 30.0f && z > 26.0f)
                novosSummonerGUID4 = creature->GetGUID();
            else if (x < -415.0f && x > -419.0f && y > -723.0f && y < -719.0f && z < 30.0f && z > 26.0f)
                novosSummonerGUID5 = creature->GetGUID();
        }

        ObjectGuid GetGuidData(uint32 type) const
        {
            switch (type)
            {
                case DATA_TROLLGORE:          
                    return trollgoreGUID;
                case DATA_NOVOS:              
                    return novosGUID;
                case DATA_DRED:               
                    return dredGUID;
                case DATA_THARON_JA:          
                    return tharonJaGUID;
                case DATA_TROLLGORE_INVADER_SUMMONER_1:
                case DATA_TROLLGORE_INVADER_SUMMONER_2:
                case DATA_TROLLGORE_INVADER_SUMMONER_3:
                    return TrollgoreInvaderSummonerGuids[type - DATA_TROLLGORE_INVADER_SUMMONER_1];
                case DATA_NOVOS_CRYSTAL_1:   
                    return novosCrystalGUID1;
                case DATA_NOVOS_CRYSTAL_2:    
                    return novosCrystalGUID2;
                case DATA_NOVOS_CRYSTAL_3:    
                    return novosCrystalGUID3;
                case DATA_NOVOS_CRYSTAL_4:    
                    return novosCrystalGUID4;
                case DATA_NOVOS_SUMMONER_1:   
                    return novosSummonerGUID1;
                case DATA_NOVOS_SUMMONER_2:   
                    return novosSummonerGUID2;
                case DATA_NOVOS_SUMMONER_3:   
                    return novosSummonerGUID3;
                case DATA_NOVOS_SUMMONER_4:   
                    return novosSummonerGUID4;
                case DATA_NOVOS_SUMMONER_5:   
                    return novosSummonerGUID5;
            }

            return ObjectGuid::Empty;
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_KING_DRED_ACHIEV:
                    dredAchievCounter = data;
                    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_KING_DRED_ACHIEV:   return dredAchievCounter;
            }
            return 0;
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (unit->GetEntry() == NPC_CRYSTAL_HANDLER)
                if (novosGUID)
                    if (Creature* novos = instance->GetCreature(novosGUID))
                        novos->AI()->DoAction(ACTION_CRYSTAL_HANDLER_DIED);
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_drak_tharon_InstanceScript(map);
    }
};

enum DarkwebVictim
{
    NPC_DARKWEB_VICTIM    = 27909,
    NPC_KURZEL            = 26664,
    NPC_DRAKKARI_GUARDIAN = 26620
};

class npc_darkweb_victim : public CreatureScript
{
public:
    npc_darkweb_victim() : CreatureScript("npc_darkweb_victim") { }

    struct npc_darkweb_victimAI : public ScriptedAI
    {
        npc_darkweb_victimAI(Creature* creature) : ScriptedAI(creature) {}
       
        void Reset()
        {
            victims = 0;
        }

        void JustDied(Unit* /*killer*/)
        {
            std::list<Creature*> victimlist;
            me->GetCreatureListWithEntryInGrid(victimlist, NPC_DARKWEB_VICTIM, 50000.0f);

            victims = 0;
            for (std::list<Creature*>::const_iterator itr = victimlist.begin(); itr != victimlist.end(); ++itr)
            {
                if (Creature* tmpvictim = *itr)
                    if (tmpvictim->IsAlive())
                        victims++;
            }

            if (urand(0, victims) == 0)
            {
                if (me->FindNearestCreature(NPC_KURZEL, 50000.0f))
                    me->SummonCreature(NPC_DRAKKARI_GUARDIAN, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                else
                    me->SummonCreature(NPC_KURZEL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
            }
            else
            {
                me->SummonCreature(NPC_DRAKKARI_GUARDIAN, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
            }

            me->DespawnOrUnsummon();
        }

    private:
        uint32 victims;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_darkweb_victimAI(creature);
    }
};

Position const SpiderCave[] =
{
    {-231.13f, -724.85f, 29.63f, 6.16f},
    {-253.71f, -752.12f, 29.54f, 4.74f},
    {-296.35f, -743.95f, 29.63f, 4.35f}
};

enum DarkwebRecluse
{
    SPELL_ENCASING_WEBS   = 49704,
    SPELL_POISON_SPIT     = 49708,
    NPC_DARKWEB_HATCHLING = 26674
};

class npc_darkweb_recluse : public CreatureScript
{
public:
    npc_darkweb_recluse() : CreatureScript("npc_darkweb_recluse") { }

    struct npc_darkweb_recluseAI : public ScriptedAI
    {
        npc_darkweb_recluseAI(Creature* creature) : ScriptedAI(creature) {}
        
        void Reset()
        {
            isfleeing = false;
            addssummoned = false;
            nearestcave = 0;
            encasingwebstimer = urand(10000, 15000);
            spittimer = urand(2000, 5000);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
        {
            if (HealthBelowPct(25) && !isfleeing && !addssummoned)
            {
                isfleeing = true;
                nearestcave = 0;
                for (uint8 i = 0; i < 3; ++i)
                {
                    if(me->GetDistance(SpiderCave[i]) < me->GetDistance(SpiderCave[nearestcave]))
                        nearestcave = i;
                }
                me->GetMotionMaster()->MovePoint(1, SpiderCave[nearestcave]);
            }
        }

        void MovementInform(uint32 /*type*/, uint32 id)
        {
            if (id == 1)
            {
                for (uint8 i = 0; i < 3; ++i)
                {
                    if (Creature* hatchling = me->SummonCreature(NPC_DARKWEB_HATCHLING, SpiderCave[nearestcave]))
                        if (me->GetVictim())
                            hatchling->AI()->AttackStart(me->GetVictim());
                }
                addssummoned = true;
                isfleeing = false;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (!isfleeing)
                me->GetMotionMaster()->MoveChase(me->GetVictim());

            if (encasingwebstimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, true))
                {
                    DoCast(target, SPELL_ENCASING_WEBS);
                    encasingwebstimer = urand(10000, 15000);
                }
            }
            else
            {
                encasingwebstimer -= diff;
            }

            if (spittimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, DUNGEON_MODE(35.0f, 30.0f), true))
                {
                    DoCast(target, SPELL_POISON_SPIT);
                    spittimer = urand(5000, 10000);
                }
            }
            else
            {
                spittimer -= diff;
            }

            DoMeleeAttackIfReady();
        }

    private:
        bool isfleeing;
        bool addssummoned;
        uint32 nearestcave;
        uint32 encasingwebstimer;
        uint32 spittimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_darkweb_recluseAI(creature);
    }
};

void AddSC_instance_drak_tharon_keep()
{
    new instance_drak_tharon;
    new npc_darkweb_victim();
    new npc_darkweb_recluse();
}
