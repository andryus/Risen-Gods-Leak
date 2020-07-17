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
#include "ScriptedGossip.h"
#include "Player.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "GameObjectAI.h"
#include "GameEventMgr.h"
#include "ArenaReplaySystem.h"

/*####
## npc_dalaran_visitor
####*/

const Position VisitorWays[12] =
{
    {5814.348f, 406.094f, 665.788f}, //right gate way
    {5800.514f, 408.191f, 665.788f}, //left gate way
    {5814.882f, 438.045f, 658.764f}, //Landing place 1
    {5825.698f, 452.426f, 658.767f}, //landing place 2
    {5804.465f, 446.407f, 658.761f}, //landing place 3
    {5820.251f, 460.150f, 658.766f}, //wp 1
    {5830.558f, 481.217f, 658.212f}, //wp 2
    { 5834.898f, 498.338f, 657.4130f }, //wp 3 and now split  
    {5831.750f, 514.859f, 657.740f}, //right way
    {5827.224f, 513.499f, 657.748f}, //left way
    {5840.401f, 533.341f, 657.657f}, //right
    {5811.073f, 522.970f, 658.026f}, //left
};

class npc_dalaran_visitor : public CreatureScript
{
public:
    npc_dalaran_visitor() : CreatureScript("npc_dalaran_visitor") { }

    struct npc_dalaran_visitorAI : public ScriptedAI
    {
        npc_dalaran_visitorAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetCanFly(true);
            me->SetSpeedRate(MOVE_FLIGHT, 2.5f);
        }

        uint8 wp;
        uint32 phaseTimer;
        Position pos;
        uint8 way;

        void Reset()
        {
            wp = 0;
            way = 0;
            phaseTimer = 2000;
            me->SetCanFly(true);
            me->SetCorpseDelay(1);
            me->SetRespawnDelay(0);
        }

