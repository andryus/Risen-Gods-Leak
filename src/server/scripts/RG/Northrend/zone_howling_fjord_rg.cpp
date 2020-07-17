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
#include "SmartAI.h"
#include "SpellScript.h"
#include "Vehicle.h"

/*######
## Quest Drop it Rock it
######*/

enum Data
{
    QUEST_DROP_IT_ROCK_IT = 11429,
    NPC_WINTERSKORN_DEFENDER = 24015,
};

class npc_banner : public CreatureScript
{
public:
    npc_banner() : CreatureScript("npc_banner") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_bannerAI(creature);
    }

    struct npc_bannerAI : public ScriptedAI
    {
        npc_bannerAI(Creature* c) : ScriptedAI(c) { }

        uint32 uiWaveTimer;
        uint8 killCounter;

        void Reset()
        {
            killCounter = 0;
            uiWaveTimer = 2000;
            me->SetReactState(REACT_PASSIVE);
            me->SetControlled(true, UNIT_STATE_ROOT);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Player* player = me->GetOwner()->ToPlayer())
                player->FailQuest(QUEST_DROP_IT_ROCK_IT);
        }

        void UpdateAI(uint32 diff)
        {
            if(uiWaveTimer < diff)
            {
                if(Creature* pVrykul = me->SummonCreature(NPC_WINTERSKORN_DEFENDER, (1476.85f + rand32()%20), (-5327.56f + rand32()%20), (194.8f  + rand32()%2), 0.0f, TEMPSUMMON_CORPSE_DESPAWN))
                {
                    pVrykul->AI()->AttackStart(me);
                    pVrykul->GetMotionMaster()->Clear();
                    pVrykul->GetMotionMaster()->MoveChase(me);
                }
                uiWaveTimer = urand(8000, 16000);
            }
            else
               uiWaveTimer -= diff;
        }

        void SummonedCreatureDespawn(Creature* summon)
        {
            if (summon->GetEntry() == NPC_WINTERSKORN_DEFENDER)
                killCounter++;

            if(killCounter >= 3)
            {
                if (Player* player = me->GetOwner()->ToPlayer())
                    player->GroupEventHappens(QUEST_DROP_IT_ROCK_IT, me);

                me->DespawnOrUnsummon(2000);
            }
        }
    };
};

/*######
## Quest Scare the Guano Out of Them!
######*/

enum FeknutBunnyData
{
    NPC_DARKCLAW_BAT        = 23959,
    SPELL_SUMMON_GUANO      = 43307
};

class npc_feknut_bunny : public CreatureScript
{
public:
    npc_feknut_bunny() : CreatureScript("npc_feknut_bunny") {}

    struct npc_feknut_bunnyAI : public ScriptedAI
    {
        npc_feknut_bunnyAI (Creature* creature) : ScriptedAI(creature) {}

        uint32 CheckTimer;
        bool Checked;

        void Reset()
        {
            CheckTimer = 1000;
            Checked = false;
        }

        void UpdateAI(uint32 diff)
        {
            if(!Checked)
            {
                if (CheckTimer <= diff)
                {
                    if(Creature* bat = me->FindNearestCreature(NPC_DARKCLAW_BAT, 35.0f))
                    {
                        if(bat->IsAlive())
                        {
                            bat->CastSpell(me, SPELL_SUMMON_GUANO, false);
                            Checked = true;
                        }
                        if(Player* player = me->GetOwner()->ToPlayer())
                        {
                            bat->Attack(player, true);
                            bat->GetMotionMaster()->MoveChase(player);
                        }
                    }
                    me->DespawnOrUnsummon();
                } else CheckTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_feknut_bunnyAI(creature);
    }
};

class npc_bat : public CreatureScript
{
public:
    npc_bat() : CreatureScript("npc_bat") {}

    struct npc_batAI : public ScriptedAI
    {
        npc_batAI (Creature* creature) : ScriptedAI(creature) {}

        void Rest(){}

