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
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "CombatAI.h"
#include "ReputationMgr.h"
#include "Vehicle.h"
#include "SpellInfo.h"
#include "SmartAI.h"

/*######
# Quest A Rough Ride
######*/

#define GOSSIP_ITEM_CROCO "Reise zur Zuflucht der Nebelflüsterer"

class npc_captive_croco_gossip : public CreatureScript
{
public:
    npc_captive_croco_gossip() : CreatureScript("npc_captive_croco_gossip") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {

        if (player->GetQuestStatus(12536) == QUEST_STATUS_INCOMPLETE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_CROCO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*uiSender*/, uint32 /*uiAction*/)
    {
        player->CLOSE_GOSSIP_MENU();

        if (!player->GetVehicle())
        {
            if (Creature* pTemp = player->SummonCreature(28308, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 0))
            {
                pTemp->SetCreatorGUID(player->GetGUID());
                player->EnterVehicle(pTemp, 0);
            }
            if (Unit* base = player->GetVehicleBase())
                if (base->IsCharmed())
                    base->RemoveCharmedBy(base->GetCharmer());
        }
        return true;
    }
};

class npc_captive_croco_vehicle : public CreatureScript
{
public:
    npc_captive_croco_vehicle() : CreatureScript("npc_captive_croco_vehicle") { }

    struct npc_captive_croco_vehicleAI : public npc_escortAI
    {
        npc_captive_croco_vehicleAI(Creature* creature) : npc_escortAI(creature) { }

        void Reset()
        {
            me->SetSpeedRate(MOVE_SWIM, 2.0f);
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            if (who->GetTypeId() == TYPEID_PLAYER)
            {
                if (apply)
                    Start(false, true, who->GetGUID());
            }
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);
        }

        void WaypointReached(uint32 waypointId)
        {
            if (waypointId == 32)
            {
                Unit* player = me->GetVehicleKit()->GetPassenger(0);
                if (player && player->GetTypeId() == TYPEID_PLAYER)
                {
                    player->ToPlayer()->CompleteQuest(12536);
                    player->ExitVehicle();
                }
                me->DisappearAndDie();
            }
        }


        void AttackStart(Unit* /*who*/) {}
        void EnterCombat(Unit* /*who*/) {}
        void EnterEvadeMode(EvadeReason /*why*/) {}
        void JustDied(Unit* /*killer*/) {}
        void OnCharmed(bool /*apply*/) {}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_captive_croco_vehicleAI(creature);
    }
};

/*##########
# Quest The Angry Gorloc
###########*/

enum MoodleData
{
    AREA_MOSSWALKER_VILLAGE = 4297,
    QUEST_THE_ANGRY_GORLOC = 12578
};

class npc_moodle : public CreatureScript
{
public:
    npc_moodle() : CreatureScript("npc_moodle") { }

    struct npc_moodleAI : public ScriptedAI
    {
        npc_moodleAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            if (Unit* owner = me->GetOwner())
                if (owner->GetTypeId() == TYPEID_PLAYER)
                    if (owner->ToPlayer()->GetAreaId() == AREA_MOSSWALKER_VILLAGE && owner->ToPlayer()->GetQuestStatus(QUEST_THE_ANGRY_GORLOC) == QUEST_STATUS_INCOMPLETE)
                        owner->ToPlayer()->GroupEventHappens(QUEST_THE_ANGRY_GORLOC, me);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_moodleAI(creature);
    }
};

/*######
## Quest Song of Wind and Water
######*/

enum eHaiphoon
{
    SPELL_DEVOUR_WIND       = 52862,
    SPELL_DEVOUR_WATER      = 52864,

    NPC_HAIPHOON_WATER      = 28999,
    NPC_HAIPHOON_AIR        = 28985
};

