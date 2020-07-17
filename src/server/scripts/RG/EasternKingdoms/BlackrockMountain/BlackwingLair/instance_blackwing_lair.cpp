
/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2015 Rising Gods <http://www.rising-gods.de/>
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
#include "PassiveAI.h"
#include "blackwing_lair.h"
#include "Player.h"
#include "MoveSpline.h"
#include "SpellScript.h"

uint32 const QUEST_RUBY_PREEVENT = 110207;

class instance_blackwing_lair : public InstanceMapScript
{
    public:
        instance_blackwing_lair() : InstanceMapScript("instance_blackwing_lair", 469) { }
    
        struct instance_blackwing_lair_InstanceMapScript : public InstanceScript
        {
            instance_blackwing_lair_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(MAX_ENCOUNTER);

                // Razorgore
                RazorgoreTheUntamedGUID.Clear();
                GrethokTheControllerGUID.Clear();
                RazorgoreDoor1GUID.Clear();
                RazorgoreDoor2GUID.Clear();
                // Vaelastrasz the Corrupt
                VaelastraszTheCorruptGUID.Clear();
                VaelastraszDoorGUID.Clear();
                VaelastraszPreEvent = NOT_STARTED;
                // Broodlord Lashlayer
                BroodlordLashlayerGUID.Clear();
                BroodlordDoorGUID.Clear();
                // 3 Dragons
                FiremawGUID.Clear();
                EbonrocGUID.Clear();
                FlamegorGUID.Clear();
                ChrommagusDoor1GUID.Clear();
                // Chormaggus
                ChromaggusGUID.Clear();
                ChrommagusDoor2GUID.Clear();
                ChrommagusDoor3GUID.Clear();
                NefarianDoorGUID.Clear();
                // Nefarian
                LordVictorNefariusGUID.Clear();
                NefarianGUID.Clear();
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_RAZORGORE:
                        RazorgoreTheUntamedGUID = creature->GetGUID();
                        break;
                    case NPC_GRETHOK_THE_CONTROLLER:
                        GrethokTheControllerGUID = creature->GetGUID();
                        break;
                    case NPC_VAELASTRAZ:
                        VaelastraszTheCorruptGUID = creature->GetGUID();
                        break;
                    case NPC_BROODLORD:
                        BroodlordLashlayerGUID = creature->GetGUID();
                        break;
                    case NPC_FIRENAW:
                        FiremawGUID = creature->GetGUID();
                        break;
                    case NPC_EBONROC:
                        EbonrocGUID = creature->GetGUID();
                        break;
                    case NPC_FLAMEGOR:
                        FlamegorGUID = creature->GetGUID();
                        break;
                    case NPC_CHROMAGGUS:
                        ChromaggusGUID = creature->GetGUID();
                        break;
                    case NPC_VICTOR_NEFARIUS:
                        LordVictorNefariusGUID = creature->GetGUID();
                        break;
                    case NPC_NEFARIAN:
                        NefarianGUID = creature->GetGUID();
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_SUPPRESSION_DEVICE:
                        if (GetBossState(DATA_BROODLORD_LASHLAYER) != DONE)
                            instance->SummonCreature(NPC_SUPPRESSION_TARGET, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
                        break;
                    case GO_RAZORGORE_DOOR_1:
                        RazorgoreDoor1GUID = go->GetGUID();
                        HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_RAZORGORE_DOOR_2:
                        RazorgoreDoor2GUID = go->GetGUID();
                        HandleGameObject(ObjectGuid::Empty, GetBossState(BOSS_RAZORGORE) == DONE, go);
                        break;
                    case GO_VAELASTRAZ_DOOR:
                        VaelastraszDoorGUID = go->GetGUID();
                        HandleGameObject(ObjectGuid::Empty, GetBossState(BOSS_VAELASTRAZ) == DONE, go);
                        break;
                    case GO_BROODLORD_DOOR:
                        BroodlordDoorGUID = go->GetGUID();
                        HandleGameObject(ObjectGuid::Empty, GetBossState(BOSS_BROODLORD) == DONE, go);
                        break;
                    case GO_CHROMAGGUS_DOOR_1:
                        ChrommagusDoor1GUID = go->GetGUID();
                        HandleGameObject(ObjectGuid::Empty, GetBossState(BOSS_FIREMAW) == DONE && GetBossState(BOSS_EBONROC) == DONE && GetBossState(BOSS_FLAMEGOR) == DONE, go);
                        break;
                    case GO_CHROMAGGUS_DOOR_2:
                        ChrommagusDoor2GUID = go->GetGUID();
                        HandleGameObject(ObjectGuid::Empty, GetBossState(BOSS_CHROMAGGUS) == DONE, go);
                        break;
                    case GO_CHROMAGGUS_DOOR_3:
                        ChrommagusDoor3GUID = go->GetGUID();
                        HandleGameObject(ObjectGuid::Empty, GetBossState(BOSS_CHROMAGGUS) == DONE, go);
                        break;
                    case GO_NEFARIAN_DOOR:
                        NefarianDoorGUID = go->GetGUID();
                        HandleGameObject(ObjectGuid::Empty, GetBossState(BOSS_CHROMAGGUS) == DONE, go);
                        break;
                }
            }

