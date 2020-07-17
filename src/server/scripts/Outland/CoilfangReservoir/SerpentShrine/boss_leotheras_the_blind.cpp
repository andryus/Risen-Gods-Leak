/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Boss_Leotheras_The_Blind
SD%Complete: 80
SDComment: Possesion Support
SDCategory: Coilfang Resevoir, Serpent Shrine Cavern
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "serpent_shrine.h"
#include "Player.h"

enum LeotherasTheBlind
{
    // Spells used by Leotheras The Blind
    SPELL_WHIRLWIND         = 37640,
    SPELL_CHAOS_BLAST       = 37674,
    SPELL_BERSERK           = 26662,
    SPELL_INSIDIOUS_WHISPER = 37676,
    SPELL_DUAL_WIELD        = 42459,

    // Spells used in banish phase
    BANISH_BEAM             = 38909,
    AURA_BANISH             = 37833,

    // Spells used by Greyheart Spellbinders
    SPELL_EARTHSHOCK        = 39076,
    SPELL_MINDBLAST         = 37531,

    // Spells used by Inner Demons and Creature ID
    INNER_DEMON_ID          = 21857,
    AURA_DEMONIC_ALIGNMENT  = 37713,
    SPELL_SHADOWBOLT        = 39309,
    SPELL_SOUL_LINK         = 38007,
    SPELL_CONSUMING_MADNESS = 37749,

    //Misc.
    MODEL_DEMON             = 20125,
    MODEL_NIGHTELF          = 20514,
    DEMON_FORM              = 21875,
    NPC_SPELLBINDER         = 21806,
    INNER_DEMON_VICTIM      = 1,

    SAY_AGGRO               = 0,
    SAY_SWITCH_TO_DEMON     = 1,
    SAY_INNER_DEMONS        = 2,
    SAY_DEMON_SLAY          = 3,
    SAY_NIGHTELF_SLAY       = 4,
    SAY_FINAL_FORM          = 5,
    SAY_FREE                = 6,
    SAY_DEATH               = 7
};

class npc_inner_demon : public CreatureScript
{
public:
    npc_inner_demon() : CreatureScript("npc_inner_demon") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_inner_demonAI(creature);
    }

    struct npc_inner_demonAI : public ScriptedAI
    {
        npc_inner_demonAI(Creature* creature) : ScriptedAI(creature)
        {
            victimGUID.Clear();
        }

        uint32 ShadowBolt_Timer;

        uint32 Link_Timer;
        ObjectGuid victimGUID;

        void Reset() override
        {
            ShadowBolt_Timer = 10000;
            Link_Timer = 1000;
        }

        void SetGuidData(ObjectGuid guid, uint32 id/* = 0 */) override
        {
            if (id == INNER_DEMON_VICTIM)
                victimGUID = guid;
        }

        ObjectGuid GetGuidData(uint32 id/* = 0 */) const override
        {
            if (id == INNER_DEMON_VICTIM)
                return victimGUID;
            return ObjectGuid::Empty;
        }

        void JustDied(Unit* /*killer*/) override
        {
            Unit* unit = ObjectAccessor::GetUnit(*me, victimGUID);
            if (unit && unit->HasAura(SPELL_INSIDIOUS_WHISPER))
                unit->RemoveAurasDueToSpell(SPELL_INSIDIOUS_WHISPER);
        }

        void DamageTaken(Unit* done_by, uint32 &damage) override
        {
            if (done_by->GetGUID() != victimGUID && done_by->GetGUID() != me->GetGUID())
            {
                damage = 1;
                ModifyThreatByPercent(done_by, -100);
            }
        }

        void EnterCombat(Unit* /*who*/) override
        {
            if (!victimGUID)
                return;
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim() || !me->GetVictim())
                return;

            if (me->EnsureVictim()->GetGUID() != victimGUID)
            {
                ModifyThreatByPercent(me->GetVictim(), -100);
                Unit* owner = ObjectAccessor::GetUnit(*me, victimGUID);
                if (owner && owner->IsAlive())
                {
                    AddThreat(owner, 999999);
                    AttackStart(owner);
                } else if (owner && owner->isDead())
                {
                    me->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                    return;
                }
            }

            if (Link_Timer <= diff)
            {
                DoCastVictim(SPELL_SOUL_LINK, true);
                Link_Timer = 1000;
            } else Link_Timer -= diff;

            if (!me->HasAura(AURA_DEMONIC_ALIGNMENT))
                DoCastSelf(AURA_DEMONIC_ALIGNMENT, true);

            if (ShadowBolt_Timer <= diff)
            {
                DoCastVictim(SPELL_SHADOWBOLT, false);
                ShadowBolt_Timer = 10000;
            } else ShadowBolt_Timer -= diff;

           DoMeleeAttackIfReady();
        }
    };
};

