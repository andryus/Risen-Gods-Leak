#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "event.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "DBCStores.h"

/*######
## http://www.wowhead.com/object=180574
## Wickerman Ember
## go_wickerman_ember
######*/

#define GOSSIP_WICKERMAN_SPELL "Smear the ash on my face like war paint!"

enum WickermanEmberGo
{
    SPELL_GRIM_VISAGE   = 24705,    // http://www.wowhead.com/spell=24705/grim-visage
};

class go_wickerman_ember : public GameObjectScript
{
public:
    go_wickerman_ember() : GameObjectScript("go_wickerman_ember") { }

    bool OnGossipHello(Player* player, GameObject* pGO)
    {
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_WICKERMAN_SPELL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(pGO), pGO->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, GameObject* pGO, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();
        if (player->HasAura(SPELL_GRIM_VISAGE))
        {
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(pGO), pGO->GetGUID());
            player->CLOSE_GOSSIP_MENU();

        }
        else if (uiAction == GOSSIP_ACTION_INFO_DEF +1)
        {
            pGO->CastSpell(player, SPELL_GRIM_VISAGE);
            player->CLOSE_GOSSIP_MENU();
        }
        return true;
    }
};

enum WickermanGuardian
{
    SPELL_KNOCKDOWN                = 19128,
    SPELL_STRIKE                   = 18368,
    SPELL_WICKERMAN_GUARDIAN_EMBER = 25007
};

class npc_wickerman_guardian : public CreatureScript
{
public:
    npc_wickerman_guardian() : CreatureScript("npc_wickerman_guardian") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_wickerman_guardianAI(creature);
    }

    struct npc_wickerman_guardianAI : public ScriptedAI
    {
        npc_wickerman_guardianAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 KnockdownTimer;
        uint32 StrikeTimer;

        void Reset()
        {
            KnockdownTimer = 7000;
            StrikeTimer = 10000;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (KnockdownTimer <= diff)
            {
                DoCast(me->GetVictim(),SPELL_KNOCKDOWN);
                KnockdownTimer = 7000;
            }else KnockdownTimer -= diff;

            if (StrikeTimer <= diff)
            {
                DoCast(me->GetVictim(),SPELL_STRIKE);
                StrikeTimer = 10000;
            }else StrikeTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer)
        {
            DoCast(killer, SPELL_WICKERMAN_GUARDIAN_EMBER, true);
        }
    };
};

enum BansheeQueen
{
    /* EVENTS */
    EVENT_YELL_1 = 1,
    EVENT_YELL_2,
    EVENT_YELL_3,
    EVENT_YELL_4,
    EVENT_YELL_5,
    EVENT_YELL_6,
    EVENT_BURN,

    /* YELLS */
    BANSHEE_YELL_1 = -1998997,
    BANSHEE_YELL_2 = -1998996,
    BANSHEE_YELL_3 = -1998995,
    BANSHEE_YELL_4 = -1998994,
    BANSHEE_YELL_5 = -1998993,
    BANSHEE_YELL_6 = -1998992,

    /* MISC */
    GOB_WICKERMAN = 180433,
    SPELL_THROW_TORCH = 42118,
    NPC_TORCH_TRIGGER = 1010502,
    NPC_EVENT_GENERATOR = 1010501,
    ACTION_SPAWN_EMBERS = 1
};

class npc_he_banshee_queen : public  CreatureScript
{
public:
    npc_he_banshee_queen() : CreatureScript("npc_he_banshee_queen") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_he_banshee_queenAI(creature);
    }

    struct npc_he_banshee_queenAI : public ScriptedAI
    {
        npc_he_banshee_queenAI(Creature* creature) : ScriptedAI(creature)
        {
            me->setActive(true);
        }

        void Reset()
        {
            if (!IsHolidayActive(HOLIDAY_HALLOWS_END))
                return;

            DoCastSelf(35838, true); //blue ghost aura

            events.Reset();

            time_t rawtime;
            struct tm * tmi;
            time ( &rawtime );
            tmi = localtime ( &rawtime );

            if ( (tmi->tm_hour >= 20 && tmi->tm_min >= 10 && tmi->tm_hour <= 0) || (tmi->tm_hour >= 0 && tmi->tm_hour <= 7) )
            {
                return; //server crashed and we're too late.
            }

            events.ScheduleEvent(EVENT_YELL_1, 2000);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                    switch(eventId)
                    {
                        case EVENT_YELL_1:
                            me->MonsterYell(BANSHEE_YELL_1, LANG_UNIVERSAL, 0);
                            events.ScheduleEvent(EVENT_YELL_2, 15000);
                            break;
                        case EVENT_YELL_2:
                            me->MonsterYell(BANSHEE_YELL_2, LANG_UNIVERSAL, 0);
                            events.ScheduleEvent(EVENT_YELL_3, 25000);
                            break;
                        case EVENT_YELL_3:
                            me->MonsterYell(BANSHEE_YELL_3, LANG_UNIVERSAL, 0);
                            events.ScheduleEvent(EVENT_YELL_4, 25000);
                            break;
                        case EVENT_YELL_4:
                            me->MonsterYell(BANSHEE_YELL_4, LANG_UNIVERSAL, 0);
                            events.ScheduleEvent(EVENT_YELL_5, 25000);
                            break;
                        case EVENT_YELL_5:
                            me->MonsterYell(BANSHEE_YELL_5, LANG_UNIVERSAL, 0);
                            events.ScheduleEvent(EVENT_YELL_6, 25000);
                            break;
                        case EVENT_YELL_6:
                            me->MonsterYell(BANSHEE_YELL_6, LANG_UNIVERSAL, 0);
                            events.ScheduleEvent(EVENT_BURN, 2000);
                            break;
                        case EVENT_BURN:
                            if (Creature* trigger = me->FindNearestCreature(NPC_TORCH_TRIGGER, 40.0f))
                                me->CastSpell(trigger, SPELL_THROW_TORCH, true);

                            if (GameObject* wickerman = me->FindNearestGameObject(GOB_WICKERMAN, 40.0f))
                                wickerman->Use(me);

                            if (Creature* eventGenerator = me->FindNearestCreature(NPC_EVENT_GENERATOR, 40.0f))
                                eventGenerator->AI()->DoAction(ACTION_SPAWN_EMBERS);

                            break;
                    }
            }


        }

    private:
        EventMap events;
    };
};

#define MAX_EMBERS 9

const Position emberpos[MAX_EMBERS] =
{
    { 1735.377930f, 513.244446f, 39.700611f, 1.142932f },
    { 1741.476807f, 510.456909f, 39.969326f, 1.182200f },
    { 1743.437256f, 505.085876f, 41.051010f, 6.189110f },
    { 1743.502930f, 498.092712f, 41.601238f, 6.240150f },
    { 1725.016724f, 514.657471f, 38.908882f, 1.751604f },
    { 1720.138428f, 511.699036f, 38.879662f, 1.991143f },
    { 1718.841187f, 506.684174f, 39.953556f, 2.591973f },
    { 1720.066650f, 501.310577f, 41.009216f, 3.444131f },
    { 0, 0, 0, 0}
};