        void JustDied(Unit* killer)
        {
            if(killer && killer->GetTypeId() == TYPEID_PLAYER)
                if(killer->ToPlayer()->GetQuestStatus(11154) == QUEST_STATUS_INCOMPLETE)
                    killer->SummonGameObject(186325, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, 0);
        }
    };

    CreatureAI *GetAI(Creature* creature) const
    {
        return new npc_batAI(creature);
    }
};

/*######
## Quest Guided by Honor
######*/

enum irulonData
{
    QUEST_GUIDED_BY_HONOR = 11289,
    NPC_TIRION            = 24232,
    NPC_CLERIC            = 24233,
    ITEM_ASHBRINGER       = 13262,
    SAY_TIRION_1          = 0,
    EMOTE_TIRION_1        = 1,
    SAY_IRULON_1          = 2,
    SAY_TIRION_2          = 3,
    SAY_TIRION_3          = 4,
    SAY_IRULON_2          = 5,
    EMOTE_TIRION_2        = 6,
    SAY_TIRION_4          = 7,
    SAY_TIRION_5          = 8,
    EMOTE_TIRION_3        = 9,
    YELL_TIRION           = 10,
    ACTION_START,
};

enum iluronEvents
{
    EVENT_NONE,
    EVENT_00,
    EVENT_01,
    EVENT_02,
    EVENT_03,
    EVENT_04,
    EVENT_05,
    EVENT_06,
    EVENT_07,
    EVENT_08,
    EVENT_09,
    EVENT_10,
};

class npc_irulon_trueblade : public CreatureScript
{
    public:
        npc_irulon_trueblade() : CreatureScript("npc_irulon_trueblade") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_irulon_truebladeAI(creature);
    }

    struct npc_irulon_truebladeAI : public ScriptedAI
    {
        npc_irulon_truebladeAI(Creature* c) : ScriptedAI(c) { }

        EventMap events;
        ObjectGuid uiTirion;
        ObjectGuid playerGUID;
        bool queststart;

        void Reset()
        {
            uiTirion.Clear();
            events.Reset();
            queststart = true;
            playerGUID.Clear();
        }

        void MoveInLineOfSight(Unit* target)
        {
            if (queststart)
            {
                if (target && me->GetDistance2d(target) <= 10 && target->ToPlayer())
                {
                    if (target->ToPlayer()->GetQuestStatus(QUEST_GUIDED_BY_HONOR) == QUEST_STATUS_INCOMPLETE)
                    {
                        if (Creature* pTirion = me->FindNearestCreature(NPC_CLERIC, 7.0f, true))
                        {
                            queststart = false;
                            me->AI()->DoAction(ACTION_START);
                        }
                        else
                        {
                            me->SummonCreature(NPC_CLERIC,601.464f,-5003.82f,3.90394f,1.76278f,TEMPSUMMON_MANUAL_DESPAWN,0);
                            queststart = false;
                            me->AI()->DoAction(ACTION_START);
                        }
                        playerGUID = target->GetGUID();
                    }
                }
            }
        }

