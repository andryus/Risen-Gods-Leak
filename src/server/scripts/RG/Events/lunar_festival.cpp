/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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

#include "event.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"

/*####
# npc_omen
####*/

enum OmenSpell
{
    SPELL_OMEN_CLEAVE                   = 15284, // SPELL_EFFECT_WEAPON_PERCENT_DAMAGE

    SPELL_OMEN_STARFALL                 = 26540, // SPELL_EFFECT_PERSISTENT_AREA_AURA

    SPELL_OMEN_SUMMON_SPOTLIGHT         = 26392, // (+) SPELL_EFFECT_SUMMON(15902) | SPELL_EFFECT_SUMMON_OBJECT_WILD | SPELL_EFFECT_SUMMON_OBJECT_WILD
    SPELL_ELUNES_BLESSING               = 26393, // (+) -> SPELL_EFFECT_APPLY_AURA | SPELL_EFFECT_SCRIPT_EFFECT
    SPELL_ELUNE_QUEST_CREDIT            = 26394, // (+) -> SPELL_EFFECT_QUEST_COMPLETE

    SPELL_ELUNE_CANDLE                  = 26374, // SPELL_EFFECT_DUMMY
};

enum OmenMisc
{
    NPC_OMEN                            = 15467,

    GO_ELUNE_TRAP_1                     = 180876,
    GO_ELUNE_TRAP_2                     = 180877,

    EVENT_CAST_CLEAVE                   = 1,
    EVENT_CAST_STARFALL                 = 2,

    POINT_OMEN_INTRO                    = 1,

    // misc
    QUEST_ELUENS_BLESSING               = 8868,
    SPELL_ELUNES_CANDLE_TRIGGER_SPELL   = 26636
};

constexpr Position OmenIntroPos{ 7549.977f, -2855.137f, 456.9678f };

struct npc_omenAI : public WorldBossAI
{
    npc_omenAI(Creature* creature) : WorldBossAI(creature)
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
        me->GetMotionMaster()->MovePoint(POINT_OMEN_INTRO, OmenIntroPos);
    }

    void MovementInform(uint32 type, uint32 pointId) override
    {
        if (type != POINT_MOTION_TYPE)
            return;

        if (pointId == POINT_OMEN_INTRO)
        {
            me->SetHomePosition(*me);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            if (Player* player = me->FindNearestPlayer(40.0f))
                AttackStart(player);
        }
    }

    void EnterCombat(Unit* who) override
    {
        WorldBossAI::EnterCombat(who);
        events.ScheduleEvent(EVENT_CAST_CLEAVE, 3s, 5s);
        events.ScheduleEvent(EVENT_CAST_STARFALL, 8s, 10s);
    }

    void JustDied(Unit* killer) override
    {
        WorldBossAI::JustDied(killer);
        DoCast(SPELL_OMEN_SUMMON_SPOTLIGHT);
        me->DespawnOrUnsummon(5 * MINUTE * IN_MILLISECONDS);
    }

    void SpellHit(Unit* /*caster*/, const SpellInfo* spell) override
    {
        if (spell->Id == SPELL_ELUNE_CANDLE)
            events.RescheduleEvent(EVENT_CAST_STARFALL, 14s, 16s);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_CAST_CLEAVE:
                DoCastVictim(SPELL_OMEN_CLEAVE);
                events.ScheduleEvent(EVENT_CAST_CLEAVE, 8s, 10s);
                break;
            case EVENT_CAST_STARFALL:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_OMEN_STARFALL);
                events.ScheduleEvent(EVENT_CAST_STARFALL, 14s , 16s);
                break;
            default:
                break;
        }
    }
};

struct npc_giant_spotlightAI : public ScriptedAI
{
    npc_giant_spotlightAI(Creature* creature) : ScriptedAI(creature)
    {
        DoCastSelf(SPELL_ELUNES_BLESSING, true);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (who->ToPlayer() && !who->ToPlayer()->IsGameMaster() && who->IsWithinDist(me, 5.0f))
            if (who->ToPlayer()->GetQuestStatus(QUEST_ELUENS_BLESSING) == QUEST_STATUS_INCOMPLETE)
                who->CastSpell(who, SPELL_ELUNE_QUEST_CREDIT, true);
    }
};

#define NPC_GREEN_FIREWORK  15874
#define NPC_RED_FIREWORK    15872
#define NPC_BLUE_FIREWORK   15873
#define NPC_MINON_OF_OMEN   15466