enum Misc
{
    NPC_DARKCALLER_YANKA = 15197,
    DARKCALLER_15MIN_YELL = -1999000,
    DARKCALLER_10MIN_YELL = -1998999,
    DARKCALLER_5MIN_YELL = -1998998,
    GOB_EMBER = 180574,
    NPC_BANSHEE_QUEEN = 15193
};

const Position bansheePos = {1732.661133f, 513.421387f, 39.850609f, 1.416237f};

class npc_wickerman_event_generator : public  CreatureScript
{
public:
    npc_wickerman_event_generator() : CreatureScript("npc_wickerman_event_generator") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_wickerman_event_generatorAI(creature);
    }

    struct npc_wickerman_event_generatorAI : public ScriptedAI
    {
        npc_wickerman_event_generatorAI(Creature* creature) : ScriptedAI(creature)
        {
            me->setActive(true);
        }

        void Reset()
        {

            first_announce = false;
            second_announce = false;
            third_announce = false;
            eventStarted = false;
            checkTimer = 3000;
        }

        void StartEvent(bool prepare = false)
        {
            if (eventStarted)
                return;

            eventStarted = true;
            if (prepare)
            {
                 if (GameObject* wickerman = me->FindNearestGameObject(GOB_WICKERMAN, 40.0f))
                    wickerman->Use(me);

                 SpawnEmbers();
            }
                me->SummonCreature(NPC_BANSHEE_QUEEN, bansheePos.GetPositionX(), bansheePos.GetPositionY(), bansheePos.GetPositionZ(), bansheePos.GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 1*IN_MILLISECONDS*HOUR);
        }

        void SpawnEmbers()
        {
            std::list<GameObject*> EmberList;
            me->GetGameObjectListWithEntryInGrid(EmberList, GOB_EMBER, 30.0f);

            for (std::list<GameObject*>::const_iterator itr = EmberList.begin(); itr != EmberList.end(); ++itr)
                if (GameObject* ember = (*itr))
                    ember->Delete();

            for (int i = 0; i <= MAX_EMBERS-1; ++i)
                me->SummonGameObject(GOB_EMBER, emberpos[i].GetPositionX(), emberpos[i].GetPositionY(), emberpos[i].GetPositionZ(), emberpos[i].GetOrientation(), 0, 0, 0, 0, 0);
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_SPAWN_EMBERS)
                SpawnEmbers();
        }


        void UpdateAI(uint32 diff)
        {

            if (checkTimer <= diff)
            {
                checkTimer = 30000;

                time ( &rawtime );
                tmi = localtime ( &rawtime );

                if (Creature* darkcaller = me->FindNearestCreature(NPC_DARKCALLER_YANKA, 100.0f, true))
                {
                    if (tmi->tm_hour == 19 && tmi->tm_min >= 45 && tmi->tm_min < 50 && !first_announce)
                        {
                        darkcaller->MonsterYell(DARKCALLER_15MIN_YELL, LANG_UNIVERSAL, 0);
                            first_announce = true;
                        }

                    if (tmi->tm_hour == 19 && tmi->tm_min >= 50 && tmi->tm_min <= 59 && !second_announce)
                        {
                        darkcaller->MonsterYell(DARKCALLER_15MIN_YELL, LANG_UNIVERSAL, 0);
                            second_announce = true;
                        }

                    if (tmi->tm_hour == 19 && tmi->tm_min >= 0 && tmi->tm_min <= 5 && !third_announce)
                        {
                        darkcaller->MonsterYell(DARKCALLER_15MIN_YELL, LANG_UNIVERSAL, 0);
                            third_announce = true;
                        }

                    if (tmi->tm_hour == 20 && !eventStarted)
                        StartEvent();

                    if ( (tmi->tm_hour >= 20 && tmi->tm_hour <= 23 && tmi->tm_min <= 59) || (tmi->tm_hour >= 0 && tmi->tm_hour <= 7) )  //server crashed or something, we will set the wickerman on fire and spawn the embers (time range= 20:10 - 7:59)
                        StartEvent(true);


                }
            } else checkTimer -= diff;

        }

    private:
        bool first_announce;
        bool second_announce;
        bool third_announce;
        bool eventStarted;
        uint32 checkTimer;
        time_t rawtime;
        struct tm * tmi;
    };
};


/*
###############################################################################
## horseman fire event!
###############################################################################
*/

enum HallowsEndGOBs
{
    GO_LARGE_JACK                           = 186887,
};

enum HallowsEndNPCs
{
    NPC_HEADLESS_HORSEMAN_FIRE_DND          = 23537,
    NPC_SHADE_OF_THE_HORSEMAN               = 23543,
    NPC_FIRE_HELPER                         = 1010504,
    NPC_FIRE_EVENT_GENERATOR                = 1010503,
    NPC_BUCKET_DUMMY                        = 1,
};

enum HallowsEndSpells
{
    SPELL_HEADLES_HORSEMAN_QUEST_CREDIT     = 42242,
    SPELL_HEADLESS_HORSEMAN_START_FIRE      = 42132,
    SPELL_BUCKET_LANDS                      = 42339,
    SPELL_CREATE_WATER_BUCKET               = 42349,
    SPELL_WATER_SPOUT_VISUAL                = 42348,
    SPELL_HEADLESS_HORSEMAN_LARGE_JACK      = 44231,

    SPELL_HALLOWEEN_WAND_PIRATE         = 24717,
    SPELL_HALLOWEEN_WAND_NINJA          = 24718,
    SPELL_HALLOWEEN_WAND_LEPER_GNOME    = 24719,
    SPELL_HALLOWEEN_WAND_RANDOM         = 24720,
    SPELL_HALLOWEEN_WAND_SKELETON       = 24724,
    SPELL_HALLOWEEN_WAND_WISP           = 24733,
    SPELL_HALLOWEEN_WAND_GHOST          = 24737,
    SPELL_HALLOWEEN_WAND_BAT            = 24741,

    SPELL_CONFLAGRATION                 = 42380,

    //Fire Event - Fire Ranks
    SPELL_HEADLESS_HORSEMAN_BURNING         = 42971,
    SPELL_HEADLESS_HORSEMAN_FIRE            = 42074,
    SPELL_HEADLESS_HORSEMAN_VISUAL_FIRE     = 42075
};

enum HallowsEndMisc
{
    // Fire Event
    ACTION_FAIL_EVENT = 1,
    ACTION_RESET_OR_PASS_EVENT,
    ACTION_INITIAL_START,
    ACTION_FIRE_JUMP,
    ACTION_FIRE_AGAIN,
    ACTION_BURN_TOWN,
    ACTION_LAND_AND_ATTACK,

    MOUNT_SHADE_HORSE             = 25159,

    EVENT_FIRE_HIT_BY_BUCKET,