        void DoAction(int32 actionId)
        {
            switch(actionId)
            {
                case ACTION_START:
                    uiTirion.Clear();
                    events.ScheduleEvent(EVENT_00, 1500);
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            switch(events.ExecuteEvent())
            {
                case EVENT_00:
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    if(Creature* pTirion = me->FindNearestCreature(NPC_CLERIC, 7.0f))
                    {
                        uiTirion = pTirion->GetGUID();
                        pTirion->AI()->Talk(SAY_TIRION_1);
                        pTirion->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                        pTirion->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                        pTirion->GetMotionMaster()->MovePoint(0, me->GetPositionX() + 3.0f, me->GetPositionY() + 3.0f, me->GetPositionZ() + 0.5f);
                    }
                    events.ScheduleEvent(EVENT_01, 5000);
                    break;
                case EVENT_01:
                    if(Creature* pTirion = ObjectAccessor::GetCreature(*me, uiTirion))
                    {
                        pTirion->AI()->Talk(EMOTE_TIRION_1);
                        pTirion->UpdateEntry(NPC_TIRION);
                        pTirion->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 0);
                    }
                    events.ScheduleEvent(EVENT_02, 4000);
                    break;
                case EVENT_02:
                    Talk(SAY_IRULON_1);
                    events.ScheduleEvent(EVENT_03, 5000);
                    break;
                case EVENT_03:
                    if(Creature* pTirion = ObjectAccessor::GetCreature(*me, uiTirion))
                        pTirion->AI()->Talk(SAY_TIRION_2);
                    events.ScheduleEvent(EVENT_04, 6000);
                    break;
                case EVENT_04:
                    if(Creature* pTirion = ObjectAccessor::GetCreature(*me, uiTirion))
                        pTirion->AI()->Talk(SAY_TIRION_3);
                    events.ScheduleEvent(EVENT_05,3000);
                    break;
                case EVENT_05:
                    Talk(SAY_IRULON_2);
                    events.ScheduleEvent(EVENT_06, 5500);
                    break;
                case EVENT_06:
                    if(Creature* pTirion = ObjectAccessor::GetCreature(*me, uiTirion))
                        pTirion->AI()->Talk(EMOTE_TIRION_2);
                    events.ScheduleEvent(EVENT_07,3000);
                    break;
                case EVENT_07:
                    if(Creature* pTirion = ObjectAccessor::GetCreature(*me, uiTirion))
                        pTirion->AI()->Talk(SAY_TIRION_4);
                    events.ScheduleEvent(EVENT_08,4500);
                    break;
                case EVENT_08:
                    if(Creature* pTirion = ObjectAccessor::GetCreature(*me, uiTirion))
                        pTirion->AI()->Talk(SAY_TIRION_5);
                    events.ScheduleEvent(EVENT_09,4500);
                    break;
                case EVENT_09:
                    if(Creature* pTirion = ObjectAccessor::GetCreature(*me, uiTirion))
                    {
                        pTirion->AI()->Talk(EMOTE_TIRION_3);
                        pTirion->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, ITEM_ASHBRINGER);
                    }
                    events.ScheduleEvent(EVENT_10,5000);
                    break;
                case EVENT_10:
                    if(Creature* pTirion = ObjectAccessor::GetCreature(*me, uiTirion))
                    {
                        pTirion->AI()->Talk(YELL_TIRION);
                        pTirion->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 0);
                        pTirion->DespawnOrUnsummon(5000);
                    }
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                        player->GroupEventHappens(QUEST_GUIDED_BY_HONOR, me);
                    queststart = true;
                    break;
           }
        }
    };
};

/*################
# Quest The Echo of Ymiron - 11343
###############*/

enum Yells
{
    SAY_YMIRON_LK_1                                = 0,
    SAY_YMIRON_LK_2                                = 1,
    SAY_YMIRON_LK_3                                = 2,
    SAY_YMIRON_LK_4                                = 3,
    SAY_YMIRON_LK_5                                = 4,
    SAY_VALKYR_0                                   = 0,
    SAY_ANCIENT_MALE_VRYKUL_1                      = 0,
    SAY_ANCIENT_MALE_VRYKUL_2                      = 1,
    SAY_ANCIENT_MALE_VRYKUL_3                      = 2,
    SAY_ANCIENT_FEMALE_VRYKUL_1                    = 0,
    SAY_ANCIENT_FEMALE_VRYKUL_2                    = 1,
};

enum Spells
{
    SPELL_WRATH_OF_THE_LICH_KING        = 50156,
    SPELL_GRASP_OF_THE_LICH_KING        = 43489,
    SPELL_MAGNETIC_PULL                 = 29661,
};

uint32 uiPhase_Ymiron_LK;
uint32 uiPhaseTimer_Ymiron_LK;
uint32 uiEventCounter;
uint32 EventAncientVrykulPhase;
uint32 EventAncientVrykulTimer;
bool EventStartedYmiron;
bool EventStarted;
bool EventLich;

