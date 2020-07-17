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
#include "SpellScript.h"
#include "DBCStores.h"

/*########
# Quest Bomb Them Again!
#########*/

enum QuestBombThemAgainMisc
{
    // Achievements
    ACHIEV_WINTER_FALALA               = 1282,
    ACHIEVEMENT_BLADES_EDGE_BOMBERMAN  = 1276,
    
    // Spells
    SPELL_WINTER_AURA                  = 62061,
    SPELL_WINTER_AURA2                 = 25860,
    
    // Wuests
    QUEST_BOMB_THEM_AGAIN              = 11023
};

class npc_sky_sergeant_vanderlip : public CreatureScript
{
    public:
        npc_sky_sergeant_vanderlip() : CreatureScript("npc_sky_sergeant_vanderlip") { }
    
        std::map<ObjectGuid, time_t> QuestTimerMap;
    
        bool OnQuestAccept(Player* player, Creature* /*creature*/, Quest const* quest) override
        {
            if (quest->GetQuestId() == QUEST_BOMB_THEM_AGAIN)
                QuestTimerMap[player->GetGUID()] = time (NULL);
            return true;
        }
    
        bool OnQuestReward(Player* player, Creature* /*creature*/, const Quest* quest, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == QUEST_BOMB_THEM_AGAIN)
            {
                if (player->HasAura(SPELL_WINTER_AURA) || player->HasAura(SPELL_WINTER_AURA2) )
                    player->CompletedAchievement(sAchievementStore.LookupEntry(ACHIEV_WINTER_FALALA));
                if ( (time(NULL) - QuestTimerMap[player->GetGUID()]) <= 135) // 2min 15sec -> 2*60+15
                   player->CompletedAchievement(sAchievementStore.LookupEntry(ACHIEVEMENT_BLADES_EDGE_BOMBERMAN));
    
                QuestTimerMap.erase(player->GetGUID());
            }
            return true;
        }
};

enum MiscCanon
{
    // Creatures
    NPC_FEL_CANNON_DUMMY              = 23077,
    
    // Spells
    SPELL_FEL_CANNON_BOLT             = 40109,
    SPELL_FLAK_FIRE_VISUAL            = 41603,
    SPELL_FEL_FLAK_FIRE_DEBUFF        = 40075
};

class npc_legion_flak_cannon : public CreatureScript
{
    public:
        npc_legion_flak_cannon() : CreatureScript("npc_legion_flak_cannon") { }
    
        struct npc_legion_flak_cannonAI : public ScriptedAI
        {
            npc_legion_flak_cannonAI(Creature* creature) : ScriptedAI(creature) 
            { 
                SetCombatMovement(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetImmuneToNPC(true);
            }
           
            void Reset() override
            {
                canShoot = true;
            }
    
            void MoveInLineOfSight(Unit* who) override
            {
                if (!canShoot)
                    return;
    
                if (who->GetTypeId() == TYPEID_PLAYER && me->IsWithinDist(who, 40.0f))
                {
                    // Should not only attack when player is in air, but also on ground with a flying mount
                    if (who->HasAuraType(SPELL_AURA_FLY) || who->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED))
                    {
                        me->SetFacingToObject(who);
    
                        if (Creature* bunny = me->SummonCreature(NPC_FEL_CANNON_DUMMY, who->GetPositionX(), who->GetPositionY(), who->GetPositionZ()))
                            DoCast(bunny, SPELL_FEL_CANNON_BOLT);
    
                        shootTimer = 3.5 * IN_MILLISECONDS;
                        canShoot = false;
                    }
                }
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (!canShoot)
                {
                    if (shootTimer <= diff)
                        canShoot = true;
                    else
                        shootTimer -= diff;
                }
            }
            
        private:
            bool canShoot;
            uint32 shootTimer;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_legion_flak_cannonAI(creature);
        }
};

class npc_flak_cannon_target : public CreatureScript
{
    public:
        npc_flak_cannon_target() : CreatureScript("npc_flak_cannon_target") { }
    
        struct npc_flak_cannon_targetAI : public ScriptedAI
        {
            npc_flak_cannon_targetAI(Creature* creature) : ScriptedAI(creature) 
            { 
                SetCombatMovement(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }
    
            void DamageTaken(Unit* /*who*/, uint32 &damage) override
            {
                damage = 0;
            }
    
            void SpellHit(Unit* /*caster*/, const SpellInfo* spell) override
            {
                if (spell->Id == SPELL_FEL_CANNON_BOLT)
                {
                    DoCastSelf(SPELL_FLAK_FIRE_VISUAL, true);
                    me->DespawnOrUnsummon(5 * IN_MILLISECONDS);
                    if (Player* target = me->FindNearestPlayer(2.0f))
                        me->CastSpell(target, SPELL_FEL_FLAK_FIRE_DEBUFF, true);
                }
            }        
        };
    
        CreatureAI *GetAI(Creature *creature) const override
        {
            return new npc_flak_cannon_targetAI(creature);
        }
};

/*#####
# npc_Vimgol_the_Vile
######*/

enum VimgolSpells
{
    SPELL_UNHOLY_GROWTH  = 40545,
};

class npc_vimgol_the_vile : public CreatureScript
{
    public:
        npc_vimgol_the_vile() : CreatureScript("npc_vimgol_the_vile") { }
    
