/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "ScriptedGossip.h"
#include "trial_of_the_crusader.h"
#include "Player.h"

enum eYells
{
    // Highlord Tirion Fordring - 34996
    SAY_STAGE_0_01            = 0,
    SAY_STAGE_0_02            = 1,
    SAY_STAGE_0_04            = 2,
    SAY_STAGE_0_05            = 3,
    SAY_STAGE_0_06            = 4,
    SAY_STAGE_0_WIPE          = 5,
    SAY_STAGE_1_01            = 6,
    SAY_STAGE_1_07            = 7,
    SAY_STAGE_1_08            = 8,
    SAY_STAGE_1_11            = 9,
    SAY_STAGE_2_01            = 10,
    SAY_STAGE_2_03            = 11,
    SAY_STAGE_2_06            = 12,
    SAY_STAGE_3_01            = 13,
    SAY_STAGE_3_02            = 14,
    SAY_STAGE_4_01            = 15,
    SAY_STAGE_4_03            = 16,

    // Varian Wrynn
    SAY_STAGE_0_03a           = 0,
    SAY_STAGE_1_10            = 1,
    SAY_STAGE_2_02a           = 2,
    SAY_STAGE_2_04a           = 3,
    SAY_STAGE_2_05a           = 4,
    SAY_STAGE_3_03a           = 5,

    // Garrosh
    SAY_STAGE_0_03h           = 0,
    SAY_STAGE_1_09            = 1,
    SAY_STAGE_2_02h           = 2,
    SAY_STAGE_2_04h           = 3,
    SAY_STAGE_2_05h           = 4,
    SAY_STAGE_3_03h           = 5,

    // Wilfred Fizzlebang
    SAY_STAGE_1_02            = 0,
    SAY_STAGE_1_03            = 1,
    SAY_STAGE_1_04            = 2,
    SAY_STAGE_1_06            = 3,

    // Lord Jaraxxus
    SAY_STAGE_1_05            = 0,

    //  The Lich King
    SAY_STAGE_4_02            = 0,
    SAY_STAGE_4_05            = 1,
    SAY_STAGE_4_04            = 2,

    // Highlord Tirion Fordring - 36095
    SAY_STAGE_4_06            = 0,
    SAY_STAGE_4_07            = 1
};

enum TocAnnouncerGossipItems
{
    MENU_ID                             = 60056,

    GOSSIP_ITEM_BEASTS_FIRST            = 1,
    GOSSIP_ITEM_BEASTS_SECOND           = 2,
    GOSSIP_ITEM_JARAXXUS_FIRST          = 3,
    GOSSIP_ITEM_JARAXXUS_SECOND         = 4,
    GOSSIP_ITEM_CRUSADERS_FIRST         = 5,
    GOSSIP_ITEM_CRUSADERS_SECOND        = 6,
    GOSSIP_ITEM_VALKIRIES_FIRST         = 7,
    GOSSIP_ITEM_VALKIRIES_SECOND        = 8,
    GOSSIP_ITEM_ANUBARAK_FIRST          = 9,
    GOSSIP_ITEM_ANUBARAK_SECOND         = 10,

    GOSSIP_ITEM_HERO_FIRST              = 11,
    GOSSIP_ITEM_HERO_SECOND             = 12,
    GOSSIP_ITEM_HERO_NO_ACCESS          = 13,
};

enum ToCAnnouncerGossipActions
{
    G_ACTION_BEASTS            = 1000,
    G_ACTION_JARAXXUS          = 1001,
    G_ACTION_CRUSADERS         = 1002,
    G_ACTION_VALKIRIES         = 1003,
    G_ACTION_ANUBARAK          = 1004,

    G_ACTION_HERO_FIRST        = 1005,
    G_ACTION_HERO_SECOND       = 1006,
    G_ACTION_HERO_NO_ACCESS    = 1007,

    G_ACTION_START_ENCOUNTER   = 1500,
};

enum TocAnnouncerMessages
{
    MSG_BEASTS_FIRST            = 2002023,
    MSG_BEASTS_SECOND           = 2002024,
    MSG_JARAXXUS_FIRST          = 2002025,
    MSG_JARAXXUS_SECOND         = 2002026,
    MSG_CRUSADERS_FIRST         = 2002027,
    MSG_CRUSADERS_SECOND        = 2002028,
    MSG_VALKIRIES_FIRST         = 2002029,
    MSG_VALKIRIES_SECOND        = 2002030,
    MSG_ANUBARAK_FIRST          = 2002031,
    MSG_ANUBARAK_SECOND         = 2002032,

    MSG_HERO_FIRST              = 2002033,
    MSG_HERO_SECOND             = 2002034,
    MSG_HERO_NO_ACCESS          = 2002035,

    MSG_TIRION_END_OF_TOC       = 2002036,
    MSG_TIRION_END_OF_TOC_HERO  = 2002037,
    MSG_MAGE_END_OF_TOC         = 2002038,
};

struct Messages
{
    TocAnnouncerMessages message;
    uint32 actionId;
    TocAnnouncerGossipItems item;
    uint32 encounter;
};

#define NUM_MESSAGES    5