            bool CheckRequiredBosses(uint32 bossId, Player const* /*player*/ = NULL) const
            {
                switch (bossId)
                {
                    case BOSS_RAZORGORE:
                        return true;
                    case BOSS_VAELASTRAZ:
                        return GetBossState(BOSS_RAZORGORE) == DONE;
                    case BOSS_BROODLORD:
                        return GetBossState(BOSS_VAELASTRAZ) == DONE;
                    case BOSS_FIREMAW:
                    case BOSS_EBONROC:
                    case BOSS_FLAMEGOR:
                        return GetBossState(BOSS_BROODLORD) == DONE;
                    case BOSS_CHROMAGGUS:
                        return GetBossState(BOSS_FIREMAW) == DONE && GetBossState(BOSS_EBONROC) == DONE && GetBossState(BOSS_FLAMEGOR) == DONE;
                    case BOSS_NEFARIAN:
                        return GetBossState(BOSS_CHROMAGGUS) == DONE;
                    default:
                        return false;
                }
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
                    case BOSS_RAZORGORE:
                        HandleGameObject(RazorgoreDoor1GUID, state != IN_PROGRESS);
                        HandleGameObject(RazorgoreDoor2GUID, state == DONE);
                        break;
                    case BOSS_VAELASTRAZ:
                        HandleGameObject(VaelastraszDoorGUID, state == DONE);
                        break;
                    case BOSS_BROODLORD:
                        HandleGameObject(BroodlordDoorGUID, state == DONE);
                        break;
                    case BOSS_FIREMAW:
                    case BOSS_EBONROC:
                    case BOSS_FLAMEGOR:
                        HandleGameObject(ChrommagusDoor1GUID, GetBossState(BOSS_FIREMAW) == DONE && GetBossState(BOSS_EBONROC) == DONE && GetBossState(BOSS_FLAMEGOR) == DONE);
                        break;
                    case BOSS_CHROMAGGUS:
                        HandleGameObject(ChrommagusDoor3GUID, state == DONE);
                        HandleGameObject(NefarianDoorGUID, state == DONE);
                        break;
                    case BOSS_NEFARIAN:
                        HandleGameObject(NefarianDoorGUID, state != IN_PROGRESS);
                        switch (state)
                        {
                            case NOT_STARTED:
                                if (Creature* nefarian = instance->GetCreature(NefarianGUID))
                                    nefarian->DespawnOrUnsummon();
                                break;
                            case FAIL:
                                _events.ScheduleEvent(EVENT_RESPAWN_NEFARIUS, 5 * MINUTE * IN_MILLISECONDS);
                                SetBossState(BOSS_NEFARIAN, NOT_STARTED);
                                break;
                            case DONE:
                            {
                                _events.CancelEvent(EVENT_RESPAWN_NEFARIUS);
                                break;
                            }
                            default:
                                break;
                        }
                        break;
                }
                return true;
            }

            ObjectGuid GetGuidData(uint32 data) const override
            {
                switch (data)
                {
                    case DATA_RAZORGORE_THE_UNTAMED:  return RazorgoreTheUntamedGUID;
                    case DATA_GRETHOK_THE_CONTROLLER: return GrethokTheControllerGUID;
                    case DATA_VAELASTRAZ_THE_CORRUPT: return VaelastraszTheCorruptGUID;
                    case DATA_BROODLORD_LASHLAYER:    return BroodlordLashlayerGUID;
                    case DATA_FIRENAW:                return FiremawGUID;
                    case DATA_EBONROC:                return EbonrocGUID;
                    case DATA_FLAMEGOR:               return FlamegorGUID;
                    case DATA_CHROMAGGUS:             return ChromaggusGUID;
                    case DATA_LORD_VICTOR_NEFARIUS:   return LordVictorNefariusGUID;
                    case DATA_NEFARIAN:               return NefarianGUID;
                }
    
                return ObjectGuid::Empty;
            }

            uint32 GetData(uint32 data) const override
            {
                if (data == DATA_VAELASTRAZ_PRE_EVENT)
                    return VaelastraszPreEvent;

                return 0;
            }
            
            void SetData(uint32 type, uint32 data) override
            {
                if (type == DATA_VAELASTRAZ_PRE_EVENT) {
                    VaelastraszPreEvent = data;
                    _events.ScheduleEvent(EVENT_VAELASTRAZ_PRE_EVENT_1, IN_MILLISECONDS);
                }
            }

            void Update(uint32 diff) override
            {
                if (_events.Empty())
                    return;
    
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_VAELASTRAZ_PRE_EVENT_1:
                            {
                                Creature* vaelastrasz = instance->GetCreature(VaelastraszTheCorruptGUID);
                                if (!vaelastrasz)
                                    return;

                                Position technicanEvadePos = { -7527.055664f, -962.210205f, 427.603088f };
                                Position nefariusSpawnPos = { -7466.157f, -1040.802f, 412.0533f };
                                std::list<Creature*> creatures;
                                vaelastrasz->GetCreatureListWithEntryInGrid(creatures, NPC_BLACKWING_TECHNICAN, 50.0f);
                                bool talk = false;
                                for (auto var : creatures) {
                                    if (var->isDead())
                                        continue;
                                    if (!talk) {
                                        var->AI()->Talk(0);
                                        talk = true;
                                    }
                                    var->SetReactState(REACT_PASSIVE);
                                    var->SetHomePosition(technicanEvadePos);
                                    var->AI()->EnterEvadeMode();
                                    var->DespawnOrUnsummon(var->movespline->Duration());
                                }
                                if (Creature* nefarius = instance->SummonCreature(NPC_VICTOR_NEFARIUS, nefariusSpawnPos))
                                {
                                    nefarius->DespawnOrUnsummon(28 * IN_MILLISECONDS);
                                    nefarius->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                                    vaelastrasz->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                                    nefarius->SetFacingToObject(vaelastrasz);
                                }
                            }
                            _events.ScheduleEvent(EVENT_VAELASTRAZ_PRE_EVENT_2, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_VAELASTRAZ_PRE_EVENT_2:
                            {
                            if (Creature* vaelastrasz = instance->GetCreature(VaelastraszTheCorruptGUID))
                                if (Creature* nefarius = vaelastrasz->FindNearestCreature(NPC_VICTOR_NEFARIUS, 50.0f))
                                {
                                    nefarius->CastSpell(vaelastrasz, SPELL_NEFARIUS_CORRUPTION);
                                    nefarius->AI()->Talk(SAY_NEFARIUS_PRE_EVENT_1);
                                }
                                _events.ScheduleEvent(EVENT_VAELASTRAZ_PRE_EVENT_3, 20 * IN_MILLISECONDS);
                            }
                            break;
                        case EVENT_VAELASTRAZ_PRE_EVENT_3:
                            {
                                if (Creature* vaelastrasz = instance->GetCreature(VaelastraszTheCorruptGUID))
                                    if (Creature* nefarius = vaelastrasz->FindNearestCreature(NPC_VICTOR_NEFARIUS, 50.0f)) {
                                        nefarius->AI()->Talk(SAY_NEFARIUS_PRE_EVENT_2);
                                        vaelastrasz->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                                        vaelastrasz->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                    }
                            }
                            break;
                        case EVENT_RESPAWN_NEFARIUS:
                            if (Creature* nefarius = instance->GetCreature(LordVictorNefariusGUID))
                            {
                                nefarius->setActive(true);
                                nefarius->Respawn();
                                nefarius->GetMotionMaster()->MoveTargetedHome();
                            }
                            break;
                    }
                }
            }

        protected:
            // Razorgore
            ObjectGuid RazorgoreTheUntamedGUID;
            ObjectGuid GrethokTheControllerGUID;
            ObjectGuid RazorgoreDoor1GUID;
            ObjectGuid RazorgoreDoor2GUID;

            // Vaelastrasz the Corrupt
            ObjectGuid VaelastraszTheCorruptGUID;
            ObjectGuid VaelastraszDoorGUID;
            uint32 VaelastraszPreEvent;

            // Broodlord Lashlayer
            ObjectGuid BroodlordLashlayerGUID;
            ObjectGuid BroodlordDoorGUID;

            // 3 Dragons
            ObjectGuid FiremawGUID;
            ObjectGuid EbonrocGUID;
            ObjectGuid FlamegorGUID;
            ObjectGuid ChrommagusDoor1GUID;
            ObjectGuid ChrommagusDoor2GUID;
            ObjectGuid ChrommagusDoor3GUID;

            // Chormaggus
            ObjectGuid ChromaggusGUID;
            ObjectGuid NefarianDoorGUID;

            // Nefarian
            ObjectGuid LordVictorNefariusGUID;
            ObjectGuid NefarianGUID;

            // Misc
            EventMap _events;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_blackwing_lair_InstanceMapScript(map);
        }
};