    DATA_FIRE_STATUS = 1,
    DATA_EVENT = 2,
    DATA_IMMUNE_STATUS = 3,

    QUEST_LET_THE_FIRES_COME_A          = 12135,
    QUEST_LET_THE_FIRES_COME_H          = 12139,

    QUEST_STOP_FIRES_A                  = 11131,
    QUEST_STOP_FIRES_H                  = 11219,

    QUEST_FIRE_BRIGADE_PRACTICE_1 = 11360,
    QUEST_FIRE_BRIGADE_PRACTICE_2 = 11439,
    QUEST_FIRE_BRIGADE_PRACTICE_3 = 11440,

    QUEST_FIRE_TRAINING_1         = 11361,
    QUEST_FIRE_TRAINING_2         = 11449,
    QUEST_FIRE_TRAINING_3         = 11450,

    QUEST_CRASHING_WICKERMAN_FESTIVAL = 1658,
    
    GOSSIP_ORPHAN_EVENT_INACTIVE  = 60211,
    GOSSIP_ORPHAN_EVENT_ACTIVE    = 60212,
    GOSSIP_MATRON_MENU_ID         = 8763,

    TEXT_FIRE_EVENT_ACTIVE        = 11177,
    TEXT_FIRE_EVENT_INACTIVE      = 11147,
    TEXT_HEADLESS_HORSEMEN        = 11590,
};

enum Events
{
    EVENT_COUNT_PLAYERS = 1,
    EVENT_HALFWAY,
    EVENT_YELL_NEAR_END,
    EVENT_CHECK_COMPLETE,
    EVENT_END,
    EVENT_CONFLAGRATION,
};

enum Yells
{
    YELL_EVENT_START        = 0,
    YELL_HALF_EVENT         = 1,
    YELL_NEAR_END           = 2,
    YELL_FAIL_EVENT         = 3,
    YELL_COMPLETE_EVENT     = 4,
    YELL_HORSEMAN_DIE       = 5,
    YELL_CONFLAGRATE        = 6,
};

enum FireTraining
{
    EVENT_INCREASE_FIRE = 1,
    NPC_FIRE_TRAINING_TRIGGER = 1010514,

};

class npc_hallowend : public CreatureScript
{
public:
    npc_hallowend() : CreatureScript("npc_hallowend") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_hallowendAI(creature);
    }

    struct npc_hallowendAI : public ScriptedAI
    {
        npc_hallowendAI(Creature* c) : ScriptedAI(c)
        {
            me->SetVisible(false);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetLevel(85);
        }

        uint32 CountPlayersEvent;
        EventMap events;

        void Reset()
        {
            CountPlayersEvent = 0;
            events.Reset();
        }

        uint32 GetData(uint32 ) const
        {
            return CountPlayersEvent;
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_INITIAL_START:
                    if (!me->FindNearestCreature(NPC_SHADE_OF_THE_HORSEMAN, 300.0f, true)
                        && !me->FindNearestGameObject(GO_LARGE_JACK, 300.0f))
                        EventBegin();
                    break;
            }
        }

        uint32 PlayersCountRange(float dist) const
        {
            Trinity::AnyPlayerInObjectRangeCheck checker(me, dist);

            uint32_t count = 0;
            auto countPlayers = [&count](Player*)
            {
                count++;
            };

            me->VisitAnyNearbyObject<Player, Trinity::FunctionAction>(dist, countPlayers, checker);

            return count;
        }

        Player* GetRandomPlayer(const float dist)
        {
            std::list<Player*> players;
            Trinity::AnyPlayerInObjectRangeCheck checker(me, dist);
            me->VisitAnyNearbyObject<Player, Trinity::ContainerAction>(dist, players, checker);

            if (!players.empty())
            {
                std::list<Player*>::const_iterator itr = players.begin();
                advance(itr, urand(0,players.size()-1));

                return (*itr);
            }
            return NULL;
        }

        void EventBegin()
        {
            CountPlayersEvent = PlayersCountRange(300.0f);

            if (!CountPlayersEvent)
                return;

            events.Reset();
            events.ScheduleEvent(EVENT_COUNT_PLAYERS, 1*MINUTE*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CHECK_COMPLETE, 1*MINUTE*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CONFLAGRATION, urand(40, 80)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_HALFWAY, (2*MINUTE+30)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_YELL_NEAR_END, (4*MINUTE+30)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_END, (5*MINUTE+30)*IN_MILLISECONDS);

            std::list<Creature*> FireList;
            me->GetCreatureListWithEntryInGrid(FireList, NPC_HEADLESS_HORSEMAN_FIRE_DND, 200.0f);

            if(!FireList.empty())
                for (std::list<Creature*>::const_iterator itr = FireList.begin(); itr != FireList.end(); ++itr)
                    if (Creature* fire = (*itr))
                        fire->AI()->DoAction(ACTION_RESET_OR_PASS_EVENT);
            if (Creature* horseman = me->SummonCreature(NPC_SHADE_OF_THE_HORSEMAN, 0.0f, 0.0f, 0.0f))
                horseman->AI()->Talk(YELL_EVENT_START);
        }

        bool CheckEvent()
        {
            std::list<Creature*> FireList;
            me->GetCreatureListWithEntryInGrid(FireList, NPC_HEADLESS_HORSEMAN_FIRE_DND, 200.0f);
            for (std::list<Creature*>::const_iterator itr = FireList.begin(); itr != FireList.end(); ++itr)
                if (Creature* fire = (*itr))
                    if (fire->AI()->GetData(DATA_FIRE_STATUS))
                        return false;
            return true;
        }

        void EventComplete(float dist)
        {
            std::list<Player*> players;
            Trinity::AnyPlayerInObjectRangeCheck checker(me, dist);
            me->VisitAnyNearbyObject<Player, Trinity::ContainerAction>(dist, players, checker);

            for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* player = *itr;
                if (player->GetQuestStatus(QUEST_LET_THE_FIRES_COME_A) == QUEST_STATUS_INCOMPLETE ||
                    player->GetQuestStatus(QUEST_LET_THE_FIRES_COME_H) == QUEST_STATUS_INCOMPLETE ||
                    player->GetQuestStatus(QUEST_STOP_FIRES_A) == QUEST_STATUS_INCOMPLETE ||
                    player->GetQuestStatus(QUEST_STOP_FIRES_H) == QUEST_STATUS_INCOMPLETE)
                        player->CastSpell((*itr),SPELL_HEADLES_HORSEMAN_QUEST_CREDIT,true);
            }

            if (Creature* horseman = me->FindNearestCreature(NPC_SHADE_OF_THE_HORSEMAN, 300.0f, true))
                horseman->AI()->DoAction(ACTION_LAND_AND_ATTACK);

            events.Reset();
        }

        void EventEnd()
        {
            if (!CheckEvent())
            {
                if (Creature* horseman = me->FindNearestCreature(NPC_SHADE_OF_THE_HORSEMAN, 300.0f, true))
                    horseman->AI()->DoAction(ACTION_FAIL_EVENT);

                std::list<Creature*> FireList;
                me->GetCreatureListWithEntryInGrid(FireList, NPC_HEADLESS_HORSEMAN_FIRE_DND, 200.0f);
                for (std::list<Creature*>::const_iterator itr = FireList.begin(); itr != FireList.end(); ++itr)
                    if (Creature* fire = (*itr))
                        fire->AI()->DoAction(ACTION_FAIL_EVENT);
            }

            events.Reset();
        }

        void UpdateAI(uint32 diff)
        {
            if (!IsHolidayActive(HOLIDAY_HALLOWS_END))
                return;

            if (Creature* horseman = me->FindNearestCreature(NPC_SHADE_OF_THE_HORSEMAN, 300.0f, true))
            {
                events.Update(diff);
                while (int8 event = events.ExecuteEvent())
                {
                    switch (event)
                    {
                    case EVENT_COUNT_PLAYERS:
                        CountPlayersEvent = PlayersCountRange(200.0f);
                        events.ScheduleEvent(EVENT_COUNT_PLAYERS, 1*MINUTE*IN_MILLISECONDS);
                        break;
                    case EVENT_HALFWAY:
                        horseman->AI()->DoAction(ACTION_BURN_TOWN);
                        break;
                    case EVENT_YELL_NEAR_END:
                        horseman->AI()->Talk(YELL_NEAR_END);
                        break;
                    case EVENT_CONFLAGRATION:
                        horseman->AI()->Talk(YELL_CONFLAGRATE);
                        if (Player* target = GetRandomPlayer(40.0f))
                            horseman->CastSpell(target, SPELL_CONFLAGRATION, true);
                        events.ScheduleEvent(EVENT_CONFLAGRATION, urand(40, 80) * IN_MILLISECONDS);
                        break;
                    case EVENT_CHECK_COMPLETE:
                        if (CheckEvent())
                            EventComplete(100.0f);
                        else
                            events.ScheduleEvent(EVENT_CHECK_COMPLETE, 5*IN_MILLISECONDS);
                        break;
                    case EVENT_END:
                        EventEnd();
                        break;
                    }
                }
            }
        }
    };
};