static Messages GossipMessage[NUM_MESSAGES] =
{
//  1 - message           2 - actionId        3 - item                     4 - encounter
    {MSG_BEASTS_FIRST,    G_ACTION_BEASTS,    GOSSIP_ITEM_BEASTS_FIRST,    BOSS_BEASTS},
    {MSG_JARAXXUS_FIRST,  G_ACTION_JARAXXUS,  GOSSIP_ITEM_JARAXXUS_FIRST,  BOSS_JARAXXUS},
    {MSG_CRUSADERS_FIRST, G_ACTION_CRUSADERS, GOSSIP_ITEM_CRUSADERS_FIRST, BOSS_CRUSADERS},
    {MSG_VALKIRIES_FIRST, G_ACTION_VALKIRIES, GOSSIP_ITEM_VALKIRIES_FIRST, BOSS_VALKIRIES},
    {MSG_ANUBARAK_FIRST,  G_ACTION_ANUBARAK,  GOSSIP_ITEM_ANUBARAK_FIRST,  BOSS_LICH_KING},
};

class npc_announcer_toc10 : public CreatureScript
{
    public:
        npc_announcer_toc10() : CreatureScript("npc_announcer_toc10") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            creature->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
            InstanceScript* instance = creature->GetInstanceScript();
            if (!instance)
                return true;

            if (player->IsInCombat() || instance->IsEncounterInProgress() || instance->GetData(TYPE_EVENT))
                return true;

            if (instance->instance->IsHeroic())
            {
                if (instance->GetData(TYPE_COUNTER) == 0)
                    return true;

                bool heroEvent = false;
                if (instance->instance->Is25ManRaid() && player->HasAchieved(ACHIEVMENT_CALL_OF_THE_CRUSADE_25))
                    heroEvent = true;
                else if (!instance->instance->Is25ManRaid() && player->HasAchieved(ACHIEVMENT_CALL_OF_THE_CRUSADE_10))
                    heroEvent = true;

                if (heroEvent && instance->GetBossState(BOSS_BEASTS) != DONE)
                {
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_HERO_FIRST), GOSSIP_SENDER_MAIN, G_ACTION_HERO_FIRST);
                    player->SEND_GOSSIP_MENU(MSG_HERO_FIRST, creature->GetGUID());
                    return true;
                }
                else if (!heroEvent && instance->GetBossState(BOSS_BEASTS) != DONE)
                {
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_HERO_NO_ACCESS), GOSSIP_SENDER_MAIN, G_ACTION_HERO_NO_ACCESS);
                    player->SEND_GOSSIP_MENU(MSG_HERO_FIRST, creature->GetGUID());
                    return true;
                }
            }

            uint8 i = 0;
            for (; i < NUM_MESSAGES; ++i)
            {
                if (instance->GetBossState(GossipMessage[i].encounter) != DONE)
                {
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GossipMessage[i].item), GOSSIP_SENDER_MAIN, GossipMessage[i].actionId);
                    break;
                }
            }
            player->SEND_GOSSIP_MENU(GossipMessage[i].message, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            creature->HandleEmoteCommand(EMOTE_ONESHOT_TALK);

            switch (action)
            {
                case G_ACTION_BEASTS: // Northend Beasts
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_BEASTS_SECOND), GOSSIP_SENDER_MAIN, G_ACTION_START_ENCOUNTER);
                    player->SEND_GOSSIP_MENU(MSG_BEASTS_SECOND, creature->GetGUID());
                    break;
                case G_ACTION_JARAXXUS: // Lord Jaraxxus
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_JARAXXUS_SECOND), GOSSIP_SENDER_MAIN, G_ACTION_START_ENCOUNTER);
                    player->SEND_GOSSIP_MENU(MSG_JARAXXUS_SECOND, creature->GetGUID());
                    break;
                case G_ACTION_CRUSADERS: // Faction Champions
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_CRUSADERS_SECOND), GOSSIP_SENDER_MAIN, G_ACTION_START_ENCOUNTER);
                    player->SEND_GOSSIP_MENU(MSG_CRUSADERS_SECOND, creature->GetGUID());
                    break;
                case G_ACTION_VALKIRIES: // Twin Valkiries
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_VALKIRIES_SECOND), GOSSIP_SENDER_MAIN, G_ACTION_START_ENCOUNTER);
                    player->SEND_GOSSIP_MENU(MSG_VALKIRIES_SECOND, creature->GetGUID());
                    break;
                case G_ACTION_ANUBARAK: // Anub'arak
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_ANUBARAK_SECOND), GOSSIP_SENDER_MAIN, G_ACTION_START_ENCOUNTER);
                    player->SEND_GOSSIP_MENU(MSG_ANUBARAK_SECOND, creature->GetGUID());
                    break;

                case G_ACTION_HERO_NO_ACCESS:
                    player->SEND_GOSSIP_MENU(MSG_HERO_NO_ACCESS, creature->GetGUID());
                    break;
                case G_ACTION_HERO_FIRST:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_HERO_SECOND), GOSSIP_SENDER_MAIN, G_ACTION_HERO_SECOND);
                    player->SEND_GOSSIP_MENU(MSG_HERO_SECOND, creature->GetGUID());
                    break;
                case G_ACTION_HERO_SECOND:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(MENU_ID, GOSSIP_ITEM_BEASTS_SECOND), GOSSIP_SENDER_MAIN, G_ACTION_START_ENCOUNTER);
                    player->SEND_GOSSIP_MENU(MSG_BEASTS_SECOND, creature->GetGUID());
                    break;

                default:
                    break;
            }

            if (action != G_ACTION_START_ENCOUNTER)
                return true;

            player->CLOSE_GOSSIP_MENU();

            InstanceScript* instance = creature->GetInstanceScript();
            if (!instance)
                return true;

            if (instance->instance->IsHeroic() && instance->GetData(TYPE_COUNTER) == 0)
                return true;

            if (instance->GetBossState(BOSS_BEASTS) != DONE)
            {
                instance->SetData(TYPE_EVENT, 110);
                instance->SetData(TYPE_NORTHREND_BEASTS, NOT_STARTED);
                instance->SetBossState(BOSS_BEASTS, NOT_STARTED);
            }
            else if (instance->GetBossState(BOSS_JARAXXUS) != DONE)
            {
                // if Jaraxxus is spawned, but the raid wiped
                if (Creature* jaraxxus = ObjectAccessor::GetCreature(*player, ObjectGuid(instance->GetGuidData(NPC_JARAXXUS))))
                {
                    jaraxxus->RemoveAurasDueToSpell(SPELL_JARAXXUS_CHAINS);
                    jaraxxus->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    jaraxxus->SetReactState(REACT_DEFENSIVE);
                    jaraxxus->SetInCombatWithZone();
                }
                else
                {
                    instance->SetData(TYPE_EVENT, 1010);
                    instance->SetBossState(BOSS_JARAXXUS, NOT_STARTED);
                }
            }
            else if (instance->GetBossState(BOSS_CRUSADERS) != DONE)
            {
                if (player->GetTeam() == ALLIANCE)
                    instance->SetData(TYPE_EVENT, 3000);
                else
                    instance->SetData(TYPE_EVENT, 3001);
                instance->SetBossState(BOSS_CRUSADERS, NOT_STARTED);
            }
            else if (instance->GetBossState(BOSS_VALKIRIES) != DONE)
            {
                instance->SetData(TYPE_EVENT, 4000);
                instance->SetBossState(BOSS_VALKIRIES, NOT_STARTED);
            }
            else if (instance->GetBossState(BOSS_LICH_KING) != DONE)
            {
                instance->SetData(TYPE_EVENT, 4040);
                instance->SetBossState(BOSS_LICH_KING, NOT_STARTED);
            }
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            creature->GetMotionMaster()->MovePoint(1, ToCCommonLoc[23]);
            creature->SetSpeedRate(MOVE_RUN, 1.0f);
            return true;
        }

        struct npc_announcer_toc10AI : public ScriptedAI
        {
            npc_announcer_toc10AI(Creature* creature) : ScriptedAI(creature) { }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type != POINT_MOTION_TYPE || pointId != 1)
                    return;

                me->RemoveFromWorld();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_announcer_toc10AI(creature);
        }
};