        void UpdateAI(uint32 diff)
        {
            if (!me->isMoving())
            {
                switch(wp)
                {
                case 0:
                    way = urand(0,1);
                    if(way)
                        me->GetMotionMaster()->MovePoint(0, VisitorWays[0]);
                    else me->GetMotionMaster()->MovePoint(0, VisitorWays[1]);
                    wp++;
                    break;
                case 1:
                    way = urand(0,3);
                    if(way == 0)
                        me->GetMotionMaster()->MovePoint(0, VisitorWays[2]);
                    else if(way == 1)
                        me->GetMotionMaster()->MovePoint(0, VisitorWays[3]);
                    else me->GetMotionMaster()->MovePoint(0, VisitorWays[4]);
                    wp++;
                    break;
                case 2:
                    me->Dismount();
                    me->SetCanFly(false); //landing
                    me->SetWalk(false); 
                    me->SetSpeedRate(MOVE_RUN, 1.0f);
                    me->SetDisableGravity(false);
                    me->SetHover(false);
                    me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    me->SendMovementFlagUpdate();
                    me->GetMotionMaster()->MovePath(me->GetEntry() * 10, false);
                    wp++;
                    break;
                default:
                    me->DespawnOrUnsummon();
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_dalaran_visitorAI(creature);
    }
};

enum RhoninEnums
{
    ACTION_START_EVENT,

    GOSSIP_ACTION_0                     = 0,

    QUEST_ALL_IS_WELL_THAT_ENDS_WELL    = 13631,
    QUEST_ALL_IS_WELL_THAT_ENDS_WELL_H  = 13819,

    SPELL_BEAM_VISUAL_STAGE_I           = 64367,
    SPELL_BEAM_VISUAL_STAGE_II          = 64580,
    SPELL_PORTAL                        = 39952,
    SPELL_SCHOOLS_OF_ARCANE_MAGIC       = 59983,

    NPC_BRANN                           = 34044,

    NPC_BEAM_VISUAL_TRIGGER             = 1010581,
    NPC_BEAM_VISUAL_TARGET              = 1010582,

    WP_RHONIN                           = 953660,
    WP_BRANN                            = 953661,

    LIGHT_DALARAN_DEFAULT               = 1802,
    LIGHT_DALARAN_ALGALON               = 1369,

    ZONE_DALARAN                        = 4395,
};

Position const rhoninPositions[] =
{
    {5821.6875f, 836.771118f, 680.3797f, 3.631439f}, // Brann Spawning Position
    {5718.487793f, 645.770081f, 646.227356f, 2.942181f}, // Brann End Position
};

class npc_rhonin : public CreatureScript
{
public:
    npc_rhonin() : CreatureScript("npc_rhonin") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_rhoninAI (creature);
    }

    bool OnQuestReward(Player* /*player*/, Creature* creature, Quest const* quest, uint32 /*opt*/)
    {
        if(quest->GetQuestId() == QUEST_ALL_IS_WELL_THAT_ENDS_WELL || quest->GetQuestId() == QUEST_ALL_IS_WELL_THAT_ENDS_WELL_H)
           creature->AI()->DoAction(ACTION_START_EVENT);

        return true;
    }

    struct npc_rhoninAI : public ScriptedAI
    {
        npc_rhoninAI(Creature* creature) : ScriptedAI(creature), eventStep(0)
        {
            homepos = creature->GetHomePosition();
        }

        uint8 eventStep;
        uint32 eventTimer;

        Position homepos;

        ObjectGuid brannGuid;

        // Player List for Light Override Function
        std::list<ObjectGuid> PlayerInDalaranList;

        std::list<ObjectGuid> SelectSkyboxTargetsInDalaran()
        {
            PlayerInDalaranList.clear();

            for (Player& player : me->GetMap()->GetPlayers())
                if (player.GetZoneId() == ZONE_DALARAN)
                    PlayerInDalaranList.push_back(player.GetGUID());

            return PlayerInDalaranList;
        }

        void SendLightOverride(uint32 transition, bool direction)
        {
            uint32 newLight = LIGHT_DALARAN_ALGALON;
            if (!direction)
                newLight = LIGHT_DALARAN_DEFAULT;

            WorldPacket data(SMSG_OVERRIDE_LIGHT, 12);
            data << uint32(LIGHT_DALARAN_DEFAULT);
            data << uint32(newLight);
            data << uint32(transition);

            for (std::list<ObjectGuid>::iterator itr = PlayerInDalaranList.begin(); itr != PlayerInDalaranList.end(); ++itr)
                if (Player* player = ObjectAccessor::GetPlayer(*me, (*itr)))
                    player->GetSession()->SendPacket(data);
        }

        void sGossipSelect(Player* player, uint32 /*sender*/, uint32 action) override
        {
            player->PlayerTalkClass->ClearMenus();
            if (action == GOSSIP_ACTION_0)
                player->CastSpell(player, SPELL_SCHOOLS_OF_ARCANE_MAGIC, true);

            player->CLOSE_GOSSIP_MENU();
        }

        void UpdateAI(uint32 diff)
        {
            if(eventStep == 0)
                return;

            if (eventTimer <= diff)
            {
                switch(eventStep)
                {
                case 1:
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);

                    if (Creature* brann = me->SummonCreature(NPC_BRANN, rhoninPositions[0], TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        brannGuid = brann->GetGUID();
                        brann->GetMotionMaster()->MovePath(WP_BRANN, false);
                    }

                    eventTimer = 17000;
                    break;

                case 2:
                    Talk(0);
                    eventTimer = 4500;
                    break;

                case 3:
                    if (Creature* brann = ObjectAccessor::GetCreature(*me, brannGuid))
                        brann->AI()->Talk(0);
                    eventTimer = 2000;
                    break;
                case 4:
                    if (Creature* brann = ObjectAccessor::GetCreature(*me, brannGuid))
                        brann->GetMotionMaster()->MoveFollow(me, 0, 90);

                    me->GetMotionMaster()->MovePath(WP_RHONIN, false);
                    eventTimer = 30000;
                    break;
                case 5:
                    if(me->GetDistance(rhoninPositions[1]) > 5.f)
                    {
                        eventTimer = 1500;

                        return;
                    }

                    if (Creature* brann = ObjectAccessor::GetCreature(*me, brannGuid))
                        brann->GetMotionMaster()->MovePoint(0, rhoninPositions[1]);

                    break;
                case 6:
                    if(Unit* casterTrigger = me->FindNearestCreature(NPC_BEAM_VISUAL_TRIGGER,500.f))
                        if (Unit* target = me->FindNearestCreature(NPC_BEAM_VISUAL_TARGET, 500.f))
                        {
                            casterTrigger->CastSpell(target, SPELL_BEAM_VISUAL_STAGE_I, true);
                            target->SetCanFly(true);
                            target->GetMotionMaster()->MoveCharge(target->GetPositionX(), target->GetPositionY(), 750.0f);
                        }
                    eventTimer = 500;
                    break;
                case 7:
                    Talk(1);
                    if (Unit* target = me->FindNearestCreature(NPC_BEAM_VISUAL_TARGET, 500.f))
                        target->CastSpell(target, SPELL_PORTAL);
                    eventTimer = 8000;
                    break;
                case 8:
                    Talk(2);
                    eventTimer = 10300;
                    break;
                case 9:
                    Talk(3);
                    eventTimer = 11700;
                    break;
                case 10:
                    Talk(4);
                    eventTimer = 5800;
                    break;
                case 11:
                    Talk(5);
                    eventTimer = 10600;
                    break;
                case 12:
                    if(Unit* casterTrigger = me->FindNearestCreature(NPC_BEAM_VISUAL_TRIGGER,500.f))
                    {
                        casterTrigger->InterruptNonMeleeSpells(false, SPELL_BEAM_VISUAL_STAGE_II);
                        casterTrigger->RemoveAllAuras();

                        if(Unit* target = me->FindNearestCreature(NPC_BEAM_VISUAL_TARGET,500.f))
                            casterTrigger->CastSpell(target, SPELL_BEAM_VISUAL_STAGE_II, true);
                    }
                    Talk(6);
                    eventTimer = 13600;
                    SelectSkyboxTargetsInDalaran();
                    SendLightOverride(4000, true);
                    break;
                case 13:
                    Talk(7);
                    eventTimer = 10000;
                    break;
                case 14:
                    if(Unit* casterTrigger = me->FindNearestCreature(NPC_BEAM_VISUAL_TRIGGER,500.f))
                    {
                        casterTrigger->InterruptNonMeleeSpells(false, SPELL_BEAM_VISUAL_STAGE_II);
                        casterTrigger->RemoveAllAuras();
                    }
                    if (Unit* target = me->FindNearestCreature(NPC_BEAM_VISUAL_TARGET, 500.f))
                        target->RemoveAllAuras();
                    eventTimer = 25000;
                    break;
                case 15:
                    if (Creature* brann = ObjectAccessor::GetCreature(*me, brannGuid))
                        brann->DespawnOrUnsummon();
                    SendLightOverride(1500, false);
                    me->NearTeleportTo(homepos.GetPositionX(),homepos.GetPositionY(),homepos.GetPositionZ(),homepos.GetOrientation());
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
                    eventStep = 0;
                    break;
                }

                if(eventStep > 0)
                    ++eventStep;

            }else eventTimer -= diff;
        }

        void DoAction(int32 action)
        {
            if(action != ACTION_START_EVENT)
                return;

            eventTimer = 500;
            eventStep = 1;
        }
    };
};

/*******************************************************
 * RG-Custom TCG-Vendor
 *******************************************************/