class npc_omen_trigger : public CreatureScript
{
public:
    npc_omen_trigger() : CreatureScript("npc_omen_trigger") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_omen_triggerAI (creature);
    }

    struct npc_omen_triggerAI : public ScriptedAI
    {
        npc_omen_triggerAI(Creature* creature) : ScriptedAI(creature) { }

        uint8 fireworkCount;
        uint8 maxCount;
        bool stopCount;
        uint32 countTimer;

        uint32 respawnTimer;
        bool spawnPhase;

        void Reset()
        {
            fireworkCount = 0;
            countTimer = 3000;
            maxCount = urand(40, 60);
            stopCount = false;
            spawnPhase = false;
        }

        void UpdateAI(uint32 diff)
        {
            if (!stopCount)
            {
                if (fireworkCount == maxCount)
                {
                    if (!me->FindNearestCreature(NPC_OMEN, 1000.0f, true))
                        me->SummonCreature(NPC_OMEN, 7564.9155f, -2833.3898f, 446.2745f, 0.f, TEMPSUMMON_TIMED_DESPAWN, 900000);

                    respawnTimer = 900000;
                    spawnPhase = true;
                    stopCount = true;
                }

                if (countTimer <= diff)
                {
                    if ((me->FindNearestCreature(NPC_GREEN_FIREWORK, 10.0f, true)) ||
                        (me->FindNearestCreature(NPC_RED_FIREWORK, 10.0f, true)) ||
                        (me->FindNearestCreature(NPC_BLUE_FIREWORK, 10.0f, true)))
                    {
                        if (Creature* minion = me->SummonCreature(NPC_MINON_OF_OMEN, 7508.8920f, -2839.1840f, 460.0981f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 600000))
                            if (Player* target = minion->FindNearestPlayer(100.0f))
                                minion->GetMotionMaster()->MovePoint(0, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());

                        if (Creature* minion = me->SummonCreature(NPC_MINON_OF_OMEN, 7508.8920f, -2839.1840f, 460.0981f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 600000))
                            if (Player* target = minion->FindNearestPlayer(100.0f))
                                minion->GetMotionMaster()->MovePoint(0, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());

                        ++fireworkCount;
                    }
                    countTimer = 3000;
                } else countTimer -= diff;
            }

            if (spawnPhase)
            {
                if (respawnTimer <= diff)
                {
                    fireworkCount = 0;
                    stopCount = false;
                    spawnPhase = false;
                } else respawnTimer -= diff;
            }
        }
    };
};


#define SPELL_MURLOC                42365
#define ITEM_TIGER                  45047
#define SPELL_LOVE                  45153
#define NPC_BACCATA                 1010158
#define GOSSIP_Firework             "Starte ein Feuerwek! 200 Gold"
#define GOSSIP_ITEM_SPEKTRAL        "GIb mir einen Spektraltiger! 200 Gold"
#define GOSSIP_MURLOC               "I am a murloc! 200 Gold"
#define SAY_WRONG                   "Ihr habt nicht genug Gold!"
#define YELL_FIREWORK               "Es wird gleich ein grosses Feuerwerk gestartet. Richtet eure Blicke gen Himmel und erbaut euch an der Kunst der Goblins!"
#define YELL_FIRE_FAIL              "Es besteht derzeitig noch ein Feuerwerk. Wartet erst ab, bevor Ihr ein neues bestellen koennt!"
#define YELL_FIRE_END               "Dieses Feuerwerk wurde Ihnen praesentiert von Rising Gods - mehr als nur Gaming!"

#define SAY_RANDOM1                 "Immer mit der Ruhe ... !"
#define SAY_RANDOM2                 "Das ... ist ... kurios!"
#define SAY_RANDOM3                 "Beeilt euch! Ich muss noch dabei mithelfen, Elevims Wohnung weihnachtlich zu dekorieren!"
const Position mainTriggerLoc[]= { {5804.702f, 637.635f, 669.34466f, 0} };
#define NPC_FIREWORK_CHRISTMAS      1010521

enum costs
{
    COST_200        = 2000000,
    COST_100        = 1000000,
};