class boss_lich_king_toc : public CreatureScript
{
    public:
        boss_lich_king_toc() : CreatureScript("boss_lich_king_toc") { }

        struct boss_lich_king_tocAI : public ScriptedAI
        {
            boss_lich_king_tocAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                _updateTimer = 0;
                me->SetReactState(REACT_PASSIVE);
                if (Creature* summoned = me->SummonCreature(NPC_TRIGGER, ToCCommonLoc[2].GetPositionX(), ToCCommonLoc[2].GetPositionY(), ToCCommonLoc[2].GetPositionZ(), 5, TEMPSUMMON_TIMED_DESPAWN, 1*MINUTE*IN_MILLISECONDS))
                {
                    summoned->CastSpell(summoned, 51807, false);
                    summoned->SetDisplayId(summoned->GetCreatureTemplate()->Modelid2);
                }
                if (_instance)
                    _instance->SetBossState(BOSS_LICH_KING, IN_PROGRESS);
                me->SetWalk(true);
            }

            void MovementInform(uint32 uiType, uint32 uiId)
            {
                if (uiType != POINT_MOTION_TYPE || !_instance)
                    return;

                switch (uiId)
                {
                    case 0:
                        _instance->SetData(TYPE_EVENT, 5030);
                        break;
                    case 1:
                        _instance->SetData(TYPE_EVENT, 5050);
                        break;
                    default:
                        break;
                }
            }