 enum TCGVendor
{
    QUEST_TCG_HORDE    = 110135,
    QUEST_TCG_ALLIANCE = 110136,

    TEXT_TCG_OK        = 2002021,
    TEXT_TCG_NOT_OK    = 2002022
};

class tcg_vendor : public CreatureScript
{
    public:
        tcg_vendor() : CreatureScript("tcg_vendor")  {}

        bool OnGossipHello(Player* player, Creature* creature)
        {
            player->PrepareQuestMenu(creature->GetGUID());

            if (player->CanSeeStartQuest(sObjectMgr->GetQuestTemplate(QUEST_TCG_HORDE)) || player->CanSeeStartQuest(sObjectMgr->GetQuestTemplate(QUEST_TCG_ALLIANCE)))
                player->PlayerTalkClass->SendGossipMenu(TEXT_TCG_OK, creature->GetGUID());
            else
                player->PlayerTalkClass->SendGossipMenu(TEXT_TCG_NOT_OK, creature->GetGUID());

            return true;
        }
};

enum MinigobData
{
    SPELL_MANABONKED        = 61834,
    SPELL_TELEPORT_VISUAL   = 51347,
    SPELL_IMPROVED_BLINK    = 61995,

    EVENT_SELECT_TARGET     = 1,
    EVENT_MANABONK_TARGET   = 2,
    EVENT_BLINK             = 3,
    EVENT_DESPAWN_VISUAL    = 4,
    EVENT_DESPAWN           = 5,

