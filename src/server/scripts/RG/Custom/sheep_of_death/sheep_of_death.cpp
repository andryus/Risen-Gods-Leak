/*
 * Copyright (C) 2008-2015 Rising Gods <http://www.rising-gods.de/>
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
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "GameObjectAI.h"
#include "CombatAI.h"
#include "Group.h" 
#include "Unit.h"
#include "Player.h"
#include "WorldSession.h"
#include "sheep_of_death.h"
#include "Entities/Player/PlayerCache.hpp"

bool CheckPlayer(Player* p)
{
    if (!p || !p->IsInWorld() || !p->IsAlive())
        return false;
    
    float x = p->GetPositionX();
    float y = p->GetPositionY();
    if (x <= 1904.73f || x >= 1924.62f || y >= 1299.07f || y <= 1275.74f)
        return false;

    return true;
}

class npc_sod_phase_setter : public CreatureScript
{
    public:
        npc_sod_phase_setter() : CreatureScript("npc_sod_phase_setter") {}

        bool OnGossipHello(Player *player, Creature *creature)
        {
            player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_PHASE);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(GOSSIP_PHASE_SETTER, creature) , creature->GetGUID());
            return true;
        }
        
        bool OnGossipSelect(Player* player, Creature* /* creature */, uint32 /*uiSender*/, uint32 action)
        {
            player->PlayerTalkClass->SendCloseGossip();

            if (action == GOSSIP_ACTION_INFO_DEF+1)
            {
                player->SetPhaseMask(player->GetPhaseMask() == 1 ? 2 : 1, true);
                return true;
            }
            return false;
        }
};

