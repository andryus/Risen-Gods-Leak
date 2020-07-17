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
#include "ScriptedFollowerAI.h"
#include "ScriptedGossip.h"
#include "Group.h"
#include "Vehicle.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "SmartAI.h"
#include "SpellScript.h"

enum QuestMasterTheStorm
{
    QUEST_MASTER_THE_STORM = 11895,
    NPC_STORM_TEMPEST      = 26045
};

Position StormTempestSummonPosition = {3403.83f, 4133.07f, 18.1375f, 5.75959f};

class npc_storm_totem : public CreatureScript
{
public:
    npc_storm_totem() : CreatureScript("npc_storm_totem") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->GetQuestStatus(QUEST_MASTER_THE_STORM) == QUEST_STATUS_INCOMPLETE)
        {
            if (!creature->FindNearestCreature(NPC_STORM_TEMPEST, 1000.0f, true))
            {
                if (Creature* storm = creature->SummonCreature(NPC_STORM_TEMPEST, StormTempestSummonPosition, TEMPSUMMON_DEAD_DESPAWN, 60000))
                    storm->AI()->AttackStart(player);
            }
        }
        return true;
    }
};

enum NaxxarPorterMisc
{
    MAP_NORTHREND = 571
};

class npc_naxxanar_teleport_trigger : public CreatureScript
{
public:
    npc_naxxanar_teleport_trigger() : CreatureScript("npc_naxxanar_teleport_trigger") { }

    struct npc_naxxanar_teleport_triggerAI : public ScriptedAI
    {
        npc_naxxanar_teleport_triggerAI(Creature* creature) : ScriptedAI(creature){}

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetDistance(me) <= 3.0f)
            {
                if (Player* player = who->ToPlayer())
                {
                    player->TeleportTo(MAP_NORTHREND, 3733.68f, 3563.25f, 290.812f, 3.665192f);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_naxxanar_teleport_triggerAI (creature);
    }
};

enum LongrunnerProudhoofMisc
{
    QUEST_WE_STRIKE          = 11592,
    NPC_STEELJAW             = 25359,
    NPC_WARSONG_RAIDER       = 31435,
    ACTION_START             = 1,
    SPELL_DEMORALIZING_SHOUT = 13730,
    SPELL_CLEAVE             = 35473,
    POINT_END                = 6
};

static Position weStrikeLocation[]=
{
    {4141.928f, 5783.671f, 60.562f},  //0
    {4142.567f, 5786.920f, 61.1796f}, //1
    {4140.004f, 5781.885f, 60.2525f}, //2
    {3878.59f, 5717.11f, 66.0835f}    //3
};

class npc_longrunner_proudhoof : public CreatureScript
{
public:
    npc_longrunner_proudhoof() : CreatureScript("npc_longrunner_proudhoof") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest) override
    {
        if (quest->GetQuestId() == QUEST_WE_STRIKE)
        {
            creature->AI()->SetGuidData(player->GetGUID(), ACTION_START);
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            if (npc_escortAI* pEscortAI = CAST_AI(npc_longrunner_proudhoof::npc_longrunner_proudhoofAI, creature->AI()))
                pEscortAI->Start(true, false, player->GetGUID());     

            for (uint8 i = 0; i < 3; i++)
                if (Creature* raider = creature->SummonCreature(NPC_WARSONG_RAIDER, weStrikeLocation[i]))
                {
                   raider->SetOwnerGUID(creature->GetGUID());
                   raider->GetMotionMaster()->MoveFollow(creature, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE + i);
                   raider->SetReactState(REACT_AGGRESSIVE);
                   raider->setFaction(FACTION_ESCORT_H_ACTIVE);
                   raider->SetFacingTo(raider->GetAngle(creature));
                }
            creature->SetReactState(REACT_AGGRESSIVE);
            creature->setFaction(FACTION_ESCORT_H_ACTIVE);
        }
        return true;
    }

    struct npc_longrunner_proudhoofAI : public npc_escortAI
    {
        npc_longrunner_proudhoofAI(Creature* creature) : npc_escortAI(creature), summons(me) { }
        
        void Reset() override
        {
            CleaveTimer = 3 * IN_MILLISECONDS;
            summons.DespawnAll();
            playerGUID.Clear();
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        }

        void SetGuidData(ObjectGuid guid, uint32 dataId) override
        {
            if (dataId == ACTION_START)
            {
                playerGUID = guid;
            }
        }