// http://www.wowhead.com/npc=23537
// Headless Horseman - Fire (DND)
class npc_headless_horseman_fire : public CreatureScript
{
public:
    npc_headless_horseman_fire() : CreatureScript("npc_headless_horseman_fire") { }

    CreatureAI* GetAI(Creature *creature) const
    {
        return new npc_headless_horseman_fireAI(creature);
    }

    struct npc_headless_horseman_fireAI : public ScriptedAI
    {
        npc_headless_horseman_fireAI(Creature* c) : ScriptedAI(c)
        {
            FireRank[0] = SPELL_HEADLESS_HORSEMAN_BURNING;
            FireRank[1] = SPELL_HEADLESS_HORSEMAN_FIRE;
            FireRank[2] = SPELL_HEADLESS_HORSEMAN_VISUAL_FIRE;
        }

        uint32 FireRank[3];
        uint32 RateFire;
        uint32 IncreaseFireTimer;
        uint32 fireJumpTimer;
        uint32 ImmunedTimer;
        bool Fire;
        bool Immuned;
        bool Jumped;
        uint8 curFireRank;

        void Reset()
        {
            fireJumpTimer = 0;
            Fire = false;
            Immuned = false;
            Jumped = false;
            me->RemoveAllAuras();
            curFireRank = 0;
            ImmunedTimer = 5000;
        }

        uint32 GetData(uint32 data ) const
        {
            if (data == DATA_FIRE_STATUS)
                return Fire;
            else if (DATA_IMMUNE_STATUS)
                return Immuned;
            return 0;
        }

        void AddFireAura(bool again = false)
        {
            if (me->HasAura(FireRank[0]) || me->HasAura(FireRank[1]) || me->HasAura(FireRank[2]) || Immuned)
                return;

            if (!again)
            {
                switch (urand(0, 10))
                {
                case 0: case 1: case 2: case 3: case 4: case 5:
                    me->AddAura(FireRank[0], me);
                    curFireRank = 0;
                    break;
                case 6: case 7: case 8:
                    me->AddAura(FireRank[1], me);
                    curFireRank = 1;
                    break;
                case 9: case 10:
                    me->AddAura(FireRank[2], me);
                    curFireRank = 2;
                    break;
                default:
                    me->AddAura(FireRank[0], me);
                    curFireRank = 0;
                    break;
                }
                Immuned = true; //set immuned for 5 sec to spread the fires
                ImmunedTimer = 5000;
            }
            else
            {
                me->AddAura(FireRank[0], me);
                curFireRank = 0;
            }

            Fire = true;
            if (Creature* bunny = me->FindNearestCreature(NPC_FIRE_EVENT_GENERATOR, 300.0f, true))
                RateFire = bunny->AI()->GetData(0);
            if (!RateFire)
                RateFire = 1;

            fireJumpTimer = 15000 / RateFire;
            if (fireJumpTimer < 5000)
                fireJumpTimer = 5000;
            IncreaseFireTimer = 60000 / RateFire;
            if (IncreaseFireTimer < 20000)
                IncreaseFireTimer = 20000;

            return;
        }

        Creature* FireNodeNext(float range = 2.0f)
        {
           std::list<Creature*> FireNodsList;
           FireNodsList.clear();
           me->GetCreatureListWithEntryInGrid(FireNodsList, NPC_HEADLESS_HORSEMAN_FIRE_DND, range);

           if (!FireNodsList.empty())
           {
               FireNodsList.sort(Trinity::ObjectDistanceOrderPred(me));

               for (std::list<Creature*>::const_iterator itr = FireNodsList.begin(); itr != FireNodsList.end(); ++itr)
                   if (Creature* pNodeFire = *itr)
                       if (!pNodeFire->AI()->GetData(DATA_FIRE_STATUS) && !pNodeFire->AI()->GetData(DATA_IMMUNE_STATUS))
                           return pNodeFire;
           }
           return NULL;
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_FAIL_EVENT:
                case ACTION_RESET_OR_PASS_EVENT:
                    Reset();
                    break;
                case ACTION_FIRE_AGAIN:
                    AddFireAura(true);
                    break;
                case ACTION_FIRE_JUMP:
                    AddFireAura(false);
                    break;
                case ACTION_INITIAL_START:
                    AddFireAura(false);
                    fireJumpTimer = 5000;
                    IncreaseFireTimer = 5000;
                    break;
            }
        }