class go_suppression_device  : public GameObjectScript
{
    public:
        go_suppression_device() : GameObjectScript("go_suppression_device") { }

        bool OnGossipHello(Player* player, GameObject* /*go*/) override
        {
            if (Creature* suppression = player->FindNearestCreature(NPC_SUPPRESSION_TARGET, 5.0f))
                suppression->AI()->DoAction(player->getClass() == CLASS_ROGUE ? ACTION_CASTSTOP : ACTION_INSTANT_REPAIR);
            return false;
        }
};

class npc_suppression_caster : public CreatureScript
{
    public:
        npc_suppression_caster() : CreatureScript("npc_suppression_caster") { }
    
        struct npc_suppression_casterAI : public ScriptedAI
        {
            npc_suppression_casterAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
            }
    
            void Reset() override
            {
                SuppressionTimer = urand(1, 6) * IN_MILLISECONDS;
                CanCast = true;
                instance = me->GetInstanceScript();
            }
    
            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_CASTSTOP:
                        CanCast = false;
                        RepairTimer = 25 * IN_MILLISECONDS;
                        break;
                    case ACTION_INSTANT_REPAIR:
                        DoCastAOE(SPELL_SUPPRESSION);
                        CanCast = false;
                        RepairTimer = 2 * IN_MILLISECONDS;
                        break;
                }
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (CanCast)
                {
                    if (SuppressionTimer <= diff)
                    {
                        if (instance && instance->GetBossState(BOSS_BROODLORD) != DONE && instance->GetBossState(BOSS_VAELASTRAZ) == DONE)
                            DoCastAOE(SPELL_SUPPRESSION);
                        SuppressionTimer = 4 * IN_MILLISECONDS;
                    }
                    else SuppressionTimer -= diff;
                }
                else
                {
                    if (RepairTimer <= diff)
                    {
                        if (GameObject* device = me->FindNearestGameObject(GO_SUPPRESSION_DEVICE, 5.0f))
                        {
                            device->ResetDoorOrButton();
                            device->Refresh();
                            CanCast = true;
                        }
                    }
                    else RepairTimer -= diff;
                }
            }
    
        private:
            InstanceScript* instance;
            uint32 SuppressionTimer;
            uint32 RepairTimer;
            bool CanCast;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_suppression_casterAI(creature);
        }
};

