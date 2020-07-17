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
#include "SpellScript.h"
#include "CombatAI.h"
#include "Vehicle.h"
#include "SpellScript.h"
#include "SmartAI.h"

/*######
## Quests: 12327 "Out of Body Experience"
## spell_q12327_out_of_body_experience
## TODO: event with speaking npc
######*/

class spell_q12327_out_of_body_experience : public SpellScriptLoader
{
    public:
        spell_q12327_out_of_body_experience() : SpellScriptLoader("spell_q12327_out_of_body_experience") { }

        class spell_q12327_out_of_body_experience_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_q12327_out_of_body_experience_AuraScript)

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* owner = GetUnitOwner())
                    if (owner->ToPlayer())
                    {
                        owner->ToPlayer()->TeleportTo(571, 3838.439f, -3429.021f, 293.10f, 1.703f);
                        owner->ToPlayer()->CompleteQuest(12327);
                    }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_q12327_out_of_body_experience_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_MOD_INVISIBILITY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_q12327_out_of_body_experience_AuraScript();
        }
};

/*######
## go_school_of_northern_salmon
######*/
class go_school_of_northern_salmon : public GameObjectScript
{
public:
    go_school_of_northern_salmon() : GameObjectScript("go_school_of_northern_salmon") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go)
    {
        go->SetLootState(GO_JUST_DEACTIVATED);
        return false;
    }

};

/*######
## Quests: 12432, 12437 "Riding the Red Rocket"
## npc_rocket_propelled_warhead
######*/

enum QuestRidingTheRedRocket
{
    SPELL_RIDE_VEHICLE_HARDCODED = 46598,
    SPELL_PARACHUTE              = 44795,
    SPELL_KNOCKBACK_35              = 24199,
    SPELL_PERPETUAL_INSTABILITY  = 49163,
    NPC_ALLIANCE_LUMBERBOAT      = 27688,
    NPC_HORDE_LUMBERBOAT         = 27702,
    ITEM_ELEMENT_115             = 37664
};

class npc_rocket_propelled_warhead : public CreatureScript
{
public:
    npc_rocket_propelled_warhead() : CreatureScript("npc_rocket_propelled_warhead") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_rocket_propelled_warheadAI (creature);
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->HasAura(SPELL_PERPETUAL_INSTABILITY))
        {
            player->EnterVehicle(creature, 0);
            player->DestroyItemCount(ITEM_ELEMENT_115, 1, true);
        }
        
        return true;
    }

    struct npc_rocket_propelled_warheadAI : public VehicleAI
    {
        npc_rocket_propelled_warheadAI(Creature* creature) : VehicleAI(creature) {}

        float OldPosZ;
        uint32 checkTimer;
        
        void Reset()
        {
            OldPosZ = me->GetPositionZ();
            checkTimer = 1000;
        }

        void JustReachedHome()
        {
            me->SetVisible(true);
            me->GetMotionMaster()->MovementExpired();
            me->GetMotionMaster()->Clear(true);
            me->StopMoving();
            me->GetMotionMaster()->MoveIdle();
        }

        void UpdateAI(uint32 diff)
        {
            if (me->HasAura(SPELL_RIDE_VEHICLE_HARDCODED))
            {
                float deltaz = me->GetPositionZ() - OldPosZ;

                // it seems that are no absolute plane walls in the game so it should work to just check the z-coordinate to detect collisions
                Creature* horde = NULL;
                Creature* alliance = NULL;
                if (checkTimer <= diff)
                {
                    horde = me->FindNearestCreature(NPC_HORDE_LUMBERBOAT, 20.0f);
                    alliance = me->FindNearestCreature(NPC_ALLIANCE_LUMBERBOAT, 20.0f);
                    checkTimer = urand(1000, 1500);
                }
                else
                    checkTimer -= diff;

                if ((deltaz != 0.0f) || alliance || horde) 
                {
                    if (Vehicle* vehicle = me->GetVehicleKit())
                    {
                        if (Unit* passenger = vehicle->GetPassenger(0))
                        {
                            passenger->ExitVehicle();
                            if (Player* player = passenger->ToPlayer())
                            {
                                if (alliance)
                                {
                                    player->KilledMonsterCredit(NPC_ALLIANCE_LUMBERBOAT);
                                }
                                if (horde)
                                {
                                    player->KilledMonsterCredit(NPC_HORDE_LUMBERBOAT);
                                }
                                player->CastSpell(player, SPELL_KNOCKBACK_35);
                                player->CastSpell(player, SPELL_PARACHUTE);
                            }
                            me->SetVisible(false);
                            me->GetMotionMaster()->MoveTargetedHome();
                        }
                    }
                }
                OldPosZ = me->GetPositionZ();
            }
        }
    };
};

