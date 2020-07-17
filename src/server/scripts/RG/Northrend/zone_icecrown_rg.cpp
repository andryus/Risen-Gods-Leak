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
#include "CreatureTextMgr.h"
#include "Group.h"
#include "Vehicle.h"
#include "SpellScript.h"
#include "CombatAI.h"
#include "SpellAuras.h"
#include "SmartAI.h"

/*##########################
# spell_construct_barricade
###########################*/

enum BarricadeData
{
    // Spells
    SPELL_CONSTRUCT_BARRICADE = 59925,
    SPELL_SUMMON_BARRICADE_A  = 59922,
    SPELL_SUMMON_BARRICADE_B  = 59923,
    SPELL_SUMMON_BARRICADE_C  = 59924,

    // Creatures
    NPC_EBON_BLADE_MARKER_H   = 31887,
    NPC_EBON_BLADE_MARKER_A   = 32288
};

class spell_construct_barricade : public SpellScriptLoader
{
    public:
        spell_construct_barricade() : SpellScriptLoader("spell_construct_barricade") {}

        class spell_construct_barricade_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_construct_barricade_SpellScript)

            bool Validate(SpellInfo const * /*spellInfo*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_CONSTRUCT_BARRICADE))
                    return false;
                return true;
            }

            SpellCastResult CheckCast()
            {
                if (Unit* caster = GetCaster())
                    if (caster->FindNearestCreature(NPC_EBON_BLADE_MARKER_H, 10.0f) || caster->FindNearestCreature(NPC_EBON_BLADE_MARKER_A, 10.0f))
                        return SPELL_CAST_OK;

                return SPELL_FAILED_NO_VALID_TARGETS;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                    if (Player* player = caster->ToPlayer())
                    {
                        uint32 SummonBarricadeSpell = 0;
                        switch (urand(1,3))
                        {
                            case 1: 
                                SummonBarricadeSpell = SPELL_SUMMON_BARRICADE_A; 
                                break;
                            case 2: 
                                SummonBarricadeSpell = SPELL_SUMMON_BARRICADE_B; 
                                break;
                            case 3: 
                                SummonBarricadeSpell = SPELL_SUMMON_BARRICADE_C; 
                                break;
                        }
                        player->CastSpell(player, SummonBarricadeSpell, true);
                    
                        // Only this killcredit, the other one is used for another quest
                        player->KilledMonsterCredit(NPC_EBON_BLADE_MARKER_H);

                        // Despawn Dummies
                        if (Creature* dummy = player->FindNearestCreature(NPC_EBON_BLADE_MARKER_H, 10.0f))
                            dummy->DespawnOrUnsummon();
                    }
            }
            
            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_construct_barricade_SpellScript::CheckCast);
                OnEffectHitTarget += SpellEffectFn(spell_construct_barricade_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_construct_barricade_SpellScript();
        }
};

/*######
## npc_alorah_and_grimmin
######*/

enum EalorahGrimmin
{
    // Spells
    SPELL_CHAIN                     = 68341,

    // Creatures
    NPC_FJOLA_LIGHTBANE             = 36065,
    NPC_EYDIS_DARKBANE              = 36066,
    NPC_PRIESTESS_ALORAH            = 36101,
    NPC_PRIEST_GRIMMIN              = 36102
};

class npc_alorah_and_grimmin : public CreatureScript
{
public:
    npc_alorah_and_grimmin() : CreatureScript("npc_alorah_and_grimmin") { }

    struct npc_alorah_and_grimminAI : public ScriptedAI
    {
        npc_alorah_and_grimminAI(Creature* creature) : ScriptedAI(creature) { }

        bool uiCast;

        void Reset() override
        {
            uiCast = false;
        }

        void UpdateAI(uint32 /*uiDiff*/) override
        {
            if (uiCast)
                return;

            uiCast = true;
            Creature* target = NULL;

            switch(me->GetEntry())
            {
                case NPC_PRIESTESS_ALORAH:
                    target = me->FindNearestCreature(NPC_EYDIS_DARKBANE, 10.0f);
                    break;
                case NPC_PRIEST_GRIMMIN:
                    target = me->FindNearestCreature(NPC_FJOLA_LIGHTBANE, 10.0f);
                    break;
            }
            if (target)
                DoCast(target, SPELL_CHAIN);

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_alorah_and_grimminAI(creature);
    }
};

#define NPC_RAVENOUS_JAWS_KILL_CREDIT 29391
#define NPC_RAVENOUS_JAWS 29392

class ItemUse_gore_bladder : public ItemScript
{
public:

    ItemUse_gore_bladder()
        : ItemScript("gore_bladder")
    {
    }

    bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& /*targets*/)
    {
        Creature* target = player->FindNearestCreature(NPC_RAVENOUS_JAWS_KILL_CREDIT, 10.0f, true);
        Creature* shark = player->FindNearestCreature(NPC_RAVENOUS_JAWS,10.0f,false);

        if(target && target->IsAlive())
        {
            player->Kill(target);
            player->KilledMonsterCredit(target->GetEntry(),target->GetGUID());
        }
        else return false;

        if(shark)
        {
            shark->DespawnOrUnsummon(0);
        }
        else return false;

        return true;
    }
};

#define QUEST_NOT_A_BUG_SUMMON_CHANNELED    60561
#define QUEST_NOT_A_BUG_SUMMON_TRIGGERED    60563
#define QUEST_NOT_A_BUG_KC                  32314
#define QUEST_NOT_A_BUG_DARK_MATTER         44434

class npc_not_a_bug_summon_bunny : public CreatureScript
{
public:
    npc_not_a_bug_summon_bunny() : CreatureScript("npc_not_a_bug_summon_bunny") { }

    struct npc_not_a_bug_summon_bunnyAI : public ScriptedAI
    {
        npc_not_a_bug_summon_bunnyAI(Creature* creature) : ScriptedAI(creature) {}

        void SpellHit(Unit* unit, const SpellInfo* spell) override
        {
            if(spell && unit && spell->Id == QUEST_NOT_A_BUG_SUMMON_TRIGGERED)
                if(Aura* aura = unit->GetAura(QUEST_NOT_A_BUG_SUMMON_CHANNELED))
                    if(Player* player = aura->GetCaster()->ToPlayer())
                    {
                        player->KilledMonsterCredit(QUEST_NOT_A_BUG_KC);
                        player->DestroyItemCount(QUEST_NOT_A_BUG_DARK_MATTER, 5, true);
                    }
        }
    };

    CreatureAI *GetAI(Creature* creature) const override
    {
        return new npc_not_a_bug_summon_bunnyAI(creature);
    }
};

#define QUEST_FIND_THE_ANCIENT_HERO     13133
#define QUEST_FIND_THE_ANCIENT_HERO_KC  30880

#define NPC_SUBJUGATED_ISKALDER         30886
#define NPC_ISKALDER                    30884
#define NPC_MJORDIN_COMBATANT           30037
#define NPC_MJORDIN_WATER_MAGUS         30632

#define GOSSIP_SLUMBERING_MJORDIN_ITEM  "Den schlummernden Mjordin aufwecken, um zu schauen ob es der alte Held Iskalder ist."
#define GOSSIP_SLUMBERING_MJORDIN_TEXT  13871

class npc_slumbering_mjordin : public CreatureScript
{
public:
    npc_slumbering_mjordin() : CreatureScript("npc_slumbering_mjordin") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        bool hasIskalder = false;

        player->PlayerTalkClass->ClearMenus();

        std::list<Creature*> MinionList;
        player->GetAllMinionsByEntry(MinionList,NPC_SUBJUGATED_ISKALDER);
        if (!MinionList.empty())
            hasIskalder = true;

        if (player->GetQuestStatus(QUEST_FIND_THE_ANCIENT_HERO) == QUEST_STATUS_INCOMPLETE && !hasIskalder)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SLUMBERING_MJORDIN_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        player->SEND_GOSSIP_MENU(GOSSIP_SLUMBERING_MJORDIN_TEXT, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->SendCloseGossip();

        if(uiAction!=GOSSIP_ACTION_INFO_DEF+1)
            return false;

        switch(urand(1,3))
        {
            case 1: creature->UpdateEntry(NPC_MJORDIN_COMBATANT);
                creature->AI()->AttackStart(player);
                break;
            case 2: creature->UpdateEntry(NPC_MJORDIN_WATER_MAGUS);
                creature->AI()->AttackStart(player);
                break;
            case 3: creature->UpdateEntry(NPC_ISKALDER);
                creature->AI()->AttackStart(player);
                break;
            default: break;
        }
        return true;
    }
};

class npc_subjugated_iskalder : public CreatureScript
{
public:
    npc_subjugated_iskalder() : CreatureScript("npc_subjugated_iskalder") { }

    struct npc_subjugated_iskalderAI : public ScriptedAI
    {
        npc_subjugated_iskalderAI(Creature* creature) : ScriptedAI(creature) { }
        
        void IsSummonedBy(Unit* who) override
        {
            if(!who || who->GetTypeId()!=TYPEID_PLAYER)
                return;

            uiCheckTimer = 5 * IN_MILLISECONDS;

            me->GetMotionMaster()->MoveFollow(me->ToTempSummon()->GetSummoner(),PET_FOLLOW_DIST,PET_FOLLOW_ANGLE);

        }

        void Reset() override
        {
            if (Unit* summoner = me->ToTempSummon()->GetSummoner())
                me->GetMotionMaster()->MoveFollow(summoner,PET_FOLLOW_DIST,PET_FOLLOW_ANGLE);
        }

        void UpdateAI(uint32 diff) override
        {
            if (UpdateVictim())
            {
                DoMeleeAttackIfReady();
                return;
            }

            if (uiCheckTimer <= diff)
            {
                if (Unit* summoner = me->ToTempSummon()->GetSummoner())
                    if (me->GetDistance2d(summoner)>=40.0f)
                        me->DespawnOrUnsummon();
                uiCheckTimer = 5 * IN_MILLISECONDS;
            } 
            else
                uiCheckTimer -= diff;
        }
    private:
        uint32 uiCheckTimer;
    };

    CreatureAI *GetAI(Creature* creature) const override
    {
        return new npc_subjugated_iskalderAI(creature);
    }
};


class npc_bone_witch : public CreatureScript
{
public:
    npc_bone_witch() : CreatureScript("npc_bone_witch") { }

    struct npc_bone_witchAI : public ScriptedAI
    {
        npc_bone_witchAI(Creature* creature) : ScriptedAI(creature) { }

        void MoveInLineOfSight(Unit* who) override
        {
            if(!who || me->GetDistance2d(who)>10.0f)
                return;

            if(Creature* creature = who->ToCreature())
                if(creature->GetEntry()==NPC_SUBJUGATED_ISKALDER)
                {
                    if(Player* summoner = creature->ToTempSummon()->GetSummoner()->ToPlayer())
                        summoner->KilledMonsterCredit(QUEST_FIND_THE_ANCIENT_HERO_KC);

                    creature->DespawnOrUnsummon();
                }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bone_witchAI(creature);
    }
};

/*######
## npc_matthias_lehner
######*/

enum MatthisLehner
{
    // Quests
    QUEST_DO_YOUR_WORST_H           = 13305,
    QUEST_DO_YOUR_WORST_A           = 13394,

    // Spells
    SPELL_REFURBISHED_DEMOLISHER    = 59724,
};

class npc_matthias_lehner : public CreatureScript
{
public:
    npc_matthias_lehner() : CreatureScript("npc_matthias_lehner") { }

    bool OnQuestAccept(Player* player, Creature* /*creature*/, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_DO_YOUR_WORST_A || quest->GetQuestId() == QUEST_DO_YOUR_WORST_H)
            player->CastSpell(player, SPELL_REFURBISHED_DEMOLISHER, true);

        return false;
    }
};

/*######
## Quest The Art of Being a Water Terror
######*/

class npc_water_terror : public CreatureScript
{
public:
    npc_water_terror() : CreatureScript("npc_water_terror") { }
    
    struct npc_water_terrorAI : public VehicleAI
    {
        npc_water_terrorAI(Creature* creature) : VehicleAI(creature) { }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetEntry() == 30644 || victim->GetEntry() == 30725 || victim->GetEntry() == 29880 ||
                victim->GetEntry() == 30632 || victim->GetEntry() == 30243 || victim->GetEntry() == 30250)
            {
                if (Player* player = me->GetVehicleKit()->GetPassenger(0)->ToPlayer())
                    player->KilledMonsterCredit(30644);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_water_terrorAI(creature);
    }
};

/*#####
# npc_Apprentice_Osterkilgr
######*/
enum ApprenticeOsterkilgrDatas
{
    SPELL_BLAST_WAVE          = 60290,
    SPELL_FIREBALL            = 14034,
    QUEST_DEEP_IN_THE_BOWELS  = 13042,
    NPC_KILL_CREDIT_BUNNY     = 30412,
};

class npc_apprentice_osterkilgr : public CreatureScript
{
public:
    npc_apprentice_osterkilgr() : CreatureScript("npc_apprentice_osterkilgr") { }
    
    struct npc_apprentice_osterkilgrAI : public ScriptedAI
    {
        npc_apprentice_osterkilgrAI(Creature* creature) : ScriptedAI(creature) { }
        
        void Reset() override
        {
            toggle       = true;
            spellTimer   = 4000;
            questCheck   = 100;
            questStepEnd = false;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (spellTimer <= diff)
            {
                DoCastVictim(toggle ? SPELL_BLAST_WAVE : SPELL_FIREBALL);
                spellTimer = 5 * IN_MILLISECONDS;
                toggle = !toggle;
            }
            else
                spellTimer -= diff;

            if (questCheck <= diff && !questStepEnd)
            {
                if (me->GetHealth() <= 5 * IN_MILLISECONDS)
                {
                    questStepEnd = true;
                    me->MonsterSay("The doctor entrusted me with the plans to Nergeld, our flesh giant amalgamation made entirely of vargul! It will be the most powerful creation of its kind and a whole legion of them will be created to destroy your pitiful forces!",0,NULL);

                    std::list<Player*> players;
                    me->GetPlayerListInGrid(players, 50.f);

                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        Player* player = *itr;

                        if (player->GetQuestStatus(QUEST_DEEP_IN_THE_BOWELS) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (Creature* tempBunny = me->SummonCreature(NPC_KILL_CREDIT_BUNNY,*me ,TEMPSUMMON_TIMED_DESPAWN,1000))
                                player->KilledMonsterCredit(NPC_KILL_CREDIT_BUNNY, tempBunny->GetGUID());
                        }
                    }
                }
                questCheck = 0.1 * IN_MILLISECONDS;
            }
            else
                questCheck -= diff;

            DoMeleeAttackIfReady();
        }
    private:
        bool toggle;
        uint64 spellTimer;
        uint64 questCheck;
        bool questStepEnd;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_apprentice_osterkilgrAI(creature);
    }
};