class npc_lich_king_ymiron : public CreatureScript
{
public:
    npc_lich_king_ymiron() : CreatureScript("npc_lich_king_ymiron") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_lich_king_ymironAI (creature);
    }

    struct npc_lich_king_ymironAI : public ScriptedAI
    {
       npc_lich_king_ymironAI(Creature *c) : ScriptedAI(c) {}

        void Reset()
        {
            uiPhase_Ymiron_LK = 0;
            uiPhaseTimer_Ymiron_LK = 1000;
            EventStartedYmiron = false;
        }

        void UpdateAI(uint32 uiDiff)
        {
            me->StopMoving();
            if (EventLich)
            {
                if (me->GetVictim())
                {
                    Unit *LK_VICTIM = me->GetVictim();

                    if (LK_VICTIM == NULL)
                        return;

                    if (uiPhaseTimer_Ymiron_LK <= uiDiff)
                    {
                        switch (uiPhase_Ymiron_LK)
                        {
                            case 0:
                                me->CastSpell(me->GetVictim(), SPELL_MAGNETIC_PULL, false, 0, 0);
                                me->CastSpell(me->GetVictim(), SPELL_GRASP_OF_THE_LICH_KING, false, 0, 0);
                                me->AttackStop();
                                uiPhase_Ymiron_LK = 1;
                                break;
                            case 1:
                                Talk(SAY_YMIRON_LK_1);
                                uiPhaseTimer_Ymiron_LK = 15000;
                                uiPhase_Ymiron_LK = 2;
                                break;
                            case 2:
                                EventStartedYmiron = true;
                                uiPhaseTimer_Ymiron_LK = 10000;
                                uiPhase_Ymiron_LK = 3;
                                break;
                            case 3:
                                Talk(SAY_YMIRON_LK_2);
                                uiPhaseTimer_Ymiron_LK = 25000;
                                uiPhase_Ymiron_LK = 4;
                                break;
                            case 4:
                                Talk(SAY_YMIRON_LK_3);
                                uiPhaseTimer_Ymiron_LK = 20000;
                                uiPhase_Ymiron_LK = 5;
                                break;
                            case 5:
                                Talk(SAY_YMIRON_LK_4);
                                uiPhaseTimer_Ymiron_LK = 20000;
                                uiPhase_Ymiron_LK = 6;
                                break;
                            case 6:
                                Talk(SAY_YMIRON_LK_5);
                                uiPhaseTimer_Ymiron_LK = 10000;
                                uiPhase_Ymiron_LK = 7;
                                break;
                            case 7:
                                me->CastSpell(LK_VICTIM, SPELL_WRATH_OF_THE_LICH_KING, false, 0, 0);
                                me->Kill(LK_VICTIM);
                                uiPhase_Ymiron_LK = 8;
                                break;
                            case 8:
                                Reset();
                                LK_VICTIM = NULL;
                                EventLich = false;
                                break;
                        }
                    }
                    else
                        uiPhaseTimer_Ymiron_LK -= uiDiff;
                }
            }
        }
    };
};

class npc_valkyre_ymiron : public CreatureScript
{
public:
    npc_valkyre_ymiron() : CreatureScript("npc_valkyre_ymiron") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_valkyre_ymironAI (creature);
    }

    struct npc_valkyre_ymironAI : public ScriptedAI
    {
        npc_valkyre_ymironAI(Creature *c) : ScriptedAI(c) {}

        uint32 EventValkyreTimer;

        void Reset()
        {
            EventValkyreTimer = 31000;
            me->SetReactState(REACT_PASSIVE);
            me->SetCanFly(true);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (me->IsInCombat())
            {
                me->StopMoving();
                me->AttackStop();
            }
            if (EventLich)
            {
                if (EventValkyreTimer <= uiDiff)
                {
                    if (EventStartedYmiron == true)
                    {
                        Talk(SAY_VALKYR_0);
                        EventStartedYmiron = false;
                    }
                }
                else
                    EventValkyreTimer -= uiDiff;
            }
        }
    };
};

class npc_ancient_male_vrykul : public CreatureScript
{
public:
    npc_ancient_male_vrykul() : CreatureScript("npc_ancient_male_vrykul") { }

    struct npc_ancient_male_vrykulAI : public ScriptedAI
    {
        npc_ancient_male_vrykulAI(Creature *c) : ScriptedAI(c) {}