class vehicle_haiphoon : public CreatureScript
{
public:
    vehicle_haiphoon() : CreatureScript("vehicle_haiphoon") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new vehicle_haiphoonAI(creature);
    }

    struct vehicle_haiphoonAI : public VehicleAI
    {
        vehicle_haiphoonAI(Creature* creature) : VehicleAI(creature) { }

        void SpellHitTarget(Unit* target,SpellInfo const* spell)
        {
            if(target == me)
                return;

            if(spell->Id == SPELL_DEVOUR_WIND)
            {
                if(Player* player = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                {
                    me->UpdateEntry(NPC_HAIPHOON_AIR);
                    player->VehicleSpellInitialize();
                    me->setFaction(player->getFaction());
                }
            }

            if(spell->Id == SPELL_DEVOUR_WATER)
            {
                if(Player* player = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                {
                    me->UpdateEntry(NPC_HAIPHOON_WATER);
                    player->VehicleSpellInitialize();
                    me->setFaction(player->getFaction());
                }
            }
        }
    };
};

/*####
## npc_lafoo
## UPDATE creature_template SET ScriptName='npc_lafoo' WHERE  entry=28120;
###*/
enum Lafoo{
    //Summoning GOs
    SPELL_DIG_UP_SHINING_CRYSTAL = 51397,
    SPELL_DIG_UP_GLINTING_ARMOR = 51398,
    SPELL_DIG_UP_GLOWING_GEM = 51400,
    SPELL_DIG_UP_POLISHED_PLATE = 51401,
    SPELL_DIG_UP_SPARKLING_TREASURE = 51402,
    SPELL_DIG_UP_SPARKLING_HARE = 51441,
    SPELL_DIG_UP_GLINTING_BUG = 51443,

    NPC_SHINY_THING_VISUAL = 1010579,
    GO_SHINY_BAUBLE = 192949
};

class npc_lafoo : public CreatureScript
{
public:

    npc_lafoo() : CreatureScript("npc_lafoo"){}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lafooAI(creature);
    }

    struct npc_lafooAI : public ScriptedAI
    {
        npc_lafooAI(Creature* c) : ScriptedAI(c)
        {
            DigTimer = 1500;
            MoveToGO = false;
            DigUpSpells[0] = SPELL_DIG_UP_GLINTING_ARMOR;
            DigUpSpells[1] = SPELL_DIG_UP_GLINTING_BUG;
            DigUpSpells[2] = SPELL_DIG_UP_GLOWING_GEM;
            DigUpSpells[3] = SPELL_DIG_UP_POLISHED_PLATE;
            DigUpSpells[4] = SPELL_DIG_UP_SHINING_CRYSTAL;
            DigUpSpells[5] = SPELL_DIG_UP_SPARKLING_HARE;
            DigUpSpells[6] = SPELL_DIG_UP_SPARKLING_TREASURE;
        }

        uint32 DigTimer;
        bool MoveToGO;
        uint32 DigUpSpells[7];

        void UpdateAI(uint32 diff){
            if(!MoveToGO)
            {
                if(Creature* shiny = me->FindNearestCreature(NPC_SHINY_THING_VISUAL,15.0f))
                {
                    me->GetMotionMaster()->MovePoint(0,shiny->GetPositionX(),shiny->GetPositionY(),shiny->GetPositionZ());
                    MoveToGO  = true;
                }
                else
                    return;
            }

            if(MoveToGO)
            {
                if(DigTimer<=diff)
                {
                    if(Creature* shiny = me->FindNearestCreature(NPC_SHINY_THING_VISUAL, 1.0f))
                    {
                        me->CastSpell(shiny,DigUpSpells[urand(0,6)],false);
                        if(GameObject* shinyGO = shiny->FindNearestGameObject(GO_SHINY_BAUBLE,1.0f))
                        {
                            shinyGO->SetPhaseMask(2,true);
                        }
                        shiny->DespawnOrUnsummon();
                    }
                    me->GetMotionMaster()->MoveFollow(me->GetOwner(), PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                    MoveToGO = false;
                    DigTimer = 1500;
                }
                else if(me->FindNearestCreature(NPC_SHINY_THING_VISUAL, 1.0f))
                    DigTimer -= diff;
            }
        }
    };
};

/*######
## npc_shiny_thing_visual
######*/
class npc_shiny_thing_visual : public CreatureScript
{
public:
    npc_shiny_thing_visual() : CreatureScript("npc_shiny_thing_visual")
    {
    }

    struct npc_shiny_thing_visualAI : public ScriptedAI
    {
        npc_shiny_thing_visualAI(Creature* pCreature) : ScriptedAI (pCreature)
        {
        }

        void Reset()
        {
            if(GameObject* shinyGO = me->FindNearestGameObject(GO_SHINY_BAUBLE,1.0f))
            {
                shinyGO->SetPhaseMask(1,true);
            }
            me->AddAura(51444,me);
        }
    };
    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_shiny_thing_visualAI(pCreature);
    }
};