        struct npc_vimgol_the_vileAI : public ScriptedAI
        {
            npc_vimgol_the_vileAI(Creature* c) : ScriptedAI(c) {}
    
            void InitializeAI() override
            {
                Talk(0);
                Reset();
            }
    
            void Reset() override
            {
                growth_casted = false;
                me->RemoveAllAuras();
            }
    
            void UpdateAI(uint32 /*diff*/) override
            {
                if (!UpdateVictim())
                    return;
    
                if(me->GetHealthPct() < 50.0f && !growth_casted)
                {
                    Talk(1);
                    DoCastSelf(SPELL_UNHOLY_GROWTH);
                    growth_casted = true;
                }
    
                DoMeleeAttackIfReady();
            }
            
        private:
            bool growth_casted;  
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_vimgol_the_vileAI (creature);
        }
};

/*#####
# npc_trigger_business //Triger for Quest Grim(oire) Business
######*/

enum BusinessDatas
{
    // Creatures
    NPC_VIMGOL               = 22911,
    
    // Gameobjects
    GO_VIMGOLS_VILE_GRIMOIRE = 185562,
    GO_FLAME_CIRCLE          = 185555
};

class npc_trigger_business : public CreatureScript
{
    public:
        npc_trigger_business() : CreatureScript("npc_trigger_business") { }
    
        struct npc_trigger_businessAI : public ScriptedAI
        {
            npc_trigger_businessAI(Creature* creature) : ScriptedAI(creature) {}
    
            uint32 players_in_circles;
            uint32 checkTimer;
    
            void Reset() override
            {
                players_in_circles = 0;
                checkTimer         = 1 * IN_MILLISECONDS;
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (checkTimer <= diff)
                {
                    players_in_circles = 0;
    
                    std::list<Player*> players;
                    me->GetPlayerListInGrid(players, 150.f);
                    std::list<GameObject*> circles;
                    me->GetGameObjectListWithEntryInGrid(circles, GO_FLAME_CIRCLE, 150.0f);
    
                    for (std::list<GameObject*>::const_iterator itr = circles.begin(); itr != circles.end(); ++itr)
                    {
                        GameObject* circle = *itr;
                        for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
                        {
                            Player* player = *pitr;
                            if (circle->GetDistance(player) <= 1.0f)
                            {
                                players_in_circles++;
                                break;
                            }
                        }
                    }
    
                    if (Creature* vimgol = me->FindNearestCreature(NPC_VIMGOL, 150.0f, true)) // there is a living Vim'gol, check if there are players in circles
                    {
                        if (players_in_circles == 5)
                            vimgol->InterruptNonMeleeSpells(false);
                    }
                    else
                    {
                        // there is no living Vim'gol, check if we should summon one
                        // also check if there is a corpse of Vim'Gol and quest-GO.
                        // If no GO there, spawn one. If corpse is despawned, remove GO
                        // only summon Vim'Gol when there is no corpse of him
                        if (Creature* deadvimgol = me->FindNearestCreature(NPC_VIMGOL, 150.0f, false))
                        {
                            if (!me->FindNearestGameObject(GO_VIMGOLS_VILE_GRIMOIRE, 10.0f))
                                me->SummonGameObject(GO_VIMGOLS_VILE_GRIMOIRE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, 0);
                        }
                        else
                        {
                            if (GameObject* book = me->FindNearestGameObject(GO_VIMGOLS_VILE_GRIMOIRE, 10.0f))
                                book->Delete();
    
                            if (players_in_circles > 0)
                                me->SummonCreature(NPC_VIMGOL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 300000);
                        }
                    }
    
                    checkTimer = urand(1000, 1500);
                }
                else
                    checkTimer -= diff;
            }
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_trigger_businessAI (creature);
        }
};

/*#####
## Quest 10544: A Curse Upon Both of Your Clans!
## spell_wicked_strong_fetish
######*/

enum WickedStrongFetish
{
    // Areas
    AREA_BLOODMAUL_OUTPOST     = 3776,
    AREA_BLADESPIRE_HOLD       = 3773,
    
    // Creatures
    NPC_BLADESPIRE_EVIL_SPIRIT = 21446,
    NPC_BLOODMAUL_EVIL_SPIRIT  = 21452
};

class spell_wicked_strong_fetish : public SpellScriptLoader
{
    public:
        spell_wicked_strong_fetish() : SpellScriptLoader("spell_wicked_strong_fetish") { }

        class spell_wicked_strong_fetish_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_wicked_strong_fetish_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Player* player = caster->ToPlayer())
                {
                    if (player->GetAreaId() == AREA_BLOODMAUL_OUTPOST)
                        player->SummonCreature(NPC_BLOODMAUL_EVIL_SPIRIT, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 240000);

                    if (player->GetAreaId() == AREA_BLADESPIRE_HOLD)
                        player->SummonCreature(NPC_BLADESPIRE_EVIL_SPIRIT, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 240000);
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_wicked_strong_fetish_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_wicked_strong_fetish_SpellScript();
        }
};