/*######
## Quest: 12093 "Runes of Compulsion"
## npc_directional_rune
######*/

enum QuestRunesOfCompulsion
{

    NPC_DIRECTIONAL_RUNE_GUID_1 = 111145, 
    NPC_DIRECTIONAL_RUNE_GUID_2 = 111146, 
    NPC_DIRECTIONAL_RUNE_GUID_3 = 111147, 
    NPC_DIRECTIONAL_RUNE_GUID_4 = 202405,

    NPC_DURVAL                  = 26920,
    NPC_KORGAN                  = 26921,
    NPC_LOCHLI                  = 26922,
    NPC_BRUNON                  = 26923,
    NPC_RUNE_WEAVER             = 26820,

    GO_DIRECTIONAL_RUNE_1       = 188471,
    GO_DIRECTIONAL_RUNE_2       = 188505,
    GO_DIRECTIONAL_RUNE_3       = 188506,
    GO_DIRECTIONAL_RUNE_4       = 188507,

    DATA_DWARF_SPAWN            = 0,
    DATA_DWARF_DEATH            = 1
};

class npc_directional_rune : public CreatureScript
{
public:
    npc_directional_rune() : CreatureScript("npc_directional_rune") { }

    struct npc_directional_runeAI : public ScriptedAI
    {
        npc_directional_runeAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            overseerEntry = 0;
            runeEntry = 0;

            switch (me->GetGUID().GetCounter())
            {
                case NPC_DIRECTIONAL_RUNE_GUID_1:    overseerEntry = NPC_DURVAL;    runeEntry = GO_DIRECTIONAL_RUNE_1;    break;
                case NPC_DIRECTIONAL_RUNE_GUID_2:    overseerEntry = NPC_KORGAN;    runeEntry = GO_DIRECTIONAL_RUNE_2;    break;
                case NPC_DIRECTIONAL_RUNE_GUID_3:    overseerEntry = NPC_LOCHLI;    runeEntry = GO_DIRECTIONAL_RUNE_3;    break;
                case NPC_DIRECTIONAL_RUNE_GUID_4:    overseerEntry = NPC_BRUNON;    runeEntry = GO_DIRECTIONAL_RUNE_4;    break;
                default: break;
            }
        }

        void ChangeRuneState(bool activate)
        {
            me->GetGameObjectListWithEntryInGrid(runeList, runeEntry, 10.0f);
            if (!runeList.empty())
            {
                // There should only be one rune
                GameObject* rune = runeList.front();
                if (activate)
                {
                    rune->SetGoState(GO_STATE_ACTIVE);
                    return;
                }
                rune->SetGoState(GO_STATE_READY);
            }
        }

        void SetData(uint32 type, uint32 /*Data*/)
        {
            switch (type)
            {
            case DATA_DWARF_SPAWN:
                ChangeRuneState(true);
                if (Creature* overseer = me->FindNearestCreature(overseerEntry, 30.0f, true)) 
                {
                    overseer->DespawnOrUnsummon();
                } 
                break;
            case DATA_DWARF_DEATH:
                if (!me->FindNearestCreature(NPC_RUNE_WEAVER, 30.0f, true) &&
                    !me->FindNearestCreature(overseerEntry, 30.0f, true))
                {
                    if (overseerEntry == 0) break;
                    me->SummonCreature(overseerEntry, 0.0f, 0.0f, 0.0f);
                    ChangeRuneState(false);
                }
                break;
            }         
        }