        void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
        {
            if (!Fire || Immuned)
                return;

            if (Player* player = ObjectAccessor::GetPlayer(*me, guid))
                if (player->GetQuestStatus(QUEST_FIRE_BRIGADE_PRACTICE_1) == QUEST_STATUS_INCOMPLETE ||
                    player->GetQuestStatus(QUEST_FIRE_BRIGADE_PRACTICE_2) == QUEST_STATUS_INCOMPLETE ||
                    player->GetQuestStatus(QUEST_FIRE_BRIGADE_PRACTICE_3) == QUEST_STATUS_INCOMPLETE ||
                    player->GetQuestStatus(QUEST_FIRE_TRAINING_1) == QUEST_STATUS_INCOMPLETE ||
                    player->GetQuestStatus(QUEST_FIRE_TRAINING_2) == QUEST_STATUS_INCOMPLETE ||
                    player->GetQuestStatus(QUEST_FIRE_TRAINING_3) == QUEST_STATUS_INCOMPLETE)
                        player->KilledMonsterCredit(NPC_HEADLESS_HORSEMAN_FIRE_DND,player->GetGUID());

            me->CastSpell(me,SPELL_WATER_SPOUT_VISUAL,true);

            if (curFireRank)
            {
                me->RemoveAura(FireRank[curFireRank]);
                --curFireRank;
                me->AddAura(FireRank[curFireRank],me);
            }
            else
            {
                me->RemoveAllAuras();
                Immuned = true;
                Fire = false;
                ImmunedTimer = 5000;
            }
            return;
        }

        void UpdateAI(uint32 diff)
        {
            if (Immuned)
            {
                if (ImmunedTimer <= diff)
                {
                    Immuned = false;
                    ImmunedTimer = 5000;
                }
                else ImmunedTimer -= diff;
            }

            if (Fire && !Jumped && fireJumpTimer <= diff)
            {
                if (Creature* fire = FireNodeNext(5.0f))
                    fire->AI()->DoAction(ACTION_FIRE_JUMP);

                if (Creature* bunny = me->FindNearestCreature(NPC_FIRE_EVENT_GENERATOR, 300.0f, true))
                    RateFire = bunny->AI()->GetData(0);
                if (!RateFire)
                    RateFire = 1;
                Jumped = true;
                fireJumpTimer = 15000 / RateFire;
                if (fireJumpTimer < 5000)
                    fireJumpTimer = 5000;
            } else fireJumpTimer -= diff;

            if (Fire && !Immuned)
            {
                if (Creature* bunny = me->FindNearestCreature(NPC_FIRE_EVENT_GENERATOR, 300.0f, true))
                    RateFire = bunny->AI()->GetData(0);
                if (!RateFire)
                    RateFire = 1;

                if (IncreaseFireTimer <= diff)
                {
                    if (curFireRank < 2)
                    {
                        me->RemoveAura(FireRank[curFireRank]);
                        curFireRank++;
                        me->AddAura(FireRank[curFireRank],me);
                    }
                    else
                    {
                        if (Creature* fire = FireNodeNext(5.0f))
                            fire->AI()->DoAction(ACTION_FIRE_AGAIN);
                    }
                    IncreaseFireTimer = 60000 / RateFire;
                    if (IncreaseFireTimer < 20000)
                        IncreaseFireTimer = 20000;
                } else
                    IncreaseFireTimer -= diff;
            }
        }
    };
};

// http://www.wowhead.com/npc=23543
// Shade of the Horseman

class npc_shade_of_the_horseman : public CreatureScript
{
public:
    npc_shade_of_the_horseman() : CreatureScript("npc_shade_of_the_horseman") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_shade_of_the_horsemanAI(creature);
    }

    struct npc_shade_of_the_horsemanAI : public ScriptedAI
    {
        npc_shade_of_the_horsemanAI(Creature* c) : ScriptedAI(c)
        {
            area = me->GetAreaId();
            waypointId = 235430;
            switch(area){
                //case 87: waypointId += 0; break;
                case 131: waypointId += 1; break;
                case 3576: waypointId += 2; break;
                case 362: waypointId += 3; break;
                case 159: waypointId +=4; break;
                case 3665: waypointId +=5; break;
            }
            
            flying = true;
            burnTown = true;

            FlyMode();
            me->GetMotionMaster()->MovePath(waypointId, true);
        }

        uint32 area;
        uint32 waypointId;

        bool flying;
        bool burnTown;

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_FAIL_EVENT:
                    Talk(YELL_FAIL_EVENT);
                    me->DespawnOrUnsummon();
                    break;
                case ACTION_BURN_TOWN:
                    Talk(YELL_HALF_EVENT);
                    burnTown = true;
                    break;
                case ACTION_LAND_AND_ATTACK:
                    me->SetLevel(11);
                    me->GetMotionMaster()->Clear(false);
                    Talk(YELL_COMPLETE_EVENT);
                    if (Creature* bunny = me->FindNearestCreature(NPC_FIRE_EVENT_GENERATOR, 300.0f, true))
                        me->GetMotionMaster()->MovePoint(0, bunny->GetPositionX(), bunny->GetPositionY(), bunny->GetPositionZ());
                    else
                        AttackMode();
                    break;
            }
        }

        void FlyMode()
        {
            me->SetLevel(85); // To be able to hit players with conflagration
            flying = true;
            burnTown = true;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
			me->SetImmuneToAll(true);
            me->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->Mount(MOUNT_SHADE_HORSE);
            me->SetSpeedRate(MOVE_WALK, 5.0f);
        }

        void AttackMode()
        {
            me->SetLevel(11);
            flying = false;
            burnTown = false;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
			me->SetImmuneToAll(false);
            me->RemoveUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
            me->SetCanFly(false);
            me->SetDisableGravity(false);
            me->Dismount();
            me->SetReactState(REACT_AGGRESSIVE);
            if (Player* target = me->FindNearestPlayer(40.0f))
                AttackStart(target);
        }

        void CreateFires()
        {
            std::list<Creature*> FireNodsList;
            FireNodsList.clear();

            me->GetCreatureListWithEntryInGrid(FireNodsList, NPC_FIRE_HELPER, 200.0f);

            for (std::list<Creature*>::const_iterator itr = FireNodsList.begin(); itr != FireNodsList.end(); ++itr)
                me->CastSpell((*itr), SPELL_HEADLESS_HORSEMAN_START_FIRE, true);
        }

        void JustDied(Unit* /*Killer*/)
        {
            Talk(YELL_HORSEMAN_DIE);
            me->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 1.0f, SPELL_HEADLESS_HORSEMAN_LARGE_JACK, true);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE && id == 0)
            {
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                AttackMode();
            }

            if (type != WAYPOINT_MOTION_TYPE)
                return;

            if (id == 0)
            {
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            }
            else
            {
                if (burnTown && ReadyToCastSpell(id))
                {
                    CreateFires();
                    burnTown = false;
                }
            }
        }

        bool ReadyToCastSpell(uint32 id)
        {
            switch(me->GetAreaId())
            {
                case 87: return id == 9;
                case 131: return id == 11;
                case 3576: return id == 7;
                case 362: return id == 3;
                case 159: return id == 4;
                case 3665: return id == 5;
                default: return false;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                    return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_hallows_end_fire_helper : public CreatureScript
{
public:
    npc_hallows_end_fire_helper() : CreatureScript("npc_hallows_end_fire_helper") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_hallows_end_fire_helperAI(creature);
    }

    struct npc_hallows_end_fire_helperAI : public ScriptedAI
    {
        npc_hallows_end_fire_helperAI(Creature* c) : ScriptedAI(c)
        {
            me->ApplySpellImmune(42971, IMMUNITY_ID, 42971, true); // do not burn the trigger, looks confusing for players (still something that burns)
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_HEADLESS_HORSEMAN_START_FIRE)
            {
                if (Creature* fire = me->FindNearestCreature(NPC_HEADLESS_HORSEMAN_FIRE_DND, 100.0f, true))
                    fire->AI()->DoAction(ACTION_INITIAL_START);

                me->RemoveAura(SPELL_HEADLESS_HORSEMAN_START_FIRE);
            }
            return;
        }

    };
};