        void JustSummoned(Creature* summoned) override
        {
            summons.Summon(summoned);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            DoCastSelf(SPELL_DEMORALIZING_SHOUT);
        }

        void WaypointReached(uint32 point) override
        {
            if (point == POINT_END)
                me->SummonCreature(NPC_STEELJAW, weStrikeLocation[3], TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120 * IN_MILLISECONDS);
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (summon->GetEntry() == NPC_STEELJAW)
            {
                CompleteQuest();
                me->DespawnOrUnsummon(10 * IN_MILLISECONDS);
                me->SetRespawnDelay(10);
            }
        }

        void CompleteQuest()
        {
            if (Map *map = me->GetMap())
            {
                for (Player& player : map->GetPlayers())
                {
                    if (me->IsInRange(&player, 0.0f, 50.0f))
                        player.CompleteQuest(QUEST_WE_STRIKE);
                }
            }
        }

        void CorpseRemoved(uint32& /*time*/) override
        {
            for (SummonList::const_iterator iter = summons.begin(); iter != summons.end(); ++iter)
                if (Creature* creature = ObjectAccessor::GetCreature(*me, *iter))
                    creature->SetOwnerGUID(ObjectGuid::Empty);
            summons.DespawnAll();
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->DespawnOrUnsummon(6 * IN_MILLISECONDS/*0*/);
        }

        void UpdateAI(uint32 diff) override
        {
            npc_escortAI::UpdateAI(diff);

            if (!UpdateVictim())
                return;

            if (CleaveTimer <= diff)
            {
                DoCastVictim(SPELL_CLEAVE, true);
                CleaveTimer = 15 * IN_MILLISECONDS;
            }
            else
                CleaveTimer -= diff;

            DoMeleeAttackIfReady();
        }
    private:
        SummonList summons;
        ObjectGuid playerGUID;
        uint32 CleaveTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_longrunner_proudhoofAI (creature);
    }
};


enum TheAssassinationofHaroldLaneMisc
{
    //Spells
    SPELL_BLOW_CENARION_HORN           = 46368,
    SPELL_BEAR_TRAP                    = 53432,
    SPELL_THROW_BEAR_PELT              = 53425,

    // Events
    EVENT_BLOW_CENARION_HORN           = 1,
    EVENT_BEAR_TRAP                    = 2,
    EVENT_THROW_BEAR_PELT              = 3,

    // Texts
    SAY_AGGRO                          = 0,
    SAY_MAMMOTH                        = 1,

    // Creatures
    NPC_STAMPEDING_MAMMOTH             = 25988,
    NPC_STAMPEDING_RHINO               = 25990,
    NPC_STAMPEDING_CARIBOU             = 25989,

    // Data
    DATA_STAMPEDING_MAMMOTH            = 4
};

class npc_harold_lane : public CreatureScript
{
public:
    npc_harold_lane() : CreatureScript("npc_harold_lane") { }

    struct npc_harold_laneAI : public ScriptedAI
    {
        npc_harold_laneAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            SummonedAnimals = false;
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
        {
            if (spell->Id == SPELL_BLOW_CENARION_HORN)
                if (!SummonedAnimals)
                {
                    SummonedAnimals = true;
                    _events.ScheduleEvent(EVENT_BLOW_CENARION_HORN, 1 * IN_MILLISECONDS);
                }
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_STAMPEDING_MAMMOTH && data == DATA_STAMPEDING_MAMMOTH)
            { 
                me->DealDamage(me, me->GetMaxHealth() / 2, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL);
                Talk(SAY_MAMMOTH);
            }
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            _events.ScheduleEvent(EVENT_BEAR_TRAP,       10 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_THROW_BEAR_PELT, 15 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BEAR_TRAP:
                        DoCastSelf(SPELL_BEAR_TRAP, true);
                        _events.ScheduleEvent(EVENT_BEAR_TRAP, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_THROW_BEAR_PELT:
                        DoCastVictim(SPELL_THROW_BEAR_PELT);
                        _events.ScheduleEvent(EVENT_THROW_BEAR_PELT, 15 * IN_MILLISECONDS);
                        break;
                    case EVENT_BLOW_CENARION_HORN:
                        Talk(SAY_AGGRO);
                        me->SummonCreature(NPC_STAMPEDING_MAMMOTH, 3289.853027f, 5672.9135f, 54.4626f, 1.27f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 20000);
                        me->SummonCreature(NPC_STAMPEDING_RHINO, 3282.0024f, 5670.0600f, 53.072f, 1.27f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 20000);
                        me->SummonCreature(NPC_STAMPEDING_RHINO, 3292.905f, 5664.6748f, 53.447f, 1.27f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 20000);
                        me->SummonCreature(NPC_STAMPEDING_CARIBOU, 3285.068f, 5665.163f, 52.990f, 1.27f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 20000);
                        me->SummonCreature(NPC_STAMPEDING_CARIBOU, 3288.323f, 5664.452f, 53.101f, 1.27f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 20000);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool SummonedAnimals;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_harold_laneAI(creature);
    }
};

enum DurgotMisc
{
    TAXI_QUEST = 11916,
    SPELL_TAXI = 46621
};

#define SAY_WRONG1 "Du hast keinen Mistelzweig"
#define GOSSIP_ITEM_MISTLETOE1 "Bewerft mich mit einem Mistelzweig!"
#define GOSSIP_TAXI "Bringt mich nach Taunka\'le!"

class npc_durgot : public CreatureScript
{
    public:
        npc_durgot() : CreatureScript("npc_durgot")  {}

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (player->GetQuestStatus(TAXI_QUEST) == QUEST_STATUS_COMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_TAXI, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 0);