    private:
        std::list<GameObject*> runeList;
        ObjectGuid runeGUID;
        uint32 overseerEntry;
        uint32 runeEntry;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_directional_runeAI (creature);
    }
};

enum EscapefromSilverbrookMisc
{
    WAYPOINT_DUCAL                = 2740900,
    SPELL_SILVERBROOK_KILLCREDIT  = 50473,
    SPELL_ESCAPE_FROM_SILVERBROOK = 48683,
    NPC_PRISONER                  = 27411
};

class npc_ducals_horse : public CreatureScript
{
    public:
        npc_ducals_horse() : CreatureScript("npc_ducals_horse") { }

        struct npc_ducals_horseAI : public VehicleAI
        {
            npc_ducals_horseAI(Creature* creature) : VehicleAI(creature) { }

            void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) override
            {
                if (apply)
                {
                    if (passenger->GetTypeId() == TYPEID_PLAYER)
                    {
                        me->GetMotionMaster()->MovePath(WAYPOINT_DUCAL, false);
                        me->SetReactState(REACT_PASSIVE);
                        DoCastSelf(SPELL_ESCAPE_FROM_SILVERBROOK, true);
                        if (Creature* prisoner = me->FindNearestCreature(NPC_PRISONER, 100.0f))
                            prisoner->AI()->SetData(1, 1);
                    }                    
                }
                else
                {
                    if (passenger->GetTypeId() == TYPEID_PLAYER)
                    {                        
                        if (Creature* prisoner = me->FindNearestCreature(NPC_PRISONER, 100.0f))
                            prisoner->DespawnOrUnsummon();
                        me->DespawnOrUnsummon(1 * IN_MILLISECONDS);
                    }                    
                }
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != WAYPOINT_MOTION_TYPE)
                    return;
                                
                switch (id)
                {
                    case 101:
                        if (Player* player = me->FindNearestPlayer(100.0f))
                            player->CastSpell(player, SPELL_SILVERBROOK_KILLCREDIT, true);
                        me->DespawnOrUnsummon(2 * IN_MILLISECONDS);
                        if (Creature* prisoner = me->FindNearestCreature(NPC_PRISONER, 100.0f))
                            prisoner->DespawnOrUnsummon();
                        break;
                    default:
                        break;
                }
            }

            void UpdateAI(uint32 /*diff*/) override { } // No reaction to enemies
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ducals_horseAI(creature);
        }
};

enum ItTakesGutsMisc
{
    NPC_ANCIENT_DRAKKARI_SOOTHSAYER = 26812,
    NPC_ANCIENT_DRAKKARI_WARMONGER  = 26811,
    SAY_AGGRO                       = 1
};

const uint32 NpcEntry[] =
{
    NPC_ANCIENT_DRAKKARI_SOOTHSAYER, NPC_ANCIENT_DRAKKARI_WARMONGER
};

class go_drakkari_canopic_jar : public GameObjectScript
{
    public:
        go_drakkari_canopic_jar() : GameObjectScript("go_drakkari_canopic_jar") { }

        bool OnGossipHello(Player* player, GameObject* go)
        {        
            int Random = rand32() % (sizeof(NpcEntry) / sizeof(uint32));
            if (Creature* ghost = player->FindNearestCreature(NpcEntry[Random], 100.0f))
            {
                ghost->AI()->AttackStart(player);
                ghost->AI()->Talk(SAY_AGGRO, player);
            }
            player->SendLoot(go->GetGUID(), LOOT_CORPSE);
            
            return true;
        }
};

enum BuddMisc
{
    NPC_BUDD = 32663
};