class item_water_bucket : public ItemScript
{
    public:

        item_water_bucket() : ItemScript("item_water_bucket") { }

        bool OnUse(Player* player, Item* item, SpellCastTargets const& targets)
        {
            if (targets.GetDist2d() > 30)
            {
                player->SendEquipError(EQUIP_ERR_OUT_OF_RANGE, item);
                return true;
            }

            if (Creature* dummy = player->SummonCreature(NPC_BUCKET_DUMMY, targets.GetDst()->_position.GetPositionX(), targets.GetDst()->_position.GetPositionY(), targets.GetDst()->_position.GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 500))
            {
                if (Creature* fire_training_trigger = dummy->FindNearestCreature(NPC_FIRE_TRAINING_TRIGGER, 3.0f, true))
                    fire_training_trigger->AI()->SetGuidData(player->GetGUID(), EVENT_FIRE_HIT_BY_BUCKET);

                std::list<Creature*> firesList;
                Trinity::AllCreaturesOfEntryInRange checker(dummy, NPC_HEADLESS_HORSEMAN_FIRE_DND, 3.0f);
                dummy->VisitAnyNearbyObject<Creature, Trinity::ContainerAction>(3.0f, firesList, checker);

                if (firesList.empty())
                {
                    // Just some extra checks...
                    Creature* fire = dummy->FindNearestCreature(NPC_HEADLESS_HORSEMAN_FIRE_DND, 3.0f, true);
                    if (fire && fire->IsAlive())
                        fire->AI()->SetGuidData(player->GetGUID(), EVENT_FIRE_HIT_BY_BUCKET);

                    else if (Player* friendPlr = dummy->FindNearestPlayer(3.0f))
                    {
                        if (friendPlr->IsFriendlyTo(player) && friendPlr->IsAlive())
                        {
                            player->CastSpell(friendPlr, SPELL_CREATE_WATER_BUCKET, true);
                            player->RemoveAura(SPELL_CONFLAGRATION);
                            player->RemoveAura(42381);
                            player->CastSpell(friendPlr, SPELL_WATER_SPOUT_VISUAL, true);
                        }
                    }
                    else
                        return false;
                }

                firesList.sort(Trinity::ObjectDistanceOrderPred(dummy));

                int8 max_targets = firesList.size() > 2 ? 2 : firesList.size();

                while (max_targets)
                {
                    if (Creature* fire = *(firesList.begin()))
                    {
                        fire->AI()->SetGuidData(player->GetGUID(), EVENT_FIRE_HIT_BY_BUCKET);
                        firesList.erase(firesList.begin());
                    }
                    --max_targets;
                }

            return false;
           }

            return true;
        }
};

// 24750 Trick
enum eTrickSpells
{
    SPELL_PIRATE_COSTUME_MALE           = 24708,
    SPELL_PIRATE_COSTUME_FEMALE         = 24709,
    SPELL_NINJA_COSTUME_MALE            = 24710,
    SPELL_NINJA_COSTUME_FEMALE          = 24711,
    SPELL_LEPER_GNOME_COSTUME_MALE      = 24712,
    SPELL_LEPER_GNOME_COSTUME_FEMALE    = 24713,
    SPELL_SKELETON_COSTUME              = 24723,
    SPELL_GHOST_COSTUME_MALE            = 24735,
    SPELL_GHOST_COSTUME_FEMALE          = 24736,
    SPELL_TRICK_BUFF                    = 24753,
};

class spell_halloween_wand : public SpellScriptLoader
{
public:
    spell_halloween_wand() : SpellScriptLoader("spell_halloween_wand") {}

    class spell_halloween_wand_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_halloween_wand_SpellScript)

        bool Validate(SpellInfo const* /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_PIRATE_COSTUME_MALE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_PIRATE_COSTUME_FEMALE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_NINJA_COSTUME_MALE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_NINJA_COSTUME_FEMALE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_LEPER_GNOME_COSTUME_MALE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_LEPER_GNOME_COSTUME_FEMALE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_GHOST_COSTUME_MALE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_GHOST_COSTUME_FEMALE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_HALLOWEEN_WAND_PIRATE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_HALLOWEEN_WAND_NINJA))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_HALLOWEEN_WAND_LEPER_GNOME))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_HALLOWEEN_WAND_RANDOM))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_HALLOWEEN_WAND_SKELETON))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_HALLOWEEN_WAND_WISP))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_HALLOWEEN_WAND_GHOST))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_HALLOWEEN_WAND_BAT))
                return false;
            return true;
        }

        void HandleScriptEffect()
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitPlayer();

            if (!caster || !target)
                return;

            uint32 spellId = 0;
            uint8 gender = target->getGender();

            switch (GetSpellInfo()->Id)
            {
                case SPELL_HALLOWEEN_WAND_LEPER_GNOME:
                    spellId = gender ? SPELL_LEPER_GNOME_COSTUME_FEMALE : SPELL_LEPER_GNOME_COSTUME_MALE;
                    break;
                case SPELL_HALLOWEEN_WAND_PIRATE:
                    spellId = gender ? SPELL_PIRATE_COSTUME_FEMALE : SPELL_PIRATE_COSTUME_MALE;
                    break;
                case SPELL_HALLOWEEN_WAND_GHOST:
                    spellId = gender ? SPELL_GHOST_COSTUME_FEMALE : SPELL_GHOST_COSTUME_MALE;
                    break;
                case SPELL_HALLOWEEN_WAND_NINJA:
                    spellId = gender ? SPELL_NINJA_COSTUME_FEMALE : SPELL_NINJA_COSTUME_MALE;
                    break;
                case SPELL_HALLOWEEN_WAND_RANDOM:
                    spellId = RAND(SPELL_HALLOWEEN_WAND_PIRATE, SPELL_HALLOWEEN_WAND_NINJA, SPELL_HALLOWEEN_WAND_LEPER_GNOME, SPELL_HALLOWEEN_WAND_SKELETON, SPELL_HALLOWEEN_WAND_WISP, SPELL_HALLOWEEN_WAND_GHOST, SPELL_HALLOWEEN_WAND_BAT);
                    break;
            }
            caster->CastSpell(target, spellId, true);
        }

        void Register()
        {
            AfterHit += SpellHitFn(spell_halloween_wand_SpellScript::HandleScriptEffect);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_halloween_wand_SpellScript();
    }
};