//Original Leotheras the Blind AI
class boss_leotheras_the_blind : public CreatureScript
{
public:
    boss_leotheras_the_blind() : CreatureScript("boss_leotheras_the_blind") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<boss_leotheras_the_blindAI>(creature);
    }

    struct boss_leotheras_the_blindAI : public ScriptedAI
    {
        boss_leotheras_the_blindAI(Creature* creature) : ScriptedAI(creature)
        {
            creature->GetPosition(x, y, z);
            instance = creature->GetInstanceScript();
            Demon.Clear();

            for (uint8 i = 0; i < 3; ++i)//clear guids
                SpellBinderGUID[i].Clear();
        }

        InstanceScript* instance;

        uint32 Whirlwind_Timer;
        uint32 ChaosBlast_Timer;
        uint32 SwitchToDemon_Timer;
        uint32 SwitchToHuman_Timer;
        uint32 Berserk_Timer;
        uint32 InnerDemons_Timer;
        uint32 BanishTimer;

        bool DealDamage;
        bool NeedThreatReset;
        bool DemonForm;
        bool IsFinalForm;
        bool EnrageUsed;
        float x, y, z;

        ObjectGuid InnderDemon[5];
        uint32 InnerDemon_Count;
        ObjectGuid Demon;
        ObjectGuid SpellBinderGUID[3];

        void Reset() override
        {
            CheckChannelers();
            BanishTimer = 1000;
            Whirlwind_Timer = 15000;
            ChaosBlast_Timer = 1000;
            SwitchToDemon_Timer = 45000;
            SwitchToHuman_Timer = 60000;
            Berserk_Timer = 600000;
            InnerDemons_Timer = 30000;
            me->SetCanDualWield(true);
            DealDamage = true;
            DemonForm = false;
            IsFinalForm = false;
            NeedThreatReset = false;
            EnrageUsed = false;

            InnerDemon_Count = 0;
            me->SetSpeedRate(MOVE_RUN, 2.0f);
            me->SetDisplayId(MODEL_NIGHTELF);
            me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID  , 0);
            me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID+1, 0);
            DoCastSelf(SPELL_DUAL_WIELD, true);
            me->SetCorpseDelay(1000*60*60);
            instance->SetData(DATA_LEOTHERASTHEBLINDEVENT, NOT_STARTED);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
        }

        void CheckChannelers(/*bool DoEvade = true*/)
        {
            for (uint8 i = 0; i < 3; ++i)
            {
                if (Creature* add = ObjectAccessor::GetCreature(*me, SpellBinderGUID[i]))
                    add->DisappearAndDie();

                float nx = x;
                float ny = y;
                float o = 2.4f;
                if (i == 0) {nx += 10; ny -= 5; o=2.5f;}
                if (i == 1) {nx -= 8; ny -= 7; o=0.9f;}
                if (i == 2) {nx -= 3; ny += 9; o=5.0f;}
                Creature* binder = me->SummonCreature(NPC_SPELLBINDER, nx, ny, z, o, TEMPSUMMON_DEAD_DESPAWN, 0);
                if (binder)
                    SpellBinderGUID[i] = binder->GetGUID();
            }
        }
        void MoveInLineOfSight(Unit* who) override

        {
            if (me->HasAura(AURA_BANISH))
                return;

            if (!me->GetVictim() && me->CanCreatureAttack(who))
            {
                if (me->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
                    return;

                float attackRadius = me->GetAttackDistance(who);
                if (me->IsWithinDistInMap(who, attackRadius))
                {
                    // Check first that object is in an angle in front of this one before LoS check
                    if (me->HasInArc(float(M_PI) / 2.0f, who) && me->IsWithinLOSInMap(who))
                    {
                        AttackStart(who);
                    }
                }
            }
        }

        void StartEvent()
        {
            Talk(SAY_AGGRO);
            instance->SetData(DATA_LEOTHERASTHEBLINDEVENT, IN_PROGRESS);
        }

        void CheckBanish()
        {
            uint8 AliveChannelers = 0;
            for (uint8 i = 0; i < 3; ++i)
            {
                Unit* add = ObjectAccessor::GetUnit(*me, SpellBinderGUID[i]);
                if (add && add->IsAlive())
                    ++AliveChannelers;
            }

            // channelers == 0 remove banish aura
            if (AliveChannelers == 0 && me->HasAura(AURA_BANISH))
            {
                // removing banish aura
                me->RemoveAurasDueToSpell(AURA_BANISH);

                // Leotheras is getting immune again
                me->ApplySpellImmune(AURA_BANISH, IMMUNITY_MECHANIC, MECHANIC_BANISH, true);

                // changing model to bloodelf
                me->SetDisplayId(MODEL_NIGHTELF);

                // and reseting equipment
                me->LoadEquipment();

                if (instance->GetGuidData(DATA_LEOTHERAS_EVENT_STARTER))
                {
                    Unit* victim = NULL;
                    victim = ObjectAccessor::GetUnit(*me, instance->GetGuidData(DATA_LEOTHERAS_EVENT_STARTER));
                    if (victim)
                        me->GetThreatManager().addThreat(victim, 1);
                    StartEvent();
                }
            }
            else if (AliveChannelers != 0 && !me->HasAura(AURA_BANISH))
            {
                // channelers != 0 apply banish aura
                // removing Leotheras banish immune to apply AURA_BANISH
                me->ApplySpellImmune(AURA_BANISH, IMMUNITY_MECHANIC, MECHANIC_BANISH, false);
                DoCastSelf(AURA_BANISH);

                // changing model
                me->SetDisplayId(MODEL_DEMON);

                // and removing weapons
                me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID  , 0);
                me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID+1, 0);
            }
        }

        //Despawn all Inner Demon summoned
        void DespawnDemon()
        {
            for (uint8 i=0; i<5; ++i)
            {
                if (InnderDemon[i])
                {
                        //delete creature
                        Creature* creature = ObjectAccessor::GetCreature((*me), InnderDemon[i]);
                        if (creature && creature->IsAlive())
                        {
                            creature->DespawnOrUnsummon();
                        }
                        InnderDemon[i].Clear();
                }
            }

            InnerDemon_Count = 0;
        }

        void CastConsumingMadness() //remove this once SPELL_INSIDIOUS_WHISPER is supported by core
        {
            for (uint8 i=0; i<5; ++i)
            {
                if (InnderDemon[i])
                {
                    Creature* unit = ObjectAccessor::GetCreature((*me), InnderDemon[i]);
                    if (unit && unit->IsAlive())
                    {
                        Unit* unit_target = ObjectAccessor::GetUnit(*unit, unit->AI()->GetGuidData(INNER_DEMON_VICTIM));
                        if (unit_target && unit_target->IsAlive())
                        {
                            DoCast(unit_target, SPELL_CONSUMING_MADNESS, true);
                            ModifyThreatByPercent(unit_target, -100);
                        }
                    }
                }
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            Talk(DemonForm ? SAY_DEMON_SLAY : SAY_NIGHTELF_SLAY);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);

            //despawn copy
            if (Demon)
            {
                if (Creature* pDemon = ObjectAccessor::GetCreature(*me, Demon))
                    pDemon->DespawnOrUnsummon();
            }
            instance->SetData(DATA_LEOTHERASTHEBLINDEVENT, DONE);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            if (me->HasAura(AURA_BANISH))
            return;

            me->LoadEquipment();
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            for (Player& player : me->GetMap()->GetPlayers())
                if (player.HasAura(SPELL_CONSUMING_MADNESS))
                    me->Kill(&player);

            ScriptedAI::EnterEvadeMode();
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (me->HasAura(AURA_BANISH) || !UpdateVictim())
            {
                if (BanishTimer <= diff)
                {
                    CheckBanish();//no need to check every update tick
                    BanishTimer = 1000;
                } else BanishTimer -= diff;
                return;
            }
            if (me->HasAura(SPELL_WHIRLWIND))
            {
                if (Whirlwind_Timer <= diff)
                {
                    Unit* newTarget = SelectTarget(SELECT_TARGET_RANDOM, 0);
                    if (newTarget)
                    {
                        ResetThreatList();
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MovePoint(0, newTarget->GetPositionX(), newTarget->GetPositionY(), newTarget->GetPositionZ());
                    }
                    Whirlwind_Timer = 2000;
                } else Whirlwind_Timer -= diff;
            }

            // reseting after changing forms and after ending whirlwind
            if (NeedThreatReset && !me->HasAura(SPELL_WHIRLWIND))
            {
                // when changing forms seting timers (or when ending whirlwind - to avoid adding new variable i use Whirlwind_Timer to countdown 2s while whirlwinding)
                if (DemonForm)
                    InnerDemons_Timer = 30000;
                else
                    Whirlwind_Timer =  15000;

                NeedThreatReset = false;
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                ResetThreatList();
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MoveChase(me->GetVictim());
            }

            //Enrage_Timer (10 min)
            if (Berserk_Timer < diff && !EnrageUsed)
            {
                me->InterruptNonMeleeSpells(false);
                DoCastSelf(SPELL_BERSERK);
                EnrageUsed = true;
            } else Berserk_Timer -= diff;

            if (!DemonForm)
            {
                //Whirldind Timer
                if (!me->HasAura(SPELL_WHIRLWIND))
                {
                    if (Whirlwind_Timer <= diff)
                    {
                        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                        DoCastSelf(SPELL_WHIRLWIND);
                        // while whirlwinding this variable is used to countdown target's change
                        Whirlwind_Timer = 2000;
                        NeedThreatReset = true;
                    } else Whirlwind_Timer -= diff;
                }
                //Switch_Timer

                if (!IsFinalForm)
                {
                    if (SwitchToDemon_Timer <= diff)
                    {
                        //switch to demon form
                        me->RemoveAurasDueToSpell(SPELL_WHIRLWIND);
                        me->SetDisplayId(MODEL_DEMON);
                        Talk(SAY_SWITCH_TO_DEMON);
                        me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID  , 0);
                        me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID+1, 0);
                        DemonForm = true;
                        NeedThreatReset = true;
                        SwitchToDemon_Timer = 45000;
                    } else SwitchToDemon_Timer -= diff;
                }
                DoMeleeAttackIfReady();
            }
            else
            {
                //ChaosBlast_Timer
                if (!me->GetVictim())
                    return;
                if (me->IsWithinDist(me->GetVictim(), 30))
                    me->StopMoving();
                if (ChaosBlast_Timer <= diff)
                {
                    // will cast only when in range of spell
                    if (me->IsWithinDist(me->GetVictim(), 30))
                    {
                        //DoCastVictim(SPELL_CHAOS_BLAST, true);
                        int damage = 100;
                        me->CastCustomSpell(me->GetVictim(), SPELL_CHAOS_BLAST, &damage, NULL, NULL, false, NULL, NULL, me->GetGUID());
                    }
                    ChaosBlast_Timer = 3000;
                } else ChaosBlast_Timer -= diff;
                //Summon Inner Demon
                if (InnerDemons_Timer <= diff)
                {
                    std::vector<Unit*> TargetList;
                    Unit* currentVictim = me->GetThreatManager().GetCurrentVictim();
                    for (ThreatReference const* ref : me->GetThreatManager().GetSortedThreatList())
                    {
                        if (Player* tempTarget = ref->GetVictim()->ToPlayer())
                            if (tempTarget != currentVictim && tempTarget->IsAlive() && TargetList.size()<5)
                                TargetList.push_back(tempTarget);
                    }
                    //SpellInfo* spell = GET_SPELL(SPELL_INSIDIOUS_WHISPER);
                    for (Unit* target : TargetList)
                    {
                        Creature* demon = me->SummonCreature(INNER_DEMON_ID, target->GetPositionX()+10, target->GetPositionY()+10, target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
                        if (demon)
                        {
                            demon->AI()->AttackStart(target);
                            demon->AI()->SetGuidData(target->GetGUID(), INNER_DEMON_VICTIM);

                            target->AddAura(SPELL_INSIDIOUS_WHISPER, target);

                            if (InnerDemon_Count > 4)
                                InnerDemon_Count = 0;

                            //Safe storing of creatures
                            InnderDemon[InnerDemon_Count] = demon->GetGUID();

                            //Update demon count
                            ++InnerDemon_Count;
                        }
                    }
                    Talk(SAY_INNER_DEMONS);

                    InnerDemons_Timer = 999999;
                } else InnerDemons_Timer -= diff;

                //Switch_Timer
                if (SwitchToHuman_Timer <= diff)
                {
                    //switch to nightelf form
                    me->SetDisplayId(MODEL_NIGHTELF);
                    me->LoadEquipment();

                    CastConsumingMadness();
                    DespawnDemon();

                    DemonForm = false;
                    NeedThreatReset = true;

                    SwitchToHuman_Timer = 60000;
                } else SwitchToHuman_Timer -= diff;
            }

            if (!IsFinalForm && HealthBelowPct(15))
            {
                //at this point he divides himself in two parts
                CastConsumingMadness();
                DespawnDemon();
                Creature* Copy = NULL;
                Copy = DoSpawnCreature(DEMON_FORM, 0, 0, 0, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 6000);
                if (Copy)
                 {
                     Demon = Copy->GetGUID();
                    if (me->GetVictim())
                        Copy->AI()->AttackStart(me->GetVictim());
                }
                //set nightelf final form
                IsFinalForm = true;
                DemonForm = false;

                Talk(SAY_FINAL_FORM);
                me->SetDisplayId(MODEL_NIGHTELF);
                me->LoadEquipment();
            }
        }
    };
};