            void UpdateAI(uint32 uiDiff)
            {
                if (!_instance)
                    return;

                if (_instance->GetData(TYPE_EVENT_NPC) != NPC_LICH_KING)
                    return;

                _updateTimer = _instance->GetData(TYPE_EVENT_TIMER);
                if (_updateTimer <= uiDiff)
                {
                    switch (_instance->GetData(TYPE_EVENT))
                    {
                        case 5010:
                            Talk(SAY_STAGE_4_02);
                            _updateTimer = 6*IN_MILLISECONDS;
                            me->GetMotionMaster()->MovePoint(0, LichKingLoc[0]);
                            _instance->SetData(TYPE_EVENT, 5020);
                            break;
                        case 5030:
                            Talk(SAY_STAGE_4_04);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_TALK);
                            _updateTimer = 15*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 5040);
                            break;
                        case 5040:
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                            me->GetMotionMaster()->MovePoint(1, LichKingLoc[1]);
                            _updateTimer = 1*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        case 5050:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 5060);
                            break;
                        case 5060:
                            Talk(SAY_STAGE_4_05);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
                            _updateTimer = 2.5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 5070);
                            break;
                        case 5070:
                            DoCastSelf(68198, false);
                            _updateTimer = 1.5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 5080);
                            break;
                        case 5080:
                            if (GameObject* go = _instance->instance->GetGameObject(ObjectGuid(_instance->GetGuidData(GO_ARGENT_COLISEUM_FLOOR))))
                            {
                                go->SetDisplayId(DISPLAYID_DESTROYED_FLOOR);
                                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED | GO_FLAG_NODESPAWN);
                                go->SetGoState(GO_STATE_ACTIVE);
                            }

                            DoCastSelf(SPELL_CORPSE_TELEPORT, false);
                            DoCastSelf(SPELL_DESTROY_FLOOR_KNOCKUP, false);

                            if (_instance)
                            {
                                _instance->SetBossState(BOSS_LICH_KING, DONE);
                                Creature* temp = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(NPC_ANUBARAK)));
                                if (!temp || !temp->IsAlive())
                                    temp = me->SummonCreature(NPC_ANUBARAK, AnubarakLoc[0].GetPositionX(), AnubarakLoc[0].GetPositionY(), AnubarakLoc[0].GetPositionZ(), 3, TEMPSUMMON_DEAD_DESPAWN);

                                _instance->SetData(TYPE_EVENT, 0);
                            }
                            me->DespawnOrUnsummon();
                            _updateTimer = 20*IN_MILLISECONDS;
                            break;
                        default:
                            break;
                    }
                }
                else
                    _updateTimer -= uiDiff;

                _instance->SetData(TYPE_EVENT_TIMER, _updateTimer);
            }

            private:
                InstanceScript* _instance;
                uint32 _updateTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_lich_king_tocAI(creature);
        }
};

class npc_fizzlebang_toc : public CreatureScript
{
    public:
        npc_fizzlebang_toc() : CreatureScript("npc_fizzlebang_toc") { }

        struct npc_fizzlebang_tocAI : public ScriptedAI
        {
            npc_fizzlebang_tocAI(Creature* creature) : ScriptedAI(creature), _summons(me)
            {
                _instance = me->GetInstanceScript();
                me->SetSpeedRate(MOVE_WALK, 1.7f);
            }

            void JustDied(Unit* killer)
            {
                Talk(SAY_STAGE_1_06, killer);
                _instance->SetData(TYPE_EVENT, 1180);
                _instance->SetData(TYPE_EVENT_TIMER, 4*IN_MILLISECONDS);
                if (Creature* temp = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(NPC_JARAXXUS))))
                    temp->AI()->DoAction(ACTION_JARAXXUS_START_FIGHT);
            }

            void Reset()
            {
                me->SetWalk(true);
                _portalGUID.Clear();
                me->GetMotionMaster()->MovePoint(1, ToCCommonLoc[10].GetPositionX(), ToCCommonLoc[10].GetPositionY()-60, ToCCommonLoc[10].GetPositionZ());
            }

            void MovementInform(uint32 uiType, uint32 uiId)
            {
                if (uiType != POINT_MOTION_TYPE)
                    return;

                switch (uiId)
                {
                    case 1:
                        me->SetWalk(false);
                        if (_instance)
                        {
                            _instance->SetData(TYPE_EVENT, 1120);
                            _instance->SetData(TYPE_EVENT_TIMER, 1*IN_MILLISECONDS);
                        }
                        break;
                    default:
                        break;
                }
            }

            void JustSummoned(Creature* summoned)
            {
                _summons.Summon(summoned);
            }

            void UpdateAI(uint32 uiDiff)
            {
                if (!_instance)
                    return;

                if (_instance->GetData(TYPE_EVENT_NPC) != NPC_FIZZLEBANG)
                    return;

                _updateTimer = _instance->GetData(TYPE_EVENT_TIMER);
                if (_updateTimer <= uiDiff)
                {
                    switch (_instance->GetData(TYPE_EVENT))
                    {
                        case 1110:
                            _instance->SetData(TYPE_EVENT, 1120);
                            _updateTimer = 4*IN_MILLISECONDS;
                            break;
                        case 1120:
                            Talk(SAY_STAGE_1_02);
                            _instance->SetData(TYPE_EVENT, 1130);
                            _updateTimer = 12*IN_MILLISECONDS;
                            break;
                        case 1130:
                            me->GetMotionMaster()->MovementExpired();
                            Talk(SAY_STAGE_1_03);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST_OMNI);
                            if (Unit* pTrigger =  me->SummonCreature(NPC_TRIGGER, ToCCommonLoc[1].GetPositionX(), ToCCommonLoc[1].GetPositionY(), ToCCommonLoc[1].GetPositionZ(), 4.69494f, TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                _triggerGUID = pTrigger->GetGUID();
                                pTrigger->SetObjectScale(2.0f);
                                pTrigger->SetDisplayId(pTrigger->ToCreature()->GetCreatureTemplate()->Modelid1);
                                pTrigger->CastSpell(pTrigger, SPELL_WILFRED_PORTAL, false);
                            }
                            _instance->SetData(TYPE_EVENT, 1132);
                            _updateTimer = 4*IN_MILLISECONDS;
                            break;
                        case 1132:
                            me->GetMotionMaster()->MovementExpired();
                            _instance->SetData(TYPE_EVENT, 1134);
                            _updateTimer = 4*IN_MILLISECONDS;
                            break;
                        case 1134:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST_OMNI);
                            if (Creature* pPortal = me->SummonCreature(NPC_WILFRED_PORTAL, ToCCommonLoc[1].GetPositionX(), ToCCommonLoc[1].GetPositionY(), ToCCommonLoc[1].GetPositionZ() + 5.0f, 4.71239f, TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                pPortal->SetReactState(REACT_PASSIVE);
                                pPortal->CastSpell(pPortal, SPELL_WILFRED_PORTAL, false);
                                _portalGUID = pPortal->GetGUID();
                            }
                            _updateTimer = 4*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 1135);
                            break;
                        case 1135:
                            _instance->SetData(TYPE_EVENT, 1140);
                            _updateTimer = 3*IN_MILLISECONDS;
                            break;
                        case 1140:
                            Talk(SAY_STAGE_1_04);
                            if (Creature* temp = me->SummonCreature(NPC_JARAXXUS, ToCCommonLoc[1].GetPositionX(), ToCCommonLoc[1].GetPositionY(), ToCCommonLoc[1].GetPositionZ(), 5.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, DESPAWN_TIME))
                            {
                                temp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                temp->SetReactState(REACT_PASSIVE);
                                temp->GetMotionMaster()->MovePoint(0, ToCCommonLoc[1].GetPositionX(), ToCCommonLoc[1].GetPositionY()-10, ToCCommonLoc[1].GetPositionZ());
                            }
                            _instance->SetData(TYPE_EVENT, 1142);
                            _updateTimer = 5*IN_MILLISECONDS;
                            break;
                        case 1142:
                            if (Creature* temp = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(NPC_JARAXXUS))))
                                temp->SetTarget(me->GetGUID());
                            if (Creature* pTrigger = ObjectAccessor::GetCreature(*me, _triggerGUID))
                                pTrigger->DespawnOrUnsummon();
                            if (Creature* pPortal = ObjectAccessor::GetCreature(*me, _portalGUID))
                                pPortal->DespawnOrUnsummon();
                            _instance->SetData(TYPE_EVENT, 1144);
                            _updateTimer = 10*IN_MILLISECONDS;
                            break;
                        case 1144:
                            if (Creature* temp = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(NPC_JARAXXUS))))
                                temp->AI()->Talk(SAY_STAGE_1_05);
                            _instance->SetData(TYPE_EVENT, 1150);
                            _updateTimer = 5.1*IN_MILLISECONDS;
                            break;
                        case 1150:
                            if (Creature* temp = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(NPC_JARAXXUS))))
                            {
                                //1-shot Fizzlebang
                                temp->CastSpell(me, 67888, false);
                                me->SetInCombatWith(temp);
                                AddThreat(me, 1000.0f, temp);
                                temp->AI()->AttackStart(me);
                            }
                            _instance->SetData(TYPE_EVENT, 1180);
                            _updateTimer = 3*IN_MILLISECONDS;
                            break;
                    }
                }
                else
                    _updateTimer -= uiDiff;
                _instance->SetData(TYPE_EVENT_TIMER, _updateTimer);
            }

            private:
                InstanceScript* _instance;
                SummonList _summons;
                uint32 _updateTimer;
                ObjectGuid _portalGUID;
                ObjectGuid _triggerGUID;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_fizzlebang_tocAI(creature);
        }
};

