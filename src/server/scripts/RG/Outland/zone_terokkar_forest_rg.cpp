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
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"

enum manabomb_questdata
{
    SPELL_BOMB_EXPLOSION  = 35513, //Kills all enemies in a 45 yard radius.  BOOM!

    ITEM_BOMB_CODE        = 29912,
    NPC_QUEST_CREDIT      = 21039,
    NPC_EXPLOSION_TRIGGER = 20767,

    ACTION_BOMB           = 1,
};

class npc_explosion_trigger : public CreatureScript
{
public:
    npc_explosion_trigger() : CreatureScript("npc_explosion_trigger") { }

    struct npc_explosion_triggerAI : public ScriptedAI
    {
        npc_explosion_triggerAI(Creature* creature) : ScriptedAI(creature) { }

        bool _bombisActive;
        uint32 boomTimer;

            void Reset()
            {
                boomTimer = 13000;
                _bombisActive = false;
            }

            void DoAction(int32 action)
            {
                if (action == ACTION_BOMB)
                _bombisActive = true;
            }

            void UpdateAI(uint32 diff)
            {
                if(_bombisActive)
                {
                    if (boomTimer < diff)
                    {
                        me->CastSpell((Unit*) NULL, SPELL_BOMB_EXPLOSION, true);
                        boomTimer = 13000;
                        _bombisActive = false;
                    }
                    else boomTimer -= diff;
                }
            }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_explosion_triggerAI(creature);
    }
};

enum ancientSkullPileData
{
    ITEM_TIMELOST_OFFERING                  = 32720,
    NPC_TEROKK                              = 21838,
    QUEST_TEROKK_DOWNFALL                   = 11073,
    
    SPELL_CLEAVE                            = 15284,
    SPELL_DIVINE_SHIELD                     = 40733,
    SPELL_FRENZY                            = 28747,
    SPELL_SHADOW_BOLT_VOLLEY                = 40721,
    SPELL_WILL_OF_THE_ARAKKOA               = 40722,
    SPELL_VISUAL_MARKER                     = 40656,
    
    NPC_MARKER_TRIGGER                      = 97016,
    
    SAY_SUMMONED                            = 0,
    SAY_CHOSEN                              = 1,
    SAY_IMMUNE                              = 2,
    
    EVENT_CLEAVE                            = 0,
    EVENT_VOLLEY                            = 1,
    EVENT_MARKER                            = 2,
    EVENT_SHIELD                            = 3,
};

class npc_terokk : public CreatureScript
{
    public:
        npc_terokk() : CreatureScript("npc_terokk") {}

        struct npc_terokkAI : public ScriptedAI
        {
            npc_terokkAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                Events.Reset();
                isChosen = false;
                isImmune = false;
                isFirst = false;
            }

            void EnterCombat (Unit* /*who*/)
            {
                Talk(SAY_SUMMONED);
                
                Events.ScheduleEvent(EVENT_CLEAVE, 5000);
                Events.ScheduleEvent(EVENT_VOLLEY, 3000);
                Events.ScheduleEvent(EVENT_MARKER, 15000);
            }