//Leotheras the Blind Demon Form AI
class boss_leotheras_the_blind_demonform : public CreatureScript
{
public:
    boss_leotheras_the_blind_demonform() : CreatureScript("boss_leotheras_the_blind_demonform") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_leotheras_the_blind_demonformAI(creature);
    }

    struct boss_leotheras_the_blind_demonformAI : public ScriptedAI
    {
        boss_leotheras_the_blind_demonformAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 ChaosBlast_Timer;
        bool DealDamage;

        void Reset() override
        {
            ChaosBlast_Timer = 1000;
            DealDamage = true;
        }

        void StartEvent()
        {
            Talk(SAY_FREE);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            Talk(SAY_DEMON_SLAY);
        }

        void JustDied(Unit* /*killer*/) override
        {
            //invisibility (blizzlike, at the end of the fight he doesn't die, he disappears)
            DoCastSelf(8149, true);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            StartEvent();
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;
            //ChaosBlast_Timer
            if (me->IsWithinDist(me->GetVictim(), 30))
                me->StopMoving();

            if (ChaosBlast_Timer <= diff)
             {
                // will cast only when in range od spell
                if (me->IsWithinDist(me->GetVictim(), 30))
                {
                    //DoCastVictim(SPELL_CHAOS_BLAST, true);
                    int damage = 100;
                    me->CastCustomSpell(me->GetVictim(), SPELL_CHAOS_BLAST, &damage, NULL, NULL, false, NULL, NULL, me->GetGUID());
                    ChaosBlast_Timer = 3000;
                }
             } else ChaosBlast_Timer -= diff;

            //Do NOT deal any melee damage to the target.
        }
    };
};