class npc_tirion_toc : public CreatureScript
{
    public:
        npc_tirion_toc() : CreatureScript("npc_tirion_toc") { }

        struct npc_tirion_tocAI : public ScriptedAI
        {
            npc_tirion_tocAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = me->GetInstanceScript();
            }

            void Reset() {}

            void AttackStart(Unit* /*who*/) {}

            void UpdateAI(uint32 uiDiff)
            {
                if (!_instance)
                    return;

                if (_instance->GetData(TYPE_EVENT_NPC) != NPC_TIRION)
                    return;

                _updateTimer = _instance->GetData(TYPE_EVENT_TIMER);
                if (_updateTimer <= uiDiff)
                {
                    switch (_instance->GetData(TYPE_EVENT))
                    {
                        case 110:
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_TALK);
                            Talk(SAY_STAGE_0_01);
                            _updateTimer = 25*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 120);
                            break;
                        case 140:
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_TALK);
                            Talk(SAY_STAGE_0_02);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 150);
                            break;
                        case 150:
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                            if (_instance->GetBossState(BOSS_BEASTS) != DONE)
                            {
                                _instance->DoUseDoorOrButton(ObjectGuid(_instance->GetGuidData(GO_MAIN_GATE_DOOR)));

                                if (Creature* temp = me->SummonCreature(NPC_GORMOK, ToCSpawnLoc[0].GetPositionX(), ToCSpawnLoc[0].GetPositionY(), ToCSpawnLoc[0].GetPositionZ(), 5, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30*IN_MILLISECONDS))
                                {
                                    temp->GetMotionMaster()->MovePoint(0, ToCCommonLoc[5].GetPositionX(), ToCCommonLoc[5].GetPositionY(), ToCCommonLoc[5].GetPositionZ());
                                    temp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                    temp->SetReactState(REACT_PASSIVE);
                                }
                            }
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 155);
                            break;
                        case 155:
                            // keep the raid in combat for the whole encounter, pauses included
                            me->SetInCombatWithZone();
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 160);
                            break;
                        case 200:
                            Talk(SAY_STAGE_0_04);
                            if (_instance->GetBossState(BOSS_BEASTS) != DONE)
                            {
                                _instance->DoUseDoorOrButton(ObjectGuid(_instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                                if (Creature* temp = me->SummonCreature(NPC_DREADSCALE, ToCSpawnLoc[1].GetPositionX(), ToCSpawnLoc[1].GetPositionY(), ToCSpawnLoc[1].GetPositionZ(), 5, TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    temp->GetMotionMaster()->MovePoint(0, ToCCommonLoc[8].GetPositionX(), ToCCommonLoc[8].GetPositionY(), ToCCommonLoc[8].GetPositionZ());
                                    temp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                    temp->SetReactState(REACT_PASSIVE);
                                }
                            }
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 220);
                            break;
                        case 220:
                            _instance->SetData(TYPE_EVENT, 230);
                            break;
                        case 300:
                            Talk(SAY_STAGE_0_05);
                            if (_instance->GetBossState(BOSS_BEASTS) != DONE)
                            {
                                _instance->DoUseDoorOrButton(ObjectGuid(_instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                                if (Creature* temp = me->SummonCreature(NPC_ICEHOWL, ToCSpawnLoc[0].GetPositionX(), ToCSpawnLoc[0].GetPositionY(), ToCSpawnLoc[0].GetPositionZ(), 5, TEMPSUMMON_DEAD_DESPAWN))
                                {
                                    temp->GetMotionMaster()->MovePoint(2, ToCCommonLoc[5].GetPositionX(), ToCCommonLoc[5].GetPositionY(), ToCCommonLoc[5].GetPositionZ());
                                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                    me->SetReactState(REACT_PASSIVE);
                                }
                            }
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 315);
                            break;
                        case 315:
                            _instance->SetData(TYPE_EVENT, 320);
                            break;
                        case 400:
                            Talk(SAY_STAGE_0_06);
                            me->GetThreatManager().clearReferences();
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        case 666:
                            Talk(SAY_STAGE_0_WIPE);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        case 1010:
                            Talk(SAY_STAGE_1_01);
                            _updateTimer = 7*IN_MILLISECONDS;
                            _instance->DoUseDoorOrButton(ObjectGuid(_instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                            me->SummonCreature(NPC_FIZZLEBANG, ToCSpawnLoc[0].GetPositionX(), ToCSpawnLoc[0].GetPositionY(), ToCSpawnLoc[0].GetPositionZ(), 2, TEMPSUMMON_CORPSE_TIMED_DESPAWN, DESPAWN_TIME);
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        case 1180:
                            Talk(SAY_STAGE_1_07);
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        case 2000:
                            Talk(SAY_STAGE_1_08);
                            _updateTimer = 18*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 2010);
                            break;
                        case 2030:
                            Talk(SAY_STAGE_1_11);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        case 3000:
                            Talk(SAY_STAGE_2_01);
                            _updateTimer = 12*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3050);
                            break;
                        case 3001:
                            Talk(SAY_STAGE_2_01);
                            _updateTimer = 10*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3051);
                            break;
                        case 3060:
                            Talk(SAY_STAGE_2_03);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3070);
                            break;
                        case 3061:
                            Talk(SAY_STAGE_2_03);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3071);
                            break;
                        //Summoning crusaders
                        case 3091:
                            if (Creature* pChampionController = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_CHAMPIONS_CONTROLLER))))
                                pChampionController->AI()->SetData(0, HORDE);
                            else if (Creature* pChampionController = me->SummonCreature(NPC_CHAMPIONS_CONTROLLER, ToCCommonLoc[1]))
                                pChampionController->AI()->SetData(0, HORDE);
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3092);
                            break;
                        //Summoning crusaders
                        case 3090:
                            if (Creature* pChampionController = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_CHAMPIONS_CONTROLLER))))
                                pChampionController->AI()->SetData(0, ALLIANCE);
                            else if (Creature* pChampionController = me->SummonCreature(NPC_CHAMPIONS_CONTROLLER, ToCCommonLoc[1]))
                                pChampionController->AI()->SetData(0, ALLIANCE);
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3092);
                            break;
                        case 3092:
                            if (Creature* pChampionController = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_CHAMPIONS_CONTROLLER))))
                                pChampionController->AI()->SetData(1, NOT_STARTED);
                            _instance->SetData(TYPE_EVENT, 3095);
                            break;
                        //Crusaders battle end
                        case 3100:
                            if (Creature* garroshhellscream = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_GARROSH))))
                            {
                                garroshhellscream->AI()->Talk(SAY_STAGE_2_05h);
                            }
                            Talk(SAY_STAGE_2_06);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        case 4000:
                            Talk(SAY_STAGE_3_01);
                            _updateTimer = 13*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 4010);
                            break;
                        case 4010:
                            Talk(SAY_STAGE_3_02);
                            if (Creature* temp = me->SummonCreature(NPC_LIGHTBANE, ToCSpawnLoc[1].GetPositionX(), ToCSpawnLoc[1].GetPositionY(), ToCSpawnLoc[1].GetPositionZ(), 5, TEMPSUMMON_CORPSE_TIMED_DESPAWN, DESPAWN_TIME))
                            {
                                temp->SetReactState(REACT_PASSIVE);
                                temp->SummonCreature(NPC_LIGHT_ESSENCE, TwinValkyrsLoc[0].GetPositionX(), TwinValkyrsLoc[0].GetPositionY(), TwinValkyrsLoc[0].GetPositionZ());
                                temp->SummonCreature(NPC_LIGHT_ESSENCE, TwinValkyrsLoc[1].GetPositionX(), TwinValkyrsLoc[1].GetPositionY(), TwinValkyrsLoc[1].GetPositionZ());
                            }
                            if (Creature* temp = me->SummonCreature(NPC_DARKBANE, ToCSpawnLoc[2].GetPositionX(), ToCSpawnLoc[2].GetPositionY(), ToCSpawnLoc[2].GetPositionZ(), 5, TEMPSUMMON_CORPSE_TIMED_DESPAWN, DESPAWN_TIME))
                            {
                                temp->SetReactState(REACT_PASSIVE);
                                temp->SummonCreature(NPC_DARK_ESSENCE, TwinValkyrsLoc[2].GetPositionX(), TwinValkyrsLoc[2].GetPositionY(), TwinValkyrsLoc[2].GetPositionZ());
                                temp->SummonCreature(NPC_DARK_ESSENCE, TwinValkyrsLoc[3].GetPositionX(), TwinValkyrsLoc[3].GetPositionY(), TwinValkyrsLoc[3].GetPositionZ());
                            }
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 4015);
                            break;
                        case 4015:
                            _instance->DoUseDoorOrButton(ObjectGuid(_instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                            if (Creature* temp = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_LIGHTBANE))))
                            {
                                temp->GetMotionMaster()->MovePoint(1, ToCCommonLoc[8].GetPositionX(), ToCCommonLoc[8].GetPositionY(), ToCCommonLoc[8].GetPositionZ());
                            }
                            if (Creature* temp = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_DARKBANE))))
                            {
                                temp->GetMotionMaster()->MovePoint(1, ToCCommonLoc[9].GetPositionX(), ToCCommonLoc[9].GetPositionY(), ToCCommonLoc[9].GetPositionZ());
                            }
                            _updateTimer = 6000;
                            _instance->SetData(TYPE_EVENT, 4016);
                            break;
                        case 4016:
                            if (Creature* temp = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_LIGHTBANE))))
                                temp->GetMotionMaster()->MovePoint(1, ToCCommonLoc[8].GetPositionX() + 10.0f, ToCCommonLoc[8].GetPositionY(), ToCCommonLoc[8].GetPositionZ());
                            if (Creature* temp = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_DARKBANE))))
                                temp->GetMotionMaster()->MovePoint(1, ToCCommonLoc[9].GetPositionX() - 10.0f, ToCCommonLoc[9].GetPositionY(), ToCCommonLoc[9].GetPositionZ());
                            _updateTimer = 1900;
                            _instance->SetData(TYPE_EVENT, 4017);
                            break;
                        case 4017:
                            if (Creature* temp = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_LIGHTBANE))))
                                temp->GetMotionMaster()->MovePoint(1, ToCCommonLoc[8].GetPositionX() + 10.0f, ToCCommonLoc[8].GetPositionY() - 1.0f, ToCCommonLoc[8].GetPositionZ());
                            if (Creature* temp = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_DARKBANE))))
                                temp->GetMotionMaster()->MovePoint(1, ToCCommonLoc[9].GetPositionX() - 10.0f, ToCCommonLoc[9].GetPositionY() - 1.0f, ToCCommonLoc[9].GetPositionZ());
                            _updateTimer = 2 * IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 4018);
                            break;
                        case 4018:
                            _instance->DoUseDoorOrButton(ObjectGuid(_instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        case 4040:
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 5000);
                            break;
                        case 5000:
                            Talk(SAY_STAGE_4_01);
                            _updateTimer = 10*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 5005);
                            break;
                        case 5005:
                            _updateTimer = 8*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 5010);
                            me->SummonCreature(NPC_LICH_KING, ToCCommonLoc[2].GetPositionX(), ToCCommonLoc[2].GetPositionY(), ToCCommonLoc[2].GetPositionZ(), 5);
                            break;
                        case 5020:
                            Talk(SAY_STAGE_4_03);
                            _updateTimer = 7*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        case 6000:
                            me->SummonCreature(NPC_TIRION_FORDRING, EndSpawnLoc[0].GetPositionX(), EndSpawnLoc[0].GetPositionY(), EndSpawnLoc[0].GetPositionZ());
                            me->SummonCreature(NPC_ARGENT_MAGE, EndSpawnLoc[1].GetPositionX(), EndSpawnLoc[1].GetPositionY(), EndSpawnLoc[1].GetPositionZ());
                            me->SummonGameObject(GO_PORTAL_TO_DALARAN, EndSpawnLoc[2].GetPositionX(), EndSpawnLoc[2].GetPositionY(), EndSpawnLoc[2].GetPositionZ(), 5, 0, 0, 0, 0, 0);
                            _updateTimer = 20*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 6005);
                            break;
                        case 6005:
                            if (Creature* tirionFordring = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_TIRION_FORDRING))))
                                tirionFordring->AI()->Talk(SAY_STAGE_4_06);
                            _updateTimer = 20*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 6010);
                            break;
                        case 6010:
                            if (IsHeroic())
                            {
                                if (Creature* tirionFordring = ObjectAccessor::GetCreature((*me), ObjectGuid(_instance->GetGuidData(NPC_TIRION_FORDRING))))
                                    tirionFordring->AI()->Talk(SAY_STAGE_4_07);
                                _updateTimer = 1*MINUTE*IN_MILLISECONDS;
                                _instance->SetBossState(BOSS_ANUBARAK, SPECIAL);
                                _instance->SetData(TYPE_EVENT, 6020);
                            }
                            else
                                _instance->SetData(TYPE_EVENT, 6030);
                            break;
                        case 6020:
                            me->DespawnOrUnsummon();
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 6030);
                            break;
                        default:
                            break;
                    }
                }
                else
                    _updateTimer -= uiDiff;
                _instance->SetData(TYPE_EVENT_TIMER, _updateTimer);
            }
            private:
                InstanceScript* _instance;
                uint32 _updateTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_tirion_tocAI(creature);
        }
};