            if (player->HasItemCount(21519, 1, false))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_ITEM_MISTLETOE1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->PlayerTalkClass->SendGossipMenu(player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
        {
            player->PlayerTalkClass->ClearMenus();
            switch (uiAction)
            {
                case GOSSIP_ACTION_INFO_DEF + 0:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_TAXI, true);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 1:
                {
                    switch (creature->GetEntry())
                    {
                        case 26044:
                            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2, 26004, 0, creature);
                            break;
                        default:
                            break;
                    }
                    player->DestroyItemCount(21519, 1, true);
                    player->CastSpell(creature, 26004, true);
                }
                break;
                default:
                    player->CLOSE_GOSSIP_MENU();
                    break;
            }
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
};

enum GearmasterMechazodMisc
{
    // Creatures
    NPC_GEARMASTER_MECHAZOD = 25834
};

class npc_gearmaster_mechazod : public CreatureScript
{
    public:
        npc_gearmaster_mechazod() : CreatureScript("npc_gearmaster_mechazod") { }
    
        struct npc_gearmaster_mechazodAI : public SmartAI
        {
            npc_gearmaster_mechazodAI(Creature* creature) : SmartAI(creature) { }
    
            void IsSummonedBy(Unit* summoner) override
            {
                std::list<Creature*> creaturelist;
                me->GetCreatureListWithEntryInGrid(creaturelist, NPC_GEARMASTER_MECHAZOD, 100.0f);
                if (creaturelist.size() > 1)
                {
                    me->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                    me->RemoveCorpse();
                    return;
                }

                SmartAI::IsSummonedBy(summoner);
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_gearmaster_mechazodAI(creature);
        }
};

enum jenniPackMule
{
    SPELL_CRATES_CARRIED    = 46340,
    SPELL_DROP_CRATE        = 46342,
    SPELL_JENNI_CREDIT      = 46358,

    NPC_FEZZIX_GEARTWIST    = 25849,
};

class npc_jenni_pack_mule : public CreatureScript
{
public:
    npc_jenni_pack_mule() : CreatureScript("npc_jenni_pack_mule") { }

    struct npc_jenni_pack_muleAI : public ScriptedAI
    {
        npc_jenni_pack_muleAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override 
        {
            me->SetReactState(REACT_PASSIVE);
        }