/*#####
## Quest 10525 - Vision Guide
## npc_vision_guide_trigger
######*/

enum VisionGuideTrigger
{
    // Quests
    QUEST_VISION_GUIDE   = 10525,
    
    // Spells
    AURA_VISION          = 36573
};

class npc_vision_guide_trigger : public CreatureScript
{
    public:
        npc_vision_guide_trigger() : CreatureScript("npc_vision_guide_trigger") { }
    
        struct npc_vision_guide_triggerAI : public ScriptedAI
        {
            npc_vision_guide_triggerAI(Creature* c) : ScriptedAI(c) {}
    
            void MoveInLineOfSight(Unit* who) override
            {
                if (!who || (!who->IsAlive()))
                    return;
    
                if (me->IsWithinDistInMap(who, 1.0f))
                {
                    if (who->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (Player* player = who->ToPlayer())
                        {
                            if (player->GetQuestStatus(QUEST_VISION_GUIDE) == QUEST_STATUS_INCOMPLETE)
                            {
                                player->RemoveAura(AURA_VISION);
                                player->CompleteQuest(QUEST_VISION_GUIDE);
                                player->TeleportTo(530, 2277.69f, 5980.52f, 142.652f, 1.87f);
                            }
                        }
                    }
                }
            }
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_vision_guide_triggerAI (creature);
        }
};

/*#####
## Quest 10714 - On Spirit's Wings
## spell_summon_spirit
######*/

enum OnSpiritsWings
{
    // Creatures
    NPC_SPIRIT                   = 22492,
    NPC_BLOODMAUL_SOOTHSAYER     = 22384,
    NPC_BLOODMAUL_TASKMASTER     = 22160,
    NPC_BLOODMAUL_CHATTER_CREDIT = 22383
};

class spell_summon_spirit : public SpellScriptLoader
{
    public:
        spell_summon_spirit() : SpellScriptLoader("spell_summon_spirit") { }

        class spell_summon_spirit_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_summon_spirit_SpellScript);

            void SummonSpirit()
            {
                Unit* caster = GetCaster();
                if (Player* player = caster->ToPlayer())
                    if (Creature* soothsayer = player->FindNearestCreature(NPC_BLOODMAUL_SOOTHSAYER, 30.0f))
                        if (Creature* taskmaster = player->FindNearestCreature(NPC_BLOODMAUL_TASKMASTER, 30.0f))
                            if (Creature* spirit = player->SummonCreature(NPC_SPIRIT, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()))
                                spirit->GetMotionMaster()->MovePoint(1, (soothsayer->GetPositionX() + taskmaster->GetPositionX())/2 , (soothsayer->GetPositionY() + taskmaster->GetPositionY())/2, (soothsayer->GetPositionZ() + taskmaster->GetPositionZ())/2);
            }

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                if (Player* player = caster->ToPlayer())
                {
                    if (Creature* soothsayer = player->FindNearestCreature(NPC_BLOODMAUL_SOOTHSAYER, 30.0f))
                        if (Creature* taskmaster = player->FindNearestCreature(NPC_BLOODMAUL_TASKMASTER, 30.0f))
                            return SPELL_CAST_OK;
                }
                return SPELL_FAILED_BAD_TARGETS;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_summon_spirit_SpellScript::CheckCast);
                AfterCast += SpellCastFn(spell_summon_spirit_SpellScript::SummonSpirit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_summon_spirit_SpellScript();
        }
};

/*#####
## Quest 10714 - On Spirit's Wings
## Quest 10724 - Prisoner of the Bladespire
## npc_spirit
######*/

class npc_spirit : public CreatureScript
{
    public:
        npc_spirit() : CreatureScript("npc_spirit") { }
    
        struct npc_spiritAI : public ScriptedAI
        {
            npc_spiritAI(Creature* c) : ScriptedAI(c) { }
    
            void MovementInform(uint32 /*type*/, uint32 id) override
            {
                // There are two different NPCs named Spirit. They look exactly the same, but are for two different quests
                if (me->GetEntry() == 22460) // Entry 22460 is for Quest 10724 - Prisoner of the Bladespire
                {
                    if (id == 1)
                    {
                        if (GameObject* cage = me->FindNearestGameObject(185296, 100.0f))
                        {
                            cage->Use(me);
                            if (Creature* leokk = me->FindNearestCreature(22268, 100.0f))
                            {
                                leokk->GetMotionMaster()->MovePoint(1, 3584.18f, 5372.97f , 56.84f);
                                leokk->DespawnOrUnsummon(5000);
                            }
                            me->DespawnOrUnsummon(0);
                        }
                    }
                }
                else // Entry 22492 is for Quest 10714 - On Spirit's Wings
                {
                    if (id == 1)
                    {
                        if (TempSummon* tempsummon = me->ToTempSummon())
                            if (Unit* summoner = tempsummon->GetSummoner())
                                me->GetMotionMaster()->MoveFollow(summoner, 0.0f, 0.0f);
                    }
                    else
                    {
                        if (TempSummon* tempsummon = me->ToTempSummon())
                            if (Unit* summoner = tempsummon->GetSummoner())
                                if (Player* player = summoner->ToPlayer())
                                    player->KilledMonsterCredit(NPC_BLOODMAUL_CHATTER_CREDIT);
    
                        me->DespawnOrUnsummon(0);
                    }
                }
    
            }
    