class npc_garrosh_toc : public CreatureScript
{
    public:
        npc_garrosh_toc() : CreatureScript("npc_garrosh_toc") { }

        struct npc_garrosh_tocAI : public ScriptedAI
        {
            npc_garrosh_tocAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = me->GetInstanceScript();
            }

            void Reset() {}

            void AttackStart(Unit* /*who*/) {}

            void UpdateAI(uint32 uiDiff)
            {
                if (!_instance)
                    return;

                if (_instance->GetData(TYPE_EVENT_NPC) != NPC_GARROSH)
                    return;

                _updateTimer = _instance->GetData(TYPE_EVENT_TIMER);
                if (_updateTimer <= uiDiff)
                {
                    switch (_instance->GetData(TYPE_EVENT))
                    {
                        case 130:
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_TALK);
                            Talk(SAY_STAGE_0_03h);
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 132);
                            break;
                        case 132:
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 140);
                            break;
                        case 2010:
                            Talk(SAY_STAGE_1_09);
                            _updateTimer = 11*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 2020);
                            break;
                        case 3050:
                            Talk(SAY_STAGE_2_02h);
                            _updateTimer = 15*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3060);
                            break;
                        case 3070:
                            Talk(SAY_STAGE_2_04h);
                            _updateTimer = 6*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3080);
                            break;
                        case 3081:
                            //Talk(SAY_STAGE_2_05h);
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3091);
                            break;
                        case 4030:
                            Talk(SAY_STAGE_3_03h);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        default:
                            break;
                    }
                }
                else
                    _updateTimer -= uiDiff;
                _instance->SetData(TYPE_EVENT_TIMER, _updateTimer);
            }
            private:
                InstanceScript* _instance;
                uint32 _updateTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_garrosh_tocAI(creature);
        }
};