class spell_budds_attention_span : public SpellScriptLoader
{
    public:
        spell_budds_attention_span() : SpellScriptLoader("spell_budds_attention_span") { }

        class spell_budds_attention_span_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_budds_attention_span_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (!GetCaster())
                    return;

                if (Unit* caster = GetCaster())
                    if (caster->GetPetGUID())
                        if (Guardian* pet = caster->GetGuardianPet())
                            if (pet->GetEntry() == NPC_BUDD)
                                pet->DespawnOrUnsummon();
            }

            void Register() override
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_budds_attention_span_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_budds_attention_span_AuraScript();
        }
};

enum HarrisonJonesMisc
{
    // Gameobjects
    GO_FIRE_DOOR = 188480,

    // Datatypes
    DATA_DOOR    = 7
};

class npc_harrison_jones_grizzly : public CreatureScript
{
    public:
        npc_harrison_jones_grizzly() : CreatureScript("npc_harrison_jones_grizzly") { }
    
        struct npc_harrison_jones_grizzlyAI : public SmartAI
        {
            npc_harrison_jones_grizzlyAI(Creature* creature) : SmartAI(creature) { }
    
            void SetData(uint32 type, uint32 data) override
            {
                if (type == DATA_DOOR)
                {
                    // Make shure that the Door always despawns
                    if (GameObject* door = me->FindNearestGameObject(GO_FIRE_DOOR, 300.0f))
                        door->SetGoState(GO_STATE_ACTIVE);
                }
    
                SmartAI::SetData(type, data);
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_harrison_jones_grizzlyAI(creature);
        }
};

enum ShadeOfArugalSpells
{
    SPELL_SHADOW_BOLT       = 53086,
    SPELL_PHADE_OUT         = 53052,
    SPELL_WORGENS_COMMAND   = 53070,
    SPELL_ARUGAL_CHANNEL    = 48185
};

enum ShadeOfArugalPhases
{
    ARUGAL_PHASE_1,
    ARUGAL_PHASE_2,
    ARUGAL_PHASE_3,
    ARUGAL_PHASE_4,
};

enum ShadeOfArugalMisc
{
    NPC_BLOODMOON_SERVANT = 29082,
    NPC_BLOODMOON_CULTIST = 27024
};

// maybe some refinements here are good
const Position servants[9] = {
        {4603.678223f, -5706.382324f, 184.5069f, 2.94f},
        {4597.931152f, -5697.2827f, 184.5069f, 4.2f},
        {4590.0f, -5712.43f, 184.507f, 1.0f},
        {4585.0986f, -5706.3447f, 184.5069f, 0.2726f},
        {4586.9179f, -5710.0561f, 184.5069f, 0.5043f},
        {4594.91845f, -5713.9121f, 184.5069f, 1.592f},
        {4604.6757f, -5703.9472f, 184.5069f, 3.308f},
        {4593.8867f, -5694.8085f, 184.5069f, 4.914f},
        {4590.1376f, -5695.8613f, 184.5069f, 5.1539f}
};

// Shade of Arugal - 27018
class npc_shade_of_arugal : public CreatureScript
{
    public:
        npc_shade_of_arugal() : CreatureScript("npc_shade_of_arugal") { }

        struct npc_shade_of_arugalAI : public ScriptedAI
        {
            npc_shade_of_arugalAI(Creature* creature) : ScriptedAI(creature) { }

            void DamageTaken(Unit* /*who*/, uint32 &damage)
            {
                if(HealthBelowPct(25)){
                    SetPhase(ARUGAL_PHASE_4);
                    return;
                }

                if(HealthBelowPct(50)){
                    SetPhase(ARUGAL_PHASE_3);
                    return;
                }

                if(HealthBelowPct(75))
                    SetPhase(ARUGAL_PHASE_2);
            }

            void EnterCombat(Unit* /*who*/)
            {
                me->CastStop();
            }

            void Reset()
            {
                phase = ARUGAL_PHASE_1;
                summons = 0;
                if(Creature* creature = me->FindNearestCreature(NPC_BLOODMOON_CULTIST, 20.0f))
                    me->CastSpell(creature, SPELL_ARUGAL_CHANNEL);
            }