            void Reset() override
            {
                if (me->GetEntry() == 22460) // Entry 22460 is for Quest 10724 - Prisoner of the Bladespire
                    me->GetMotionMaster()->MovePoint(1, 3674.19f, 5285.33f, 21.5f);
            }
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_spiritAI (creature);
        }
};

/*#####
## Quest 10720 - The Smallest Creatures
## spell_poison_keg
## npc_marmot
######*/

enum TheSmallestCreatures
{
    // Creatures
    NPC_GREEN_SPOT_GROG_KEG_CREDIT      = 22356,
    NPC_RIPE_MOONSHINE_KEG_CREDIT       = 22367,
    NPC_FERMENTED_SEED_BEER_KEG_CREDIT  = 22368,
    
    // Spells
    SPELL_POSSESS                       = 530,
    SPELL_COAX_MARMOT                   = 38544
};

class spell_poison_keg : public SpellScriptLoader
{
    public:
        spell_poison_keg() : SpellScriptLoader("spell_poison_keg") { }

        class spell_poison_keg_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_poison_keg_SpellScript);

            void PoisonKeg()
            {
                Unit* caster = GetCaster();

                if (TempSummon* tempsummon = caster->ToTempSummon())
                {
                    if (Unit* summoner = tempsummon->GetSummoner())
                    {
                        if (Player* player = summoner->ToPlayer())
                        {
                            if (caster->FindNearestCreature(NPC_GREEN_SPOT_GROG_KEG_CREDIT, 10.0f))
                                player->KilledMonsterCredit(NPC_GREEN_SPOT_GROG_KEG_CREDIT);

                            if (caster->FindNearestCreature(NPC_RIPE_MOONSHINE_KEG_CREDIT, 10.0f))
                                player->KilledMonsterCredit(NPC_RIPE_MOONSHINE_KEG_CREDIT);

                            if (caster->FindNearestCreature(NPC_FERMENTED_SEED_BEER_KEG_CREDIT, 10.0f))
                                player->KilledMonsterCredit(NPC_FERMENTED_SEED_BEER_KEG_CREDIT);
                        }
                    }
                }
            }

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                if (caster->FindNearestCreature(NPC_GREEN_SPOT_GROG_KEG_CREDIT, 10.0f) || caster->FindNearestCreature(NPC_RIPE_MOONSHINE_KEG_CREDIT, 10.0f) || caster->FindNearestCreature(NPC_FERMENTED_SEED_BEER_KEG_CREDIT, 10.0f))
                {
                    return SPELL_CAST_OK;
                }
                return SPELL_FAILED_BAD_TARGETS;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_poison_keg_SpellScript::CheckCast);
                AfterCast += SpellCastFn(spell_poison_keg_SpellScript::PoisonKeg);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_poison_keg_SpellScript();
        }
};

class npc_marmot : public CreatureScript
{
    public:
        npc_marmot() : CreatureScript("npc_marmot") { }
    
        struct npc_marmotAI : public ScriptedAI
        {
            npc_marmotAI(Creature* creature) : ScriptedAI(creature) { }
    
            void Reset() override
            {
                //use same spell like .possess to control marmot
                if (TempSummon* tempsummon = me->ToTempSummon())
                    if (Unit* summoner = tempsummon->GetSummoner())
                        if (Player* player = summoner->ToPlayer())
                            player->CastSpell(me, SPELL_POSSESS, true);
            }
    
            void UpdateAI(uint32 /*diff*/) override
            {
                //Prevent attacking former owner after setting free
                if (me->GetVictim())
                    if (me->GetVictim()->GetTypeId() == TYPEID_PLAYER)
                        me->DespawnOrUnsummon(0);
    
                //There is a bug with the invisibility aura, so remove it
                //The marmot seems to be invisible but it is not and it is therefore attacked by "invisible" ogres
                if (TempSummon* tempsummon = me->ToTempSummon())
                    if (Unit* summoner = tempsummon->GetSummoner())
                        if (Player* player = summoner->ToPlayer())
                            player->RemoveAurasDueToSpell(SPELL_COAX_MARMOT);
            }
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_marmotAI (creature);
        }
};

/*#####
## Quest 10802 - Gorgrom the Dragon-Eater
## spell_drop_grisly_totem
## npc_misha
######*/

class spell_drop_grisly_totem : public SpellScriptLoader
{
    public:
        spell_drop_grisly_totem() : SpellScriptLoader("spell_drop_grisly_totem") { }

        class spell_drop_grisly_totem_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_drop_grisly_totem_SpellScript);

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                if (Creature* gorgrom = caster->FindNearestCreature(21514, 25.0f, false))
                    if (!gorgrom->IsAlive())
                        return SPELL_CAST_OK;

                return SPELL_FAILED_BAD_TARGETS;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_drop_grisly_totem_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_drop_grisly_totem_SpellScript();
        }
};

class npc_misha : public CreatureScript
{
    public:
        npc_misha() : CreatureScript("npc_misha") { }
    