            void UpdateAI (uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                Events.Update(diff);

                while (uint32 EventId = Events.ExecuteEvent())
                {
                    switch (EventId)
                    {
                       case EVENT_CLEAVE:
                            DoCastVictim(SPELL_CLEAVE);
                            Events.ScheduleEvent(EVENT_CLEAVE, urand (5000, 9000));
                            break;
                       case EVENT_VOLLEY:
                            DoCastVictim(SPELL_SHADOW_BOLT_VOLLEY);
                            Events.ScheduleEvent(EVENT_VOLLEY, urand (9000, 12000));
                            break;
                       case EVENT_MARKER:
                            switch (urand(1,4))
                            {
                                case 1:
                                    if (Creature* trigger = me->SummonCreature(NPC_MARKER_TRIGGER,me->GetPositionX()+30,me->GetPositionY(),me->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,30000))
                                    trigger->AddAura(SPELL_VISUAL_MARKER,trigger);
                                    break;
                                case 2:
                                    if (Creature* trigger = me->SummonCreature(NPC_MARKER_TRIGGER,me->GetPositionX(),me->GetPositionY()+30,me->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,30000))
                                    trigger->AddAura(SPELL_VISUAL_MARKER,trigger);
                                    break;
                                case 3:
                                    if (Creature* trigger = me->SummonCreature(NPC_MARKER_TRIGGER,me->GetPositionX()-30,me->GetPositionY(),me->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,30000))
                                    trigger->AddAura(SPELL_VISUAL_MARKER,trigger);
                                    break;
                                case 4:
                                    if (Creature* trigger = me->SummonCreature(NPC_MARKER_TRIGGER,me->GetPositionX(),me->GetPositionY()-30,me->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,30000))
                                    trigger->AddAura(SPELL_VISUAL_MARKER,trigger);
                                    break;
                            }
                            Events.ScheduleEvent(EVENT_MARKER, urand (20000, 25000));
                            break;
                        case EVENT_SHIELD:
                            isImmune = true;
                            Talk(SAY_IMMUNE);
                            DoCastSelf(SPELL_DIVINE_SHIELD);
                            break;
                    }
                }

                if (HealthBelowPct(50) && !isChosen)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    {
                        Talk(SAY_CHOSEN);
                        DoCast(target, SPELL_WILL_OF_THE_ARAKKOA);
                        isChosen = true;
                    }
                }

                if (HealthBelowPct(25) && !isImmune && !isFirst)
                {
                    Events.ScheduleEvent(EVENT_SHIELD, 500);
                    isFirst = true;
                }

                if (isImmune && me->FindNearestCreature(NPC_MARKER_TRIGGER, 2.0f, true))
                {
                    me->RemoveAura(SPELL_DIVINE_SHIELD);
                    DoCastSelf(SPELL_FRENZY);
                    isImmune = false;
                    Events.ScheduleEvent(EVENT_SHIELD, 15000);
                }

                DoMeleeAttackIfReady();
            }
            
        private :
            EventMap Events;
            bool isChosen;
            bool isImmune;
            bool isFirst;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_terokkAI(creature);
    }
};

enum sarthis_rg_Misc 
{
    QUEST_SOUL_CANNON             = 11089,
    NPC_FLAWLESS_ARCANE_ELEMENTAL = 23100,
    NPC_ACOLYTE_OF_AIR            = 23096,
    NPC_ACOLYTE_OF_WATER          = 23097,
    NPC_ACOLYTE_OF_EARTH          = 23098,
    NPC_ACOLYTE_OF_FIRE           = 23099,
    SPELL_SUMMON_ARCANE_ELEMENTAL = 40134,
    GOSSIP_MENU                   = 800
};

#define GOSSIP_ITEM "Eine markellose arkane Essenz bekommen"