/*######
## npc_artruis_the_heartless
######*/

#define SPELL_ARTRUIS_FROST_NOVA            11831
#define SPELL_ARTRUIS_FROST_BOLT            15530
#define SPELL_ARTRUIS_ICY_VEINS             54792
#define SPELL_ARTRUIS_ICE_LANCE             54261
#define SPELL_BINDINGS_OF_SUBMISSION        52185
#define SPELL_TOMB_OF_THE_HEARTLESS         52182

#define NPC_JALOOT                          28667
#define NPC_ZEPTIK                          28668
#define NPC_ARTRUIS                         28659
#define ENTRY_ARTRUIS_URN                   190777
#define QUEST_HAND_OF_THE_ORACELS           12689
#define QUEST_FRENZYHEART_CHAMPION          12582
#define FACTION_FRENZYHEART                 1104

class npc_artruis_the_heartless : public CreatureScript
{
public:
    npc_artruis_the_heartless() : CreatureScript("npc_artruis_the_heartless") { }

    struct npc_artruis_the_heartlessAI: public ScriptedAI
    {
        npc_artruis_the_heartlessAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 shoot_Timer;
        uint32 nova_Timer;
        uint32 veins_Timer;
        bool binding;

        void Reset()
        {
            shoot_Timer = 2000;
            nova_Timer = urand(20000,40000);
            veins_Timer = urand(20000,40000);
            binding = true;
        }

        void JustDied(Unit* /*killer*/)
        {
            if(Creature* jaloot = me->FindNearestCreature(NPC_JALOOT, 30.0f, true))
            {
                jaloot->AI()->DoAction(0);
            }

            if(Creature* zeptik = me->FindNearestCreature(NPC_ZEPTIK, 30.0f, true))
            {
                zeptik->AI()->DoAction(0);
            }

            me->SummonGameObject(ENTRY_ARTRUIS_URN,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()+0.3,0,0,0,0,0,300);
            me->DespawnOrUnsummon();
        }

        void DoAction(int32 action)
        {
            switch(action)
            {
                case 0:
                    me->RemoveAllAuras();
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (me->HasAura(SPELL_BINDINGS_OF_SUBMISSION))
                return;

            if (binding)
            {
                if (me->GetHealthPct() <= 30)
                {
                    DoCast(SPELL_BINDINGS_OF_SUBMISSION);
                    if(Creature* jaloot = me->FindNearestCreature(NPC_JALOOT, 30.0f, true))
                    {
                        jaloot->AI()->DoAction(2);
                    }

                    if(Creature* zeptik = me->FindNearestCreature(NPC_ZEPTIK, 30.0f, true))
                    {
                        zeptik->AI()->DoAction(2);
                    }
                    binding = false;
                }
            }

            if(nova_Timer <= diff)
            {
                DoCastSelf(SPELL_ARTRUIS_FROST_NOVA, false);
                nova_Timer = urand(20000,40000);
            }else nova_Timer -= diff;

            if(veins_Timer <= diff)
            {
                me->CastSpell(me->GetVictim(), SPELL_ARTRUIS_ICY_VEINS, false);
                veins_Timer = urand(20000,40000);
            }else veins_Timer -= diff;

            if(shoot_Timer <= diff)
            {
                me->CastSpell(me->GetVictim(), RAND(SPELL_ARTRUIS_FROST_BOLT,SPELL_ARTRUIS_ICE_LANCE), false);
                shoot_Timer = 5000;
            }else shoot_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_artruis_the_heartlessAI(pCreature);
    }
};

enum SpellsJaloot
{
    SPELL_LIGHTNING_STRIKE      = 52944,
    SPELL_LIGHTNING_WHIRL       = 52943,
    SPELL_ENERGY_SIPHON         = 52969,
    SPELL_SPARK_FRENZY          = 52964,

    FACTION_ORCLES                      = 1105
};

class npc_jaloot : public CreatureScript
{
public:
    npc_jaloot() : CreatureScript("npc_jaloot") { }