class npc_greyheart_spellbinder : public CreatureScript
{
public:
    npc_greyheart_spellbinder() : CreatureScript("npc_greyheart_spellbinder") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<npc_greyheart_spellbinderAI>(creature);
    }

    struct npc_greyheart_spellbinderAI : public ScriptedAI
    {
        npc_greyheart_spellbinderAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            leotherasGUID.Clear();
            AddedBanish = false;
        }

        InstanceScript* instance;

        ObjectGuid leotherasGUID;

        uint32 Mindblast_Timer;
        uint32 Earthshock_Timer;

        bool AddedBanish;

        void Reset() override
        {
            Mindblast_Timer  = urand(3000, 8000);
            Earthshock_Timer = urand(5000, 10000);

            instance->SetGuidData(DATA_LEOTHERAS_EVENT_STARTER, ObjectGuid::Empty);
            Creature* leotheras = ObjectAccessor::GetCreature(*me, leotherasGUID);
            if (leotheras && leotheras->IsAlive())
                ENSURE_AI(boss_leotheras_the_blind::boss_leotheras_the_blindAI, leotheras->AI())->CheckChannelers(/*false*/);
        }

        void EnterCombat(Unit* who) override
        {
            me->InterruptNonMeleeSpells(false);
            instance->SetGuidData(DATA_LEOTHERAS_EVENT_STARTER, who->GetGUID());
        }

        void JustRespawned() override
        {
            AddedBanish = false;
            Reset();
        }

        void CastChanneling()
        {
            if (!me->IsInCombat() && !me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            {
                if (leotherasGUID)
                {
                    Creature* leotheras = ObjectAccessor::GetCreature(*me, leotherasGUID);
                    if (leotheras && leotheras->IsAlive())
                        DoCast(leotheras, BANISH_BEAM);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!leotherasGUID)
                leotherasGUID = instance->GetGuidData(DATA_LEOTHERAS);

            if (!me->IsInCombat() && instance->GetGuidData(DATA_LEOTHERAS_EVENT_STARTER))
            {
                Unit* victim = NULL;
                victim = ObjectAccessor::GetUnit(*me, instance->GetGuidData(DATA_LEOTHERAS_EVENT_STARTER));
                if (victim)
                    AttackStart(victim);
            }

            if (!UpdateVictim())
            {
                CastChanneling();
                return;
            }

            if (!instance->GetGuidData(DATA_LEOTHERAS_EVENT_STARTER))
            {
                EnterEvadeMode();
                return;
            }

            if (Mindblast_Timer <= diff)
            {
                Unit* target = NULL;
                target = SelectTarget(SELECT_TARGET_RANDOM, 0);

                if (target)DoCast(target, SPELL_MINDBLAST);

                Mindblast_Timer = urand(10000, 15000);
            } else Mindblast_Timer -= diff;

            if (Earthshock_Timer <= diff)
            {
                Map* map = me->GetMap();
                for (Player& player : map->GetPlayers())
                {
                    bool isCasting = false;
                    for (uint8 i = 0; i < CURRENT_MAX_SPELL; ++i)
                        if (player.GetCurrentSpell(i))
                            isCasting = true;

                    if (isCasting)
                    {
                        DoCast(&player, SPELL_EARTHSHOCK);
                        break;
                    }
                }
                Earthshock_Timer = urand(8000, 15000);
            } else Earthshock_Timer -= diff;
            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) override { }
    };
};

void AddSC_boss_leotheras_the_blind()
{
    new boss_leotheras_the_blind();
    new boss_leotheras_the_blind_demonform();
    new npc_greyheart_spellbinder();
    new npc_inner_demon();
}