        void Reset()
        {
            EventAncientVrykulPhase = 0;
            EventAncientVrykulTimer = 5000;
            EventStarted            = false;
            EventLich               = false;
            me->SetPhaseMask(2, true);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who && me->GetDistance2d(who) <= 15 && who->ToPlayer())
               if (who->HasAuraEffect(42786, 1))
                    EventStarted = true;
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (me->getFaction() == 14)
            {
                me->setFaction(11);
                me->SetPhaseMask(2, true);
            }

            if (EventStarted == true)
            {
                if (EventAncientVrykulTimer <= uiDiff)
                {
                    switch (EventAncientVrykulPhase)
                    {
                        case 0:
                            Talk(SAY_ANCIENT_MALE_VRYKUL_1);
                            EventAncientVrykulTimer = 7000;
                            EventAncientVrykulPhase++;
                            break;
                        case 1:
                            Talk(SAY_ANCIENT_MALE_VRYKUL_2);
                            EventAncientVrykulTimer = 7000;
                            EventAncientVrykulPhase++;
                            break;
                        case 3:
                            Talk(SAY_ANCIENT_MALE_VRYKUL_3);
                            EventAncientVrykulTimer = 7000;
                            EventAncientVrykulPhase++;
                            break;
                        case 5:
                            EventLich = true;
                            break;
                    }
                }
                else EventAncientVrykulTimer -= uiDiff;
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_ancient_male_vrykulAI (creature);
    }
};

class npc_ancient_female_vrykul : public CreatureScript
{
public:
    npc_ancient_female_vrykul() : CreatureScript("npc_ancient_female_vrykul") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_ancient_female_vrykulAI (creature);
    }

    struct npc_ancient_female_vrykulAI : public ScriptedAI
    {
        npc_ancient_female_vrykulAI(Creature *c) : ScriptedAI(c) {}

         void CompleteQuest()
         {
            if (Map *pMap = me->GetMap())
            {
                for (Player& player : pMap->GetPlayers())
                {
                    if (me->IsInRange(&player, 0.0f, 30.0f))
                        player.CompleteQuest(11343);
                }
            }
        }
 
        void Reset()
        {
            me->SetPhaseMask(2, true);
            me->SetReactState(REACT_PASSIVE);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (me->getFaction() == 14)
            {
                me->setFaction(11);
                me->SetPhaseMask(2, true);
            }

            if (EventStarted == true)
            {
                if (EventAncientVrykulTimer <= uiDiff)
                {
                    switch (EventAncientVrykulPhase)
                    {
                        case 2:
                            Talk(SAY_ANCIENT_FEMALE_VRYKUL_1);
                            EventAncientVrykulTimer = 7000;
                            EventAncientVrykulPhase++;
                            break;
                        case 4:
                            Talk(SAY_ANCIENT_FEMALE_VRYKUL_2);
                            EventAncientVrykulTimer = 7000;
                            CompleteQuest();
                            EventAncientVrykulPhase++;
                            break;
                    }
                }
                else EventAncientVrykulTimer -= uiDiff;
            }
        }
    };
};

/*#####
# item_ymiron_raeuchergefaess
#####*/
enum
{
    QUEST_THE_ECHO_OF_YMIRON    = 11343,
    NPC_ANCIENT_FEMALE_VRYKUL   = 24315,
    NPC_ANCIENT_MALE_VRYKUL     = 24314
};

class item_ymiron_raeuchergefaess : public ItemScript
{
public:
    item_ymiron_raeuchergefaess() : ItemScript("item_ymiron_raeuchergefaess") { }

    bool OnUse(Player* player, Item* /*item*/, const SpellCastTargets& /*pTargets*/)
    {
        if (player->GetQuestStatus(QUEST_THE_ECHO_OF_YMIRON) == QUEST_STATUS_INCOMPLETE)
        {
            player->SummonCreature(NPC_ANCIENT_FEMALE_VRYKUL, 1076.130005f, -5035.040039f, 9.690260f, 2.745330f, TEMPSUMMON_TIMED_DESPAWN, 60000);
            player->SummonCreature(NPC_ANCIENT_MALE_VRYKUL, 1073.099976f, -5026.250000f, 9.718090f, 4.607560f, TEMPSUMMON_TIMED_DESPAWN, 60000);
        }
        return false;
    }
};