class npc_varian_toc : public CreatureScript
{
    public:
        npc_varian_toc() : CreatureScript("npc_varian_toc") { }

        struct npc_varian_tocAI : public ScriptedAI
        {
            npc_varian_tocAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = me->GetInstanceScript();
            }

            void Reset() {}

            void AttackStart(Unit* /*who*/) {}

            void UpdateAI(uint32 uiDiff)
            {
                if (!_instance)
                    return;

                if (_instance->GetData(TYPE_EVENT_NPC) != NPC_VARIAN)
                    return;

                _updateTimer = _instance->GetData(TYPE_EVENT_TIMER);
                if (_updateTimer <= uiDiff)
                {
                    switch (_instance->GetData(TYPE_EVENT))
                    {
                        case 120:
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_TALK);
                            Talk(SAY_STAGE_0_03a);
                            _updateTimer = 2*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 122);
                            break;
                        case 122:
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 130);
                            break;
                        case 2020:
                            Talk(SAY_STAGE_1_10);
                            _updateTimer = 10*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 2030);
                            break;
                        case 3051:
                            Talk(SAY_STAGE_2_02a);
                            _updateTimer = 17*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3061);
                            break;
                        case 3071:
                            Talk(SAY_STAGE_2_04a);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3081);
                            break;
                        case 3080:
                            Talk(SAY_STAGE_2_05a);
                            _updateTimer = 3*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 3090);
                            break;
                        case 4020:
                            Talk(SAY_STAGE_3_03a);
                            _updateTimer = 5*IN_MILLISECONDS;
                            _instance->SetData(TYPE_EVENT, 0);
                            break;
                        default:
                            break;
                    }
                }
                else
                    _updateTimer -= uiDiff;
                _instance->SetData(TYPE_EVENT_TIMER, _updateTimer);
            }
            private:
                InstanceScript* _instance;
                uint32 _updateTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_varian_tocAI(creature);
        }
};

class npc_tirion_end_of_toc : public CreatureScript
{
    public:
        npc_tirion_end_of_toc() : CreatureScript("npc_tirion_end_of_toc") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            player->SEND_GOSSIP_MENU(creature->GetMap()->IsHeroic() ? MSG_TIRION_END_OF_TOC_HERO : MSG_TIRION_END_OF_TOC, creature->GetGUID());
            return true;
        }
};

class npc_mage_end_of_toc : public CreatureScript
{
    public:
        npc_mage_end_of_toc() : CreatureScript("npc_mage_end_of_toc") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            player->SEND_GOSSIP_MENU(MSG_MAGE_END_OF_TOC, creature->GetGUID());
            return true;
        }
};

void AddSC_trial_of_the_crusader()
{
    new boss_lich_king_toc();
    new npc_announcer_toc10();
    new npc_fizzlebang_toc();
    new npc_tirion_toc();
    new npc_garrosh_toc();
    new npc_varian_toc();
    new npc_tirion_end_of_toc();
    new npc_mage_end_of_toc();
}