    MAIL_MINIGOB_ENTRY      = 264,
    MAIL_DELIVER_DELAY_MIN  = 5*MINUTE,
    MAIL_DELIVER_DELAY_MAX  = 15*MINUTE,
};

class npc_minigob_manabonk : public CreatureScript
{
    public:
        npc_minigob_manabonk() : CreatureScript("npc_minigob_manabonk") {}

        struct npc_minigob_manabonkAI : public ScriptedAI
        {
            npc_minigob_manabonkAI(Creature* creature) : ScriptedAI(creature)
            {
                me->setActive(true);
            }

            void Reset()
            {
                PlayerGUID.Clear();
                me->setActive(true);
                me->SetVisible(false);
                events.ScheduleEvent(EVENT_SELECT_TARGET, IN_MILLISECONDS);
            }

            ObjectGuid SelectTargetInDalaran()
            {
                std::list<ObjectGuid> PlayerInDalaranList;
                PlayerInDalaranList.clear();

                for (Player& player : me->GetMap()->GetPlayers())
                    if (player.GetZoneId() == ZONE_DALARAN && !player.IsFlying() && !player.IsMounted() && !player.IsGameMaster())
                        PlayerInDalaranList.push_back(player.GetGUID());

                if (PlayerInDalaranList.empty())
                    return ObjectGuid::Empty;
                return Trinity::Containers::SelectRandomContainerElement(PlayerInDalaranList);
            }

            void SendMailToPlayer(Player* player)
            {
                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                int16 deliverDelay = irand(MAIL_DELIVER_DELAY_MIN, MAIL_DELIVER_DELAY_MAX);
                MailDraft(MAIL_MINIGOB_ENTRY, true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, me->GetEntry()), MAIL_CHECK_MASK_NONE, deliverDelay);
                CharacterDatabase.CommitTransaction(trans);
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SELECT_TARGET:
                            me->SetVisible(true);
                            DoCastSelf(SPELL_TELEPORT_VISUAL);
                            PlayerGUID = SelectTargetInDalaran();
                            if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                                me->NearTeleportTo(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 0.0f);
                            events.ScheduleEvent(EVENT_MANABONK_TARGET, 1);
                            break;
                        case EVENT_MANABONK_TARGET:
                            if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                            {
                                DoCast(player, SPELL_MANABONKED);
                                SendMailToPlayer(player);
                            }
                            events.ScheduleEvent(EVENT_BLINK, 3*IN_MILLISECONDS);
                            break;
                        case EVENT_BLINK:
                        {
                            DoCastSelf(SPELL_IMPROVED_BLINK);
                            Position pos;
                            me->GetRandomNearPosition(pos, (urand(15,40)));
                            me->GetMotionMaster()->MovePoint(0, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
                            events.ScheduleEvent(EVENT_DESPAWN, 3*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_DESPAWN_VISUAL, 2.5*IN_MILLISECONDS);
                            break;
                        }
                        case EVENT_DESPAWN_VISUAL:
                            DoCastSelf(SPELL_TELEPORT_VISUAL);
                            break;
                        case EVENT_DESPAWN:
                            me->DespawnOrUnsummon();
                            break;
                        default:
                            break;
                    }
                }
            }

        private:
            EventMap events;
            ObjectGuid PlayerGUID;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_minigob_manabonkAI(creature);
    }
};

/*######
## npc_archmage_landalock
######*/

enum QuestsAndNPCs
{
    NPC_ARCHMAGE_LANDALOCK                  = 20735,