            void UpdateAI(uint32 diff)
            {
                if(!UpdateVictim()) {
                    if(me->IsInCombat() || me->HasUnitState(UNIT_STATE_CASTING))
                        return;

                    if(Creature* creature = me->FindNearestCreature(27024, 20.0f))
                        me->CastSpell(creature, SPELL_ARUGAL_CHANNEL);
                }

                if(me->HasUnitState(UNIT_STATE_CASTING) || me->HasAura(SPELL_PHADE_OUT))
                    return;

                me->CastSpell(me->GetVictim(), SPELL_SHADOW_BOLT);

                DoMeleeAttackIfReady();
            }

            void SetPhase(ShadeOfArugalPhases newPhase)
            {
                if (phase == newPhase)
                    return;

                phase = newPhase;
                switch(newPhase)
                {
                    case ARUGAL_PHASE_2:
                        SummonWave(3);
                        DoCastSelf(SPELL_PHADE_OUT);
                        break;
                    case ARUGAL_PHASE_3:
                        if (Player* target = SelectRandomPlayer())
                            me->CastSpell(target, SPELL_WORGENS_COMMAND);
                        break;
                    case ARUGAL_PHASE_4:
                        SummonWave(9);
                        DoCastSelf(SPELL_PHADE_OUT);
                        break;
                    default:
                        break;
                }
            }

            Player* SelectRandomPlayer()
            {
                std::list<Player*> list;
                me->GetPlayerListInGrid(list, 40.f);
                if (list.size() <= 1)
                    return nullptr;
                return Trinity::Containers::SelectRandomContainerElement(list);
            }

            void SummonedCreatureDies(Creature* creature, Unit* /*killer*/)
            {
                if(--summons <= 0 && me->HasAura(SPELL_PHADE_OUT))
                    me->RemoveAura(SPELL_PHADE_OUT);
            }

            void SummonWave(uint8 count)
            {
                summons = count;
                for(uint8 i = 0; i<count; i++){
                    if (Creature* summon = me->SummonCreature(NPC_BLOODMOON_SERVANT, servants[i]))
                    {
                        summon->SetReactState(REACT_AGGRESSIVE);
                        if (auto victim = me->GetVictim())
                            summon->SetInCombatWith(victim);
                    }
                }
            }

            private:
                uint8 summons;
                ShadeOfArugalPhases phase;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_shade_of_arugalAI(creature);
        }
};

enum GrizzlyBearFishEvent
{
    EVENT_SUMMON_FISH       = 1,
    EVENT_CATCH_FISH        = 2,
    EVENT_WAIT_FOR_FISH     = 3,
    EVENT_EAT_FISH          = 4,
    EVENT_RESET_ME          = 5,

    SPELL_SUMMON_FISH       = 48758,
    SPELL_CATCH_FISH        = 48752,
    SPELL_CRITTER_BITE      = 57388,

    NPC_GRIZZLY             = 27131,
    NPC_FISH                = 27438,
};

class npc_grizzly_bear_event_fish : public CreatureScript
{
public:
    npc_grizzly_bear_event_fish() : CreatureScript("npc_grizzly_bear_event_fish") { }

    struct npc_grizzly_bear_event_fishAI : public ScriptedAI
    {
        npc_grizzly_bear_event_fishAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->DespawnOrUnsummon(40 * IN_MILLISECONDS);
        }