class gossip_kurioses_lunar : public CreatureScript
{
public:
    gossip_kurioses_lunar() : CreatureScript("gossip_kurioses_lunar")  {}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_Firework, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_MURLOC, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
        player->PlayerTalkClass->SendGossipMenu(2002003, creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                DoAction(player, creature, 1, COST_100);
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                DoAction(player, creature, 2, COST_200);
                break;
            default:
                player->CLOSE_GOSSIP_MENU();
                break;
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    void DoAction(Player* player, Creature* creature, int action, uint32 money)
    {
        bool hasEnoughMoney = false;
        bool isTriggerThere = false;

        hasEnoughMoney = (player->GetMoney() >= money);
        isTriggerThere = (creature->FindNearestCreature(NPC_FIREWORK_CHRISTMAS, 1000.0f, true));

        if(hasEnoughMoney)
        {
            switch(action)
            {
                case 1:
                    if (!isTriggerThere)
                    {
                        creature->MonsterYell(YELL_FIREWORK, 0, 0);
                        player->SummonCreature(NPC_FIREWORK_CHRISTMAS, mainTriggerLoc[0], TEMPSUMMON_MANUAL_DESPAWN);
                        player->SetMoney(player->GetMoney() - money);
                    }
                    else
                        creature->MonsterSay(YELL_FIRE_FAIL, 0, 0);
                    break;
                case 2:
                    player->CastSpell(player, SPELL_MURLOC, player);
                    player->SetMoney(player->GetMoney() - money);
                default:
                    break;
            }
        }
        else
            creature->MonsterWhisper(SAY_WRONG, player, true);
    }

    struct gossip_kurioses_lunarAI : public ScriptedAI
    {
        gossip_kurioses_lunarAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 sayTimer;

        void Reset()
        {
            sayTimer = 10000;
        }

        void UpdateAI (uint32 diff)
        {
            if (sayTimer < diff)
            {
                uint8 random = urand(0, 3);

                if (random < 1)
                    me->MonsterSay(SAY_RANDOM1, 0, 0);
                else if (random >= 1 && random < 2)
                    me->MonsterSay(SAY_RANDOM2, 0, 0);
                else if (random >= 2)
                    me->MonsterSay(SAY_RANDOM3, 0, 0);

                sayTimer = 300000;
            }sayTimer -= diff;

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new gossip_kurioses_lunarAI(creature);
    }
};

#define NPC_VENDOR_FIRE             1010520

const Position groundLoc[]=
{
    {5701.5698f, 644.9523f, 691.2269f, 0}, //1
    {5890.0917f, 650.9889f, 690.982f, 0}, //2
    {5835.2675f, 478.5011f, 690.982f, 0}, //3
    {5804.702f, 637.635f, 669.34466f, 0},
};



class npc_lunar_fireworktrigger : public CreatureScript
{
public:
    npc_lunar_fireworktrigger() : CreatureScript("npc_lunar_fireworktrigger") { }

    struct npc_lunar_fireworktriggerAI : public ScriptedAI
    {
        npc_lunar_fireworktriggerAI(Creature* c) : ScriptedAI(c) {}

        uint32 fireTimer;
        uint8 stepCount;

        void Reset()
        {
            fireTimer = 5000;
            stepCount = 0;
        }

        void bodenAOE(uint32 entry, Position pos, uint32 entry2, uint32 entry3)
        {
            for (uint8 i = 0; i < 3; ++i)
            {
                if (Creature* trigger = me->SummonCreature(entry, pos, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    trigger->GetPosition(&pos);
                    trigger->GetRandomNearPosition(pos, float(urand(5, 75)));
                    trigger->SummonCreature(entry2, pos, TEMPSUMMON_MANUAL_DESPAWN);
                    trigger->GetRandomNearPosition(pos, float(urand(5, 80)));
                    trigger->SummonCreature(entry3, pos, TEMPSUMMON_MANUAL_DESPAWN);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (fireTimer < diff)
            {
                switch(stepCount)
                {
                    case 0:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_GREEN_BIG, groundLoc[2], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_FIRWORK_BLUE_FLAMES, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        break;
                    case 1:
                        bodenAOE(NPC_EXPLOSION, groundLoc[0], NPC_CLUSTER_BLUE_BIG, NPC_CLUSTER_RED_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_GREEN_BIG, groundLoc[2], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_EXPLOSION, groundLoc[3], NPC_CLUSTER_BLUE_BIG, NPC_CLUSTER_RED_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        break;
                    case 2:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[2], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 3:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_CLUSTER_GREEN_SMALL);
                        bodenAOE(NPC_YELLOW_ROSE, groundLoc[0], NPC_YELLOW_ROSE, NPC_YELLOW_ROSE);
                        bodenAOE(NPC_CLUSTER_GREEN_BIG, groundLoc[2], NPC_CLUSTER_RED_BIG, NPC_EXPLOSION_LARGE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_YELLOW_ROSE, groundLoc[3], NPC_YELLOW_ROSE, NPC_YELLOW_ROSE);
                        bodenAOE(NPC_CLUSTER_GREEN_BIG, groundLoc[3], NPC_CLUSTER_RED_BIG, NPC_EXPLOSION_LARGE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        break;
                    case 4:
                        bodenAOE(NPC_DALARAN_FIRE, groundLoc[2], NPC_DALARAN_FIRE, NPC_DALARAN_FIRE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_DALARAN_FIRE, groundLoc[0], NPC_DALARAN_FIRE, NPC_DALARAN_FIRE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[0], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_DALARAN_FIRE, groundLoc[3], NPC_DALARAN_FIRE, NPC_DALARAN_FIRE);
                        break;
                    case 5:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        break;
                    case 6:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[0], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        break;
                    case 7:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[1], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 8:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_CLUSTER_GREEN_BIG, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[2], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 9:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[0], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 10:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_CLUSTER_GREEN_BIG, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[1], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_CLUSTER_GREEN_BIG, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        break;
                    case 11:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[2], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        break;
                    case 12:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_CLUSTER_GREEN_BIG, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[0], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 13:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[1], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 14:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_CLUSTER_GREEN_BIG, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[2], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        break;
                    case 15:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[0], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 16:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[1], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        break;
                    case 17:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_CLUSTER_GREEN_BIG, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[2], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        break;
                    case 18:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[1], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 19:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_CLUSTER_GREEN_BIG, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[2], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 20:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[0], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        break;
                    case 21:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_CLUSTER_GREEN_BIG, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[1], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_CLUSTER_GREEN_BIG, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        break;
                    case 22:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[2], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 23:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIREWORK_RANDOM, NPC_CLUSTER_GREEN_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[0], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_GREEN_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 24:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[1], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 25:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[2], NPC_CLUSTER_GREEN_BIG, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_CLUSTER_GREEN_BIG, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        break;
                    case 26:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[0], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[3], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        break;
                    case 27:
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[2], NPC_BLUE_SMALL, NPC_CLUSTER_BLUE_SMALL);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[0], NPC_FIREWORK_RANDOM, NPC_CLUSTER_YELLOW_BIG);
                        bodenAOE(NPC_RED_BLUE_WHITE, groundLoc[1], NPC_RED_BLUE_WHITE, NPC_RED_BLUE_WHITE);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[1], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[1], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);
                        bodenAOE(NPC_PURPLE_BIG, groundLoc[3], NPC_FIRWORK_BLUE_FLAMES, NPC_FIREWORK_RED_FLAMES);
                        bodenAOE(NPC_CLUSTER_RED_BIG, groundLoc[3], NPC_CLUSTER_WHITE_BIG, NPC_CLUSTER_BLUE_BIG);

                        if (Creature* vendor = me->FindNearestCreature(NPC_VENDOR_FIRE, 1000.0f, true))
                            vendor->MonsterYell(YELL_FIRE_END, 0, 0);
                        me->DespawnOrUnsummon();
                        break;
                    default:
                        break;
                }
                ++stepCount;
                fireTimer = 5000;
            }fireTimer -= diff;
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_lunar_fireworktriggerAI(creature);
    }
};

enum npcs
{
    NPC_TAURE                   = 1010524,
    NPC_ORC                     = 1010525,
    NPC_UNDEATH                 = 1010526,
    NPC_BLOOD                   = 1010527,
    NPC_TROLL                   = 1010528,

    NPC_NANA                    = 1010530,
    NPC_AYLEEN                  = 1010531,
    NPC_RHAPSODY                = 1010532,
    NPC_YULIWEE                 = 1010533,
    NPC_NYX                     = 1010534,

    NPC_BAND_TRIGGER            = 1010522,
    NPC_SOUND_TRIGGER           = 1010523,

    SOUND_HORDE                 = 11803,

    SPELL_EXPLODE               = 40162,
    SPELL_LIGHT                 = 50236,
    SPELL_FIRE                  = 73119,
    SPELL_EXPLODE2              = 64753,
    SPELL_PUFF                  = 71495,
};

#define GOSSIP_BAND             "Lass es rocken! 200 Gold"
#define GOSSIP_SHAKESPEARE      "Under Construction" //"Zeigt mir eine Romanze! 200 Gold"
#define SAY_EVENT_START         "Hier sollte Ihre Werbung stehen!"

#define SAY_RANDOM_EVENT        "Kommt nur her - tretet naeher und erlebt ein Schauspiel, dass Ihr nicht mehr vergessen werdet!"

const Position bandLoc[]=
{
    {5801.507f, 636.988f, 615.787231f}, //Taure
    {5807.042f, 643.750f, 612.150024f}, //Orc
    {5802.277f, 646.562f, 612.150024f}, //VO Rechts
    {5811.025f, 639.410f, 612.150024f}, //VO links
    {5806.006f, 639.639f, 612.150024f}, //Hinten
};

const Position moveupLoc[]=
{
    {5802.094f, 625.236f, 609.885f},
    {5795.833f, 629.866f, 609.885f},
    {5800.183f, 635.345f, 612.150024f},
    {5807.019f, 644.477f, 612.150024f},
};

const Position triggerFireLoc[]=
{
    {5801.507f, 636.988f, 615.787231f}, //Taure
    {5807.042f, 643.750f, 612.150024f}, //Orc
    {5802.277f, 646.562f, 612.150024f}, //VO Rechts
    {5811.025f, 639.410f, 612.150024f}, //VO links
    {5806.006f, 639.639f, 612.150024f}, //Hinten
};

const Position danceLoc[]=
{
    {5809.760f, 646.424f, 609.886f},
    {5814.195f, 643.082f, 609.886f},
    {5812.064f, 644.480f, 609.886f},
    {5804.997f, 650.559f, 609.886f},
    {5807.308f, 648.985f, 609.886f},
};


enum events
{
    EVENT_BAND          = 1,
    EVENT_SHAKESPEARE   = 2,
};

class gossip_event_lunar : public CreatureScript
{
public:
    gossip_event_lunar() : CreatureScript("gossip_event_lunar") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_BAND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_SHAKESPEARE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
        //player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_MURLOC, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
        player->PlayerTalkClass->SendGossipMenu(2002003, creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                DoActionEvent(player, creature, 1, 2000000);
                break;
            //case GOSSIP_ACTION_INFO_DEF+2:
                //DoActionEvent(player, creature, 2, 2000000);
                //break;
            //case GOSSIP_ACTION_INFO_DEF+3:
                //DoActionEvent(player, creature, 3, 1000000);
                //break;
            default:
                player->CLOSE_GOSSIP_MENU();
                break;
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    void DoActionEvent(Player* player, Creature* creature, int action, uint32 money)
    {
        bool hasEnoughMoney = false;
        hasEnoughMoney = (player->GetMoney() >= money);

        if(hasEnoughMoney)
        {
            switch(action)
            {
                case 1:
                    creature->AI()->DoAction(EVENT_BAND);
                    player->SetMoney(player->GetMoney() - money);
                    break;
                //case 2:
                    //creature->AI()->DoAction(EVENT_SHAKESPEARE);
                    //player->SetMoney(player->GetMoney() - money);
                    //break;
                    //break;
                //case 3:
                    //player->CastSpell(player, SPELL_MURLOC, player);
                    //player->SetMoney(player->GetMoney() - money);
                default:
                    break;
            }
        }
    }

    struct gossip_event_lunarAI : public ScriptedAI
    {
        gossip_event_lunarAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 waitTimer;
        bool eventBand;
        bool eventShakes;
        bool eventMove;
        bool eventStart;

        uint32 bandTimer;
        uint8 bandCount;
        uint32 bandEndTimer;
        bool bandEnd;

        uint32 moveTimer;
        uint8 stepCounter;

        void Reset()
        {
            waitTimer = 60000;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            eventBand = false;
            eventShakes = false;
            eventMove = false;
            eventStart = false;
            bandEndTimer = 0;
            bandEnd = false;

            moveTimer = 0;
            stepCounter = 0;
        }

        void DoAction(int32 action)
        {
            eventBand = false;
            eventShakes = false;
            eventMove = false;
            eventStart = false;
            bandEnd = false;

            moveTimer = 0;
            stepCounter = 0;
            bandEndTimer = 270000;

            if (action == EVENT_BAND)
                eventBand = true;
            if (action == EVENT_SHAKESPEARE)
                eventShakes = true;

            eventMove = true;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void UpdateAI (uint32 diff)
        {
            if (waitTimer < diff)
            {
                me->MonsterYell(SAY_RANDOM_EVENT, 0, 0);
                waitTimer = 900000;
            }waitTimer -= diff;

            if (eventMove)
            {
                if (moveTimer < diff)
                {
                    switch(stepCounter)
                    {
                        case 0:
                            me->GetMotionMaster()->MovePoint(0, moveupLoc[0]);
                            moveTimer = 3000;
                            ++stepCounter;
                            break;
                        case 1:
                            me->GetMotionMaster()->MovePoint(0, moveupLoc[1]);
                            moveTimer = 1500;
                            ++stepCounter;
                            break;
                        case 2:
                            me->GetMotionMaster()->MovePoint(0, moveupLoc[2]);
                            moveTimer = 1000;
                            ++stepCounter;
                            break;
                        case 3:
                            me->GetMotionMaster()->MovePoint(0, moveupLoc[3]);
                            moveTimer = 5000;
                            ++stepCounter;
                            break;
                        case 4:
                            me->MonsterSay(SAY_EVENT_START, 0, 0);
                            moveTimer = 5000;
                            ++stepCounter;
                            break;
                        case 5:
                            DoCastSelf(SPELL_PUFF, true);
                            me->SetVisible(false);
                            me->GetMotionMaster()->MovePoint(0, 5818.54f, 637.61f, 609.886f);
                            moveTimer = 3000;
                            ++stepCounter;
                            break;
                        case 6:
                            me->SetOrientation(1.35791f);
                            me->SetVisible(true);
                            me->SendMovementFlagUpdate();
                            eventStart = true;
                            bandTimer = 1000;
                            bandCount = 0;
                            eventMove = false;
                            ++stepCounter;
                            break;
                        default:
                            break;
                    }
                }moveTimer -= diff;
            }

            if (eventStart)
            {
                if (eventBand)
                {
                    if (bandTimer < diff)
                    {
                        switch(bandCount)
                        {
                            case 0:
                                if (Creature* trigger1 = me->SummonCreature(NPC_BAND_TRIGGER, triggerFireLoc[0], TEMPSUMMON_TIMED_DESPAWN, 4000))
                                    trigger1->CastSpell(trigger1, SPELL_FIRE, true);
                                if (Creature* trigger2 = me->SummonCreature(NPC_BAND_TRIGGER, triggerFireLoc[1], TEMPSUMMON_TIMED_DESPAWN, 4000))
                                    trigger2->CastSpell(trigger2, SPELL_FIRE, true);
                                if (Creature* trigger3 = me->SummonCreature(NPC_BAND_TRIGGER, triggerFireLoc[2], TEMPSUMMON_TIMED_DESPAWN, 4000))
                                    trigger3->CastSpell(trigger3, SPELL_FIRE, true);
                                if (Creature* trigger4 = me->SummonCreature(NPC_BAND_TRIGGER, triggerFireLoc[3], TEMPSUMMON_TIMED_DESPAWN, 4000))
                                    trigger4->CastSpell(trigger4, SPELL_FIRE, true);
                                if (Creature* trigger5 = me->SummonCreature(NPC_BAND_TRIGGER, triggerFireLoc[4], TEMPSUMMON_TIMED_DESPAWN, 4000))
                                    trigger5->CastSpell(trigger5, SPELL_FIRE, true);
                                bandTimer = 2000;
                                ++bandCount;
                                break;
                            case 1:
                                bandEndTimer = 270000;
                                if (Creature* sound = me->SummonCreature(NPC_SOUND_TRIGGER, 5815.121f, 652.9188f, 609.886f, TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    sound->PlayDistanceSound(SOUND_HORDE, 0);
                                    sound->DespawnOrUnsummon();
                                }
                                if (Creature* sound1 = me->SummonCreature(NPC_SOUND_TRIGGER, 5803.444f, 659.429f, 609.886f, TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    sound1->PlayDistanceSound(SOUND_HORDE, 0);
                                    sound1->DespawnOrUnsummon();
                                }
                                if (Creature* sound2 = me->SummonCreature(NPC_SOUND_TRIGGER, 5815.762f, 663.387f, 609.886f, TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    sound2->PlayDistanceSound(SOUND_HORDE, 0);
                                    sound2->DespawnOrUnsummon();
                                }
                                if (Creature* sound3 = me->SummonCreature(NPC_SOUND_TRIGGER, 5823.468f, 655.043f, 609.886f, TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    sound3->PlayDistanceSound(SOUND_HORDE, 0);
                                    sound3->DespawnOrUnsummon();
                                }
                                if (Creature* sound4 = me->SummonCreature(NPC_SOUND_TRIGGER, 5821.969f, 647.094f, 609.886f, TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    sound4->PlayDistanceSound(SOUND_HORDE, 0);
                                    sound4->DespawnOrUnsummon();
                                }
                                if (Creature* orc = me->SummonCreature(NPC_ORC, bandLoc[1], TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    orc->SetOrientation(0.976208f);
                                    orc->SendMovementFlagUpdate();
                                    orc->CastSpell(orc, SPELL_LIGHT, true);
                                    orc->CastSpell(orc, SPELL_EXPLODE, true);
                                }
                                bandTimer = 2000;
                                ++bandCount;
                                break;
                            case 2:
                                if (Creature* trigger = me->SummonCreature(NPC_BAND_TRIGGER, bandLoc[4], TEMPSUMMON_TIMED_DESPAWN, 2000))
                                    trigger->CastSpell(trigger, SPELL_EXPLODE2, true);
                                if (Creature* troll = me->SummonCreature(NPC_TROLL, bandLoc[2], TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    troll->SetOrientation(0.976208f);
                                    troll->SendMovementFlagUpdate();
                                    troll->CastSpell(troll, SPELL_EXPLODE, true);
                                }
                                if (Creature* trigger1 = me->SummonCreature(NPC_YULIWEE, danceLoc[0], TEMPSUMMON_TIMED_DESPAWN, 268000))
                                {
                                    trigger1->SetOrientation(0.976208f);
                                    trigger1->SendMovementFlagUpdate();
                                    trigger1->SetDisplayId(18309);
                                }
                                if (Creature* trigger2 = me->SummonCreature(NPC_NANA, danceLoc[1], TEMPSUMMON_TIMED_DESPAWN, 268000))
                                {
                                    trigger2->SetOrientation(0.976208f);
                                    trigger2->SendMovementFlagUpdate();
                                    trigger2->SetDisplayId(18309);
                                }
                                if (Creature* trigger3 = me->SummonCreature(NPC_AYLEEN, danceLoc[2], TEMPSUMMON_TIMED_DESPAWN, 268000))
                                {
                                    trigger3->SetOrientation(0.976208f);
                                    trigger3->SendMovementFlagUpdate();
                                    trigger3->SetDisplayId(18309);
                                }
                                if (Creature* trigger4 = me->SummonCreature(NPC_RHAPSODY, danceLoc[3], TEMPSUMMON_TIMED_DESPAWN, 268000))
                                {
                                    trigger4->SetOrientation(0.976208f);
                                    trigger4->SendMovementFlagUpdate();
                                    trigger4->SetDisplayId(18309);
                                }
                                if (Creature* trigger5 = me->SummonCreature(NPC_NYX, danceLoc[4], TEMPSUMMON_TIMED_DESPAWN, 268000))
                                {
                                    trigger5->SetOrientation(0.976208f);
                                    trigger5->SendMovementFlagUpdate();
                                    trigger5->SetDisplayId(18309);
                                }
                                bandTimer = 2000;
                                ++bandCount;
                                break;
                            case 3:
                                if (Creature* trigger1 = me->FindNearestCreature(NPC_NYX, 500.0f, true))
                                    trigger1->HandleEmoteCommand(EMOTE_STATE_DANCE);
                                if (Creature* trigger2 = me->FindNearestCreature(NPC_RHAPSODY, 500.0f, true))
                                    trigger2->HandleEmoteCommand(EMOTE_STATE_DANCE);
                                if (Creature* trigger3 = me->FindNearestCreature(NPC_AYLEEN, 500.0f, true))
                                    trigger3->HandleEmoteCommand(EMOTE_STATE_DANCE);
                                if (Creature* trigger4 = me->FindNearestCreature(NPC_NANA, 500.0f, true))
                                    trigger4->HandleEmoteCommand(EMOTE_STATE_DANCE);
                                if (Creature* trigger5 = me->FindNearestCreature(NPC_YULIWEE, 500.0f, true))
                                    trigger5->HandleEmoteCommand(EMOTE_STATE_DANCE);
                                if (Creature* death = me->SummonCreature(NPC_UNDEATH, bandLoc[3], TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    death->SetOrientation(0.976208f);
                                    death->SendMovementFlagUpdate();
                                    death->CastSpell(death, SPELL_EXPLODE, true);
                                }
                                bandTimer = 2000;
                                ++bandCount;
                                break;
                            case 4:
                                if (Creature* blood = me->SummonCreature(NPC_BLOOD, bandLoc[4], TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    blood->SetOrientation(0.976208f);
                                    blood->SendMovementFlagUpdate();
                                    blood->CastSpell(blood, SPELL_EXPLODE, true);
                                }
                                bandTimer = 2000;
                                ++bandCount;
                                break;
                            case 5:
                                if (Creature* taure = me->SummonCreature(NPC_TAURE, bandLoc[0], TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    taure->SetOrientation(0.976208f);
                                    taure->SendMovementFlagUpdate();
                                    taure->CastSpell(taure, SPELL_EXPLODE, true);
                                }
                                bandTimer = 2000;
                                ++bandCount;
                                break;
                            default:
                                break;
                        }
                    }bandTimer -= diff;
                }

                if (bandEndTimer < diff)
                {
                    bandEnd = true;
                }bandEndTimer -= diff;

                if (bandEnd)
                {
                    if (Creature* trigger = me->SummonCreature(NPC_BAND_TRIGGER, bandLoc[4], TEMPSUMMON_TIMED_DESPAWN, 2000))
                        trigger->CastSpell(trigger, SPELL_EXPLODE2, true);
                    if (Creature* taure = me->FindNearestCreature(NPC_TAURE, 5000.0f, true))
                        taure->DespawnOrUnsummon();
                    if (Creature* blood = me->FindNearestCreature(NPC_BLOOD, 5000.0f, true))
                        blood->DespawnOrUnsummon();
                    if (Creature* orc = me->FindNearestCreature(NPC_ORC, 5000.0f, true))
                        orc->DespawnOrUnsummon();
                    if (Creature* troll = me->FindNearestCreature(NPC_TROLL, 5000.0f, true))
                        troll->DespawnOrUnsummon();
                    if (Creature* death = me->FindNearestCreature(NPC_UNDEATH, 5000.0f, true))
                        death->DespawnOrUnsummon();
                    if (Creature* sound = me->FindNearestCreature(NPC_SOUND_TRIGGER, 5000.0f, true))
                        sound->DespawnOrUnsummon();

                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    eventBand = false;
                    eventStart = false;
                    bandEnd = false;
                }
            }
        }
    };
    CreatureAI *GetAI(Creature *creature) const
    {
        return new gossip_event_lunarAI(creature);
    }
};
class spell_item_elunes_candle : public SpellScriptLoader
{
public:
    spell_item_elunes_candle() : SpellScriptLoader("spell_item_elunes_candle") { }

    class spell_item_elunes_candle_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_item_elunes_candle_SpellScript)
    public:
        spell_item_elunes_candle_SpellScript() : SpellScript() {};

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Player* caster = GetCaster()->ToPlayer())
            {
                if (Unit* target = GetExplTargetUnit())
                {
                    caster->CastSpell(target, SPELL_ELUNES_CANDLE_TRIGGER_SPELL, true);
                }
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_item_elunes_candle_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_elunes_candle_SpellScript();
    }
};

enum LunarInvitation
{
    GO_LUNAR_INVITATION         = 300058,
    MAP_KALIMDOR                = 1,
    ZONE_STORMWIND              = 1519,
    ZONE_ORGRIMMAR              = 1637,
    ZONE_IRONFORGE              = 1537,
    ZONE_DARNASSUS              = 1657,
    ZONE_UNDERCITY              = 1497,
    ZONE_THUNDERBLUFF           = 1638
};

class spell_item_lunar_invitation : public SpellScriptLoader
{
public:
    spell_item_lunar_invitation() : SpellScriptLoader("spell_item_lunar_invitation") { }

    class spell_item_lunar_invitation_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_item_lunar_invitation_SpellScript)
    public:
        spell_item_lunar_invitation_SpellScript() : SpellScript() {};

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Player* caster = GetCaster()->ToPlayer())
            {
                switch (caster->GetZoneId())
                {
                    case ZONE_STORMWIND:
                        caster->TeleportTo(MAP_KALIMDOR, 7585.0136f, -2205.1704f, 475.2969f, 0.712806f, 0);
                        break;
                    case ZONE_ORGRIMMAR:
                        caster->TeleportTo(MAP_KALIMDOR, 7595.5146f, -2247.2351f, 466.8537f, 1.791947f, 0);
                        break;
                    case ZONE_IRONFORGE:
                        caster->TeleportTo(MAP_KALIMDOR, 7570.2900f, -2221.2492f, 473.3690f, 1.235096f, 0);
                        break;
                    case ZONE_DARNASSUS:
                        caster->TeleportTo(MAP_KALIMDOR, 7610.8706f, -2229.1206f, 468.6264f, 0.622490f, 0);
                        break;
                    case ZONE_UNDERCITY:
                        caster->TeleportTo(MAP_KALIMDOR, 7575.1635f, -2238.8085f, 469.8016f, 5.935705f, 0);
                        break;
                    case ZONE_THUNDERBLUFF:
                        caster->TeleportTo(MAP_KALIMDOR, 7602.6728f, -2211.3366f, 471.7972f, 5.056063f, 0);
                        break;
                    default:
                        caster->TeleportTo(MAP_KALIMDOR, 7585.0136f, -2205.1704f, 475.2969f, 0.712806f, 0);
                        break;
                }
            }
        }

        void Register()
        {
            OnEffectHit += SpellEffectFn(spell_item_lunar_invitation_SpellScript::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_lunar_invitation_SpellScript();
    }
};

void AddSC_event_lunar_festival()
{
    new CreatureScriptLoaderEx<npc_omenAI>("npc_omen");
    new CreatureScriptLoaderEx<npc_giant_spotlightAI>("npc_giant_spotlight");
    new npc_omen_trigger();
    new npc_lunar_fireworktrigger();
    new gossip_event_lunar();
    new gossip_kurioses_lunar();
    new spell_item_elunes_candle();
    new spell_item_lunar_invitation();
}