class go_chromaggus_lever : public GameObjectScript
{
    public:
        go_chromaggus_lever() : GameObjectScript("go_chromaggus_lever") {}

        void OnGameObjectStateChanged(GameObject* go, uint32 state)
        {
            if (state != GO_STATE_ACTIVE)
                return;

            if (GameObject* chromaggusDoor = go->FindNearestGameObject(GO_CHROMAGGUS_DOOR_2, 100.0f))        
                chromaggusDoor->UseDoorOrButton();
        }
};

class spell_onyxia_scale_cloak : public SpellScriptLoader
{
public:
    spell_onyxia_scale_cloak() : SpellScriptLoader("spell_onyxia_scale_cloak") { }

    class spell_onyxia_scale_cloak_AuraScripts : public AuraScript
    {
        PrepareAuraScript(spell_onyxia_scale_cloak_AuraScripts);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return sSpellMgr->GetSpellInfo(22539);
        }

        bool Load() override
        {
            caster = GetCaster();
            if (!caster)
                return false;
            return caster->GetTypeId() == TYPEID_PLAYER;
        }

        void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            caster->ApplySpellImmune(0, IMMUNITY_ID, 22539, true);
        }

        void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            caster->ApplySpellImmune(0, IMMUNITY_ID, 22539, false);
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(spell_onyxia_scale_cloak_AuraScripts::HandleOnEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_onyxia_scale_cloak_AuraScripts::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }

    private:
        Unit* caster = nullptr;
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_onyxia_scale_cloak_AuraScripts();
    }
};

void AddSC_instance_blackwing_lair()
{
    new instance_blackwing_lair();
    new go_suppression_device();
    new npc_suppression_caster();
    new go_chromaggus_lever();
    new spell_onyxia_scale_cloak();
}