/*#####
# Quest 11352: The Rune of Command
# spell_rune_of_command
#####*/

enum TheRuneOfCommand
{
    NPC_CAPTIVE_STONE_GIANT = 24345
};

class spell_rune_of_command : public SpellScriptLoader
{
    public:
        spell_rune_of_command() : SpellScriptLoader("spell_rune_of_command") { }

        class spell_rune_of_command_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rune_of_command_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* player = caster->ToPlayer())
                    {
                        player->KilledMonsterCredit(NPC_CAPTIVE_STONE_GIANT);
                        if (Unit* target = GetHitUnit())
                            if (Creature* creature = target->ToCreature())
                                creature->DespawnOrUnsummon(0);
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_rune_of_command_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rune_of_command_SpellScript();
        }
};

/*#####
# Quest 11421: It goes to 11...
# spel_q11421_it_goes_to_11
#####*/

enum itGoesTo11
{
    NPC_DRAGONFLAYER_DEFENDER = 24533,
    NPC_TRIGGER               = 23472,
    NPC_LONGHOUSE             = 24538,
    NPC_DOCKHOUSE             = 24646,
    NPC_STORAGE               = 24647,
    TEXT_1                    = 1999926,
    TEXT_2                    = 1999927,
    TEXT_3                    = 1999928,
};

class spel_q11421_it_goes_to_11 : public SpellScriptLoader
{
    public:
        spel_q11421_it_goes_to_11() : SpellScriptLoader("spel_q11421_it_goes_to_11") { }