        struct npc_mishaAI : public ScriptedAI
        {
            npc_mishaAI(Creature* creature) : ScriptedAI(creature) {}
    
            void Reset() override
            {
                checkTimer = 1 * IN_MILLISECONDS;
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (checkTimer <= diff)
                {
                    if (Creature* gorgrom = me->FindNearestCreature(21514, 100.0f))
                    {
                        gorgrom->GetMotionMaster()->MovePoint(1, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                        AddThreat(me, 1000.0f, gorgrom);
                        gorgrom->Attack(me, true);
    
                        if (gorgrom->IsInRange(me, 0.0f, 3.0f))
                        {
                            me->Kill(gorgrom);
                            me->DespawnOrUnsummon(0);
                        }
                    }
    
                    checkTimer = urand(1, 2)  * IN_MILLISECONDS;
                }
                else
                    checkTimer -= diff;
            }
            
        private:
            uint32 checkTimer;
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_mishaAI (creature);
        }
};

/*#####
## Quest 10867 - There Can Be Only One Response
## npc_blades_edge_nexus_prince_event_trigger
######*/

enum BladesEdgeNexusPrinceEventTrigger
{
    // Creatures
    NPC_DEADSOUL_ORB        = 20845,
    NPC_NEXUS_PRINCE_RAZAAN = 21057,
    
    // Gameobjects
    GO_COLLECTION_OF_SOULS  = 185033
};

class npc_blades_edge_nexus_prince_event_trigger : public CreatureScript
{
    public:
        npc_blades_edge_nexus_prince_event_trigger() : CreatureScript("npc_blades_edge_nexus_prince_event_trigger") { }
    
        struct npc_blades_edge_nexus_prince_event_triggerAI : public ScriptedAI
        {
            npc_blades_edge_nexus_prince_event_triggerAI(Creature* creature) : ScriptedAI(creature) {}
    
            void Reset() override
            {
                souls      = 0;
                summonbox  = false;
    
                checkTimer = 1* IN_MILLISECONDS;
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (checkTimer <= diff)
                {
                    if (Creature* deadsoulorb = me->FindNearestCreature(NPC_DEADSOUL_ORB, 50.0f))
                    {
                        deadsoulorb->GetMotionMaster()->MovePoint(1, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                        if (deadsoulorb->IsInRange(me, 0.0f, 1.0f))
                        {
                            souls++;
                            deadsoulorb->DespawnOrUnsummon(0);
                            if (souls == 10)
                            {
                                souls = 0;
                                summonbox = true;
                                if (!me->FindNearestCreature(NPC_NEXUS_PRINCE_RAZAAN, 1000.0f))
                                    me->SummonCreature(NPC_NEXUS_PRINCE_RAZAAN, 2814.0f, 5244.67f, 264.47f, 5.19f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 180000);
                            }
                        }
                    }
    
                    // tricky way to spawn the box, because SmartAI has problems with correct faction of the box
                    if (Creature* razaan = me->FindNearestCreature(NPC_NEXUS_PRINCE_RAZAAN, 1000.0f, false))
                    {
                        if (!razaan->IsAlive() && summonbox)
                        {
                            summonbox = false;
                            me->SummonGameObject(GO_COLLECTION_OF_SOULS, razaan->GetPositionX(), razaan->GetPositionY(), razaan->GetPositionZ(), 0, 0, 0, 0, 0, 0);
                        }
                    }
    
                    checkTimer = urand(1, 2) * IN_MILLISECONDS;
                }
                else
                    checkTimer -= diff;
            }
            
        private:
            uint32 souls;
            bool summonbox;
            uint32 checkTimer;
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_blades_edge_nexus_prince_event_triggerAI (creature);
        }
};

/*#####
## Quest 10607 - Whispers of the Raven God
## npc_whispers_of_the_raven_god_trigger
######*/

enum WhispersOfTheRavenGod
{
    // Spells
    SPELL_UNDERSTANDING_RAVENSPEECH = 37642,
    
    // Creatures
    NPC_THE_VOICE_OF_THE_RAVEN_GOD  = 21851,
    NPC_PROPHECY_1_QUEST_CREDIT     = 22798,
    NPC_PROPHECY_2_QUEST_CREDIT     = 22799,
    NPC_PROPHECY_3_QUEST_CREDIT     = 22800,
    NPC_PROPHECY_4_QUEST_CREDIT     = 22801,
};

class npc_whispers_of_the_raven_god_trigger : public CreatureScript
{
    public:
        npc_whispers_of_the_raven_god_trigger() : CreatureScript("npc_whispers_of_the_raven_god_trigger") { }
    
        struct npc_whispers_of_the_raven_god_triggerAI : public ScriptedAI
        {
            npc_whispers_of_the_raven_god_triggerAI(Creature* creature) : ScriptedAI(creature) {}
    