class at_wickerman_festival : public AreaTriggerScript
{
    public:
        at_wickerman_festival() : AreaTriggerScript("at_wickerman_festival") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/)
        {
            player->GroupEventHappens(QUEST_CRASHING_WICKERMAN_FESTIVAL, player);
            return true;
        }
};


class npc_fire_training_trigger : public CreatureScript
{
public:
    npc_fire_training_trigger() : CreatureScript("npc_fire_training_trigger") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_fire_training_triggerAI(creature);
    }

    struct npc_fire_training_triggerAI : public ScriptedAI
    {
        npc_fire_training_triggerAI(Creature* c) : ScriptedAI(c) { }

        void Reset()
        {
            checkBurnTimer = 0;
            Fire = false;
        }


        void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
        {
            if (Fire)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, guid))
                    if (player->GetQuestStatus(QUEST_FIRE_BRIGADE_PRACTICE_1) == QUEST_STATUS_INCOMPLETE ||
                        player->GetQuestStatus(QUEST_FIRE_BRIGADE_PRACTICE_2) == QUEST_STATUS_INCOMPLETE ||
                        player->GetQuestStatus(QUEST_FIRE_BRIGADE_PRACTICE_3) == QUEST_STATUS_INCOMPLETE ||
                        player->GetQuestStatus(QUEST_FIRE_TRAINING_1) == QUEST_STATUS_INCOMPLETE ||
                        player->GetQuestStatus(QUEST_FIRE_TRAINING_2) == QUEST_STATUS_INCOMPLETE ||
                        player->GetQuestStatus(QUEST_FIRE_TRAINING_3) == QUEST_STATUS_INCOMPLETE)
                            player->KilledMonsterCredit(NPC_HEADLESS_HORSEMAN_FIRE_DND, guid);

                DoCastSelf(SPELL_WATER_SPOUT_VISUAL, true);

                if (GameObject* effigy = me->FindNearestGameObject(186720, 1.0f))
                    effigy->SetGoState(GO_STATE_READY);
                Fire = false;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!Fire)
            {
                if (checkBurnTimer <= diff)
                {
                    if (GameObject* effigy = me->FindNearestGameObject(186720, 1.0f))
                        effigy->SetGoState(GO_STATE_ACTIVE);
                    Fire = true;
                    checkBurnTimer = 30000;
                } else checkBurnTimer -= diff;
            }
        }


    private:
        uint64 checkBurnTimer;
        bool Fire;
    };
};

class npc_halloween_orphan_matron : public CreatureScript
{
public:
    npc_halloween_orphan_matron() : CreatureScript("npc_halloween_orphan_matron") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* Quest)
    {
        if (Quest->GetQuestId() == QUEST_LET_THE_FIRES_COME_A || Quest->GetQuestId() == QUEST_LET_THE_FIRES_COME_H)
            if(Creature* bunny = creature->FindNearestCreature(NPC_FIRE_EVENT_GENERATOR, 300.0f))
                bunny->AI()->DoAction(ACTION_INITIAL_START);
        return true;
    }

    bool OnGossipHello(Player* player, Creature* me)
    {
        player->PrepareQuestMenu(me->GetGUID());
        
        QuestMenu &qm = player->PlayerTalkClass->GetQuestMenu();
        QuestMenu qm2;

        uint32 quest1 = player->GetTeam() == ALLIANCE ? QUEST_LET_THE_FIRES_COME_A : QUEST_LET_THE_FIRES_COME_H;
        uint32 quest2 = player->GetTeam() == ALLIANCE ? QUEST_STOP_FIRES_A : QUEST_STOP_FIRES_H;

        // Copy current quest menu ignoring some quests
        for (uint32 i = 0; i<qm.GetMenuItemCount(); ++i)
        {
            if (qm.GetItem(i).QuestId == quest1 || qm.GetItem(i).QuestId == quest2)
                continue;

            qm2.AddMenuItem(qm.GetItem(i).QuestId, qm.GetItem(i).QuestIcon);
        }
        
        bool eventIsActive = me->FindNearestCreature(NPC_SHADE_OF_THE_HORSEMAN, 300.0f) != nullptr;

        // Player must have completed pre-quest "Fire Training"
        if ((player->GetQuestStatus(QUEST_FIRE_BRIGADE_PRACTICE_1) == QUEST_STATUS_REWARDED) ||
            (player->GetQuestStatus(QUEST_FIRE_BRIGADE_PRACTICE_2) == QUEST_STATUS_REWARDED) ||
            (player->GetQuestStatus(QUEST_FIRE_BRIGADE_PRACTICE_3) == QUEST_STATUS_REWARDED) ||
            (player->GetQuestStatus(QUEST_FIRE_TRAINING_1) == QUEST_STATUS_REWARDED) ||
            (player->GetQuestStatus(QUEST_FIRE_TRAINING_2) == QUEST_STATUS_REWARDED) ||
            (player->GetQuestStatus(QUEST_FIRE_TRAINING_3) == QUEST_STATUS_REWARDED))
        {
            if (player->GetQuestStatus(quest1) == QUEST_STATUS_COMPLETE)
                qm2.AddMenuItem(quest1, 4);
            else if (player->GetQuestStatus(quest2) == QUEST_STATUS_COMPLETE)
                qm2.AddMenuItem(quest2, 4);
            else
            {
                if (player->CanTakeQuest(sObjectMgr->GetQuestTemplate(quest1), false)
                    && player->CanTakeQuest(sObjectMgr->GetQuestTemplate(quest2), false))
                    qm2.AddMenuItem(eventIsActive ? quest2 : quest1, 2);
            }
        }

        qm.ClearMenu();

        for (uint32 i = 0; i<qm2.GetMenuItemCount(); ++i)
            qm.AddMenuItem(qm2.GetItem(i).QuestId, qm2.GetItem(i).QuestIcon);

        uint32 textId;

        if (eventIsActive)
            textId = TEXT_FIRE_EVENT_ACTIVE;
        else
        {
            textId = TEXT_FIRE_EVENT_INACTIVE;
            player->ADD_GOSSIP_ITEM_DB(GOSSIP_MATRON_MENU_ID, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        player->SEND_GOSSIP_MENU(textId, me->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* me, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            player->SEND_GOSSIP_MENU(TEXT_HEADLESS_HORSEMEN, me->GetGUID());
        }
        return true;
    }
};

class npc_halloween_orphan : public CreatureScript
{
public:
    npc_halloween_orphan() : CreatureScript("npc_halloween_orphan") { }

    bool OnGossipHello(Player* player, Creature* me)
    {
        uint32 menuId = me->FindNearestCreature(NPC_SHADE_OF_THE_HORSEMAN, 300.0f) ? GOSSIP_ORPHAN_EVENT_ACTIVE : GOSSIP_ORPHAN_EVENT_INACTIVE;
        player->PrepareGossipMenu(me, menuId);

        // Select random text
        GossipMenusMapBounds menuBounds = sObjectMgr->GetGossipMenusMapBounds(menuId);
        std::vector<uint32> textIds;

        for (GossipMenusContainer::const_iterator itr = menuBounds.first; itr != menuBounds.second; ++itr)
            textIds.push_back(itr->second.text_id);

        uint32 textId = Trinity::Containers::SelectRandomContainerElement(textIds);

        player->SEND_GOSSIP_MENU(textId, me->GetGUID());

        return true;
    }
};

enum PumpkinTreats
{
    SPELL_PIRATE            = 24717, //WRONG spell! the original doesnt work
    SPELL_GHOST             = 24927,
    SPELL_ORANGE_GIANT      = 24924,
    SPELL_SKELETON          = 24925
};


class item_pumpkin_treat : public ItemScript
{
    public:

        item_pumpkin_treat() : ItemScript("item_pumpkin_treat") { }

        bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& /*targets*/)
        {

            if (player->HasAura(SPELL_PIRATE) || player->HasAura(24709) || player->HasAura(24708))
            {
                player->RemoveAura(24709);
                player->RemoveAura(24708);
                player->RemoveAura(SPELL_PIRATE);
            }

            if (player->HasAura(SPELL_GHOST))
                player->RemoveAura(SPELL_GHOST);

            if (player->HasAura(SPELL_ORANGE_GIANT))
                player->RemoveAura(SPELL_ORANGE_GIANT);

            if (player->HasAura(SPELL_SKELETON))
                player->RemoveAura(SPELL_SKELETON);

            switch (urand(0, 3))
            {
                case 0:
                    player->CastSpell(player, SPELL_PIRATE, true);
                    break;
                case 1:
                    player->CastSpell(player, SPELL_GHOST, true);
                    break;
                case 2:
                    player->CastSpell(player, SPELL_ORANGE_GIANT, true);
                    break;
                case 3:
                    player->CastSpell(player, SPELL_SKELETON, true);
                    break;
                default:
                    break;
            }

           return false;
        }
};