    QUEST_SARTHARION_MUST_DIE               = 24579,
    QUEST_ANUBREKHAN_MUST_DIE               = 24580,
    QUEST_NOTH_THE_PLAGUEBINGER_MUST_DIE    = 24581,
    QUEST_INSTRUCTOR_RAZUVIOUS_MUST_DIE     = 24582,
    QUEST_PATCHWERK_MUST_DIE                = 24583,
    QUEST_MALYGOS_MUST_DIE                  = 24584,
    QUEST_FLAME_LEVIATHAN_MUST_DIE          = 24585,
    QUEST_RAZORSCALE_MUST_DIE               = 24586,
    QUEST_IGNIS_THE_FURNACE_MASTER_MUST_DIE = 24587,
    QUEST_XT_002_DECONSTRUCTOR_MUST_DIE     = 24588,
    QUEST_LORD_JARAXXUS_MUST_DIE            = 24589,
    QUEST_LORD_MARROWGAR_MUST_DIE           = 24590,

    NPC_SARTHARION_IMAGE                    = 37849,
    NPC_ANUBREKHAN_IMAGE                    = 37850,
    NPC_NOTH_THE_PLAGUEBINGER_IMAGE         = 37851,
    NPC_INSTRUCTOR_RAZUVIOUS_IMAGE          = 37853,
    NPC_PATCHWERK_IMAGE                     = 37854,
    NPC_MALYGOS_IMAGE                       = 37855,
    NPC_FLAME_LEVIATHAN_IMAGE               = 37856,
    NPC_RAZORSCALE_IMAGE                    = 37858,
    NPC_IGNIS_THE_FURNACE_MASTER_IMAGE      = 37859,
    NPC_XT_002_DECONSTRUCTOR_IMAGE          = 37861,
    NPC_LORD_JARAXXUS_IMAGE                 = 37862,
    NPC_LORD_MARROWGAR_IMAGE                = 37864,

    // ------------------------------------------ //

    NPC_WIND_TRADER_ZHAREEM                 = 24369,

    QUEST_NAZAN                             = 11354,
    QUEST_KELIDAN                           = 11362,
    QUEST_BLADEFIST                         = 11363,
    QUEST_QUAGMIRRAN                        = 11368,
    QUEST_BLACK_STALKER                     = 11369,
    QUEST_WARLORD                           = 11370,
    QUEST_IKISS                             = 11372,
    QUEST_SHAFFAR                           = 11373,
    QUEST_EXARCH                            = 11374,
    QUEST_MURMUR                            = 11375,
    QUEST_EPOCH_HUNTER                      = 11378,
    QUEST_AEONUS                            = 11382,
    QUEST_WARP_SPLINTER                     = 11384,
    QUEST_PATHALEON                         = 11386,
    QUEST_SKYRISS                           = 11388,
    QUEST_KAELTHAS                          = 11499,

    NPC_NAZAN                               = 24410,
    NPC_KELIDAN                             = 24413,
    NPC_BLADEFIST                           = 24414,
    NPC_QUAGMIRRAN                          = 24419,
    NPC_BLACK_STALKER                       = 24420,
    NPC_WARLORD                             = 24421,
    NPC_IKISS                               = 24422,
    NPC_SHAFFAR                             = 24423,
    NPC_EXARCH                              = 24424,
    NPC_MURMUR                              = 24425,
    NPC_EPOCH_HUNTER                        = 24427,
    NPC_AEONUS                              = 24428,
    NPC_WARP_SPLINTER                       = 24431,
    NPC_PATHALEON                           = 24433,
    NPC_SKYRISS                             = 24435,
    NPC_KAELTHAS                            = 24855,

    // ------------------------------------------ //

    NPC_NETHER_STALKER_MAHDUUN              = 24370,

    QUEST_CENTURIONS                        = 11364,
    QUEST_MYRMIDONS                         = 11371,
    QUEST_INSTRUCTORS                       = 11376,
    QUEST_LORDS                             = 11383,
    QUEST_CHANNELERS                        = 11385,
    QUEST_DESTROYERS                        = 11387,
    QUEST_SENTINELS                         = 11389,
    QUEST_TORMENT                           = 11500,

    NPC_CENTURIONS                          = 24411,
    NPC_MYRMIDONS                           = 24415,
    NPC_INSTRUCTORS                         = 24426,
    NPC_LORDS                               = 24429,
    NPC_CHANNELERS                          = 24430,
    NPC_DESTROYERS                          = 24432,
    NPC_SENTINELS                           = 24434,
    NPC_TORMENT                             = 24854,
};