        void InitializeAI() override
        {
            DoCastSelf(SPELL_CRATES_CARRIED, true);
            me->SetReactState(REACT_PASSIVE);
            _searchTimer = 1000;
            _foundFezzix = false;
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage) override 
        {
            if (damage && damage > 50 && _canTakeDamage)
            {
                _canTakeDamage = false;
                _resetTimer = urand(3000, 5000);
                DoCastSelf(SPELL_DROP_CRATE, true);

                if (!me->HasAura(SPELL_CRATES_CARRIED))
                    me->DespawnOrUnsummon(1000);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!_foundFezzix)
            {
                if (_searchTimer <= diff)
                {
                    if (me->FindNearestCreature(NPC_FEZZIX_GEARTWIST, 10.0f))
                    {
                        if (me->GetOwner())
                            DoCast(me->GetOwner(), SPELL_JENNI_CREDIT, true);

                        me->GetMotionMaster()->MovePoint(0, 3473.8044f, 4103.0249f, 17.616110f);
                        me->DespawnOrUnsummon(3000);

                        _foundFezzix = true;
                    }
                        
                    _searchTimer = 1000;
                }
                else _searchTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (!_canTakeDamage)
            {
                if (_resetTimer <= diff)
                {
                    _canTakeDamage = true;
                }
                else _resetTimer -= diff;
            }
        }

    private:
        uint32 _resetTimer;
        uint32 _searchTimer;
        bool _canTakeDamage;
        bool _foundFezzix;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_jenni_pack_muleAI(creature);
    }
};



enum BloodsporeRuination
{
    NPC_BLOODMAGE_LAURITH = 25381,
    SAY_BLOODMAGE_LAURITH = 0,
    EVENT_TALK = 1,
    EVENT_RESET_ORIENTATION
};

class spell_q11719_bloodspore_ruination_45997 : public SpellScriptLoader
{
public:
    spell_q11719_bloodspore_ruination_45997() : SpellScriptLoader("spell_q11719_bloodspore_ruination_45997") { }

    class spell_q11719_bloodspore_ruination_45997_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q11719_bloodspore_ruination_45997_SpellScript);

        void HandleEffect(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
                if (Creature* laurith = caster->FindNearestCreature(NPC_BLOODMAGE_LAURITH, 100.0f))
                    laurith->AI()->SetGuidData(caster->GetGUID());
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_q11719_bloodspore_ruination_45997_SpellScript::HandleEffect, EFFECT_1, SPELL_EFFECT_SEND_EVENT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_q11719_bloodspore_ruination_45997_SpellScript();
    }
};

class npc_bloodmage_laurith : public CreatureScript
{
public:
    npc_bloodmage_laurith() : CreatureScript("npc_bloodmage_laurith") { }

    struct npc_bloodmage_laurithAI : public ScriptedAI
    {
        npc_bloodmage_laurithAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _playerGUID.Clear();
        }

        void SetGuidData(ObjectGuid guid, uint32 /*action*/) override
        {
            if (!_playerGUID.IsEmpty())
                return;

            _playerGUID = guid;

            if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
                me->SetFacingToObject(player);

            _events.ScheduleEvent(EVENT_TALK, Seconds(1));
        }

        void UpdateAI(uint32 diff) override
        {
            if (UpdateVictim())
            {
                DoMeleeAttackIfReady();
                return;
            }

            _events.Update(diff);

            if (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_TALK:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
                        Talk(SAY_BLOODMAGE_LAURITH, player);
                    _playerGUID.Clear();
                    _events.ScheduleEvent(EVENT_RESET_ORIENTATION, Seconds(5));
                    break;
                case EVENT_RESET_ORIENTATION:
                    me->SetFacingTo(me->GetHomePosition().GetOrientation());
                    break;
                }
            }
        }

    private:
        EventMap _events;
        ObjectGuid _playerGUID;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bloodmage_laurithAI(creature);
    }
};

#define NPC_QUEST_TARGET    25814

class spell_q11712_borean_tundra_re_cursive_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_q11712_borean_tundra_re_cursive_SpellScript);

    SpellCastResult CheckRequirement()
    {
        if (Unit* target = GetExplTargetUnit())
            if ((target->ToCreature() && target->ToCreature()->GetEntry() == NPC_QUEST_TARGET) && !target->IsAlive())
            {
                target->ToCreature()->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                return SPELL_CAST_OK;
            }

        return SPELL_FAILED_NO_VALID_TARGETS;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_q11712_borean_tundra_re_cursive_SpellScript::CheckRequirement);
    }
};

void AddSC_borean_tundra_rg()
{
    new npc_storm_totem();
    new npc_naxxanar_teleport_trigger();
    new npc_longrunner_proudhoof();
    new npc_harold_lane();
    new npc_durgot();
    new npc_gearmaster_mechazod();
    new npc_jenni_pack_mule();
    new spell_q11719_bloodspore_ruination_45997();
    new npc_bloodmage_laurith();
    new SpellScriptLoaderEx<spell_q11712_borean_tundra_re_cursive_SpellScript>("spell_q11712_borean_tundra_re_cursive");
}