            void Reset() override
            {
                whispertimer = 15 * IN_MILLISECONDS;
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (whispertimer <= diff)
                {
                    std::list<Player*> players;
                    me->GetPlayerListInGrid(players, 5.f);
                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        Player* player = *itr;
                        if (player->GetDistance(me) <= 5.0f)
                        {
                            if (player->HasAura(SPELL_UNDERSTANDING_RAVENSPEECH))
                            {
                                if (Creature* voice = me->FindNearestCreature(NPC_THE_VOICE_OF_THE_RAVEN_GOD, 50000.0f))
                                {
                                    switch (me->GetEntry())
                                    {
                                        case NPC_PROPHECY_1_QUEST_CREDIT:
                                            voice->AI()->Talk(0, player);
                                            player->KilledMonsterCredit(NPC_PROPHECY_1_QUEST_CREDIT);
                                            break;
                                        case NPC_PROPHECY_2_QUEST_CREDIT:
                                            voice->AI()->Talk(1, player);
                                            player->KilledMonsterCredit(NPC_PROPHECY_2_QUEST_CREDIT);
                                            break;
                                        case NPC_PROPHECY_3_QUEST_CREDIT:
                                            voice->AI()->Talk(2, player);
                                            player->KilledMonsterCredit(NPC_PROPHECY_3_QUEST_CREDIT);
                                            break;
                                        case NPC_PROPHECY_4_QUEST_CREDIT:
                                            voice->AI()->Talk(3, player);
                                            player->KilledMonsterCredit(NPC_PROPHECY_4_QUEST_CREDIT);
                                            break;
                                    }
                                }
                            }
                        }
                    }
                    whispertimer = 15 * IN_MILLISECONDS;
                }
                else
                {
                    whispertimer -= diff;
                }
            }
            
        private:
            uint32 whispertimer;
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_whispers_of_the_raven_god_triggerAI (creature);
        }
};

/*#####
## Quest 10911 - Fire At Will!
## npc_warp_gate_shield
######*/

enum FireAtWill
{
    // Creatures
    NPC_DEATHS_DOOR_FEL_CANNON       = 22443,
    NPC_VOID_HOUND                   = 22500,
    NPC_UNSTABLE_FEL_IMP             = 22474,
    NPC_DEATHS_DOOR_NORTH_WARP_GATE  = 22471,
    NPC_DEATHS_DOOR_SOUTH_WARP_GATE  = 22472,
    NPC_NORTH_WARP_GATE_CREDIT       = 22503,
    NPC_SOUTH_WARP_GATE_CREDIT       = 22504,
    
    // Spells
    SPELL_DEATHS_DOOR_FEL_CANNON     = 39219,
    SPELL_ARTILLERY_ON_THE_WARP_GATE = 39221,
};

class npc_warp_gate_shield : public CreatureScript
{
    public:
        npc_warp_gate_shield() : CreatureScript("npc_warp_gate_shield") { }
    
        struct npc_warp_gate_shieldAI : public ScriptedAI
        {
            npc_warp_gate_shieldAI(Creature* creature) : ScriptedAI(creature) {}
    
            void Reset() override
            {
                hits_taken  = 0;
                spawn_timer = 4 * IN_MILLISECONDS;
                reset_timer = 0;
                checkTimer  = 1 * IN_MILLISECONDS;
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (checkTimer <= diff)
                {
                    if (Creature* felcannon = me->FindNearestCreature(NPC_DEATHS_DOOR_FEL_CANNON, 50.0f))
                    {
                        if (felcannon->IsCharmed())
                        {
                            if (Unit* charmer = felcannon->GetCharmer())
                                felcannon->SetFacingToObject(charmer);
                        }
                        else
                        {
                            if (felcannon->GetVictim() && !felcannon->HasAura(SPELL_DEATHS_DOOR_FEL_CANNON))
                                felcannon->AI()->EnterEvadeMode();
                        }
                    }
    
                    checkTimer = urand(500, 750);
                }
                else
                    checkTimer -= diff;
    
                if (hits_taken > 0)
                {
                    if (spawn_timer <= diff)
                    {
                        if (Creature* felcannon = me->FindNearestCreature(NPC_DEATHS_DOOR_FEL_CANNON, 50.0f))
                        {
                            // spawn 3 npcs
                            for (int i = 0; i < 3; i++)
                            {
                                if (urand(0, 4) == 4) // 20% chance to spawn a dog
                                {
                                    if (Creature* dog = me->SummonCreature(NPC_VOID_HOUND, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000))
                                        dog->GetMotionMaster()->MovePoint(1, felcannon->GetPositionX(), felcannon->GetPositionY(), felcannon->GetPositionZ());
                                }
                                else
                                {
                                    if (Creature* imp = me->SummonCreature(NPC_UNSTABLE_FEL_IMP, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000))
                                        imp->GetMotionMaster()->MovePoint(1, felcannon->GetPositionX(), felcannon->GetPositionY(), felcannon->GetPositionZ());
                                }
                            }
                        }
    
                        spawn_timer = 13 * IN_MILLISECONDS;
                    }
                    else
                    {
                        spawn_timer -= diff;
                    }
    
                    if (reset_timer <= diff)
                    {
                        hits_taken = 0;
                    }
                    else
                    {
                        reset_timer -= diff;
                    }
                }
            }
    