        void SpellHit(Unit* caster, SpellInfo const* spell) override 
        {
            if (spell && spell->Id == SPELL_CRITTER_BITE && caster)
            {
                if (Unit* grizzly = me->GetVehicleBase())
                {
                    me->DespawnOrUnsummon(181 * IN_MILLISECONDS);
                    grizzly->SetInCombatWith(caster);
                    grizzly->Attack(caster, true);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_grizzly_bear_event_fishAI(creature);
    }
};

class npc_grizzly_bear_event : public CreatureScript
{
public:
    npc_grizzly_bear_event() : CreatureScript("npc_grizzly_bear_event") { }

    struct npc_grizzly_bear_eventAI : public ScriptedAI
    {
        npc_grizzly_bear_eventAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();

            if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                _events.ScheduleEvent(EVENT_SUMMON_FISH, urand(5, 20) * IN_MILLISECONDS);

            _FishGUID.Clear();
        }

        void JustSummoned(Creature* summon) override 
        {
            if (summon && summon->GetEntry() == NPC_FISH)
            {
                summon->SetSpeedRate(MOVE_WALK, 0.3f);
                summon->SetSpeedRate(MOVE_RUN, 0.3f);
                summon->SetSpeedRate(MOVE_SWIM, 0.3f);
                summon->SetSpeedRate(MOVE_FLIGHT, 0.3f);
                summon->GetMotionMaster()->MoveRandom(5.0f);

                _FishGUID = summon->GetGUID();
            } 
        }

        void MovementInform(uint32 type, uint32 /*id*/) override 
        {
            if (type == POINT_MOTION_TYPE)
                me->SetFacingTo(me->GetHomePosition().GetOrientation());
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUMMON_FISH:
                            DoCastSelf(SPELL_SUMMON_FISH);
                            _events.ScheduleEvent(EVENT_CATCH_FISH, 2 * IN_MILLISECONDS);
                            break;
                        case EVENT_CATCH_FISH:
                            DoCast(SPELL_CATCH_FISH);
                            _events.ScheduleEvent(EVENT_WAIT_FOR_FISH, 4 * IN_MILLISECONDS);
                            break;
                        case EVENT_WAIT_FOR_FISH:
                            me->GetMotionMaster()->MoveRandom(10.0f);
                            _events.ScheduleEvent(EVENT_EAT_FISH, 20 * IN_MILLISECONDS);
                            break;
                        case EVENT_EAT_FISH:
                            me->GetMotionMaster()->MoveIdle();
                            me->GetMotionMaster()->Clear();
                            me->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
                            if (Creature* fish = ObjectAccessor::GetCreature(*me, _FishGUID))
                                fish->DespawnOrUnsummon(1000);
                            _events.ScheduleEvent(EVENT_RESET_ME, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_RESET_ME:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_NONE);
                            me->GetMotionMaster()->MoveTargetedHome();
                            me->GetMotionMaster()->MovePoint(1, me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ());
                            _events.ScheduleEvent(EVENT_SUMMON_FISH, urand(15, 45) * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
            }
            else
                DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        ObjectGuid   _FishGUID;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_grizzly_bear_eventAI(creature);
    }
};

class spell_grizzly_bear_catch_fish : public SpellScriptLoader
{
public:
    spell_grizzly_bear_catch_fish() : SpellScriptLoader("spell_grizzly_bear_catch_fish") { }

    class spell_grizzly_bear_catch_fish_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_grizzly_bear_catch_fish_SpellScript);

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Unit* grizzly = GetCaster())
                if (Creature* fish = grizzly->FindNearestCreature(NPC_FISH, 10.0f))
                    fish->EnterVehicle(grizzly);
        }

        void Register()
        {
            OnEffectLaunch += SpellEffectFn(spell_grizzly_bear_catch_fish_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_grizzly_bear_catch_fish_SpellScript();
    }
};

void AddSC_grizzly_hills_rg()
{
    new spell_q12327_out_of_body_experience();
    new go_school_of_northern_salmon();
    new npc_rocket_propelled_warhead();
    new npc_directional_rune();
    new npc_ducals_horse();
    new go_drakkari_canopic_jar();
    new spell_budds_attention_span();
    new npc_harrison_jones_grizzly();
    new npc_shade_of_arugal();
    new npc_grizzly_bear_event_fish();
    new npc_grizzly_bear_event();
    new spell_grizzly_bear_catch_fish();
}