enum Spells
{
    SPELL_TRICK_OR_TREATED = 24755,
    SPELL_TREAT = 24715
};

#define LOCALE_TRICK_OR_TREAT_0 "Trick or Treat!"
#define LOCALE_TRICK_OR_TREAT_2 "Des bonbons ou des blagues!"
#define LOCALE_TRICK_OR_TREAT_3 "Suesses oder Saures!"
#define LOCALE_TRICK_OR_TREAT_6 "Truco o trato!"

#define LOCALE_INNKEEPER_0 "Make this inn my home."
#define LOCALE_INNKEEPER_3 "Ich moechte dieses Gasthaus zu meinem Heimatort machen."
#define LOCALE_GOSSIP_TEXT_BROWSE_GOODS "Ich sehe mich nur mal um."

class npc_innkeeper : public CreatureScript
{
public:
    npc_innkeeper() : CreatureScript("npc_innkeeper") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (IsHolidayActive(HOLIDAY_HALLOWS_END) && !player->HasAura(SPELL_TRICK_OR_TREATED))
        {
            const char* localizedEntry;
            switch (player->GetSession()->GetSessionDbcLocale())
            {
                case LOCALE_frFR: 
                    localizedEntry = LOCALE_TRICK_OR_TREAT_2; 
                    break;
                case LOCALE_deDE: 
                    localizedEntry = LOCALE_TRICK_OR_TREAT_3; 
                    break;
                case LOCALE_esES: 
                    localizedEntry = LOCALE_TRICK_OR_TREAT_6; 
                    break;
                case LOCALE_enUS: 
                default: 
                    localizedEntry = LOCALE_TRICK_OR_TREAT_0;
            }
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, localizedEntry, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (creature->IsVendor())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, player->GetOptionTextWithEntry(GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_TEXT_BROWSE_GOODS_ID), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        if (creature->IsInnkeeper())
        {
            const char* localizedEntry;
            switch (player->GetSession()->GetSessionDbcLocale())
            {
                case LOCALE_deDE: 
                    localizedEntry = LOCALE_INNKEEPER_3; 
                    break;
                case LOCALE_enUS: 
                default: 
                    localizedEntry = LOCALE_INNKEEPER_0;
            }
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, localizedEntry, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INN);
        }

        player->TalkedToCreature(creature->GetEntry(), creature->GetGUID());
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF + 1 && IsHolidayActive(HOLIDAY_HALLOWS_END) && !player->HasAura(SPELL_TRICK_OR_TREATED))
        {
            player->CastSpell(player, SPELL_TRICK_OR_TREATED, true);

            if (urand(0, 1))
                player->CastSpell(player, SPELL_TREAT, true);
            else
            {
                uint32 trickspell = 0;
                switch (urand(0, 13))
                {
                    case 0: trickspell = 24753; break; // cannot cast, random 30sec
                    case 1: trickspell = 24713; break; // lepper gnome costume
                    case 2: trickspell = 24735; break; // male ghost costume
                    case 3: trickspell = 24736; break; // female ghostcostume
                    case 4: trickspell = 24710; break; // male ninja costume
                    case 5: trickspell = 24711; break; // female ninja costume
                    case 6: trickspell = 24708; break; // male pirate costume
                    case 7: trickspell = 24709; break; // female pirate costume
                    case 8: trickspell = 24723; break; // skeleton costume
                    case 9: trickspell = 24753; break; // Trick
                    case 10: trickspell = 24924; break; // Hallow's End Candy
                    case 11: trickspell = 24925; break; // Hallow's End Candy
                    case 12: trickspell = 24926; break; // Hallow's End Candy
                    case 13: trickspell = 24927; break; // Hallow's End Candy
                }
                player->CastSpell(player, trickspell, true);
            }
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        player->CLOSE_GOSSIP_MENU();

        switch (action)
        {
            case GOSSIP_ACTION_TRADE: 
                player->GetSession()->SendListInventory(creature->GetGUID()); 
                break;
            case GOSSIP_ACTION_INN: 
                player->SetBindPoint(creature->GetGUID());
                break;
        }
        return true;
    }
};

void AddSC_event_hallows_end()
{
    new spell_halloween_wand();

    new item_water_bucket();
    new item_pumpkin_treat();

    new at_wickerman_festival();

    new go_wickerman_ember();
    new npc_wickerman_guardian();
    new npc_he_banshee_queen();

    new npc_hallowend();
    new npc_headless_horseman_fire();
    new npc_shade_of_the_horseman();
    new npc_wickerman_event_generator();
    new npc_hallows_end_fire_helper();
    new npc_fire_training_trigger();
    new npc_halloween_orphan_matron();
    new npc_halloween_orphan();
    new npc_innkeeper();
}