        class spel_q11421_it_goes_to_11_SpellScript : public SpellScript
        {
            PrepareSpellScript(spel_q11421_it_goes_to_11_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Player* player = NULL;

                if (Vehicle* vhc = GetCaster()->GetVehicleKit())
                    if (Unit* psg = vhc->GetPassenger(0))
                        if (psg->ToPlayer())
                            player = psg->ToPlayer();

                if (player)
                {
                    if (Creature* trigger = player->SummonCreature(NPC_TRIGGER, 975.679077f, -5227.642090f, 187.755493f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 1000))
                    {
                        int32 emoteEntry = 0;
                        if (Creature* target = GetHitUnit()->ToCreature())
                        {
                            switch (target->GetEntry())
                            {
                                case NPC_LONGHOUSE:
                                    emoteEntry = TEXT_1;
                                    break;
                                case NPC_DOCKHOUSE:
                                    emoteEntry = TEXT_2;
                                    break;
                                case NPC_STORAGE:
                                    emoteEntry = TEXT_3;
                                    break;
                            }
                        }

                        GetCaster()->MonsterTextEmote(emoteEntry, player, true);

                        Position pos;
                        for (uint32 i = 0; i < 3; i++)
                        {
                            pos.m_positionX = trigger->GetPositionX() + irand(-30, 30);
                            pos.m_positionY = trigger->GetPositionY();
                            pos.m_positionZ = trigger->GetPositionZ() + irand(-20, 20);

                            if (Creature* defender = player->SummonCreature(NPC_DRAGONFLAYER_DEFENDER, pos, TEMPSUMMON_TIMED_DESPAWN, 300 * IN_MILLISECONDS))
                            {
                                defender->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
                                defender->SetDisableGravity(true);
                                defender->SetCanFly(true);
                                defender->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);

                                defender->GetMotionMaster()->MoveChase(GetCaster(), 20.0f);
                                defender->EngageWithTarget(GetCaster());
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spel_q11421_it_goes_to_11_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spel_q11421_it_goes_to_11_SpellScript();
        }
};

enum MindlessAbominationMisc
{
    SPELL_QUEST_CREDIT       = 43399,
    NPC_MINDLESS_ABOMINATION = 23575
};

class npc_plagued_dragonflayer : public CreatureScript
{
    public:
        npc_plagued_dragonflayer() : CreatureScript("npc_plagued_dragonflayer") { }

        struct npc_plagued_dragonflayerAI : public SmartAI
        {
            npc_plagued_dragonflayerAI(Creature* creature) : SmartAI(creature) { }

            void JustDied(Unit* killer) override
            {
                if (killer->GetEntry() == NPC_MINDLESS_ABOMINATION)
                    killer->CastSpell(killer, SPELL_QUEST_CREDIT, true);

                SmartAI::JustDied(killer);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_plagued_dragonflayerAI(creature);
        }
};

enum AbandonedPackMule
{
    QUEST_SEND_THEM_PACKING = 11224,
    SPELL_SEND_THEM_PACKING_DUMMY = 43572,
    SPELL_SEND_THEM_PACKING_CREDIT = 42721,
};

class npc_abandoned_pack_mule : public CreatureScript
{
public:
    npc_abandoned_pack_mule() : CreatureScript("npc_abandoned_pack_mule") { }

    struct npc_abandoned_pack_muleAI : public ScriptedAI
    {
        npc_abandoned_pack_muleAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override 
        { 
            _moveAway = false;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void ReceiveEmote(Player* player, uint32 emote) override
        {
            if (!player || !emote)
                return;

            if (!_moveAway && emote == TEXT_EMOTE_RAISE)
            {
                if (player->GetQuestStatus(QUEST_SEND_THEM_PACKING) == QUEST_STATUS_INCOMPLETE)
                {
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->CastSpell(player, SPELL_SEND_THEM_PACKING_DUMMY, true);
                    player->CastSpell(player, SPELL_SEND_THEM_PACKING_CREDIT, true);
                    me->StopMoving();
                    me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK_UNARMED);
                    me->PlayDirectSound(725, 0);
                    _moveAwayTimer = 2000;
                    _moveAway = true;
                }
            }
        }

        void UpdateAI(uint32 diff) override 
        {
            if (_moveAway)
            {
                if (_moveAwayTimer <= diff)
                {
                    me->GetNearPoint(me, x, y, z, 1, 100, float(M_PI * 2 * rand_norm()));
                    me->GetMotionMaster()->MovePoint(0, x, y, z);

                    me->DespawnOrUnsummon(2000);
                    _moveAway = false;
                }
                else _moveAwayTimer -= diff;
            }

            if (!UpdateVictim())
                return;
        }
        private:
            uint32 _moveAwayTimer;
            bool _moveAway;
            float x, y, z;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_abandoned_pack_muleAI(creature);
    }
};

enum QuestDaggercapDivin
{
    NPC_HAROLD               = 23730,
    NPC_BUOY                 = 24707,

    SPELL_BREATHING_TUBE_NPC = 44250,
    SPELL_BREATHING_TUBE_PLA = 44270,
};

class npc_daggercap_divin : public CreatureScript
{
public:
    npc_daggercap_divin() : CreatureScript("npc_daggercap_divin") { }

    struct npc_daggercap_divinAI : public ScriptedAI
    {
        npc_daggercap_divinAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            if (me->GetEntry() == NPC_HAROLD)
                DoCastSelf(SPELL_BREATHING_TUBE_NPC);

            me->SetSpeedRate(MOVE_WALK, 0.3f);
            me->SetSpeedRate(MOVE_RUN, 0.3f);
            // Haven´t found any possibility to set those values via DB or SmartAI
            me->SetSpeedRate(MOVE_SWIM, 0.3f);
            me->SetSpeedRate(MOVE_FLIGHT, 0.3f);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_daggercap_divinAI(creature);
    }
};

void AddSC_howling_fjord_rg()
{
    new npc_banner();
    new npc_feknut_bunny();
    new npc_bat();
    new npc_irulon_trueblade();
    new npc_lich_king_ymiron();
    new npc_valkyre_ymiron();
    new npc_ancient_male_vrykul();
    new npc_ancient_female_vrykul();
    new item_ymiron_raeuchergefaess();
    new spell_rune_of_command();
    new spel_q11421_it_goes_to_11();
    new npc_plagued_dragonflayer();
    new npc_abandoned_pack_mule();
    new npc_daggercap_divin();
 }