/*#####
# npc_father_jhadras
######*/
enum FatherJMisc
{
    // Spells
    HEAL_SPELL        = 15586,
    HOLY_SMITE_SPELL  = 25054,

    // Events
    EVENT_HEAL        = 0,
    EVENT_HOLY_SMITE  = 1
};

class npc_father_jhadras : public CreatureScript
{
public:
    npc_father_jhadras() : CreatureScript("npc_father_jhadras") { }

    struct npc_father_jhadrasAI : public ScriptedAI
    {
        npc_father_jhadrasAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_HEAL, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_HEAL:
                        if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                            me->CastSpell(target, HEAL_SPELL, 0);
                        _events.ScheduleEvent(EVENT_HOLY_SMITE, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_HOLY_SMITE:
                        me->CastSpell(me->GetVictim(), HOLY_SMITE_SPELL, 0);
                        _events.ScheduleEvent(EVENT_HEAL, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_father_jhadrasAI(creature);
    }
};

/*#####
# npc_masud
######*/
enum MasudMisc 
{
    //Spells
    SPELL_CLEAVE       = 15496,
    SPELL_WHIRLWIND1   = 41056,
    SPELL_WHIRLWIND2   = 41057,

    // Events
    EVENT_CLEAVE       = 0,
    EVENT_WW1          = 1,
    EVENT_WW2          = 2
};

class npc_masud : public CreatureScript
{
public:
    npc_masud() : CreatureScript("npc_masud") { }