            void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
            {
                if (spell->Id == SPELL_ARTILLERY_ON_THE_WARP_GATE)
                {
                    hits_taken++;
                    reset_timer = 60 * IN_MILLISECONDS;
    
                    if (hits_taken ==  7)
                    {
                        hits_taken = 0;
                        if (Creature* felcannon = me->FindNearestCreature(NPC_DEATHS_DOOR_FEL_CANNON, 50.0f))
                            if (felcannon->IsCharmed())
                                if (Unit* charmer = felcannon->GetCharmer())
                                    if (Player* player = charmer->ToPlayer())
                                    {
                                        if (me->FindNearestCreature(NPC_DEATHS_DOOR_NORTH_WARP_GATE, 20.0f))
                                            player->KilledMonsterCredit(NPC_NORTH_WARP_GATE_CREDIT);
    
                                        if (me->FindNearestCreature(NPC_DEATHS_DOOR_SOUTH_WARP_GATE, 20.0f))
                                            player->KilledMonsterCredit(NPC_SOUTH_WARP_GATE_CREDIT);
                                    }
                    }
                }
            }
            
        private:
            uint32 hits_taken;
            uint32 spawn_timer;
            uint32 reset_timer;
            uint32 checkTimer;
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_warp_gate_shieldAI (creature);
        }
};
/*######
## npc_bloodmaul_brutebane_rg
######*/

enum BloodmaulMisc
{
    // Creatures
    NPC_OGRE_BRUTE_RG                      = 19995,
    NPC_OGRE_SHAMAN_RG                     = 19998,
    NPC_OGRE_COOK_RG                       = 20334,
    NPC_QUEST_CREDIT_RG                    = 21241,

    // Gameobjects
    GO_KEG_RG                              = 184315,

    // Quests
    QUEST_GETTING_THE_BLADESPIRE_TANKED_RG = 10512,
    QUEST_BLADESPIRE_KEGGER_RG             = 10545
};

class npc_bloodmaul_brutebane_rg : public CreatureScript
{
    public:
        npc_bloodmaul_brutebane_rg() : CreatureScript("npc_bloodmaul_brutebane_rg") { }

        struct npc_bloodmaul_brutebane_rgAI : public ScriptedAI
        {
            npc_bloodmaul_brutebane_rgAI(Creature* creature) : ScriptedAI(creature)
            {
                if (Creature* Ogre = me->FindNearestCreature(NPC_OGRE_BRUTE_RG, 30.0f, true))
                {
                    Ogre->SetReactState(REACT_DEFENSIVE);
                    Ogre->GetMotionMaster()->MovePoint(1, me->GetPositionX() - 1, me->GetPositionY() + 1, me->GetPositionZ());
                }
                else if (Creature* Ogre = me->FindNearestCreature(NPC_OGRE_SHAMAN_RG, 30.0f, true))
                {
                    Ogre->SetReactState(REACT_DEFENSIVE);
                    Ogre->GetMotionMaster()->MovePoint(1, me->GetPositionX() - 1, me->GetPositionY() + 1, me->GetPositionZ());
                }
                else if (Creature* Ogre = me->FindNearestCreature(NPC_OGRE_COOK_RG, 30.0f, true))
                {
                    Ogre->SetReactState(REACT_DEFENSIVE);
                    Ogre->GetMotionMaster()->MovePoint(1, me->GetPositionX() - 1, me->GetPositionY() + 1, me->GetPositionZ());
                }
            }

            void Reset() override
            {
                OgreGUID = 0;
            }

            void UpdateAI(uint32 /*diff*/) override { }
            
        private:
            uint64 OgreGUID;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bloodmaul_brutebane_rgAI(creature);
    }
};

/*######
## npc_bloodmaul_brute_rg
######*/

enum BloodmaulBrute
{
    // Events
    EVENT_CLEAVE_RG                   = 1,
    EVENT_DEBILITATING_STRIKE_RG      = 2,
    EVENT_SHIELD_RG                   = 3,
    EVENT_BOLT_RG                     = 4,
    EVENT_MEAT_SLAP_RG                = 5,
    EVENT_TENDERIZE_RG                = 6, 
    
    // Texts
    SAY_AGGRO_RG                      = 0,
    SAY_DEATH_RG                      = 1,
    SAY_ENRAGE_RG                     = 2,
    
    // Spells
    SPELL_CLEAVE_RG                   = 15496,
    SPELL_DEBILITATING_STRIKE_RG      = 37577,
    SPELL_MEAT_RG                     = 37597,
    SPELL_TENDERIZE_RG                = 37596,
    SPELL_SHIELD_RG                   = 12550,
    SPELL_BOLT_RG                     = 26098,
    SPELL_ENRAGE_RG                   = 8599,
    
    // Quests
    QUEST_INTO_THE_SOULGRINDER_RG     = 11000
};

class npc_bloodmaul_brute_rg : public CreatureScript
{
    public:
        npc_bloodmaul_brute_rg() : CreatureScript("npc_bloodmaul_brute_rg") { }

        struct npc_bloodmaul_brute_rgAI : public ScriptedAI
        {
            npc_bloodmaul_brute_rgAI(Creature* creature) : ScriptedAI(creature)
            {
                PlayerGUID.Clear();
                hp30 = false;
            }

            void Reset() override
            {
                PlayerGUID.Clear();
                hp30 = false;
            }