    struct npc_jalootAI : public ScriptedAI
    {
        npc_jalootAI(Creature* creature) : ScriptedAI (creature) { }

        ObjectGuid targetGUID;
        bool newtarget;
        bool reset;
        uint32 reset_Timer;
        uint32 strike_Timer;
        uint32 whirl_Timer;
        uint32 energy_Timer;
        uint32 spark_Timer;

        void InitializeAI()
        {
            reset = false;
            me->AddAura(SPELL_TOMB_OF_THE_HEARTLESS, me);
        }

        void Reset()
        {
            if (!reset)
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetImmuneToPC(true);
                me->setFaction(21);
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                newtarget = true;
            }
            targetGUID.Clear();

            strike_Timer = urand(15000, 25000);
            whirl_Timer = urand(20000, 35000);
            energy_Timer = urand(20000, 40000);
            spark_Timer = urand(15000, 20000);
        }

        void JustRespawned()
        {
            reset = false;
            me->AddAura(SPELL_TOMB_OF_THE_HEARTLESS, me);
        }

        void JustReachedHome()
        {
            if (!reset)
                me->AddAura(SPELL_TOMB_OF_THE_HEARTLESS, me);
        }

        void JustDied(Unit* /*killer*/)
        {
            if(Creature* zeptik = me->FindNearestCreature(NPC_ZEPTIK, 30.0f, true))
            {
                zeptik->AI()->DoAction(1);
            }
            if(Creature* artruis = me->FindNearestCreature(NPC_ARTRUIS, 30.0f, true))
            {
                artruis->AI()->DoAction(0);
            }
            me->DespawnOrUnsummon();
        }