std::map<uint32, uint32> const QuestAndNPCData =
{
    { QUEST_SARTHARION_MUST_DIE,                NPC_SARTHARION_IMAGE },
    { QUEST_ANUBREKHAN_MUST_DIE,                NPC_ANUBREKHAN_IMAGE },
    { QUEST_NOTH_THE_PLAGUEBINGER_MUST_DIE,     NPC_NOTH_THE_PLAGUEBINGER_IMAGE },
    { QUEST_INSTRUCTOR_RAZUVIOUS_MUST_DIE,      NPC_INSTRUCTOR_RAZUVIOUS_IMAGE },
    { QUEST_PATCHWERK_MUST_DIE,                 NPC_PATCHWERK_IMAGE },
    { QUEST_MALYGOS_MUST_DIE,                   NPC_MALYGOS_IMAGE },
    { QUEST_FLAME_LEVIATHAN_MUST_DIE,           NPC_FLAME_LEVIATHAN_IMAGE },
    { QUEST_RAZORSCALE_MUST_DIE,                NPC_RAZORSCALE_IMAGE },
    { QUEST_IGNIS_THE_FURNACE_MASTER_MUST_DIE,  NPC_IGNIS_THE_FURNACE_MASTER_IMAGE },
    { QUEST_XT_002_DECONSTRUCTOR_MUST_DIE,      NPC_XT_002_DECONSTRUCTOR_IMAGE },
    { QUEST_LORD_JARAXXUS_MUST_DIE,             NPC_LORD_JARAXXUS_IMAGE },
    { QUEST_LORD_MARROWGAR_MUST_DIE,            NPC_LORD_MARROWGAR_IMAGE },
    { QUEST_NAZAN,           				    NPC_NAZAN },
    { QUEST_KELIDAN,           					NPC_KELIDAN },
    { QUEST_BLADEFIST,           				NPC_BLADEFIST },
    { QUEST_QUAGMIRRAN,           				NPC_QUAGMIRRAN },
    { QUEST_BLACK_STALKER,           			NPC_BLACK_STALKER },
    { QUEST_WARLORD,           					NPC_WARLORD },
    { QUEST_IKISS,           					NPC_IKISS },
    { QUEST_SHAFFAR,           					NPC_SHAFFAR },
    { QUEST_EXARCH,           					NPC_EXARCH },
    { QUEST_MURMUR,           					NPC_MURMUR },
    { QUEST_EPOCH_HUNTER,           			NPC_EPOCH_HUNTER },
    { QUEST_AEONUS,           					NPC_AEONUS },
    { QUEST_WARP_SPLINTER,           			NPC_WARP_SPLINTER },
    { QUEST_PATHALEON,           				NPC_PATHALEON },
    { QUEST_SKYRISS,           					NPC_SKYRISS },
    { QUEST_KAELTHAS,           				NPC_KAELTHAS },

    { QUEST_CENTURIONS,           				NPC_CENTURIONS },
    { QUEST_MYRMIDONS,           				NPC_MYRMIDONS },
    { QUEST_INSTRUCTORS,           				NPC_INSTRUCTORS },
    { QUEST_LORDS,           				    NPC_LORDS },
    { QUEST_CHANNELERS,           				NPC_CHANNELERS },
    { QUEST_DESTROYERS,           				NPC_DESTROYERS },
    { QUEST_SENTINELS,           				NPC_SENTINELS },
    { QUEST_TORMENT,           				    NPC_TORMENT }
};

Position const ImageSpawnPos[3] = 
{
    {5703.077f, 583.9757f, 652.677307f, 3.926991f},     // Dalaran
    {-1787.00f, 5158.149f, -39.380299f, 0.0f     },     // Shatt HC
    {-1801.67f, 5144.319f, -39.379700f, 0.0f     }      // Shatt NHC
};

// This script will be used by Archmage Lan'dalock (Dalaran), Wind Trader Zhareem (Shattrath < HC) & Nether-Stalker Mah'duun (Shattrath < NHC)
class npc_archmage_landalock : public CreatureScript
{
public:
    npc_archmage_landalock() : CreatureScript("npc_archmage_landalock") { }