            void EnterCombat(Unit* /*who*/) override
            {
                if (urand(0, 100) < 35)
                    Talk(SAY_AGGRO_RG);

                switch (me->GetEntry())
                {
                    case NPC_OGRE_BRUTE_RG:
                        events.ScheduleEvent(EVENT_CLEAVE_RG, urand(9, 12) * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_DEBILITATING_STRIKE_RG, 15 * IN_MILLISECONDS);
                        break;
                    case NPC_OGRE_SHAMAN_RG:
                        events.ScheduleEvent(EVENT_SHIELD_RG, 1 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_BOLT_RG, urand(2, 4) * IN_MILLISECONDS);
                        break;
                    case NPC_OGRE_COOK_RG:
                        events.ScheduleEvent(EVENT_MEAT_SLAP_RG, urand(15, 20) * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_TENDERIZE_RG, urand(15, 20) * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
                events.ScheduleEvent(EVENT_CLEAVE_RG, urand(9000, 12000));
                events.ScheduleEvent(EVENT_DEBILITATING_STRIKE_RG, 15000);
            }

            void JustDied(Unit* killer) override
            {
                if (killer->GetTypeId() == TYPEID_PLAYER)
                if (killer->ToPlayer()->GetQuestRewardStatus(QUEST_INTO_THE_SOULGRINDER_RG))
                    Talk(SAY_DEATH_RG);
            }

            void MoveInLineOfSight(Unit* who) override
            {
                if (!who || (!who->IsAlive()))
                    return;

                if (me->IsWithinDistInMap(who, 50.0f))
                {
                    if (who->GetTypeId() == TYPEID_PLAYER)
                        if (who->ToPlayer()->GetQuestStatus(QUEST_GETTING_THE_BLADESPIRE_TANKED_RG) == QUEST_STATUS_INCOMPLETE
                            || who->ToPlayer()->GetQuestStatus(QUEST_BLADESPIRE_KEGGER_RG) == QUEST_STATUS_INCOMPLETE)
                            PlayerGUID = who->GetGUID();
                }
            }

            void MovementInform(uint32 /*type*/, uint32 id) override
            {
                if (id == 1)
                {
                    if (GameObject* Keg = me->FindNearestGameObject(GO_KEG_RG, 20.0f))
                        Keg->Delete();

                    me->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->GetMotionMaster()->MoveTargetedHome();

                    Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);
                    Creature* Credit = me->FindNearestCreature(NPC_QUEST_CREDIT_RG, 50.0f, true);
                    if (player && Credit)
                        player->KilledMonster(Credit->GetCreatureTemplate(), Credit->GetGUID());
                }
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
                        case EVENT_CLEAVE_RG:
                            DoCastSelf(SPELL_CLEAVE_RG);
                            events.ScheduleEvent(EVENT_CLEAVE_RG, urand(9000, 12000));
                            break;
                        case EVENT_DEBILITATING_STRIKE_RG:
                            DoCastVictim(SPELL_DEBILITATING_STRIKE_RG);
                            events.ScheduleEvent(EVENT_DEBILITATING_STRIKE_RG, urand(18000, 22000));
                            break;
                        case EVENT_MEAT_SLAP_RG:
                            DoCastVictim(SPELL_MEAT_RG);
                            events.ScheduleEvent(EVENT_MEAT_SLAP_RG, urand(15, 20) * IN_MILLISECONDS);
                            break;
                        case EVENT_TENDERIZE_RG:
                            DoCastVictim(SPELL_TENDERIZE_RG);
                            events.ScheduleEvent(EVENT_TENDERIZE_RG, urand(15, 20) * IN_MILLISECONDS);
                            break;
                        case EVENT_SHIELD_RG:
                            DoCastSelf(SPELL_SHIELD_RG);
                            break;
                        case EVENT_BOLT_RG:
                            DoCastVictim(SPELL_BOLT_RG);
                            events.ScheduleEvent(EVENT_BOLT_RG, urand(8, 12) * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                if (!hp30 && HealthBelowPct(30))
                {
                    hp30 = true;
                    Talk(SAY_ENRAGE_RG);
                    DoCastSelf(SPELL_ENRAGE_RG);
                }

                DoMeleeAttackIfReady();
            }
            
        private:
            EventMap events;
            ObjectGuid PlayerGUID;
            bool hp30;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_bloodmaul_brute_rgAI(creature);
        }
};


void AddSC_blades_edge_mountains_rg()
{
    new npc_sky_sergeant_vanderlip();
    new npc_legion_flak_cannon();
    new npc_flak_cannon_target();
    new npc_vimgol_the_vile();
    new npc_trigger_business();
    new spell_wicked_strong_fetish();
    new npc_vision_guide_trigger();
    new spell_summon_spirit();
    new npc_spirit();
    new npc_marmot();
    new spell_poison_keg();
    new spell_drop_grisly_totem();
    new npc_misha();
    new npc_blades_edge_nexus_prince_event_trigger();
    new npc_whispers_of_the_raven_god_trigger();
    new npc_warp_gate_shield();
    new npc_bloodmaul_brutebane_rg();
    new npc_bloodmaul_brute_rg();
}