    struct npc_masudAI : public ScriptedAI
    {
        npc_masudAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_CLEAVE, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CLEAVE:
                        me->CastSpell(me->GetVictim(), SPELL_CLEAVE, false);
                        _events.ScheduleEvent(EVENT_WW1, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_WW1:
                        me->CastSpell(me->GetVictim(), SPELL_WHIRLWIND1, false);
                        _events.ScheduleEvent(EVENT_WW2, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_WW2:
                        me->CastSpell(me->GetVictim(), SPELL_WHIRLWIND2, false);
                        _events.ScheduleEvent(EVENT_CLEAVE, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_masudAI(creature);
    }
};

/*#####
# npc_geness_half_soul
######*/
enum GenessHalfSoulMisc
{
    // Spells
    SPELL_BRUTAL_FIRST = 61041,
    SPELL_SNAP_KICK    = 46182,

    // Events
    EVENT_BRUTAL_FIRST = 0,
    EVENT_SNAP_KICK    = 1
};

class npc_geness_half_soul : public CreatureScript
{
public:
    npc_geness_half_soul() : CreatureScript("npc_geness_half_soul") { }

    struct npc_geness_half_soulAI : public ScriptedAI
    {
        npc_geness_half_soulAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_BRUTAL_FIRST, 3 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BRUTAL_FIRST:
                        me->CastSpell(me->GetVictim(), SPELL_BRUTAL_FIRST, false);
                        _events.ScheduleEvent(EVENT_SNAP_KICK, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_SNAP_KICK:
                        me->CastSpell(me->GetVictim(), SPELL_SNAP_KICK, false);
                        _events.ScheduleEvent(EVENT_BRUTAL_FIRST, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_geness_half_soulAI(creature);
    }
};

/*#####
# npc_talla
######*/
enum TallaMisc
{
    // Spells
    SPELL_CRIPPLING_POISON = 30981,
    SPELL_SINISTER_STRIKE  = 14873,

    // Events
    EVENT_CRIPPLING_POISON = 0,
    EVENT_SINISTER_STRIKE  = 1
};

class npc_talla : public CreatureScript
{
public:
    npc_talla() : CreatureScript("npc_talla") { }

    struct npc_tallaAI : public ScriptedAI
    {
        npc_tallaAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_CRIPPLING_POISON, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CRIPPLING_POISON:
                        me->CastSpell(me->GetVictim(), SPELL_CRIPPLING_POISON, false);
                        _events.ScheduleEvent(EVENT_SINISTER_STRIKE, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_SINISTER_STRIKE:
                        me->CastSpell(me->GetVictim(), SPELL_SINISTER_STRIKE, false);
                        _events.ScheduleEvent(EVENT_CRIPPLING_POISON, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tallaAI(creature);
    }
};

/*#####
# npc_eldreth
######*/
enum EldrethMisc
{
    // Spells
    SPELL_ARCANE_SHOT            = 34829,
    SPELL_MAGIC_DAMENDING_FIELD  = 44475,
    SPELL_SHOOT                  = 15620,

    // Events
    EVENT_ARCANE_SHOT            = 0,
    EVENT_MAGIC_DAMENDING_FIELD  = 1,
    EVENT_SHOOT                  = 2
};

class npc_eldreth : public CreatureScript
{
public:
    npc_eldreth() : CreatureScript("npc_eldreth") { }

    struct npc_eldrethAI : public ScriptedAI
    {
        npc_eldrethAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_ARCANE_SHOT, 7 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff); 

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_ARCANE_SHOT:
                        me->CastSpell(me->GetVictim(), SPELL_ARCANE_SHOT, false);
                        _events.ScheduleEvent(EVENT_MAGIC_DAMENDING_FIELD, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_MAGIC_DAMENDING_FIELD:
                        me->CastSpell(me->GetVictim(), SPELL_MAGIC_DAMENDING_FIELD, false);
                        _events.ScheduleEvent(EVENT_SHOOT, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_SHOOT:
                        me->CastSpell(me->GetVictim(), SPELL_SHOOT, false);
                        _events.ScheduleEvent(EVENT_ARCANE_SHOT, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_eldrethAI(creature);
    }
};

/*#####
# npc_rith
######*/
enum RithMisc
{
    // Spells
    SPELL_DEMORALIZING_SHOUT = 61044,
    SPELL_SUNDER_ARMOR       = 58461,

    // Events
    EVENT_DEMORALIZING_SHOUT = 0,
    EVENT_SUNDER_ARMOR       = 1
};

class npc_rith : public CreatureScript
{
public:
    npc_rith() : CreatureScript("npc_rith") { }

    struct npc_rithAI : public ScriptedAI
    {
        npc_rithAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_DEMORALIZING_SHOUT, 2.5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_DEMORALIZING_SHOUT:
                        me->CastSpell(me->GetVictim(), SPELL_DEMORALIZING_SHOUT, false);
                        _events.ScheduleEvent(EVENT_SUNDER_ARMOR, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_SUNDER_ARMOR: 
                        me->CastSpell(me->GetVictim(), SPELL_SUNDER_ARMOR, false);
                        _events.ScheduleEvent(EVENT_DEMORALIZING_SHOUT, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_rithAI(creature);
    }
};

/*#####
# npc_khitrix_the_dark_master
######*/
enum KhitrixTheDarkMasterMisc 
{
    // Spells
    SPELL_MIND_FLAY        = 38243,
    SPELL_PSYCHIC_SCREAM   = 22884,
    //SUMMON_BONE_SPIDERS  = 61055, // Wrong add dmg

    // Events
    EVENT_MIND_FLAY        = 0,
    EVENT_PSYCHIC_SCREAM   = 1,
    EVENT_SUMMON_ADD       = 2
};

enum KhitrixAdds 
{
    NPC_BONE_SPIDER        = 32484
};

class npc_khitrix_the_dark_master : public CreatureScript
{
public:
    npc_khitrix_the_dark_master() : CreatureScript("npc_khitrix_the_dark_master") { }

    struct npc_khitrix_the_dark_masterAI : public ScriptedAI
    {
        npc_khitrix_the_dark_masterAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_MIND_FLAY, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_MIND_FLAY:
                        me->CastSpell(me->GetVictim(), SPELL_MIND_FLAY, false);
                        _events.ScheduleEvent(EVENT_PSYCHIC_SCREAM, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_PSYCHIC_SCREAM:
                        me->CastSpell(me->GetVictim(), SPELL_PSYCHIC_SCREAM, false);
                        _events.ScheduleEvent(EVENT_SUMMON_ADD, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_SUMMON_ADD:
                        for (uint8 i = 0; i < 2; ++i)
                            me->SummonCreature(NPC_BONE_SPIDER, me->GetPositionX() + irand(-5, 5), me->GetPositionY() + irand(-5, 5), me->GetPositionZ(), frand(-M_PI_F, M_PI_F), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
                        _events.ScheduleEvent(EVENT_MIND_FLAY, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_khitrix_the_dark_masterAI(creature);
    }
};

/*#####
# npc_bone_spider
######*/
enum BoneSpiderSpells 
{
    SPELL_POISON           = 744,
};

class npc_bone_spider : public CreatureScript
{
public:
    npc_bone_spider() : CreatureScript("npc_bone_spider") { }

    struct npc_bone_spiderAI : public ScriptedAI
    {
        npc_bone_spiderAI(Creature* creature) : ScriptedAI(creature) { }
        
        void Reset() override
        {
            spellTimer = urand(5, 10)*IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (spellTimer <= diff)
            {
                me->CastSpell(me->GetVictim(), SPELL_POISON, false);
                spellTimer = 5*IN_MILLISECONDS;
            } else spellTimer -= diff;

            DoMeleeAttackIfReady();
        }
        private:
            uint64 spellTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bone_spiderAI(creature);
    }
};

/*#####
# npc_Sigrid_Iceborn
######*/
enum SigridIcebornMisc 
{
    // Spells
    SPELL_DISENGAGE        = 57635,
    SPELL_FROSTBITE_WEAPON = 61165,
    SPELL_FREEZE           = 61170,
    SPELL_THROW            = 61168,

    // Events
    EVENT_DISENGAGE        = 0,
    EVENT_FROSTBITE_WEAPON = 1,
    EVENT_FREEZE           = 2,
    EVENT_THROW            = 3
};

class npc_sigrid_iceborn : public CreatureScript
{
public:
    npc_sigrid_iceborn() : CreatureScript("npc_sigrid_iceborn") { }

    struct npc_sigrid_icebornAI : public ScriptedAI
    {
        npc_sigrid_icebornAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_THROW, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_THROW:
                        me->CastSpell(me->GetVictim(), SPELL_THROW, false);
                        _events.ScheduleEvent(EVENT_FREEZE, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_FREEZE:
                        me->CastSpell(me->GetVictim(), SPELL_FREEZE, false);
                        _events.ScheduleEvent(EVENT_DISENGAGE, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_DISENGAGE:
                        me->CastSpell(me->GetVictim(), SPELL_DISENGAGE, false);
                        _events.ScheduleEvent(EVENT_FROSTBITE_WEAPON, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_FROSTBITE_WEAPON:
                        me->CastSpell(me->GetVictim(), SPELL_FROSTBITE_WEAPON, false);
                        _events.ScheduleEvent(EVENT_THROW, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_sigrid_icebornAI(creature);
    }
};

/*#####
# npc_carnage
######*/
enum CarnageMisc
{
    // Spells
    SPELL_SMASH         = 61070,
    SPELL_WAR_STOMP     = 61065,

    // Events
    EVENT_SMASH         = 0,
    EVENT_WAR_STOMP     = 1
};

class npc_carnage : public CreatureScript
{
public:
    npc_carnage() : CreatureScript("npc_carnage") { }

    struct npc_carnageAI : public ScriptedAI
    {
        npc_carnageAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_SMASH, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SMASH:
                        me->CastSpell(me->GetVictim(), SPELL_SMASH, false);
                        _events.ScheduleEvent(EVENT_WAR_STOMP, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_WAR_STOMP:
                        me->CastSpell(me->GetVictim(), SPELL_WAR_STOMP, false);
                        _events.ScheduleEvent(EVENT_SMASH, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_carnageAI(creature);
    }
};

/*#####
# npc_thane_banahogg
######*/
enum ThaneBanahoggMisc
{
    // Spells
    SPELL_EXECUTE        = 61140,
    SPELL_LEAP           = 61134,
    SPELL_MORTAL_STRIKE  = 13737,
    SPELL_PUNT           = 61133,

    // Events
    EVENT_EXECUTE        = 0,
    EVENT_LEAP           = 1,
    EVENT_MORTAL_STRIKE  = 2,
    EVENT_PUNT           = 3,
    SAY_YELL             = 0
};

class npc_thane_banahogg : public CreatureScript
{
public:
    npc_thane_banahogg() : CreatureScript("npc_thane_banahogg") { }

    struct npc_thane_banahoggAI : public ScriptedAI
    {
        npc_thane_banahoggAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_EXECUTE, 5 * IN_MILLISECONDS);
        }

        void KilledUnit(Unit* /*victim*/) override
        {
            Talk(SAY_YELL);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_EXECUTE:
                        me->CastSpell(me->GetVictim(), SPELL_EXECUTE, false);
                        _events.ScheduleEvent(EVENT_MORTAL_STRIKE, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_MORTAL_STRIKE:
                        me->CastSpell(me->GetVictim(), SPELL_MORTAL_STRIKE, false);
                        _events.ScheduleEvent(EVENT_PUNT, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_PUNT:
                        me->CastSpell(me->GetVictim(), SPELL_PUNT, false);
                        _events.ScheduleEvent(EVENT_LEAP, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_LEAP:
                        me->CastSpell(SelectTarget(SELECT_TARGET_RANDOM), SPELL_LEAP, false);
                        _events.ScheduleEvent(EVENT_EXECUTE, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_thane_banahoggAI(creature);
    }
};

/*#####
# npc_prince_sandoval
######*/
enum PrinceSandovalMisc
{
    // Spells
    SPELL_EMBER_SHOWER         = 61145,
    SPELL_ENGULFING_STRIKE     = 61162,
    SPELL_FIRE_NOVA            = 61163,
    SPELL_FIRE_SHIELD          = 61144,

    // Events
    EVENT_EMBER_SHOWER         = 0,
    EVENT_ENGULFING_STRIKE     = 1,
    EVENT_FIRE_NOVA            = 2,
    EVENT_FIRE_SHIELD          = 3,
    EVENT_REMOVE_FIRE_SHIELD   = 4
};

class npc_prince_sandoval : public CreatureScript
{
public:
    npc_prince_sandoval() : CreatureScript("npc_prince_sandoval") { }
   
    struct npc_prince_sandovalAI : public ScriptedAI
    {
        npc_prince_sandovalAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_ENGULFING_STRIKE, 5 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_ENGULFING_STRIKE:
                        me->CastSpell(me->GetVictim(), SPELL_ENGULFING_STRIKE, false);
                        _events.ScheduleEvent(EVENT_FIRE_NOVA, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_FIRE_NOVA:
                        me->CastSpell(me->GetVictim(), SPELL_FIRE_NOVA, false);
                        _events.ScheduleEvent(EVENT_FIRE_SHIELD, 8 * IN_MILLISECONDS);
                        break;
                    case EVENT_FIRE_SHIELD:
                        DoCastSelf(SPELL_FIRE_SHIELD, false);
                        me->AddAura(SPELL_EMBER_SHOWER, me);
                        me->SetReactState(REACT_PASSIVE);
                        me->AttackStop();
                        _events.ScheduleEvent(EVENT_REMOVE_FIRE_SHIELD, 15 * IN_MILLISECONDS);
                        break;
                    case EVENT_REMOVE_FIRE_SHIELD:
                        me->RemoveAurasDueToSpell(SPELL_FIRE_SHIELD);
                        me->RemoveAurasDueToSpell(SPELL_EMBER_SHOWER);
                        me->SetReactState(REACT_AGGRESSIVE);
                        _events.ScheduleEvent(EVENT_ENGULFING_STRIKE, 5 * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_prince_sandovalAI(creature);
    }
};

/*#####
# npc_Geirrvif
######*/
enum Geirrvif_Quests 
{
    QUEST_FALLEN_HEROS                 = 13214,
    QUEST_KHRITRIX_THE_DARK_MASTER     = 13215,
    QUEST_THE_RETURN_OF_SIGRID_ICEBORN = 13216,
    QUEST_CARNAGE                      = 13217,
    QUEST_THANE_DEATHBLOW              = 13218,
    QUEST_FINAL_CHALLANGE              = 13219,
};

enum Geirrvif_NPCs 
{
    NPC_FATHER_JHADRAS    = 31191,
    NPC_MASUD             = 31192,
    NPC_GENESS_HALF_SOUL  = 31193,
    NPC_TALLA             = 31194,
    NPC_ELDRETH           = 31195,
    NPC_RITH              = 31196,
    NPC_KHITRIX           = 31222,
    NPC_SIGRID_ICEBORN    = 31242,
    NPC_CARNAGE           = 31271,
    NPC_THANE_BANAHOGG    = 31277,
    NPC_PRINCE_SANDOVAL   = 14688,
};

enum Geirrvif_Yells
{
    WHISPER_HEROES          =  3,
    WHISPER_KHITRIX         =  7,
    WHISPER_SIGRID          = 10,
    WHISPER_CARNAGE         = 14,
    WHISPER_BANAHOGG        = 18,
    WHISPER_SANDOVAL        = 23,
};

class npc_geirrvif : public CreatureScript
{
public:
    npc_geirrvif() : CreatureScript("npc_geirrvif") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        CAST_AI(npc_geirrvif::npc_geirrvifAI, creature->AI())->StartQuest(quest->GetQuestId(), player);
         return true;
    };
    
    struct npc_geirrvifAI : public ScriptedAI
    {
        npc_geirrvifAI(Creature* c) : ScriptedAI(c), summons(me) {}
        bool end;
        bool questAction;
        uint8 questListNum;
        uint32 c_questId;
        uint32 tcheckTimer;
        ObjectGuid temp_unitGUIDs[6];
        ObjectGuid c_playerGUID;
        Position questGiver;
        SummonList summons;

        void JustSummoned(Creature* summoned) override
        {
            summons.Summon(summoned);
        }

        void Reset() override
        {
            tcheckTimer = 4 * IN_MILLISECONDS;
            questGiver.m_positionX = 8216.250000f;
            questGiver.m_positionY = 3516.229980f;
            questGiver.m_positionZ = 624.958191f;
            questGiver.m_orientation = 3.839720f;
            questAction = false;
            c_questId = 0;
            c_playerGUID.Clear();
            questListNum = 0;
            end = false;
            for(uint8 i=0;i<=5;i++)
            {
                temp_unitGUIDs[i].Clear();
            }
            me->GetMotionMaster()->MovePoint(0,questGiver.m_positionX,questGiver.m_positionY,(questGiver.m_positionZ+5));
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            summons.DespawnAll();
        }

        void StartQuest(uint32 questId, Player* player)
        {
            if(!questAction)
            {
                me->GetMotionMaster()->MovePoint(0,8216.250000f,3516.229980f,652.197998f);
                questAction = true;
                c_questId = questId;
                c_playerGUID = player->GetGUID();
                if(questId == QUEST_FALLEN_HEROS)
                    questListNum = 1;
                else if(questId == QUEST_KHRITRIX_THE_DARK_MASTER)
                    questListNum = 2;
                else if(questId == QUEST_THE_RETURN_OF_SIGRID_ICEBORN)
                    questListNum = 3;
                else if(questId == QUEST_CARNAGE)
                    questListNum = 4;
                else if(questId == QUEST_THANE_DEATHBLOW)
                    questListNum = 5;
                else if(questId == QUEST_FINAL_CHALLANGE)
                    questListNum = 6;
            }
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void setGroupQuestStatus(Player* player, bool complete, uint32 questId)
        {
            if(player != NULL && questId > 0)
            {
                if (Group* group = player->GetGroup())
                {
                    for (GroupReference* groupRef = group->GetFirstMember(); groupRef != NULL; groupRef = groupRef->next())
                        if (Player* member = groupRef->GetSource())
                            if (member->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (complete)
                                    member->CompleteQuest(questId);
                                else
                                    member->FailQuest(questId);
                            }
                }
                else
                {
                    if (player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
                    {
                        if (complete)
                            player->CompleteQuest(questId);
                        else
                            player->FailQuest(questId);
                    }
                }
            }
        }

        void mustResetCheck(uint64 questId)
        {
               bool allDeadOrNoPlayer = true;
               std::list<Player*> players;
            me->GetPlayerListInGrid(players, 100.f);
               for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
               {
                   Player* player = *itr;
                   if (me->IsInRange(player, 0.0f, 60.0f) && player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE && player->IsAlive())
                          allDeadOrNoPlayer  = false;
                   else if (me->IsInRange(player, 60.1f, 100.0f) && player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
                          player->FailQuest(questId);
               }
               if(allDeadOrNoPlayer)
                   Reset();
        }

        void UpdateAI(uint32 diff) override
        {
            if(questAction && c_questId > 0)
            {
                if (tcheckTimer <= diff)
                {
                    if(questListNum == 1)
                    {
                            Talk(WHISPER_HEROES, ObjectAccessor::GetPlayer(*me, c_playerGUID));

                            if(Unit* unit = me->SummonCreature(NPC_FATHER_JHADRAS,8181.665527f,3489.661865f,625.571350f,0.680721f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[0] = unit->GetGUID();

                            if(Unit* unit = me->SummonCreature(NPC_MASUD,8162.127441f,3507.384277f,628.325012f,6.108286f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[1] = unit->GetGUID();

                            if(Unit* unit = me->SummonCreature(NPC_GENESS_HALF_SOUL,8190.076172f,3469.043945f,628.029175f,1.083708f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[2] = unit->GetGUID();

                            if(Unit* unit = me->SummonCreature(NPC_RITH,8220.075195f,3477.661377f,626.865173f,1.868655f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[3] = unit->GetGUID();

                            if(Unit* unit = me->SummonCreature(NPC_TALLA,8195.468750f,3472.481934f,627.487488f,1.199964f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[4] = unit->GetGUID();

                            if(Unit* unit = me->SummonCreature(NPC_ELDRETH,8165.192871f,3516.848633f,627.933960f,6.220230f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[5] = unit->GetGUID();
                    }
                    else if(questListNum == 2)
                    {
                            Talk(WHISPER_KHITRIX, ObjectAccessor::GetPlayer(*me, c_playerGUID));

                            if(Unit* unit = me->SummonCreature(NPC_KHITRIX,8181.665527f,3489.661865f,625.571350f,0.680721f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[0] = unit->GetGUID();
                    }
                    else if(questListNum == 3)
                    {
                            Talk(WHISPER_SIGRID, ObjectAccessor::GetPlayer(*me, c_playerGUID));

                            if(Unit* unit = me->SummonCreature(NPC_SIGRID_ICEBORN,8181.665527f,3489.661865f,625.571350f,0.680721f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[0] = unit->GetGUID();
                    }
                    else if(questListNum == 4)
                    {
                            Talk(WHISPER_CARNAGE, ObjectAccessor::GetPlayer(*me, c_playerGUID));

                            if(Unit* unit = me->SummonCreature(NPC_CARNAGE,8181.665527f,3489.661865f,625.571350f,0.680721f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[0] = unit->GetGUID();
                    }
                    else if(questListNum == 5)
                    {
                            Talk(WHISPER_BANAHOGG, ObjectAccessor::GetPlayer(*me, c_playerGUID));

                            if(Unit* unit = me->SummonCreature(NPC_THANE_BANAHOGG,8181.665527f,3489.661865f,625.571350f,0.680721f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[0] = unit->GetGUID();
                    }
                    else if(questListNum == 6)
                    {
                            Talk(WHISPER_SANDOVAL, ObjectAccessor::GetPlayer(*me, c_playerGUID));

                            if(Unit* unit = me->SummonCreature(NPC_PRINCE_SANDOVAL,8181.665527f,3489.661865f,625.571350f,0.680721f, TEMPSUMMON_MANUAL_DESPAWN, 120000))
                                temp_unitGUIDs[0] = unit->GetGUID();
                    }
                    else if(questListNum == 100) {  //Waiting till end
                            end = true;
                            if(c_questId == QUEST_FALLEN_HEROS)
                            {
                                for(uint8 i=0;i<=5;i++)
                                {
                                    if(temp_unitGUIDs[i])
                                    {
                                        if(Unit* unit = ObjectAccessor::GetUnit(*me,temp_unitGUIDs[i]))
                                            if(unit->IsAlive())
                                                end = false;
                                    }
                                }
                            }
                            else
                            {
                                if(Unit* unit = ObjectAccessor::GetUnit(*me,temp_unitGUIDs[0]))
                                        if(unit->IsAlive())
                                                end = false;
                            }

                            if(end)
                            {
                                questAction = false;
                                tcheckTimer = 4 * IN_MILLISECONDS;
                                if(Player* player = ObjectAccessor::GetPlayer(*me, c_playerGUID))
                                    setGroupQuestStatus(player, true, c_questId);
                            }
                    }

                    if(questListNum != 100)
                    {
                        for(uint8 i=0;i<=5;i++)
                        {
                            if(temp_unitGUIDs[i])
                                if(Unit* unit = ObjectAccessor::GetUnit(*me,temp_unitGUIDs[i]))
                                    unit->GetMotionMaster()->MovePoint(0,questGiver.GetPositionX(),questGiver.GetPositionY(),questGiver.GetPositionZ());
                        }
                    }

                    questListNum = 100;
                    mustResetCheck(c_questId);

                    tcheckTimer = 1 * IN_MILLISECONDS;
                } else tcheckTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_geirrvifAI(creature);
    }
};

/*######
## Quest 13279/13295: Basic Chemistry
## npc_neutralizing_the_plague_trigger
######*/

enum BasicChemistry
{
    // Spells
    SPELL_NEUTRALIZE_PLAGUE      = 59655,
    SPELL_CLOUD_EFFECT           = 63084,

    // Creatures
    NPC_LIVING_PLAGUE            = 32181,
    NPC_PLAGUE_DRENCHED_GHOUL    = 32176,
    NPC_RAMPAGING_GHOUL          = 32178,
    NPC_PLAGUE_CAULDRON_KC_BUNNY = 31767,

    // Texts
    SAY_BEGIN                    = 0,
    SAY_SPAWN                    = 1,
    SAY_CONTINUE                 = 2,
    SAY_WARNING                  = 3,
    SAY_FAIL                     = 4,
    SAY_SUCCESS                  = 5
};

class npc_neutralizing_the_plague_trigger : public CreatureScript
{
public:
    npc_neutralizing_the_plague_trigger() : CreatureScript("npc_neutralizing_the_plague_trigger") { }
    
    struct npc_neutralizing_the_plague_triggerAI : public ScriptedAI
    {
        npc_neutralizing_the_plague_triggerAI(Creature* creature) : ScriptedAI(creature) {}
        
        void Reset() override
        {
            EventRunning    = false;
            ShowWarning     = false;
            NeutralizeTimer = 0;
            SpawnTimer      = 0;
            WaveCounter     = 0;
            me->RemoveAura(SPELL_CLOUD_EFFECT);
        }

        void UpdateAI(uint32 diff) override
        {
            if (EventRunning)
            {
                if (SpawnTimer <= diff)
                {
                    if (WaveCounter < 8)
                    {
                        Talk(SAY_SPAWN);
                        WaveCounter++;
                        SpawnTimer = 30 * IN_MILLISECONDS;
                        switch (urand(0, 3))
                        {
                            case 0: // 1x Plague Drenched Ghoul
                                me->SummonCreature(NPC_PLAGUE_DRENCHED_GHOUL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 180000);
                                break;
                            case 1: // 2x Rampaging Ghoul
                                me->SummonCreature(NPC_RAMPAGING_GHOUL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 180000);
                                me->SummonCreature(NPC_RAMPAGING_GHOUL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 180000);
                                break;
                            case 2: // 8x Living Plague
                                for (int i = 0; i < 8; i++)
                                {
                                    me->SummonCreature(NPC_LIVING_PLAGUE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                                }
                                break;
                            case 3: // 1x Rampaging Ghoul + 4x Living Plague
                                me->SummonCreature(NPC_RAMPAGING_GHOUL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 180000);
                                for (int i = 0; i < 4; i++)
                                {
                                    me->SummonCreature(NPC_LIVING_PLAGUE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                                }
                                break;
                        }
                    }
                    else
                    {
                        Talk(SAY_SUCCESS);
                        std::list<Player*> players;
                        me->GetPlayerListInGrid(players, 50.f);
                        for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
                        {
                            Player* player = *pitr;
                            player->KilledMonsterCredit(NPC_PLAGUE_CAULDRON_KC_BUNNY);
                        }
                        Reset();
                        return;
                    }
                }
                else
                {
                    SpawnTimer -= diff;
                }

                if (NeutralizeTimer < 5 * IN_MILLISECONDS && ShowWarning)
                {
                    ShowWarning = false;
                    Talk(SAY_WARNING);
                }

                if (NeutralizeTimer <= diff)
                {
                    Talk(SAY_FAIL);
                    Reset();
                }
                else
                {
                    NeutralizeTimer -= diff;
                }
            }

        }

        void SpellHit(Unit* /*unit*/, const SpellInfo* spell) override
        {
            if (spell->Id == SPELL_NEUTRALIZE_PLAGUE)
            {
                if (!EventRunning)
                {
                    EventRunning = true;
                    ShowWarning = true;
                    SpawnTimer = 10 * IN_MILLISECONDS;
                    NeutralizeTimer = 50 * IN_MILLISECONDS;
                    WaveCounter = 0;
                    me->AddAura(SPELL_CLOUD_EFFECT, me);
                    Talk(SAY_BEGIN);
                }
                else
                {
                    Talk(SAY_CONTINUE);
                    NeutralizeTimer = 50 * IN_MILLISECONDS;
                    ShowWarning = true;
                }
            }
        }
        private:
            bool EventRunning;
            bool ShowWarning;
            uint32 NeutralizeTimer;
            uint32 SpawnTimer;
            uint32 WaveCounter;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_neutralizing_the_plague_triggerAI(creature);
    }
};

/*######
## Quest 13169: An Undead's Best Friend
## npc_bloody_meat
######*/

enum AnUndeadsBestFriend
{
    // Creatures
    NPC_HUNGERING_PLAGUEHOUND = 30952,
    NPC_BLOODY_MEAT           = 31119,

    // Spells
    SPELL_SLEEPING_SLEEP      = 42386,

    // Events
    EVENT_AWAKE               = 1
};

class npc_bloody_meat : public CreatureScript
{
public:
    npc_bloody_meat() : CreatureScript("npc_bloody_meat") { }
    
    struct npc_bloody_meatAI : public ScriptedAI
    {
        npc_bloody_meatAI(Creature* creature) : ScriptedAI(creature) {}
       
        void Reset() override
        {
            checkTimer = 1 * IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff) override
        {
            // dat number of layers
            if (checkTimer <= diff)
            {
                if (Creature* hound = me->FindNearestCreature(NPC_HUNGERING_PLAGUEHOUND, 3.0f))
                {
                    if (!hound->HasAura(SPELL_SLEEPING_SLEEP))
                    {
                        hound->CastSpell(hound, SPELL_SLEEPING_SLEEP);
                        if (TempSummon* temp = me->ToTempSummon())
                        {
                            if (Unit* summoner = temp->GetSummoner())
                            {
                                if (summoner->GetTypeId() == TYPEID_UNIT)
                                {
                                    if (Vehicle* vehicle = summoner->GetVehicleKit())
                                    {
                                        if (Unit* passenger = vehicle->GetPassenger(0))
                                        {
                                            if (Player* player = passenger->ToPlayer())
                                            {
                                                player->KilledMonsterCredit(NPC_BLOODY_MEAT);
                                                me->DespawnOrUnsummon();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                checkTimer = urand(700, 1200) ;
            }
            else
                checkTimer -= diff;
        }
        private:
            uint32 checkTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bloody_meatAI(creature);
    }
};

class npc_hungering_plaguehound : public CreatureScript
{
public:
    npc_hungering_plaguehound() : CreatureScript("npc_hungering_plaguehound") { }
    
    struct npc_hungering_plaguehoundAI : public ScriptedAI
    {
        npc_hungering_plaguehoundAI(Creature* creature) : ScriptedAI(creature) {}
        
        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);
            switch (events.ExecuteEvent())
            {
                case EVENT_AWAKE:
                    me->RemoveAura(SPELL_SLEEPING_SLEEP);
                    break;
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void SpellHit(Unit* /*unit*/, const SpellInfo* spell) override
        {
            if (spell->Id == SPELL_SLEEPING_SLEEP)
            {
                events.ScheduleEvent(EVENT_AWAKE, urand(15, 20) * IN_MILLISECONDS);
            }
        }
        private:
            EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_hungering_plaguehoundAI(creature);
    }
};

/*######
## Quests 13264/13276/13288/13289: That's Abominable!
## spell_burst_at_the_seams
######*/

enum QuestThatsAbominable
{
    // Creatures
    NPC_ICY_GHOUL               = 31142,
    NPC_RISEN_ALLIANCE_SOLDIER  = 31205,
    NPC_VICIOUS_GEIST           = 31147,
    NPC_ICY_GHOUL_KC_BUNNY      = 31743,
    NPC_VICIOUS_GEIST_KC_BUNNY  = 32168,
    NPC_RISEN_SKELETON_KC_BUNNY = 32167
};

class spell_burst_at_the_seams : public SpellScriptLoader
{
public:
    spell_burst_at_the_seams() : SpellScriptLoader("spell_burst_at_the_seams") { }

    class spell_burst_at_the_seams_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_burst_at_the_seams_SpellScript);

        void HandleScript(SpellEffIndex /*effIndex*/)
        {   
            if (Unit* caster = GetCaster())
            {
                if (Unit* charmer = caster->GetCharmerOrOwner())
                {
                    if (Player* player = charmer->ToPlayer())
                    {
                        std::list<Creature*> targetlist;
                        caster->GetCreatureListWithEntryInGrid(targetlist, NPC_ICY_GHOUL, 15.0f);
                        caster->GetCreatureListWithEntryInGrid(targetlist, NPC_RISEN_ALLIANCE_SOLDIER, 15.0f);
                        caster->GetCreatureListWithEntryInGrid(targetlist, NPC_VICIOUS_GEIST, 15.0f);
                        for (std::list<Creature*>::const_iterator itr = targetlist.begin(); itr != targetlist.end(); ++itr)
                        {
                            Creature* target = *itr;
                            if (target->IsAlive())
                            {
                                switch (target->GetEntry())
                                {
                                    case NPC_ICY_GHOUL:
                                        player->KilledMonsterCredit(NPC_ICY_GHOUL_KC_BUNNY);
                                        caster->Kill(target);
                                        break;
                                    case NPC_RISEN_ALLIANCE_SOLDIER:
                                        player->KilledMonsterCredit(NPC_RISEN_SKELETON_KC_BUNNY);
                                        caster->Kill(target);
                                        break;
                                    case NPC_VICIOUS_GEIST:
                                        player->KilledMonsterCredit(NPC_VICIOUS_GEIST_KC_BUNNY);
                                        caster->Kill(target);
                                        break;
                                }
                            }
                        }
                    }
                }
                caster->KillSelf();
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_burst_at_the_seams_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_burst_at_the_seams_SpellScript();
    }
};

/*######
## Quests 13142: Banshee's Revenge
## npc_possessed_vardmadra
## npc_overthane_balargarde
######*/

enum PossessedVardmadra
{
    // Misc
    DATA_STOP_PAUSE                 = 1,
    MODEL_ELITE_DRAKE               = 26882,

    // Creatures
    NPC_SAFIRDRANGS_CONTROLL_BUNNY  = 31117,
    NPC_SAFIDRANGS_CHILL_TARGET     = 31077,
    NPC_OVERTHANE_BALARGARDE        = 31016,
    NPC_SAFIRDRANG                  = 31050,
    NPC_BALAGARDE_ELITE             = 31030,
    NPC_LICH_KING                   = 31083,
    NPC_POSSESSED_VARDMADRA         = 31029,
    NPC_LADY_NIGHTSWOOD             = 30955,

    // Gameobjects
    GO_WAR_HORN_OF_JOTUNHEIM        = 193028,

    // Spells
    SPELL_TELEPORT_COSMETIC         = 52096,
    SPELL_POSSESSION                = 25745,
    SPELL_WHIRLDWIND                = 61076,
    SPELL_HEROIC_LEAP               = 60108,
    SPELL_FROST_BOLT                = 15043,
    SPELL_BLIZZARD                  = 61085,
    SPELL_SAFIDRANG_S_CHILL         = 4020,
};

static Position Location[]=
{
    {7093.573f, 4324.298f, 874.654f}, //0
    {7090.942f, 4393.932f, 889.661f}, //1
    {7094.547f, 4316.966f, 901.339f}, //2
    {7050.297f, 4333.915f, 901.339f}, //3
    {7061.911f, 4378.991f, 901.339f}, //4
    {7107.840f, 4363.500f, 901.339f}, //5
    {7101.296f, 4431.128f, 840.296f}, //6
    {7094.250f, 4406.616f, 898.718f}, //7
    {7082.126f, 4358.842f, 894.883f}, //8
    {7088.845f, 4358.436f, 871.909f}, //9
    {7088.871f, 4385.126f, 872.357f}, //10
    {7078.884f, 4292.316f, 874.743f}, //11
    {7076.046f, 4271.010f, 854.785f}, //12
};

class npc_possessed_vardmadra : public CreatureScript
{
public:
    npc_possessed_vardmadra() : CreatureScript("npc_possessed_vardmadra") { }

    struct npc_possessed_vardmadraAI : public ScriptedAI
    {
        npc_possessed_vardmadraAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 step;
        uint32 stepTimer;
        uint32 checkPlayerTimer;
        bool pause;

        ObjectGuid overthaneGUID;
        ObjectGuid safirdrangGUID;
        ObjectGuid lichkingGUID;

        void Reset() override
        {
            DespawnAll();
            step = 0;
            stepTimer = 0;
            checkPlayerTimer = 10 * IN_MILLISECONDS;
            pause = false;

            overthaneGUID.Clear();
            safirdrangGUID.Clear();
            lichkingGUID.Clear();
            
            me->SetCanFly(true);
            me->GetMotionMaster()->MovePoint(0, Location[0], false);
            me->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
            me->SendMovementFlagUpdate();
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);

            if (Creature* lady = me->SummonCreature(NPC_LADY_NIGHTSWOOD, Location[11]))
                lady->AddAura(SPELL_POSSESSION, me);
        }

        void SetData(uint32 data, uint32 /*value*/) override
        {
            if (data == DATA_STOP_PAUSE)
                pause = false;
        }

        void DespawnAll()
        {
            std::list<Creature*> creatureList;
            me->GetCreatureListWithEntryInGrid(creatureList, NPC_LICH_KING, 100.0f);
            me->GetCreatureListWithEntryInGrid(creatureList, NPC_BALAGARDE_ELITE, 100.0f);
            me->GetCreatureListWithEntryInGrid(creatureList, NPC_SAFIRDRANG, 100.0f);
            me->GetCreatureListWithEntryInGrid(creatureList, NPC_OVERTHANE_BALARGARDE, 100.0f);
            me->GetCreatureListWithEntryInGrid(creatureList, NPC_LADY_NIGHTSWOOD, 100.0f);
            me->GetCreatureListWithEntryInGrid(creatureList, NPC_SAFIDRANGS_CHILL_TARGET, 100.0f);
            
            if (creatureList.size())
                for (std::list<Creature*>::iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
                    (*itr)->DespawnOrUnsummon();

            if (GameObject* go = me->FindNearestGameObject(GO_WAR_HORN_OF_JOTUNHEIM, 50.0f))
                go->DespawnOrUnsummon();
        }

        void JustDied(Unit* /*killer*/) override
        {
            DespawnAll();
            me->DespawnOrUnsummon(1 * IN_MILLISECONDS);
        }

        void CorpseRemoved(uint32& /*respawnDelay*/)
        {
            DespawnAll();
        }

        void UpdateAI(uint32 diff) override
        {
            if (checkPlayerTimer <= diff)
            {
                if (!me->FindNearestPlayer(80.0f))
                {
                    DespawnAll();
                    me->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                }
                checkPlayerTimer = 10 * IN_MILLISECONDS;
            } else checkPlayerTimer -= diff;

            if (pause)
                return;

            if (stepTimer <= diff)
            {
                switch (step++)
                {
                    case 0:
                        stepTimer = 4 * IN_MILLISECONDS;
                        break;
                    case 1:
                        Talk(0);
                        if (Creature* overthane = me->SummonCreature(NPC_OVERTHANE_BALARGARDE, Location[6]))
                        {
                            overthane->setActive(true);
                            overthane->SetReactState(REACT_PASSIVE);
                            overthaneGUID = overthane->GetGUID();
                        }

                        for (uint32 i = 0; i < 4; i++)
                            if (Creature* elite = me->SummonCreature(NPC_BALAGARDE_ELITE, Location[1]))
                            {
                                elite->Mount(MODEL_ELITE_DRAKE);
                                elite->GetMotionMaster()->MovePoint(0, Location[i+2], false);
                                elite->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
                                elite->SetDisableGravity(true);
                                elite->SetCanFly(true);
                                elite->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                                elite->SetReactState(REACT_PASSIVE);
                            }
                        pause = true;
                        stepTimer = 5 * IN_MILLISECONDS;
                        break;
                    case 2:
                        if (Creature* overthane = ObjectAccessor::GetCreature(*me, overthaneGUID))
                            overthane->AI()->Talk(0);
                        stepTimer = 6 * IN_MILLISECONDS;
                        break;
                    case 3:
                        Talk(1);
                        stepTimer = 7 * IN_MILLISECONDS;
                        break;
                    case 4:
                        if (Creature* overthane = ObjectAccessor::GetCreature(*me, overthaneGUID))
                            overthane->AI()->Talk(1);
                        stepTimer = 7 * IN_MILLISECONDS;
                        break;
                    case 5:
                        if (Creature* overthane = ObjectAccessor::GetCreature(*me, overthaneGUID))
                        {
                            if (Creature* veh = overthane->GetVehicleCreatureBase())
                                safirdrangGUID = veh->GetGUID();

                            overthane->AI()->Talk(2);
                            overthane->ExitVehicle(&Location[9]);
                        }
                        stepTimer = 5 * IN_MILLISECONDS;
                        break;
                    case 6:
                        if (Creature* safirdrang = ObjectAccessor::GetCreature(*me, safirdrangGUID))
                            safirdrang->GetMotionMaster()->MovePoint(0, Location[8].m_positionX, Location[8].m_positionY, Location[8].m_positionZ + 10.0f, false);
                        stepTimer = 6 * IN_MILLISECONDS;
                        break;
                    case 7:
                        if (Creature* overthane = ObjectAccessor::GetCreature(*me, overthaneGUID))
                        {
                            if (Creature* safirdrang = ObjectAccessor::GetCreature(*me, safirdrangGUID))
                            {
                                safirdrang->SetReactState(REACT_AGGRESSIVE);
                                safirdrang->SetTarget(overthaneGUID);
                            }

                            overthane->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                            overthane->SetImmuneToAll(false);
                            overthane->AI()->SetData(DATA_STOP_PAUSE, 0);

                            if (Player* player = me->FindNearestPlayer(100.0f))
                            {
                                overthane->GetMotionMaster()->MoveChase(player);
                                overthane->Attack(player, false);
                            }
                        }
                        pause = true;
                        stepTimer = 1 * IN_MILLISECONDS;
                        break;
                    case 8:
                        if (Creature* lichking = me->SummonCreature(NPC_LICH_KING, Location[10]))
                        {
                            lichkingGUID = lichking->GetGUID();
                            lichking->SetReactState(REACT_PASSIVE);
                            lichking->SetWalk(true);
                        }

                        if (Creature* overthane = ObjectAccessor::GetCreature(*me, overthaneGUID))
                        {
                            overthane->AI()->Talk(3);
                            overthane->SetTarget(lichkingGUID);
                            overthane->HandleEmoteCommand(EMOTE_STATE_KNEEL);
                        }
                        
                        me->SetTarget(lichkingGUID);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                        me->RemoveUnitMovementFlag(MOVEMENTFLAG_HOVER);
                        me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                        me->SetCanFly(false);
                        me->SendMovementFlagUpdate();
                        stepTimer = 6 * IN_MILLISECONDS;
                        break;
                    case 9:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                        {
                            lichking->SetSpeedRate(MOVE_RUN, 0.6f); //HACK: I don't know why, but SetWalk doesen't work in this case
                            lichking->GetMotionMaster()->MoveChase(me);
                            lichking->AI()->Talk(0);
                        }
                        stepTimer = 5 * IN_MILLISECONDS;
                        break;
                    case 10:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                            lichking->AI()->Talk(1);
                        stepTimer = 10 * IN_MILLISECONDS;
                        break;
                    case 11:
                        Talk(2);
                        stepTimer = 4 * IN_MILLISECONDS;
                        break;
                    case 12:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                        lichking->AI()->Talk(2);
                        stepTimer = 7 * IN_MILLISECONDS;
                        break;
                    case 13:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                            lichking->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK1H);
                        me->SendPlaySound(15845, true);
                        stepTimer = 1 * IN_MILLISECONDS;
                        break;
                    case 14:
                        me->SetHealth(0);
                        me->GetMotionMaster()->MovementExpired(false);
                        me->GetMotionMaster()->MoveIdle();
                        me->SetStandState(UNIT_STAND_STATE_DEAD);
                        me->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
                        Talk(3);
                        stepTimer = 2 * IN_MILLISECONDS;
                        break;
                    case 15:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                            lichking->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                        stepTimer = 3 * IN_MILLISECONDS;
                        break;
                    case 16:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                        {
                            lichking->SetTarget(overthaneGUID);
                            lichking->AI()->Talk(3);
                        }
                        stepTimer = 5 * IN_MILLISECONDS;
                        break;
                    case 17:
                        if (Creature* overthane = ObjectAccessor::GetCreature(*me, overthaneGUID))
                            overthane->AI()->Talk(4);
                        stepTimer = 3 * IN_MILLISECONDS;
                        break;
                    case 18:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                            lichking->AI()->Talk(4);
                        stepTimer = 7 * IN_MILLISECONDS;
                        break;
                    case 19:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                            lichking->GetMotionMaster()->MovePoint(0, Location[10], false);
                        stepTimer = 8 * IN_MILLISECONDS;
                        break;
                    case 20:
                        if (Creature* overthane = ObjectAccessor::GetCreature(*me, overthaneGUID))
                        {
                            overthane->SetReactState(REACT_AGGRESSIVE);
                            overthane->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                            overthane->SetImmuneToAll(false);
                            overthane->AI()->SetData(DATA_STOP_PAUSE, 0);
                            overthane->AI()->Talk(5);
                            if (Unit* player = overthane->GetVictim())
                            {
                                overthane->SetTarget(player->GetGUID());
                                overthane->GetMotionMaster()->MoveChase(player);
                                overthane->Attack(player, false);
                            }
                        }
                        pause = true;
                        stepTimer = 1 * IN_MILLISECONDS;
                        break;
                    case 21:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                            if (Creature* overthane =  ObjectAccessor::GetCreature(*me, overthaneGUID))
                                lichking->GetMotionMaster()->MoveChase(overthane);
                        stepTimer = 12 * IN_MILLISECONDS;
                        break;
                    case 22:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                            lichking->AI()->Talk(5);
                        stepTimer = 7 * IN_MILLISECONDS;
                        break;
                    case 23:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                            lichking->AI()->Talk(6);
                        stepTimer = 7 * IN_MILLISECONDS;
                        break;
                    case 24:
                        if (Creature* lichking = ObjectAccessor::GetCreature(*me, lichkingGUID))
                            lichking->CastSpell(lichking, SPELL_TELEPORT_COSMETIC, false);

                        DespawnAll();
                        me->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                        break;
                }
            } else stepTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_possessed_vardmadraAI(creature);
    }

};

class npc_overthane_balargarde : public CreatureScript
{
public:
    npc_overthane_balargarde() : CreatureScript("npc_overthane_balargarde") { }

    struct npc_overthane_balargardeAI : public ScriptedAI
    {
        npc_overthane_balargardeAI(Creature* creature) : ScriptedAI(creature) {}
        
        bool reachedPosition;
        bool under50Pct;
        bool pause;
        bool safidrangTalked;

        uint32 wp;

        uint32 moveTimer;
        uint32 whirlwindTimer;
        uint32 heroicLeapTimer;
        uint32 frostboltTimer;
        uint32 blizzardTimer;
        uint32 safidrangChillTimer;

        void Reset() override
        {
            reachedPosition = false;
            under50Pct = false;
            pause = true;
            safidrangTalked = false;

            wp = 0;

            moveTimer           = 2 * IN_MILLISECONDS;
            whirlwindTimer      = 15 * IN_MILLISECONDS;
            heroicLeapTimer     = 45 * IN_MILLISECONDS;
            frostboltTimer      = 18 * IN_MILLISECONDS;
            blizzardTimer       = 12 * IN_MILLISECONDS;
            safidrangChillTimer = 24 * IN_MILLISECONDS;
            
            Creature* veh = nullptr;

            if (Creature* safidring = me->FindNearestCreature(NPC_SAFIRDRANG, 100.0f))
                veh = safidring;
            else
                veh = me->SummonCreature(NPC_SAFIRDRANG, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());

            if (veh)
            {
                me->EnterVehicle(veh, 0);
                veh->SetReactState(REACT_PASSIVE);
                veh->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
                veh->SetDisableGravity(true);
                veh->SetCanFly(true);
                veh->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            }
        }

        void DamageTaken(Unit* /*dealer*/, uint32& /*damage*/) override
        {
            if (under50Pct)
                return;

            if (me->GetHealthPct() >= 50.0f)
                return;

            under50Pct = true;
            pause = true;
            
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetImmuneToAll(true);
            DoStartNoMovement(me->GetVictim());

            if (Creature* vardmadra = me->FindNearestCreature(NPC_POSSESSED_VARDMADRA, 100.0f))
                vardmadra->AI()->SetData(DATA_STOP_PAUSE, 0);
        }

        void EnterEvadeMode(EvadeReason /*why*/) { return; }

        void SetData(uint32 data, uint32 /*value*/) override
        {
            if (data == DATA_STOP_PAUSE)
            {
                pause = false;
                DoStartMovement(me->GetVictim());
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (Creature* vardmadra = me->FindNearestCreature(NPC_POSSESSED_VARDMADRA, 100.0f))
                vardmadra->AI()->SetData(DATA_STOP_PAUSE, 0);

            if (Creature* lady = me->FindNearestCreature(NPC_LADY_NIGHTSWOOD, 100.0f))
                lady->GetMotionMaster()->MovePoint(0, Location[12]);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!reachedPosition)
            {
                if (moveTimer <= diff)
                {
                    if (Creature* veh = me->GetVehicleCreatureBase())
                    {
                        if (!veh->isMoving())
                        {
                            if (wp++ == 0)
                                veh->GetMotionMaster()->MovePoint(0, Location[7], false);
                            else
                            {
                                veh->GetMotionMaster()->MovePoint(0, Location[8], false);
                                                    
                                if (Creature* vardmadra = me->FindNearestCreature(NPC_POSSESSED_VARDMADRA, 100.0f))
                                    vardmadra->AI()->SetData(DATA_STOP_PAUSE, 0);

                                reachedPosition = true;
                            }
                        }
                    }
                    moveTimer = 1 * IN_MILLISECONDS;
                } else moveTimer -= diff;
            }
            
            if (!UpdateVictim() || pause)
                return;

            if (whirlwindTimer <= diff)
            {
                DoCast(SPELL_WHIRLDWIND);
                whirlwindTimer = 35 * IN_MILLISECONDS;
            } else whirlwindTimer -= diff;

            if (heroicLeapTimer <= diff)
            {
                DoCast(SPELL_HEROIC_LEAP);
                heroicLeapTimer = 45 * IN_MILLISECONDS;
            } else heroicLeapTimer -= diff;

            if (frostboltTimer <= diff)
            {
                DoCast(SPELL_FROST_BOLT);
                frostboltTimer = 15 * IN_MILLISECONDS;
            } else frostboltTimer -= diff;

            if (blizzardTimer <= diff)
            {
                DoCast(SPELL_BLIZZARD);
                blizzardTimer = 24 * IN_MILLISECONDS;
            } else blizzardTimer -= diff;

            if (safidrangChillTimer <= diff)
            {
                if (Creature* safi = me->FindNearestCreature(NPC_SAFIRDRANG, 100.0f))
                {
                    if (Creature* controll = me->FindNearestCreature(NPC_SAFIRDRANGS_CONTROLL_BUNNY, 100.0f))
                    {
                        Position pos;
                        controll->GetRandomNearPosition(pos, 35.0f);
                        controll->SummonCreature(NPC_SAFIDRANGS_CHILL_TARGET, pos);
                    }

                    safi->AI()->DoCast(SPELL_SAFIDRANG_S_CHILL);

                    if (!safidrangTalked)
                    {
                        safidrangTalked = true;
                        Talk(6);
                    }
                }
                safidrangChillTimer = 15 * IN_MILLISECONDS;
            } else safidrangChillTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_overthane_balargardeAI(creature);
    }

};

/*######
## Quest 13145: The Vile Hold
## Quest 13146: Generosity Abounds
## Quest 13147: Matchmaker
## go_eye_of_dominion
######*/

enum MatchmakerData
{
    //Quests
    QUEST_VILE_HOLD                         = 13145,
    QUEST_GENEROSITY_ABOUNDS                = 13146,
    QUEST_MATCHMAKER                        = 13147,
    QUEST_STUNNING_VIEW                     = 13160,

    // Spells
    SPELL_NPC_CONTROL_STALKER               = 58037
};

enum GossipData
{
    GOSSIP_MENU_ID      = 10111,
    GOSSIP_ITEM_ID      = 0,

    GOSSIP_TEXT_ID      = 13906
};

Position const LitheStalkerSpawnPos = { 6463.812f, 2037.34f, 570.931f, 3.990328f };

class go_eye_of_dominion : public GameObjectScript
{
public:
    go_eye_of_dominion() : GameObjectScript("go_eye_of_dominion") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        player->AddGossipItem(GOSSIP_MENU_ID, GOSSIP_ITEM_ID, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        player->SEND_GOSSIP_MENU(GOSSIP_TEXT_ID, go->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, GameObject* go, uint32 sender, uint32 action)
    {
        player->CLOSE_GOSSIP_MENU();
        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            if ((player->GetQuestStatus(QUEST_MATCHMAKER) == QUEST_STATUS_INCOMPLETE)
                || (player->GetQuestStatus(QUEST_VILE_HOLD) == QUEST_STATUS_INCOMPLETE)
                || (player->GetQuestStatus(QUEST_GENEROSITY_ABOUNDS) == QUEST_STATUS_INCOMPLETE)
                || (player->GetQuestStatus(QUEST_STUNNING_VIEW) == QUEST_STATUS_INCOMPLETE))
                    player->CastSpell(player,SPELL_NPC_CONTROL_STALKER);
            return true;
        }
        return true;
     }
};

enum litheStalker
{
    // Spells
    SPELL_SUBDUED_LITH_STALKER      = 58151,
    SPELL_GEIST_RETURN_KILL_CREDIT  = 58190,

    // Creatures
    NPC_GEIST_RETURN_BUNNY          = 31049,
    NPC_METAL_POST_BUNNY            = 30990
};

static Position LocationJump[] =
{
    { 6467.014f, 1999.749f, 615.722f, 0 }, // 0
    { 6480.433f, 2012.213f, 600.694f, 0 }, // 1
    { 6476.288f, 2020.399f, 591.236f, 0 }, // 2
    { 6469.236f, 2037.410f, 570.606f, 0 }  // 3
};

/*######
## Quest 13143: New Rectruit
## npc_lithe_stalker
######*/
class npc_lithe_stalker : public CreatureScript
{
    public:
        npc_lithe_stalker() : CreatureScript("npc_lithe_stalker") { }

        struct npc_lithe_stalkerAI : public ScriptedAI
        {
            npc_lithe_stalkerAI(Creature* creature) : ScriptedAI(creature) {}

            bool follow;
            bool walkHome;
            ObjectGuid owner_guid;
            uint32 checkTimer;


            void Reset() override
			{
                follow = false;
                walkHome = false;
                owner_guid.Clear();
                checkTimer = 1 * IN_MILLISECONDS;
			}

            void OnCharmed(bool apply) override { }
	            
            void MovementInform(uint32 /*type*/, uint32 id) override
            {
                switch (id)
                {
                    case 1:
                        if (Unit* owner = ObjectAccessor::GetPlayer(*me, owner_guid))
                            me->CastSpell(owner, SPELL_GEIST_RETURN_KILL_CREDIT, true);
                        Talk(0, me);
                        me->GetMotionMaster()->MoveJump(LocationJump[0], 10.0f, 10.0f, 2);
                        break;
                    case 2:
                        me->GetMotionMaster()->MoveJump(LocationJump[1], 10.0f, 10.0f, 3);
                        break;
                    case 3:
                        me->GetMotionMaster()->MoveJump(LocationJump[2], 10.0f, 5.0f, 4);
                        break;
                    case 4:
                        me->GetMotionMaster()->MoveJump(LocationJump[3], 10.0f, 5.0f, 5);                        
                        break;
                    default:
                        break;
                }
            }

            void SpellHit(Unit* caster, const SpellInfo* spell) override
            {
                if (spell->Id == SPELL_SUBDUED_LITH_STALKER)
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        follow = true;
                        me->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                        owner_guid = caster->GetGUID();
                    }
            }

            void EnterCombat(Unit* unit) override
            {
                if (walkHome)
                    return;
            }

            void UpdateAI(uint32 diff) override
			{
                if (follow && !walkHome)
                {
                    if (checkTimer <= diff)
                    {
                        if (Creature* creature = me->FindNearestCreature(NPC_GEIST_RETURN_BUNNY, 5.0f))
                        {
                            follow = false;
                            walkHome = true;
                            me->GetMotionMaster()->MovePoint(1, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), false);
                            me->DespawnOrUnsummon(12 * IN_MILLISECONDS);
                        }
                        checkTimer = 1 * IN_MILLISECONDS;
                    } else checkTimer -= diff;
                }

                if (!UpdateVictim())
                    return;

				DoMeleeAttackIfReady();
			}

		};
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_lithe_stalkerAI(creature);
        }
};

/*######
## Quest: The Flesh Giant Champion
## npc_morbidus();
## npc_margrave_dhakar();
######*/
enum FleshGiant
{
    QUEST_FLESH_GIANT_CHAMPION      = 13235,
    NPC_MORBIDUS                    = 30698,
    NPC_LICH_KING_QUEST             = 31301,

    // Factions
    FACTION_HOSTILE_QUEST           = 974,
    FACTION_BASIC                   = 2102,
    // Events
    EVENT_INTRO                     = 1,
    EVENT_LK_SAY_1                  = 2,
    EVENT_LK_SAY_2                  = 3,
    EVENT_LK_SAY_3                  = 4,
    EVENT_LK_SAY_4                  = 5,
    EVENT_LK_SAY_5                  = 6,

    EVENT_OUTRO                     = 7,
    EVENT_START                     = 8,
    // Morbidus
    EVENT_STOMP                     = 9,
    EVENT_CORRODE_FLESH             = 10,
    // Dhakar
    EVENT_SWING                     = 11,
    EVENT_TAUNT                     = 12,

    // Spells
    SPELL_SIMPLE_TELEPORT           = 64195, 
    SPELL_STOMP                     = 31277,
    SPELL_CORRODE_FLESH             = 72728,
    SPELL_SWING                     = 5547,
    SPELL_TAUNT                     = 37548,

    // Dhakar
    SAY_START                       = 1,
    // Lich King
    SAY_TEXT_1                      = 1,
    SAY_TEXT_2                      = 2,
    SAY_TEXT_3                      = 3,
    SAY_TEXT_4                      = 4,
    SAY_TEXT_5                      = 5,
    // Olakin
    SAY_PAY                         = 1,
    NPC_MARGRAVE                    = 31306,
    NPC_OLAKIN                      = 31428
};

class npc_margrave_dhakar : public CreatureScript
{
    public:
        npc_margrave_dhakar() : CreatureScript("npc_margrave_dhakar") { }

        struct npc_margrave_dhakarAI : public ScriptedAI
        {
            npc_margrave_dhakarAI(Creature* creature) : ScriptedAI(creature) , summons(me)
			{
                Reset();
			}
			
            ObjectGuid LKGuid;
			SummonList summons;

            void Reset() override
			{
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetImmuneToAll(true);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                events.Reset();
                summons.DespawnAll();
			}

            void EnterCombat(Unit* /*who*/) override
            {
                events.ScheduleEvent(EVENT_SWING, 3*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_TAUNT, 5*IN_MILLISECONDS);
            }

            void sGossipSelect(Player* player, uint32 sender, uint32 action)
			{
                if (player->GetQuestStatus(QUEST_FLESH_GIANT_CHAMPION) == QUEST_STATUS_INCOMPLETE)
                {
                    if (me->GetCreatureTemplate()->GossipMenuId == sender && !action)
                    {
                        events.ScheduleEvent(EVENT_INTRO, 1*IN_MILLISECONDS);
                        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    }
                }
            }

            void JustDied(Unit* /*killer*/) override
            {
                me->DespawnOrUnsummon();
                me->SetRespawnTime(1);
                me->SetCorpseDelay(1);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetImmuneToAll(true);
                me->SetReactState(REACT_PASSIVE);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INTRO:
                            Talk(SAY_START);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY2H);
                            if (Creature *morbidus = me->FindNearestCreature(NPC_MORBIDUS, 50.0f, true))
                            {
                                Creature *LK = me->SummonCreature(NPC_LICH_KING_QUEST, morbidus->GetPositionX()+10, morbidus->GetPositionY(), morbidus->GetPositionZ());
                                LKGuid = LK->GetGUID();
                                LK->SetFacingTo(morbidus->GetOrientation());
                                LK->CastSpell(LK, SPELL_SIMPLE_TELEPORT, true);
                            }
                            events.ScheduleEvent(EVENT_LK_SAY_1, 5*IN_MILLISECONDS);
                            events.CancelEvent(EVENT_INTRO);
                            break;
                        case EVENT_LK_SAY_1:
                            if (Creature *LK = ObjectAccessor::GetCreature(*me, LKGuid))
                            {
                                LK->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_POINT);
                                LK->AI()->Talk(SAY_TEXT_1);
                            }
                            events.ScheduleEvent(EVENT_LK_SAY_2, 5*IN_MILLISECONDS);
                            events.CancelEvent(EVENT_LK_SAY_1);
                            break;
                        case EVENT_LK_SAY_2:
                            if (Creature *LK = ObjectAccessor::GetCreature(*me, LKGuid))
                            {
                                LK->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_SHOUT);
                                LK->AI()->Talk(SAY_TEXT_2);
                            }
                            events.ScheduleEvent(EVENT_LK_SAY_3, 5*IN_MILLISECONDS);
                            events.CancelEvent(EVENT_LK_SAY_2);
                            break;
                        case EVENT_LK_SAY_3:
                            if (Creature *LK = ObjectAccessor::GetCreature(*me, LKGuid))
                            {
                                LK->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_POINT);
                                LK->AI()->Talk(SAY_TEXT_3);
                            }
                            events.ScheduleEvent(EVENT_LK_SAY_4, 5*IN_MILLISECONDS);
                            events.CancelEvent(EVENT_LK_SAY_3);
                            break;
                        case EVENT_LK_SAY_4:
                            if (Creature *LK = ObjectAccessor::GetCreature(*me, LKGuid))
                            {
                                LK->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_SHOUT);
                                LK->AI()->Talk(SAY_TEXT_4);
                            }
                            events.ScheduleEvent(EVENT_OUTRO, 12*IN_MILLISECONDS);
                            events.CancelEvent(EVENT_LK_SAY_4);
                            break;
                        case EVENT_LK_SAY_5:
                            if (Creature *LK = ObjectAccessor::GetCreature(*me, LKGuid))
                            {
                                LK->AI()->Talk(SAY_TEXT_5);
                                LK->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_POINT);
                            }
                            events.ScheduleEvent(EVENT_OUTRO, 8*IN_MILLISECONDS);
                            events.CancelEvent(EVENT_LK_SAY_5);
                            break;
                        case EVENT_OUTRO:
                            if (Creature *olakin = me->FindNearestCreature(NPC_OLAKIN, 50.0f, true))
                            {
                                olakin->AI()->Talk(SAY_PAY);
                            }
                            if (Creature *LK = ObjectAccessor::GetCreature(*me, LKGuid))
                                LK->DespawnOrUnsummon(0);
                            events.ScheduleEvent(EVENT_START, 5*IN_MILLISECONDS);
                            events.CancelEvent(EVENT_OUTRO);
                            break;
                        case EVENT_START:
                            if (Creature *morbidus = me->FindNearestCreature(NPC_MORBIDUS, 50.0f, true))
                            {
                                morbidus->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                                morbidus->SetImmuneToAll(false);
                                morbidus->setFaction(FACTION_HOSTILE_QUEST);
                                morbidus->SetReactState(REACT_AGGRESSIVE);
                            }
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                            me->SetImmuneToAll(false);
                            me->SetReactState(REACT_AGGRESSIVE);
                            events.CancelEvent(EVENT_START);
                            break;
                        case EVENT_SWING:
                            DoCastVictim(SPELL_SWING, true);
                            events.ScheduleEvent(EVENT_SWING, 8*IN_MILLISECONDS);
                            break;
                        case EVENT_TAUNT:
                            DoCastVictim(SPELL_TAUNT, true);
                            events.ScheduleEvent(EVENT_TAUNT, 5*IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
          private:
              EventMap events;
};

CreatureAI* GetAI(Creature* creature) const override
{
    return new npc_margrave_dhakarAI(creature);
}
};

class npc_morbidus : public CreatureScript
{
public:
    npc_morbidus() : CreatureScript("npc_morbidus") { }

    struct npc_morbidusAI : public ScriptedAI
    {
        npc_morbidusAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            events.Reset();
            if (Creature * dhakar = me->FindNearestCreature(NPC_MARGRAVE, 50.0f, true))
                dhakar->AI()->Reset();
            // this will prevent the event to start without morbidus being alive
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetImmuneToAll(true);
            me->SetReactState(REACT_PASSIVE);
            me->setFaction(FACTION_BASIC);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            events.ScheduleEvent(EVENT_STOMP, 3*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CORRODE_FLESH, 8*IN_MILLISECONDS);
        }
        
        void JustDied(Unit* /*killer*/) override
        {
            if (Creature * dhakar = me->FindNearestCreature(NPC_MARGRAVE, 100.0f, true))
            {
                dhakar->DespawnOrUnsummon();
                dhakar->SetCorpseDelay(1);
                dhakar->SetRespawnTime(1);               
            }
            me->SetRespawnTime(1);
            me->SetCorpseDelay(1);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_STOMP:
                        DoCastVictim(SPELL_STOMP, true);
                        events.ScheduleEvent(EVENT_STOMP, 5*IN_MILLISECONDS);
                        break;
                    case EVENT_CORRODE_FLESH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                            me->AddAura(SPELL_CORRODE_FLESH, target);
                        events.ScheduleEvent(EVENT_CORRODE_FLESH, 10*IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    private:
        EventMap events;
    };
    
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_morbidusAI(creature);
    }
};

class npc_icc_phase_shifter : public CreatureScript
{
public:
	npc_icc_phase_shifter() : CreatureScript("npc_icc_phase_shifter") { }

	struct npc_icc_phase_shifterAI : public ScriptedAI
	{
		npc_icc_phase_shifterAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
		{
            checkTimer = 1 * IN_MILLISECONDS;
		}

        void UpdateAI(uint32 diff) override
		{
			if (checkTimer <= diff)
			{
                std::list<Player*> players;
                me->GetPlayerListInGrid(players, 200.f);
				for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
				{
					Player* player = *pitr;
					if (player->GetAreaId() == 4522 && player->GetPositionZ() >= 610.0f && player->GetPositionZ() <= 700.0f)
					{
						player->RemoveAura(59073);
						player->RemoveAura(59087);
						player->RemoveAura(64576);
					}
					else
					{
						player->UpdateAreaDependentAuras(player->GetAreaId());
					}
				}

                checkTimer = 1 * IN_MILLISECONDS;
			}
			else
			{
				checkTimer -= diff;
			}
		}

	private:
		uint32 checkTimer;
	};

    CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_icc_phase_shifterAI(creature);
	}
};

enum BomberMisc
{
    // Horde Version
    SPELL_WAIT_FOR_BOMBER   = 59779,
    NPC_CANNON              = 31840,
    SPELL_RIDE_VEHICLE      = 46598,
    NPC_BOMBER              = 31856,
    NPC_ENGINEERING         = 32152,
    NPC_AMBUSHER            = 32188,
    NPC_FROSTBROOD          = 31721,
    SPELL_ROCKET            = 62363,

    // Alliance Version
    SPELL_WAIT_FOR_BOMBER_A = 59563,
    NPC_CANNON_A            = 31407,
    NPC_BOMBER_A            = 31408,
    NPC_ENGINEERING_A       = 31409
};

class spell_bomber_bay : public SpellScriptLoader
{
    public:
        spell_bomber_bay() : SpellScriptLoader("spell_bomber_bay") { }

        class spell_bomber_bay_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_bomber_bay_SpellScript)
        public:
            spell_bomber_bay_SpellScript() : SpellScript() { };

            void HandleAfterCast()
            {
                if (Unit* vehicle = GetCaster())
                {
                    if (Unit* player = vehicle->GetVehicleKit()->GetPassenger(0))
                    {
                        if (Creature* bomber = player->FindNearestCreature(NPC_BOMBER_A, 10.0f))
                        {
                            player->ExitVehicle();
                            player->EnterVehicle(bomber);
                        }
                        else if (Creature* bomber = player->FindNearestCreature(NPC_BOMBER, 10.0f))
                        {
                            player->ExitVehicle();
                            player->EnterVehicle(bomber);
                        }
                    }                                                  
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_bomber_bay_SpellScript::HandleAfterCast);
            }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_bomber_bay_SpellScript();
    }
};

class spell_anti_air_turret : public SpellScriptLoader
{
    public:
        spell_anti_air_turret() : SpellScriptLoader("spell_anti_air_turret") { }

        class spell_anti_air_turret_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_anti_air_turret_SpellScript)
        public:
            spell_anti_air_turret_SpellScript() : SpellScript() { };

            void HandleAfterCast()
            {
                if (Unit* vehicle = GetCaster())
                {
                    if (Unit* player = vehicle->GetVehicleKit()->GetPassenger(0))
                    {
                        if (Creature* turret = player->FindNearestCreature(NPC_CANNON_A, 10.0f))
                        {
                            player->ExitVehicle();
                            player->EnterVehicle(turret);
                        }
                        else if (Creature* turret = player->FindNearestCreature(NPC_CANNON, 10.0f))
                        {
                            player->ExitVehicle();
                            player->EnterVehicle(turret);
                        }
                    }                        
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_anti_air_turret_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_anti_air_turret_SpellScript();
        }
};

class spell_engineering : public SpellScriptLoader
{
    public:
        spell_engineering() : SpellScriptLoader("spell_engineering") { }

        class spell_engineering_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_engineering_SpellScript)
        public:
            spell_engineering_SpellScript() : SpellScript() { };

            void HandleAfterCast()
            {
                if (Unit* vehicle = GetCaster())
                {
                    if (Unit* player = vehicle->GetVehicleKit()->GetPassenger(0))
                    {
                        if (Creature* engineering = player->FindNearestCreature(NPC_ENGINEERING_A, 10.0f))
                        {
                            player->ExitVehicle();
                            player->EnterVehicle(engineering);
                        }
                        else if (Creature* engineering = player->FindNearestCreature(NPC_ENGINEERING, 10.0f))
                        {
                            player->ExitVehicle();
                            player->EnterVehicle(engineering);
                        }
                    }                        
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_engineering_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_engineering_SpellScript();
        }
};

class npc_bomber_horde : public CreatureScript
{
    public:
        npc_bomber_horde() : CreatureScript("npc_bomber_horde") { }

        struct npc_bomber_hordeAI : public SmartAI
        {
            npc_bomber_hordeAI(Creature* creature) : SmartAI(creature) { }

            void SetData(uint32 type, uint32 data) override
            {
                if (type == 1 && data == 1)
                {
                    if (Creature* cannon = me->FindNearestCreature(NPC_CANNON, 10.0f))
                        if (Player* player = me->FindNearestPlayer(80.0f))
                            player->CastSpell(cannon, SPELL_RIDE_VEHICLE);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_bomber_hordeAI(creature);
        }
};

class npc_bomber_alliance : public CreatureScript
{
    public:
        npc_bomber_alliance() : CreatureScript("npc_bomber_alliance") { }

        struct npc_bomber_allianceAI : public SmartAI
        {
            npc_bomber_allianceAI(Creature* creature) : SmartAI(creature) { }

            void SetData(uint32 type, uint32 data) override
            {
                if (type == 1 && data == 1)
                {
                    if (Creature* cannon = me->FindNearestCreature(NPC_CANNON_A, 10.0f))
                        if (Player* player = me->FindNearestPlayer(80.0f))
                            player->CastSpell(cannon, SPELL_RIDE_VEHICLE);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_bomber_allianceAI(creature);
        }
};

class npc_jotunheim_proto_drake : public CreatureScript
{
    public:
        npc_jotunheim_proto_drake() : CreatureScript("npc_jotunheim_proto_drake") { }

        struct npc_jotunheim_proto_drakeAI : public SmartAI
        {
            npc_jotunheim_proto_drakeAI(Creature* creature) : SmartAI(creature) { }

            void JustDied(Unit* /*killer*/) override
            {
                me->DespawnOrUnsummon();
                me->SetRespawnTime(60);
                me->SetCorpseDelay(0);
            }        
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_jotunheim_proto_drakeAI(creature);
        }
};

enum Deathstormmisc
{
    NPC_LORDAERON_FOOTSOLDIER = 31254
};

class DeathstormTargets
{   
    public:
        bool operator()(WorldObject* object) const
        {
            return object->GetEntry() != NPC_LORDAERON_FOOTSOLDIER;
        }
};

// - 58912 Deathstorm
class spell_icecrown_deathstorm : public SpellScriptLoader
{
    public:
        spell_icecrown_deathstorm() : SpellScriptLoader("spell_icecrown_deathstorm") { }

        class spell_icecrown_deathstorm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icecrown_deathstorm_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(DeathstormTargets());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_icecrown_deathstorm_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_TARGET_ANY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_icecrown_deathstorm_SpellScript();
        }
};

class npc_argent_cannon : public CreatureScript
{
    public:
        npc_argent_cannon() : CreatureScript("npc_argent_cannon") { }

        struct npc_argent_cannonAI : public VehicleAI
        {
            npc_argent_cannonAI(Creature* creature) : VehicleAI(creature) { }

            void PassengerBoarded(Unit* passenger, int8 seatId, bool apply) override
            {
                if (!apply && passenger->GetTypeId() == TYPEID_PLAYER)
                {
                    me->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                    me->SetRespawnTime(2);
                    me->SetCorpseDelay(2);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_argent_cannonAI(creature);
        }
};

enum LLODMisc
{
    NPC_FORGOTTEN_DEPTHS_SLAYER = 30593,
    NPC_FROSTBOOD_DESTROYER     = 30575 
};

class npc_llod_generic : public CreatureScript
{
    public:
        npc_llod_generic() : CreatureScript("npc_llod_generic") { }
    
        struct npc_llod_genericAI : public CombatAI
        {
            npc_llod_genericAI(Creature* pCreature) : CombatAI(pCreature) { attackTimer = 0; summonTimer = 1; }
                    
            void Reset() override
            {
                summonTimer = 1;
                CombatAI::Reset();
            }
    
            void UpdateAI(uint32 diff) override
            {
                attackTimer += diff;

                if (attackTimer >= 1.5 * IN_MILLISECONDS)
                {
                    if (!me->IsInCombat())
                        if (Unit* target = me->SelectNearbyTarget(NULL, 20.0f))
                            AttackStart(target);

                    attackTimer = 0;
                }
    
                if (summonTimer)
                {
                    summonTimer += diff;
                    if (summonTimer >= 8 * IN_MILLISECONDS)
                    {
                        for (uint8 i = 0; i < 3; ++i)
                            me->SummonCreature(NPC_FORGOTTEN_DEPTHS_SLAYER, me->GetPositionX() + irand(-5, 5), me->GetPositionY() + irand(-5, 5), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000);
                        if (roll_chance_i(10))
                            me->SummonCreature(NPC_FROSTBOOD_DESTROYER, me->GetPositionX() + irand(-5, 5), me->GetPositionY() + irand(-5, 5), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000);
                        summonTimer = 0;
                    }
                }
    
                CombatAI::UpdateAI(diff);
            }

        private:
            uint32 attackTimer;
            uint32 summonTimer;
        };
    
        CreatureAI *GetAI(Creature *creature) const override
        {
            return new npc_llod_genericAI(creature);
        }
};

/*######
## Borrowed Technology - Id: 13291, The Solution Solution (daily) - Id: 13292, Volatility - Id: 13239, Volatiliy - Id: 13261 (daily)
######*/

enum BorrowedTechnologyAndVolatility
{
    // NPCs
    NPC_FROSTBROOD_SKYTALON_KC_BUNNY    = 31364,

    // Spells
    SPELL_GRAB                          = 59318,
    SPELL_PING_BUNNY                    = 59375,
    SPELL_IMMOLATION                    = 54690,
    SPELL_EXPLOSION                     = 59335,
    SPELL_RIDE                          = 59319,

    // Points
    POINT_GRAB_DECOY                    = 1,
    POINT_FLY_AWAY                      = 2,

    // Events
    EVENT_FLY_AWAY                      = 1,

    // Fake Soldier
    SPELL_FAKE_SOLDIER_FREEZE_ANIM      = 59292,
    SPELL_SUMMON_FROST_WYRM             = 59303,
    SPELL_FAKE_SOLDIER_KILL_CREDIT      = 59329,

};

class npc_frostbrood_skytalon : public CreatureScript
{
public:
    npc_frostbrood_skytalon() : CreatureScript("npc_frostbrood_skytalon") { }

    struct npc_frostbrood_skytalonAI : public VehicleAI
    {
        npc_frostbrood_skytalonAI(Creature* creature) : VehicleAI(creature) { }

        void IsSummonedBy(Unit* summoner) override
        {
            me->GetMotionMaster()->MovePoint(POINT_GRAB_DECOY, summoner->GetPositionX(), summoner->GetPositionY(), summoner->GetPositionZ());
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == POINT_GRAB_DECOY)
                if (TempSummon* summon = me->ToTempSummon())
                    if (Unit* summoner = summon->GetSummoner())
                        DoCast(summoner, SPELL_GRAB);

            if (id == POINT_FLY_AWAY)
                DoCastSelf(SPELL_EXPLOSION);
        }

        void UpdateAI(uint32 diff) override
        {
            VehicleAI::UpdateAI(diff);
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_FLY_AWAY)
                {
                    Position randomPosOnRadius;
                    randomPosOnRadius.m_positionZ = (me->GetPositionZ() + 40.0f);
                    me->GetNearPoint2D(randomPosOnRadius.m_positionX, randomPosOnRadius.m_positionY, 40.0f, me->GetAngle(me));
                    me->GetMotionMaster()->MovePoint(POINT_FLY_AWAY, randomPosOnRadius);
                }
            }
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
        {
            switch (spell->Id)
            {
                case SPELL_EXPLOSION:
                    DoCastSelf(SPELL_IMMOLATION);
                    break;
                case SPELL_RIDE:
                    DoCastAOE(SPELL_PING_BUNNY);
                    events.ScheduleEvent(EVENT_FLY_AWAY, 100);
                    break;
            }
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_frostbrood_skytalonAI(creature);
    }
};

class npc_fake_soldier : public CreatureScript
{
public:
    npc_fake_soldier() : CreatureScript("npc_fake_soldier") { }

    struct npc_fake_soldierAI : public ScriptedAI
    {
        npc_fake_soldierAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            DoCastSelf(SPELL_FAKE_SOLDIER_FREEZE_ANIM);
            DoCastSelf(SPELL_SUMMON_FROST_WYRM);
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
        {
            if (spell->Id == SPELL_EXPLOSION)
            {
                if (TempSummon* summon = me->ToTempSummon())
                    if (Unit* summoner = summon->GetSummoner())
                        DoCast(summoner, SPELL_FAKE_SOLDIER_KILL_CREDIT, true);

                me->DespawnOrUnsummon();
            }
        }

    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_fake_soldierAI(creature);
    }
};

enum VileLikeFire
{
    SPELL_VILE_LIKE_FIRE_FIRE           = 7448,
    SPELL_STRAFE_JOTUNHEIM_BUILDING     = 7769,
    SPELL_VILE_LIKE_FIRE_SPREAD_FIRE    = 57469,

    EVENT_RESET                         = 1,
};

struct npc_vile_like_fire_kill_credit_bunnyAI : public ScriptedAI
{
    npc_vile_like_fire_kill_credit_bunnyAI(Creature* creature) : ScriptedAI(creature) { _isOnFire = false; }

    void SpellHit(Unit* caster, SpellInfo const* spell) override 
    {
        if (!_isOnFire && caster && spell)
        {
            if (spell->Id == SPELL_STRAFE_JOTUNHEIM_BUILDING)
            {
                DoCastSelf(SPELL_VILE_LIKE_FIRE_SPREAD_FIRE);

                if (caster->GetVehicleKit())
                    if (Unit* passenger = caster->GetVehicleKit()->GetPassenger(0))
                        passenger->ToPlayer()->KilledMonsterCredit(me->GetEntry(), me->GetGUID());

                _isOnFire = true;
                _events.ScheduleEvent(EVENT_RESET, Seconds(36));
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_RESET:
                    _events.Reset();
                    _isOnFire = false;
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap    _events;
    bool        _isOnFire;
};

void AddSC_icecrown_rg()
{
    new ItemUse_gore_bladder;
    new npc_alorah_and_grimmin;
    new spell_construct_barricade();
    new npc_not_a_bug_summon_bunny();
    new npc_slumbering_mjordin();
    new npc_subjugated_iskalder();
    new npc_bone_witch();
    new npc_matthias_lehner();
    new npc_water_terror();
    new npc_apprentice_osterkilgr();
    new npc_prince_sandoval();
    new npc_thane_banahogg();
    new npc_carnage();
    new npc_sigrid_iceborn();
    new npc_khitrix_the_dark_master();
    new npc_bone_spider();
    new npc_eldreth();
    new npc_talla();
    new npc_rith();
    new npc_geness_half_soul();
    new npc_masud();
    new npc_father_jhadras();
    new npc_geirrvif();
    new npc_neutralizing_the_plague_trigger();
    new npc_bloody_meat();
    new npc_hungering_plaguehound();
    new spell_burst_at_the_seams();
    new npc_possessed_vardmadra();
    new npc_overthane_balargarde();
    new go_eye_of_dominion();
    new npc_lithe_stalker();
    new npc_morbidus();
    new npc_margrave_dhakar();
	new npc_icc_phase_shifter();
    new spell_bomber_bay();
    new spell_anti_air_turret();
    new spell_engineering();
    new npc_bomber_horde();
    new npc_bomber_alliance();
    new npc_jotunheim_proto_drake();
    new spell_icecrown_deathstorm();
    new npc_argent_cannon();
    new npc_llod_generic();
    new npc_frostbrood_skytalon();
    new npc_fake_soldier();
    new CreatureScriptLoaderEx<npc_vile_like_fire_kill_credit_bunnyAI>("npc_vile_like_fire_kill_credit_bunny");
}