    struct npc_archmage_landalockAI : public ScriptedAI
    {
        npc_archmage_landalockAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            if (me->GetEntry() == NPC_ARCHMAGE_LANDALOCK)
            {
                for (auto var : QuestAndNPCData)
                    if (me->hasQuest(var.first))
                        me->SummonCreature(var.second, ImageSpawnPos[0]);
            }
            else if (me->GetEntry() == NPC_WIND_TRADER_ZHAREEM)
            {
                for (auto var : QuestAndNPCData)
                    if (me->hasQuest(var.first))
                        me->SummonCreature(var.second, ImageSpawnPos[1]);
            }
            else if (me->GetEntry() == NPC_NETHER_STALKER_MAHDUUN)
            {
                for (auto var : QuestAndNPCData)
                    if (me->hasQuest(var.first))
                        me->SummonCreature(var.second, ImageSpawnPos[2]);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_archmage_landalockAI(creature);
    }
};

enum ArenaDailyMisc
{
    GOSSIP_START      = 60227,
    GOSSIP_SPECTATE   = 60228,

    TEXT_REPLAY       = 2002194,

    QUEST_ARENA_3_V_3 = 110155,
    EVENT_ARENA_3_V_3 = 108
};

class npc_arena_quest : public CreatureScript
{
    public:
        npc_arena_quest() : CreatureScript("npc_arena_quest") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (!IsEventActive(EVENT_ARENA_3_V_3))
            { 
                player->RemoveActiveQuest(QUEST_ARENA_3_V_3, true);
                player->SendQuestUpdate();
                for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
                {
                    uint32 logQuest = player->GetQuestSlotQuestId(slot);
                    if (logQuest == QUEST_ARENA_3_V_3)
                        player->SetQuestSlot(slot, 0);
                }
            }

            player->PrepareGossipMenu(creature, GOSSIP_START, true);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());            
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override 
        {
            if (player->PlayerTalkClass->GetGossipMenu().GetMenuId() == GOSSIP_START)
            {
                player->PrepareGossipMenu(creature, GOSSIP_SPECTATE, false);
                auto replays = sArenaReplayMgr->GetLastReplayList(player->GetGUID());
                for (auto& kvp : replays)
                    player->ADD_GOSSIP_ITEM(0, kvp.second, GOSSIP_SPECTATE, GOSSIP_ACTION_INFO_DEF + kvp.first);
                player->SEND_GOSSIP_MENU(TEXT_REPLAY, creature->GetGUID());
            }
            else
            {
                player->CLOSE_GOSSIP_MENU();

                if (action > GOSSIP_ACTION_INFO_DEF)
                    sArenaReplayMgr->WatchReplay(*player, action - GOSSIP_ACTION_INFO_DEF);
                else
                    sArenaReplayMgr->WatchRandomReplay(*player);
            }
            return true; 
        }

        bool OnGossipSelectCode(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 /*action*/, const char* code) override 
        {
            player->CLOSE_GOSSIP_MENU();
            uint32 replayId = atoi(code);
            sArenaReplayMgr->WatchReplay(*player, replayId);
            return true; 
        }

};

class go_aged_dalaran_limburger : public GameObjectScript
{
    public:
        go_aged_dalaran_limburger() : GameObjectScript("go_aged_dalaran_limburger") { }

        struct go_aged_dalaran_limburgerAI : public GameObjectAI
        {
            go_aged_dalaran_limburgerAI(GameObject* go) : GameObjectAI(go)
            {
                DespawnTimer = 600 * IN_MILLISECONDS;
            }

            void UpdateAI(uint32 diff) override
            {
                if (DespawnTimer <= diff)
                {
                    go->SetLootState(GO_JUST_DEACTIVATED);
                    DespawnTimer = 600 * IN_MILLISECONDS;
                }
                else
                    DespawnTimer -= diff;
        }

    private:
        uint32 DespawnTimer;
    };

        GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_aged_dalaran_limburgerAI(go);
    }
};

void AddSC_dalaran_rg()
{
    new npc_dalaran_visitor();
    new npc_rhonin();
    new tcg_vendor();
    new npc_minigob_manabonk();
    new npc_archmage_landalock();
    new npc_arena_quest();
    new go_aged_dalaran_limburger();
}