        void DoAction(int32 action)
        {
            switch(action)
            {
                case 0:
                    me->setFaction(35);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    reset = true;
                    reset_Timer = 180000;
                    me->CombatStop(true);
                    me->RemoveAllAuras();
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveTargetedHome();
                    break;
                case 1:
                    newtarget = false;
                    me->SetReactState(REACT_PASSIVE);
                    me->GetThreatManager().ClearAllThreat();
                    if (Unit* target = me->FindNearestCreature(NPC_ARTRUIS, 30.0f, true)) 
                    {
                        targetGUID = target->GetGUID();
                        me->AI()->AttackStart(target);
                    }
                    me->setFaction(16);
                    break;
                case 2:
                    me->RemoveAllAuras();
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1);
                    me->SetImmuneToPC(false);
                    if (Unit* target = me->FindNearestPlayer(100.0f))
                    {
                        targetGUID = target->GetGUID();
                        me->AI()->AttackStart(target);
                    }
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (reset)
            {
                if(reset_Timer <= diff)
                {
                    reset = false;
                    me->AI()->JustReachedHome();
                    me->AI()->Reset();
                }else reset_Timer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            Unit* target = NULL;
            if (newtarget)
            {
                if ((target = me->GetVictim()))
                    targetGUID = target->GetGUID();
                else
                    targetGUID.Clear();
            }
            else
                target = ObjectAccessor::GetUnit(*me, targetGUID);

            if (!target)
                return;

            if(strike_Timer <= diff)
            {
                me->CastSpell(target, SPELL_LIGHTNING_STRIKE, false);
                strike_Timer = urand(15000, 25000);
            }else strike_Timer -= diff;

            if(whirl_Timer <= diff)
            {
                DoCast(SPELL_LIGHTNING_WHIRL);
                whirl_Timer = urand(20000, 35000);
            }else whirl_Timer -= diff;

            if(energy_Timer <= diff)
            {
                me->CastSpell(target, SPELL_ENERGY_SIPHON, false);
                energy_Timer = urand(20000, 40000);
            }else energy_Timer -= diff;

            if(spark_Timer <= diff)
            {
                DoCastAOE(SPELL_SPARK_FRENZY, false);
                spark_Timer = urand(15000, 20000);
            }else spark_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    bool OnQuestReward(Player* player, Creature* /*creature*/, Quest const* quest, uint32 /*item*/)
    {
        if(quest->GetQuestId() == QUEST_HAND_OF_THE_ORACELS)
            player->GetReputationMgr().SetReputation(sFactionStore.LookupEntry(FACTION_ORCLES),9000);
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_jalootAI(creature);
    }
};

enum SpellsZeptik
{
    SPELL_BANDAGE           = 52895,
    SPELL_PIERCING_ARRIOW   = 52758,
    SPELL_BARBED_NET        = 52761,
    SPELL_OPEN_WOUND        = 52873,
    SPELL_SPIKE_TRAP        = 52886,
    SPELL_ENVENOMED_SHOT    = 52889,
    SPELL_ZEPTIKS_FERVOR    = 54177
};

class npc_zeptik : public CreatureScript
{
public:
    npc_zeptik() : CreatureScript("npc_zeptik") { }

    struct npc_zeptikAI : public ScriptedAI
    {
        npc_zeptikAI(Creature* creature) : ScriptedAI (creature) { }

        ObjectGuid targetGUID;
        bool newtarget;
        bool reset;
        uint32 reset_Timer;
        uint32 bandage_Timer;
        uint32 arrow_Timer;
        uint32 barbed_Timer;
        uint32 wound_Timer;
        uint32 trap_Timer;
        uint32 shot_Timer;
        uint32 frevor_Timer;

        void InitializeAI()
        {
            reset = false;
            me->AddAura(SPELL_TOMB_OF_THE_HEARTLESS, me);
        }

        void Reset()
        {
            if (!reset)
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetImmuneToPC(true);
                me->setFaction(21);
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                newtarget = true;
                targetGUID.Clear();
            }

            bandage_Timer = urand(20000, 40000);
            arrow_Timer = urand(10000, 15000);
            barbed_Timer = urand(20000, 40000);
            wound_Timer = urand(20000, 40000);
            trap_Timer = urand(20000, 40000);
            shot_Timer = urand(15000, 20000);
            frevor_Timer = urand(20000, 40000);
        }

        void JustRespawned()
        {
            reset = false;
            me->AddAura(SPELL_TOMB_OF_THE_HEARTLESS, me);
        }

        void JustReachedHome()
        {
            if (!reset)
                me->AddAura(SPELL_TOMB_OF_THE_HEARTLESS, me);
        }

        void JustDied(Unit* /*killer*/)
        {
            if(Creature* jaloot= me->FindNearestCreature(NPC_JALOOT, 30.0f, true))
            {
                jaloot->AI()->DoAction(1);
            }
            if(Creature* artruis = me->FindNearestCreature(NPC_ARTRUIS, 30.0f, true))
            {
                artruis->AI()->DoAction(0);
            }
            me->DespawnOrUnsummon();
        }

        void DoAction(int32 action)
        {
            switch(action)
            {
                case 0:
                    me->setFaction(35);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    reset = true;
                    reset_Timer = 180000;
                    me->CombatStop(true);
                    me->RemoveAllAuras();
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveTargetedHome();
                    break;
                case 1:
                    newtarget = false;
                    me->SetReactState(REACT_PASSIVE);
                    me->GetThreatManager().ClearAllThreat();
                    newtarget = false;
                    if (Unit* target = me->FindNearestCreature(NPC_ARTRUIS, 30.0f, true))
                    {
                        targetGUID = target->GetGUID();
                        me->AI()->AttackStart(target);
                    }
                    me->setFaction(16);
                    break;
                case 2:
                    me->RemoveAllAuras();
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1);
                    me->SetImmuneToPC(false);
                    if (Unit* target = me->FindNearestPlayer(100.0f))
                    {
                        targetGUID = target->GetGUID();
                        me->AI()->AttackStart(target);
                    }
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (reset)
            {
                if(reset_Timer <= diff)
                {
                    reset = false;
                    me->AI()->JustReachedHome();
                    me->AI()->Reset();
                }else reset_Timer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            Unit* target = NULL;

            if (newtarget)
                target = me->GetVictim();
            else 
                target = ObjectAccessor::GetUnit(*me, targetGUID);

            if (!target)
                return;

            if(bandage_Timer <= diff)
            {
                DoCastSelf(SPELL_BANDAGE, false);
                bandage_Timer = urand(20000, 40000);
            }else bandage_Timer -= diff;

            if(arrow_Timer <= diff)
            {
                DoCast(target, SPELL_PIERCING_ARRIOW, false);
                arrow_Timer = urand(10000, 15000);
            }else arrow_Timer -= diff;

            if(barbed_Timer <= diff)
            {
                DoCast(target, SPELL_BARBED_NET, false);
                barbed_Timer = urand(20000, 40000);
            }else barbed_Timer -= diff;

            if(wound_Timer <= diff)
            {
                DoCast(target, SPELL_OPEN_WOUND, false);
                wound_Timer = urand(20000, 40000);
            }else wound_Timer -= diff;

            if(trap_Timer <= diff)
            {
                DoCastSelf(SPELL_SPIKE_TRAP, false);
                trap_Timer = urand(20000, 40000);
            }else trap_Timer -= diff;

            if(shot_Timer <= diff)
            {
                DoCast(target, SPELL_ENVENOMED_SHOT, false);
                shot_Timer = urand(15000, 20000);
            }else shot_Timer -= diff;

            if(frevor_Timer <= diff)
            {
                DoCastSelf(SPELL_ZEPTIKS_FERVOR, false);
                frevor_Timer = urand(20000, 40000);
            }else frevor_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    bool OnQuestReward(Player* player, Creature* /*creature*/, Quest const* quest, uint32 /*item*/)
    {
        if(quest->GetQuestId() == QUEST_FRENZYHEART_CHAMPION)
            player->GetReputationMgr().SetReputation(sFactionStore.LookupEntry(FACTION_FRENZYHEART),9000);

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_zeptikAI(creature);
    }
};

enum PentarusMachineMisc
{
    SPELL_TELEPORT_SHOLAZAR = 50987
};

class npc_penatrus_machine : public CreatureScript
{
    public:
        npc_penatrus_machine() : CreatureScript("npc_penatrus_machine") { }

        struct npc_penatrus_machineAI : public SmartAI
        {
            npc_penatrus_machineAI(Creature* creature) : SmartAI(creature) { }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_TELEPORT_SHOLAZAR)
                {
                    me->SetCanFly(true);
                    me->SetSpeedRate(MOVE_FLIGHT, 3.0f);
                    me->SetSpeedRate(MOVE_RUN, 50.0f);
                    me->SetSpeedRate(MOVE_WALK, 5.0f);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_penatrus_machineAI(creature);
        }
};

enum BroodmotherSlivina
{
    EVENT_CLAW          = 1,
    EVENT_FLAME_BREATH  = 2,

    SPELL_CLAW          = 51772,
    SPEL_FLAME_BREATH   = 51768,

    NPC_STAMPY          = 28468,
};

class npc_broodmother_slivina : public CreatureScript
{
public:
    npc_broodmother_slivina() : CreatureScript("npc_broodmother_slivina") { }

    struct npc_broodmother_slivinaAI : public ScriptedAI
    {
        npc_broodmother_slivinaAI(Creature* creature) : ScriptedAI(creature) { }

        void EnterCombat(Unit* /*who*/) override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_CLAW, urand(3, 6) * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_FLAME_BREATH, urand(9, 12) * IN_MILLISECONDS);
        }

        void JustDied(Unit* killer) override
        {
            if (Player* target = me->FindNearestPlayer(15.0f))
                target->KilledMonsterCredit(me->GetEntry(), me->GetGUID());
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
                    case EVENT_CLAW:
                        DoCastVictim(SPELL_CLAW);
                        _events.ScheduleEvent(EVENT_CLAW, urand(5, 7) * IN_MILLISECONDS);
                        break;
                    case EVENT_FLAME_BREATH:
                        DoCastVictim(SPEL_FLAME_BREATH);
                        _events.ScheduleEvent(EVENT_FLAME_BREATH, urand(12, 17) * IN_MILLISECONDS);
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

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_broodmother_slivinaAI(creature);
    }
};

void AddSC_sholazar_basin_rg()
{
    new npc_captive_croco_gossip();
    new npc_captive_croco_vehicle();
    new npc_moodle();
    new vehicle_haiphoon();
    new npc_lafoo();
    new npc_shiny_thing_visual();
    new npc_artruis_the_heartless();
    new npc_jaloot();
    new npc_zeptik();
    new npc_penatrus_machine();
    new npc_broodmother_slivina();
}