class npc_sarthis_rg : public CreatureScript
{
public:
    npc_sarthis_rg() : CreatureScript("npc_sarthis_rg") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->GetQuestStatus(QUEST_SOUL_CANNON) == QUEST_STATUS_INCOMPLETE)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }
        player->SEND_GOSSIP_MENU(GOSSIP_MENU, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            (CAST_AI(npc_sarthis_rg::npc_sarthis_rgAI, creature->AI()))->Start(false, true, player->GetGUID());
        }
        return true;
    }

    struct npc_sarthis_rgAI : public npc_escortAI
    {
        npc_sarthis_rgAI(Creature* creature) : npc_escortAI(creature) { }
      
        void Reset()
        {
            npcSpawn = false;
        }

        void UpdateEscortAI(const uint32 diff)
        {
            if (npcSpawn) 
            {
                Creature* summon = ObjectAccessor::GetCreature(*me, summonedGuid);
                if (summon->isDead()) 
                {
                    SetEscortPaused(false);
                    npcSpawn = false;
                }
                if (summon->IsAlive() && !summon->IsInCombat())               
                    me->DisappearAndDie();                
            }
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 3:
                    DoCast(SPELL_SUMMON_ARCANE_ELEMENTAL);
                    break;
                case 4:
                    me->SummonCreature(NPC_ACOLYTE_OF_AIR, -2469.54f, 4700.59f, 155.85f, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    npcSpawn = true;
                    SetEscortPaused(true);
                    break;
                case 6:
                    DoCast(SPELL_SUMMON_ARCANE_ELEMENTAL);
                    break;
                case 7:
                    me->SummonCreature(NPC_ACOLYTE_OF_WATER, -2469.54f, 4700.59f, 155.85f, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    npcSpawn = true;
                    SetEscortPaused(true);
                    break;
                case 9:
                    DoCast(SPELL_SUMMON_ARCANE_ELEMENTAL);
                    break;
                case 10:
                    me->SummonCreature(NPC_ACOLYTE_OF_EARTH, -2469.54f, 4700.59f, 155.85f, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    npcSpawn = true;
                    SetEscortPaused(true);
                    break;
                case 12:
                    DoCast(SPELL_SUMMON_ARCANE_ELEMENTAL);
                    break;
                case 13:
                    me->SummonCreature(NPC_ACOLYTE_OF_FIRE, -2469.54f, 4700.59f, 155.85f, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    npcSpawn = true;
                    SetEscortPaused(true);
                    break;
                case 15:
                    DoCast(SPELL_SUMMON_ARCANE_ELEMENTAL);
                    break;
                case 16:
                    me->SummonCreature(NPC_FLAWLESS_ARCANE_ELEMENTAL, -2469.54f, 4700.59f, 155.85f, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    npcSpawn = true;
                    SetEscortPaused(true);
                    break;
            }
        }

        void JustSummoned(Creature* summoned)
        {
            summoned->AI()->AttackStart(GetPlayerForEscort());
            summonedGuid = summoned->GetGUID();
        }
    private:
        bool npcSpawn;
        ObjectGuid summonedGuid;

    };

    CreatureAI * GetAI(Creature * creature) const
    {
        return new npc_sarthis_rgAI(creature);
    }
};

/*###################
# spell_fumping_39238
# Quest 10929
####################*/
 
enum FumpingSpellData
{
    SPELL_FUMPING = 39238,
    NPC_MATURE_BONE_SIFTER = 22482,
    NPC_SAND_GNOME = 22483
};
 
class spell_fumping_39238 : public SpellScriptLoader
{
public:
    spell_fumping_39238() : SpellScriptLoader("spell_fumping_39238") {}
 
    class spell_fumping_39238SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_fumping_39238SpellScript)
 
        bool Validate(SpellInfo const * /*spellInfo*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_FUMPING))
                return false;
            return true;
        }
 
        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* player = GetCaster())
            {
                switch (urand(1,2))
                {
                    case 1: player->SummonCreature(NPC_MATURE_BONE_SIFTER, player->GetPositionX()+rand32()%10, player->GetPositionY()+rand32()%10, player->GetPositionZ()+5, 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0); break;
                    case 2: player->SummonCreature(NPC_SAND_GNOME, player->GetPositionX()+rand32()%10, player->GetPositionY()+rand32()%10, player->GetPositionZ()+5, 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0); break;
                }
            }
        }
        void Register()
        {
            OnEffectHit += SpellEffectFn(spell_fumping_39238SpellScript::HandleDummy, EFFECT_ALL, SPELL_EFFECT_ANY);
        }
    };
 
    SpellScript* GetSpellScript() const
    {
        return new spell_fumping_39238SpellScript();
    }
};

/*###################
# spell_fumping_39246
# Quest 10930
####################*/

enum AnotherFumpingSpellData
{
    SPELL_FUMPING2 = 39246,
    NPC_HAISHULUD = 22038
};
 
class spell_fumping_39246 : public SpellScriptLoader
{
public:
    spell_fumping_39246() : SpellScriptLoader("spell_fumping_39246") {}
 
    class spell_fumping_39246SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_fumping_39246SpellScript)
 
        bool Validate(SpellInfo const * /*spellInfo*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_FUMPING2))
                return false;
            return true;
        }
 
        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* player = GetCaster())
            {
                switch (urand(1,3))
                {
                    case 1:
                        player->SummonCreature(NPC_MATURE_BONE_SIFTER, player->GetPositionX()+rand32()%10, player->GetPositionY()+rand32()%10, player->GetPositionZ()+5, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                        player->SummonCreature(NPC_MATURE_BONE_SIFTER, player->GetPositionX()+rand32()%10, player->GetPositionY()-rand32()%10, player->GetPositionZ()+5, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                        player->SummonCreature(NPC_MATURE_BONE_SIFTER, player->GetPositionX()-rand32()%10, player->GetPositionY()+rand32()%10, player->GetPositionZ()+5, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                        break;
                    case 2:
                        player->SummonCreature(NPC_SAND_GNOME, player->GetPositionX()+rand32()%10, player->GetPositionY()+rand32()%10, player->GetPositionZ()+5, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                        player->SummonCreature(NPC_SAND_GNOME, player->GetPositionX()+rand32()%10, player->GetPositionY()-rand32()%10, player->GetPositionZ()+5, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                        player->SummonCreature(NPC_SAND_GNOME, player->GetPositionX()-rand32()%10, player->GetPositionY()+rand32()%10, player->GetPositionZ()+5, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                        break;
                    case 3:
                        player->SummonCreature(NPC_HAISHULUD, player->GetPositionX()+rand32()%5, player->GetPositionY()+rand32()%5, player->GetPositionZ()+5, 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000);
                        break;
                }
            }
        }
        void Register()
        {
            OnEffectHit += SpellEffectFn(spell_fumping_39246SpellScript::HandleDummy, EFFECT_2, SPELL_EFFECT_ANY);
        }
    };
 
    SpellScript* GetSpellScript() const
    {
        return new spell_fumping_39246SpellScript();
    }
};

/*######
# npc_skyguard_prisoner
######*/

enum PrisonerSkettis
{
    SAY_PRISONER_START          = 0,
    SAY_PRISONER_STOP           = 1,
    SAY_PRISONER_END            = 2,

    QUEST_ESCAPE_FROM_SKETTIS   = 11085,
    NPC_SKETTIS_WINDWALKER      = 21649,
    NPC_SKETTIS_WING_GUARD      = 21644,
    FACTION_ESCORTEE_SKETTIS    = 1856,                     

};

static float skettisWing[]= {-4181.069336f, 3074.465820f, 333.816742f};

class npc_skyguard_prisoner : public CreatureScript
{
public:
    npc_skyguard_prisoner() : CreatureScript("npc_skyguard_prisoner") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        if (quest->GetQuestId() == QUEST_ESCAPE_FROM_SKETTIS)
        {
            creature->setFaction(FACTION_ESCORTEE_SKETTIS);

            if (npc_skyguard_prisonerAI* pEscortAI = CAST_AI(npc_skyguard_prisoner::npc_skyguard_prisonerAI, creature->AI()))
                pEscortAI->Start(false, false, player->GetGUID(), quest);
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_skyguard_prisonerAI(creature);
    }

    struct npc_skyguard_prisonerAI : public npc_escortAI
    {
        npc_skyguard_prisonerAI(Creature* creature) : npc_escortAI(creature) { }

        void Reset() {}

        void JustDied(Unit* /*killer*/)
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
                return;

            if (Player* player = GetPlayerForEscort())
            {
                if (player->GetQuestStatus(QUEST_ESCAPE_FROM_SKETTIS) != QUEST_STATUS_COMPLETE)
                    player->FailQuest(QUEST_ESCAPE_FROM_SKETTIS);
            }
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 1:
                    Talk(SAY_PRISONER_START);
                    break;
                case 10:                   
                    me->SummonCreature(NPC_SKETTIS_WINDWALKER, skettisWing[0]-1.5f, skettisWing[1]-1.5f, skettisWing[2], 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                    me->SummonCreature(NPC_SKETTIS_WING_GUARD, skettisWing[0]+1.5f, skettisWing[1]+1.5f, skettisWing[2], 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 25000);
                    break;
                case 11:
                    Talk(SAY_PRISONER_STOP);
                    break;
                case 13:
                    Talk(SAY_PRISONER_END);
                    if (Player* player = GetPlayerForEscort())
                        player->GroupEventHappens(QUEST_ESCAPE_FROM_SKETTIS, me);
                    break;
            }
        }
    };

};

enum SkywingMisc
{
    SAY_SKYWING_START            = 0,
    SAY_SKYWING_TREE_DOWN        = 1,
    SAY_SKYWING_TREE_UP          = 2,
    SAY_SKYWING_JUMP             = 3,
    SAY_SKYWING_SUMMON           = 4,
    SAY_SKYWING_END              = 5,
    SPELL_FEATHERY_CYCLONE_BURST = 39166, 
    SPELL_RILAK_THE_REDEEMED     = 39179,
    NPC_LUANGA_THE_IMPRISONER    = 18533,
    QUEST_SKYWING                = 10898,
    FACTION_A                    = 774,
    FACTION_H                    = 775
};

class npc_bird_skywing : public CreatureScript
{
public:
    npc_bird_skywing() : CreatureScript("npc_bird_skywing") { }

    struct npc_bird_skywingAI : public npc_escortAI
    {
        npc_bird_skywingAI(Creature* creature) : npc_escortAI(creature) {}

        void WaypointReached(uint32 waypointId)
        {
            Player* player = GetPlayerForEscort();
            if (!player)
                return;

            switch (waypointId)
            {
                case 6:
                    Talk(SAY_SKYWING_TREE_DOWN);
                    break;
                case 36:
                    Talk(SAY_SKYWING_TREE_UP);
                    break;
                case 60:
                    Talk(SAY_SKYWING_JUMP);
                    me->SetCanFly(true);
                    break;
                case 61:
                    me->SetCanFly(false);
                    break;
                case 80:
                    Talk(SAY_SKYWING_SUMMON);
                    me->SummonCreature(NPC_LUANGA_THE_IMPRISONER, -3507.203f, 4084.619f, 92.947f, 5.15f, TEMPSUMMON_DEAD_DESPAWN, 60000);
                    break;
                case 81:
                    DoCastSelf(SPELL_FEATHERY_CYCLONE_BURST, true);
                    DoCastSelf(SPELL_RILAK_THE_REDEEMED, true);
                    break;
                case 82:
                    Talk(SAY_SKYWING_END);
                    player->GroupEventHappens(QUEST_SKYWING, me);
                    me->SetImmuneToNPC(true);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);                    
                    me->setFaction(FACTION_FRIENDLY_TO_ALL);
                    break;
            }
        }

        void JustSummoned(Creature* summoned)
        {
            summoned->AI()->AttackStart(me);
        }

        void JustDied(Unit* /*killer*/)
        {
            me->SetImmuneToNPC(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);                    
            me->setFaction(FACTION_FRIENDLY_TO_ALL);           

            if (Player* player = GetPlayerForEscort())
                player->FailQuest(QUEST_SKYWING);
        }

        void Reset() { }
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_SKYWING)
        {
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            creature->SetImmuneToNPC(false);
            creature->AI()->Talk(SAY_SKYWING_START);
            if (npc_escortAI* pEscortAI = CAST_AI(npc_bird_skywing::npc_bird_skywingAI, creature->AI()))
                pEscortAI->Start(true, false, player->GetGUID());     

            if (player->GetTeam() == HORDE)
                creature->setFaction(FACTION_H);

            if (player->GetTeam() == ALLIANCE)
                creature->setFaction(FACTION_A);

        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_bird_skywingAI(creature);
    }
};

void AddSC_terokkar_forest_rg()
{
    new npc_explosion_trigger();
    new npc_terokk();
    new npc_sarthis_rg();
    new spell_fumping_39238();
    new spell_fumping_39246();
    new npc_skyguard_prisoner();
    new npc_bird_skywing();
}