class npc_sod_commander : public CreatureScript
{
    public:
        npc_sod_commander() : CreatureScript("npc_sod_commander") {}

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sod_commanderAI(creature);
        }
        
        bool OnGossipHello(Player *player, Creature *creature)
        {
            if (creature->AI() && creature->AI()->GetData(DATA_ACTIVE) == SPECIAL_FALSE)
            {
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_START, 0, GOSSIP_ACTION_INFO_DEF+1);
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_GUID, 0, GOSSIP_ACTION_INFO_DEF+2);
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_TIPS, 0, GOSSIP_ACTION_INFO_DEF+3);
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_SHOW_POINTS, 0, GOSSIP_ACTION_INFO_DEF+4);
            }

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(GOSSIP_COMMANDER_BASE, creature), creature->GetGUID());
            return true;
        }
        
        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();

            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF+1:
                    player->PlayerTalkClass->SendCloseGossip();
                    if (CreatureAI* cAI = creature->AI())
                        cAI->SetGuidData(player->GetGUID(), DATA_PLAYER_CONTROLL);
                    //start event
                    creature->AI()->SetData(DATA_START, DONE);
                    player->TeleportTo(player->GetMapId(), Positions[29].m_positionX, Positions[29].m_positionY, Positions[29].m_positionZ, Positions[29].m_orientation);
                    return true;
                case GOSSIP_ACTION_INFO_DEF+2:
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_START, 0, GOSSIP_ACTION_INFO_DEF+1);
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_TIPS, 0, GOSSIP_ACTION_INFO_DEF+3);
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_SHOW_POINTS, 0, GOSSIP_ACTION_INFO_DEF+4);
                    player->SEND_GOSSIP_MENU(player->GetGossipTextId(GOSSIP_COMMANDER_GUIDE, creature), creature->GetGUID());
                    return true;
                case GOSSIP_ACTION_INFO_DEF+3:
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_START, 0, GOSSIP_ACTION_INFO_DEF+1);
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_GUID, 0, GOSSIP_ACTION_INFO_DEF+2);
                    player->AddGossipItem(GOSSIP_ITEM_ID, GOSSIP_ITEM_SHOW_POINTS, 0, GOSSIP_ACTION_INFO_DEF+4);
                    player->SEND_GOSSIP_MENU(player->GetGossipTextId(GOSSIP_COMMANDER_TIPS, creature), creature->GetGUID());
                    return true;
                case GOSSIP_ACTION_INFO_DEF+4:
                    player->PlayerTalkClass->SendCloseGossip();
                    creature->AI()->SetGuidData(player->GetGUID(), DATA_SEND_POINTS);
                    break;
            }
            return false;
        }

        struct npc_sod_commanderAI : public ScriptedAI
        {
            npc_sod_commanderAI(Creature* creature) : ScriptedAI(creature) 
            {
            }
            
            uint32 _points;
            uint32 _liveCounter;
            uint32 _sheepFactor;
            uint32 bombCount;

            ObjectGuid liveSparks[26];
            ObjectGuid walls[9];

            uint32 spawnTimer;
            uint32 sheepFactorTimer;
            uint32 playerCheckTimer;
            bool isEventActive;

            ObjectGuid playerGUID;

            void Reset()
            {
                _points = 0;
                _liveCounter = 0;
                _sheepFactor = 1;
                bombCount = 0;

                playerGUID.Clear();

                //despawn live sparks
                for (uint32 i = 0; i < 26; i++)
                {
                    if (Creature* c = ObjectAccessor::GetCreature(*me, liveSparks[i]))
                        c->DespawnOrUnsummon();
                    
                    liveSparks[i].Clear();
                }

                //despawn walls
                for (uint32 i = 0; i < 9; i++)
                {
                    if (GameObject* go = me->GetMap()->GetGameObject(walls[i]))
                        go->DespawnOrUnsummon();

                    walls[i].Clear();
                }

                spawnTimer = 2 * IN_MILLISECONDS;
                sheepFactorTimer = 30 * IN_MILLISECONDS;
                playerCheckTimer = 5 * IN_MILLISECONDS;
                
                isEventActive = false;
                me->setActive(false);
            }

            void ReceiveEmote(Player* p, uint32 emote)
            {
                if (!isEventActive || !bombCount)
                    return;

                if (p && CheckPlayer(p) && emote == TEXT_EMOTE_SIGNAL) // /signal
                {
                    bombCount--;

                    std::ostringstream sText;
                    if (bombCount > 1)
                        sText << "Du hast jetzt noch " << uint32(bombCount) << " Bomben.";
                    else if (bombCount == 1)
                        sText << "Du hast jetzt noch eine Bombe.";
                    else
                        sText << "Du hast jetzt keine Bombe mehr.";

                    me->MonsterTextEmote(sText.str().c_str(), p, true);

                    std::list<Creature*> creatureList;
                    me->GetCreatureListWithEntryInGrid(creatureList, ENTRY_BIG_SHEEP, 100.0f);
                    me->GetCreatureListWithEntryInGrid(creatureList, ENTRY_LIL_SHEEP, 100.0f);
                    me->GetCreatureListWithEntryInGrid(creatureList, ENTRY_MID_SHEEP, 100.0f);

                    if (creatureList.empty())
                        return;

                    for (std::list<Creature*>::const_iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
                    {
                        Creature* sheep = (*itr)->ToCreature();

                        if (sheep && sheep->IsAlive())
                        {
                            sheep->CastSpell(sheep, SPELL_SHEEP_EXPLOSION, true);
                            sheep->DespawnOrUnsummon(100);
                        }
                    }
                }
            }

            uint32 GetData(uint32 data) const
            {
                if (data == DATA_ACTIVE)
                    if (isEventActive)
                        return SPECIAL_TRUE;
                
                return SPECIAL_FALSE;
            }

            void SetGuidData(ObjectGuid guid, uint32 data) override
            {
                if (!guid)
                    return;
                
                if (data == DATA_PLAYER_CONTROLL)
                    playerGUID = guid;
                
                if (!sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
                    return;

                else if (data == DATA_SEND_POINTS)
                {
                    for (uint32 i = 0; i < 3; i++)
                    {
                        if (QueryResult result = RGDatabase.PQuery("SELECT playerGUID, points FROM sheep_of_death_points ORDER BY points DESC LIMIT %u, 1", i))
                        {
                            uint32 guid;
                            uint32 points;
                            Field* fields = result->Fetch();
                            guid = fields[0].GetUInt32();
                            points = fields[1].GetUInt32();

                            std::string name = player::PlayerCache::GetName(playerGUID);

                            std::ostringstream sText;
                            sText << "Platz " << uint32(i+1) << " hat " << name << " mit " << uint32(points) << " Punkten.";
                            me->MonsterSay(sText.str().c_str(), 0, ObjectAccessor::GetPlayer(*me, playerGUID));
                        }
                    }
                    uint32 pGUIDLow;
                    if (Player* p = ObjectAccessor::GetPlayer(*me, playerGUID))
                        pGUIDLow = p->GetGUID().GetCounter();
                    else return;

                    std::ostringstream sText;
                    uint32 points = 0;
                    uint32 place = 0;
                    
                    if (pGUIDLow)
                    {
                        if (QueryResult result = RGDatabase.PQuery("SELECT `points` FROM `sheep_of_death_points` WHERE `playerGUID` = %d", pGUIDLow))
                        {
                            Field* fields = result->Fetch();
                            points = fields[0].GetUInt32();
                        }
                    }

                    if (points)
                    {
                        if (QueryResult result = RGDatabase.PQuery("SELECT COUNT(`points`) FROM `sheep_of_death_points` WHERE `points` >= %d", points))
                        {
                            Field* fields = result->Fetch();
                            place = fields[0].GetUInt32();
                        } 
                        
                        if (points && place)
                        {
                            sText << "Du hast den " << uint32(place) << ". Platz mit " << uint32(points) << " Punkten.";
                            me->MonsterSay(sText.str().c_str(), 0, ObjectAccessor::GetPlayer(*me, ObjectGuid(guid)));
                        }
                    }
                }
            }

            Player* GetPlayer() const
            {
                if (Player* p = ObjectAccessor::GetPlayer(*me, playerGUID))
                    return p;

                return NULL;
            }

            void SetData(uint32 data, uint32 type)
            {
                switch (data)
                {
                    case DATA_TOUCH:
                        //if type == sheep then decrease live else increase live (can be only spectator)
                        if (type == DONE_SPECTATOR)
                            _points += 2 * _sheepFactor;
                        UpdateLiveCount(type == DONE_SHEEP ? false : true);
                        break;
                    case DATA_SHEEP_DEATH:
                        if (type == DONE_SHEEP)
                            _points += _sheepFactor;
                        break;
                    case DATA_START:   
                    {
                        isEventActive = true;
                        me->setActive(true);

                        //set all other players to phase 1
                        std::list<Player*> plrList;
                        me->GetPlayerListInGrid(plrList, 30.f);
                        for (std::list<Player*>::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                        {
                            if (Player * p = (*itr)->ToPlayer())
                            {
                                if (!GetPlayer())
                                    return;

                                if (p->GetGUID() != GetPlayer()->GetGUID())
                                {
                                    p->SetPhaseMask(1, true);
                                }
                            }
                        }
                
                        //spawn invis walls
                        for (uint32 i = 33; i <= 38; i++)
                        {
                            if (GameObject* wall = me->SummonGameObject(GO_INVISABLE_WALL, Positions[i].m_positionX, Positions[i].m_positionY, Positions[i].m_positionZ, Positions[i].m_orientation, 
                                    0.0f, 0.0f, 0.0f, 0.0f, 24*IN_MILLISECONDS*HOUR))
                            walls[i - 30] = wall->GetGUID();
                        }
                        //spawn spear walls
                        for (uint32 i = 30; i <= 32; i++)
                        {
                            if (GameObject* wall = me->SummonGameObject(GO_SPEAR_WALL, Positions[i].m_positionX, Positions[i].m_positionY, Positions[i].m_positionZ, Positions[i].m_orientation, 
                                    0.0f, 0.0f, 0.0f, 0.0f, 24*IN_MILLISECONDS*HOUR))
                            walls[i - 30] = wall->GetGUID();
                        }
                        
                        //add 5 live on begun
                        for (uint8 i = 0; i < 5; i++)
                        {
                            UpdateLiveCount(true);
                        }
                        
                        Player* p;
                        if (GetPlayer())
                            p = GetPlayer();
                        else 
                        {
                            Reset(); 
                            return;
                        }
                        p->AddAura(SPELL_SPEED_NERF, p);
                        break;
                    }
                }
            }
            
            /* # UpdateLiveCount
            @ increase or decrease Counts of Live by 1
            @ summon or unsummon sparks for a visual view of Lives
            */
            void UpdateLiveCount(bool increase)
            {
                if (!isEventActive)
                    return;
                
                if (increase)
                {
                    _liveCounter++;
                    //can not show more than 26 sparks, becasue not more Positions exist
                    if (_liveCounter <= 26)
                        if (Creature* liveSpark = me->SummonCreature(ENTRY_LIVE_SPARK, Positions[_liveCounter], TEMPSUMMON_MANUAL_DESPAWN))
                            liveSparks[_liveCounter] = liveSpark->GetGUID();
                }
                else
                {
                    //can not show more than 26 sparks, becasue not more Positions exist
                    if (_liveCounter <= 26 && _liveCounter != 0)
                    {
                        if (Creature* c = ObjectAccessor::GetCreature(*me, liveSparks[_liveCounter]))
                            c->DespawnOrUnsummon();
                        liveSparks[_liveCounter].Clear();
                    }
                    _liveCounter--;
                    
                    if (_liveCounter <= 0)
                    {
                        Player* p;
                        if (GetPlayer())
                            p = GetPlayer();
                        else 
                        {
                            Reset(); 
                            return;
                        }
                        
                        if (!sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
                            return;

                        uint32 pGUIDLow = p->GetGUID().GetCounter();
                        std::ostringstream sText;

                        if (QueryResult result = RGDatabase.PQuery("SELECT `points` FROM `sheep_of_death_points` WHERE `playerGUID` = %d", pGUIDLow))
                        {
                            uint32 oldPoints = 0;
                            Field *fields = result->Fetch();
                            oldPoints = fields[0].GetUInt32();

                            if (oldPoints < _points)
                                RGDatabase.PExecute("REPLACE INTO `sheep_of_death_points` VALUES (%u, %u)", pGUIDLow, _points);

                            sText << "Du hast " << uint32(_points) << " Punkte erreicht, bei einem maximalen Faktor von " << uint32(_sheepFactor) <<
                                ". Dein Bester Wert war bisher " << uint32(oldPoints) << " Punkte.";
                        } 
                        else 
                        {
                            RGDatabase.PExecute("INSERT INTO `sheep_of_death_points` VALUES (%u, %u)", pGUIDLow, _points);

                            sText << "Du hast " << uint32(_points) << " Punkte erreicht.";
                        }
                        
                        me->MonsterSay(sText.str().c_str(), 0, p);

                        if (CheckPlayer(p) && p->HasAura(SPELL_SPEED_NERF))
                            p->RemoveAura(SPELL_SPEED_NERF);

                        Reset();
                    }
                }
            }

            void SummonSheepOrSpectator(uint32 count)
            {
                for (uint32 i = 0; i < count; i++)
                {
                    float x = Positions[28].m_positionX;
                    int32 iRandom = irand(-1100, 1100); //a little bit confuse, but so we have a more random of points, frand() use only first number after dot
                    float fRandom = iRandom/100;
                    float y = Positions[28].m_positionY + fRandom;
                    float z = Positions[28].m_positionZ;
                    float o = Positions[28].m_orientation;

                    Creature* creature;
                    
                    uint32 spawnTypeChance = urand(1, 100);
                    if (spawnTypeChance <= 30)
                    {
                        // 1-30 30% for lil sheep
                        creature = me->SummonCreature(ENTRY_LIL_SHEEP, x, y, z, o);
                        float speed = 1.5f + float(0.1*_sheepFactor);
                        if (speed >= MAX_SPEED)
                            speed = MAX_SPEED;
                        creature->SetSpeedRate(MOVE_RUN, speed);
                    }
                    else if (spawnTypeChance <= 60)
                    {
                        // 31- 60 30% for mid sheep
                        creature = me->SummonCreature(ENTRY_MID_SHEEP, x, y, z, o);
                        float speed = 1.2f + float(0.1*_sheepFactor);
                        if (speed >= MAX_SPEED)
                            speed = MAX_SPEED;
                        creature->SetSpeedRate(MOVE_RUN, speed);
                    }
                    else if (spawnTypeChance <= 90)
                    {
                        // 61-90 30% for big sheep
                        creature = me->SummonCreature(ENTRY_BIG_SHEEP, x, y, z, o);
                        float speed = 0.6f + float(0.1*_sheepFactor);
                        if (speed >= MAX_SPEED)
                            speed = MAX_SPEED;
                        creature->SetSpeedRate(MOVE_RUN, speed);
                    }
                    else
                    {
                        // 91-100 10% for spectator
                        creature = me->SummonCreature(ENTRY_SPECTATOR, x, y, z, o);
                        float speed = 1.0f + float(0.1*_sheepFactor);
                        if (speed >= MAX_SPEED)
                            speed = MAX_SPEED;
                        creature->SetSpeedRate(MOVE_RUN, speed);
                    }

                    //send GUID data to creature for bedder information feedback
                    creature->AI()->SetGuidData(me->GetGUID(), 0);
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!isEventActive)
                    return;
                
                //increase sheepFactor every 30 seconds
                if (sheepFactorTimer <= diff)
                {
                    _sheepFactor++;
                    bombCount++;
                    std::ostringstream sText;

                    if (bombCount > 1)
                        sText << "Der Faktor ist auf " << uint32(_sheepFactor) << " angestiegen! Du hast jetzt " << uint32(bombCount) << " Bomben.";
                    else 
                        sText << "Der Faktor ist auf " << uint32(_sheepFactor) << " angestiegen! Du hast jetzt eine Bombe.";

                    me->MonsterTextEmote(sText.str().c_str(), ObjectAccessor::GetPlayer(*me, playerGUID), true);
                    sheepFactorTimer = 30 * IN_MILLISECONDS;
                }
                else sheepFactorTimer -= diff;
                
                //check player
                if (playerCheckTimer <= diff)
                {
                    Player* p;
                    if (GetPlayer())
                        p = GetPlayer();
                    else 
                    {
                        Reset(); 
                        return;
                    }

                    if (!p->HasAura(SPELL_SPEED_NERF))
                        p->AddAura(SPELL_SPEED_NERF, p);

                    if (!CheckPlayer(p))
                        Reset();

                    playerCheckTimer = 5000;
                } else playerCheckTimer -= diff;

                //handle spawntimer and number of adds
                if (spawnTimer <= diff)
                {
                    uint32 repeat = 0; //counter for how many adds will spawn

                    //set basic chance, spawn 1 = 60%, 2 = 30% and 3 = 10%
                    float spawnTwo = SPAWN_TWO_CHANCE;
                    float spawnThree = SPAWN_THREE_CHANCE;

                    //increase chance to spawn 2 with 30 + 10% for each factor
                    spawnTwo = SPAWN_TWO_CHANCE * (1.0 + (0.1 * _sheepFactor));

                    //after 7 time increasing factor, the chance to spawn 3 adds will increase with basic 10 + 10% for each factor
                    if (spawnTwo >= 50.0f)
                        spawnThree = SPAWN_THREE_CHANCE * (1.0 + (0.1 * (_sheepFactor - 7)));
                    
                    uint32 repeatChance = urand(1, 100);
                    //first check for 3 adds
                    if (repeatChance <= uint32(spawnThree))
                        repeat = 3;
                    //than check for 2 adds
                    else if (repeatChance <= uint32(spawnTwo))
                        repeat = 2;
                    //in all other cases spawn 1 add
                    else repeat = 1;

                    //call summon sheeps or specators "repeat" times
                    SummonSheepOrSpectator(repeat);

                    //calc spawnTimer with float and save in uint
                    float time = 2000 * (1.1 - (0.1*_sheepFactor));
                    if (time <= 100.0f) //spawntime not lesser than 0.1 second
                        time = 100.0f;
                    spawnTimer = uint32(time);
                }
                else spawnTimer -= diff;
            }
    };
};

/*
sheeps and spectator script
@move a few kinds of ways
@despawn at end of bridge
    -> send information to commander
@despawn on line of sight, if player is to close to npc
    -> send information to commander
*/

class npc_sod_add : public CreatureScript
{
    public:
        npc_sod_add() : CreatureScript("npc_sod_add") { }
        
        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sod_addAI(creature);
        }

        struct npc_sod_addAI : public ScriptedAI
        {
            npc_sod_addAI(Creature* creature) : ScriptedAI(creature) {}
            
            uint32 pointId;
            ObjectGuid commanderGUID;

            void Reset()
            {
                pointId = 0;
                commanderGUID.Clear();
            }

            void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
            {
                commanderGUID = guid;
            }

            void UpdateAI(uint32 /*diff*/)
            {
                if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                {
                    if (pointId >= 3)
                    {
                        if (Creature* commander =  ObjectAccessor::GetCreature(*me, commanderGUID))
                            commander->AI()->SetData(DATA_SHEEP_DEATH, me->GetEntry() == ENTRY_SPECTATOR ? DONE_SPECTATOR : DONE_SHEEP);
                        DoCastSelf(SPELL_SHEEP_VISUAL, true);
                        me->DespawnOrUnsummon();
                        return;
                    }

                    float x = 0, y = 0, z = 0;
                
                    switch (me->GetEntry())
                    {
                        case ENTRY_BIG_SHEEP:
                        {
                            x = me->GetPositionX() - 30.0f;
                            y = me->GetPositionY();
                            z = me->GetPositionZ() - 4.0f;
                            pointId = 3;
                            break;
                        }
                        case ENTRY_LIL_SHEEP:
                        case ENTRY_SPECTATOR:
                        {
                            x = me->GetPositionX() - 30.0f;
                            int32 rand = irand(-1100, 1100);
                            float yrand = rand/100;
                            y = MID_LANE + yrand;
                            z = me->GetPositionZ() - 2.0f;
                            pointId = 3;
                            break;
                        }
                        case ENTRY_MID_SHEEP:
                        {
                            x = me->GetPositionX() - 10.0f;
                            int32 rand = irand(-1100, 1100);
                            float yrand = rand/100;
                            y = MID_LANE + yrand;
                            z = me->GetPositionZ() - 1.333f;
                            pointId++;
                            break;
                        }
                    }
                
                    me->GetMotionMaster()->MovePoint(pointId, x, y, z);
                }
            }

            void MoveInLineOfSight(Unit* target)
            {
                if (target && target->GetTypeId() == TYPEID_PLAYER)
                {
                    if (me->GetDistance2d(target) <= 1.0f)
                    {
                        if (Creature* commander =  ObjectAccessor::GetCreature(*me, commanderGUID))
                            commander->AI()->SetData(DATA_TOUCH, me->GetEntry() == ENTRY_SPECTATOR ? DONE_SPECTATOR : DONE_SHEEP);

                        if (me->GetEntry() == ENTRY_SPECTATOR)
                            target->CastSpell(target, SPELL_SPECTATOR_VISUAL, true);
                        else target->CastSpell(me, SPELL_SHEEP_VISUAL, true);

                        me->DespawnOrUnsummon();
                    }
                }
            }
        };
};

class SheepOfDeathPlayerScript : public PlayerScript
{
public:
    SheepOfDeathPlayerScript() : PlayerScript("sheep_of_death_aura_remover") {}

    void OnLogin(Player* player, bool /*firstLogin*/)
    {
        player->RemoveAura(SPELL_SPEED_NERF);
    }
};

void AddSC_custom_event_sheep_of_death()
{
    new npc_sod_phase_setter();
    new npc_sod_commander();
    new npc_sod_add();
    new SheepOfDeathPlayerScript();
}
