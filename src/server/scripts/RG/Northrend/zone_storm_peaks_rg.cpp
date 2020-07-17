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
#include "SpellScript.h"
#include "CombatAI.h"
#include "Vehicle.h"
#include "SmartAI.h"

enum LandmineKnockbackAchievementAura
{
    ACHIEVEMENT_MINE_SWEEPER                  = 1428,
    SPELL_LANDMINE_KNOCKBACK_ACHIEVEMENT_AURA = 57099
};

class spell_landmine_knockback_achievement_aura : public SpellScriptLoader
{
    public:
        spell_landmine_knockback_achievement_aura() : SpellScriptLoader("spell_landmine_knockback_achievement_aura") { }

        class spell_landmine_knockback_achievement_aura_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_landmine_knockback_achievement_aura_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Player* player = GetHitPlayer())
                {
                    if (player->GetAuraCount(SPELL_LANDMINE_KNOCKBACK_ACHIEVEMENT_AURA) == 10)
                    {
                        player->CompletedAchievement(sAchievementStore.LookupEntry(ACHIEVEMENT_MINE_SWEEPER));
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_landmine_knockback_achievement_aura_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_landmine_knockback_achievement_aura_SpellScript();
        }
};

/*######
## Quest 12988: Destroy the Forges!
## spell_bouldercrags_bomb
######*/

enum DestroyTheForges
{
    NORTH_LIGHTNING_FORGE   = 30209,
    CENTRAL_LIGHTNING_FORGE = 30211,
    SOUTH_LIGHTNING_FORGE   = 30212
};

class spell_bouldercrags_bomb : public SpellScriptLoader
{
public:
    spell_bouldercrags_bomb() : SpellScriptLoader("spell_bouldercrags_bomb") { }

    class spell_bouldercrags_bomb_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_bouldercrags_bomb_SpellScript);

        void QuestCredit()
        {
            if (Unit* caster = GetCaster())
            {
                if (Player* player = caster->ToPlayer())
                {
                    if (player->FindNearestCreature(NORTH_LIGHTNING_FORGE, 10.0f))
                        player->KilledMonsterCredit(NORTH_LIGHTNING_FORGE);

                    if (player->FindNearestCreature(CENTRAL_LIGHTNING_FORGE, 10.0f))
                        player->KilledMonsterCredit(CENTRAL_LIGHTNING_FORGE);

                    if (player->FindNearestCreature(SOUTH_LIGHTNING_FORGE, 10.0f))
                        player->KilledMonsterCredit(SOUTH_LIGHTNING_FORGE);
                }
            }
        }

        SpellCastResult CheckCast()
        {
            if (Unit* caster = GetCaster())
                if (caster->FindNearestCreature(NORTH_LIGHTNING_FORGE, 10.0f) || caster->FindNearestCreature(CENTRAL_LIGHTNING_FORGE, 10.0f) || caster->FindNearestCreature(SOUTH_LIGHTNING_FORGE, 10.0f))
                    return SPELL_CAST_OK;

            return SPELL_FAILED_NO_VALID_TARGETS;
        }

        void Register()
        {
            OnCheckCast += SpellCheckCastFn(spell_bouldercrags_bomb_SpellScript::CheckCast);
            AfterCast += SpellCastFn(spell_bouldercrags_bomb_SpellScript::QuestCredit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_bouldercrags_bomb_SpellScript();
    }
};

/*######
## Quest 12998: The Heart of the Storm
## npc_overseer_narvir
######*/

enum TheHeartOfTheStorms
{
    SPELL_CYCLONE       = 65859,
    GO_HEART_OF_STORMS  = 192181,
    NPC_OVERSEER_NARVIR = 30299
};

Position POSITION_NEXT_TO_HEART_OF_STORM = {7312.04f, -727.81f, 791.61f, 3.0875f};

class npc_overseer_narvir : public CreatureScript
{
public:
    npc_overseer_narvir() : CreatureScript("npc_overseer_narvir") { }

    struct npc_overseer_narvirAI : public ScriptedAI
    {
        npc_overseer_narvirAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 CycloneTimer;
        uint32 Action;
        uint32 ActionTimer;

        void Reset()
        {
            CycloneTimer = 0;
            Action = 0;
            ActionTimer = 0;
        }

        void UpdateAI(uint32 diff)
        {
            if (ActionTimer <= diff)
            {
                std::list<Player*> players;
                me->GetPlayerListInGrid(players, 30.f);

                switch (Action)
                {
                    case 0: // at Spawn
                        DoCyclone();
                        me->GetMotionMaster()->MovePoint(1, POSITION_NEXT_TO_HEART_OF_STORM);
                        Action = 1;
                        break;
                    case 1: // Walk to the Heart of the Storms
                        break;
                    case 2: // Talk 0
                        DoCyclone();
                        Talk(0);
                        Action = 3;
                        ActionTimer = 5000;
                        break;
                    case 3: // Talk 1
                        DoCyclone();
                        Talk(1);
                        Action = 4;
                        ActionTimer = 5000;
                        break;
                    case 4: // Take Heart of the Storm
                        DoCyclone();
                        if (GameObject* heart = me->FindNearestGameObject(GO_HEART_OF_STORMS, 25.0f))
                        {
                            me->SetFacingToObject(heart);
                            me->HandleEmoteCommand(EMOTE_STATE_WORK);
                        }
                        Action = 5;
                        ActionTimer = 3000;
                        break;
                    case 5: // Start teleporting and give Kill Credit
                        for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
                        {
                            if (Player* player = *pitr)
                                player->KilledMonsterCredit(NPC_OVERSEER_NARVIR);
                        }
                        me->HandleEmoteCommand(EMOTE_ONESHOT_SPELL_CAST_W_SOUND);
                        Action = 6;
                        ActionTimer = 2000;
                        break;
                    case 6: // Despawn
                        me->DespawnOrUnsummon();
                        break;
                }
            }
            else
            {
                ActionTimer -= diff;
            }
        }

        void MovementInform(uint32 /*type*/, uint32 id)
        {
            if (id == 1 && Action == 1)
            {
                Action = 2;
            }
        }

        void DoCyclone()
        {
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 30.f);
            for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
            {
                if (Player* player = *pitr)
                    me->AddAura(SPELL_CYCLONE, player);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_overseer_narvirAI(creature);
    }
};


/*######
## Quest 12997/13424: Into the Pit / Back to the Pit
## npc_hyldsmeet_warbear
######*/

enum HyldsmeetWarbear
{
    NPC_HYLDSMEET_BEAR_RIDER = 30175,
    NPC_WARBEAR_MATRIARCH    = 29918,
    SPELL_DEMORALIZING_ROAR  = 15971,
    SPELL_SMASH              = 54458,
    SPELL_CHARGE             = 54460,
    AREA_PIT_OF_THE_FANG     = 4535
};

class npc_hyldsmeet_warbear : public CreatureScript
{
    public:
        npc_hyldsmeet_warbear() : CreatureScript("npc_hyldsmeet_warbear") { }

        struct npc_hyldsmeet_warbearAI : public ScriptedAI
        {
            npc_hyldsmeet_warbearAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 ChargeTimer;
            uint32 SmashTimer;
            uint32 RoarTimer;
            ObjectGuid RiderGUID;

            void Reset()
            {
                ChargeTimer = urand(2000, 5000);
                SmashTimer = urand(1000, 3000);
                RoarTimer = urand(3000, 10000);
                RiderGUID.Clear();
            }

            void JustDied(Unit* /*who*/)
            {
                if (Creature* rider = ObjectAccessor::GetCreature(*me, RiderGUID))
                {
                    rider->DespawnOrUnsummon();
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (me->GetVehicleKit()->HasEmptySeat(0))
                {
                    if (Creature* rider = me->SummonCreature(NPC_HYLDSMEET_BEAR_RIDER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()))
                    {
                        RiderGUID = rider->GetGUID();
                        rider->EnterVehicle(me, 0);
                    }
                }

                if (me->GetAreaId() != AREA_PIT_OF_THE_FANG)
                {
                    EnterEvadeMode();
                    return;
                }

                if (!UpdateVictim())
                    return;

                if (Unit* victim = me->GetVictim())
                {
                    if ((victim->GetTypeId() != TYPEID_UNIT) || (victim->GetEntry() != NPC_WARBEAR_MATRIARCH))
                    {
                        EnterEvadeMode();
                        return;
                    }

                    if (RoarTimer <= diff)
                    {
                        DoCast(SPELL_DEMORALIZING_ROAR);
                        RoarTimer = urand(15000, 30000);
                    }
                    else
                    {
                        RoarTimer -= diff;
                    }

                    if ((SmashTimer <= diff) && victim->IsWithinMeleeRange(me))
                    {
                        DoCast(victim, SPELL_SMASH);
                        SmashTimer = urand(7000, 10000);
                    }
                    else
                    {
                        SmashTimer -= diff;
                    }

                    if ((ChargeTimer <= diff) && victim->IsInRange(me, 8.0f, 25.0f))
                    {
                        DoCast(victim, SPELL_CHARGE);
                        ChargeTimer = urand(12000, 15000);
                    }
                    else
                    {
                        ChargeTimer -= diff;
                    }

                    DoMeleeAttackIfReady();
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_hyldsmeet_warbearAI(creature);
        }
};

/*######
## npc_vyragosa_tlpd
######*/

enum TlpdData
{
    SPELL_TLPD_TIME_LAPSE       = 51020,
    SPELL_TLPD_TIME_SHIFT       = 61084,
    SPELL_VYRA_FROST_BREATH     = 47425,
    SPELL_VYRA_FROST_CLEAVE     = 51857,

    NPC_VYRAGOSA                = 32630,
    NPC_TLPD                    = 32491,

    WORLDSTATE_TLPD             = 30001,

    // db guids
    GUID_TLPD_1                 = 202461,
    GUID_TLPD_2                 = 202462,
    GUID_TLPD_3                 = 202463,
    GUID_TLPD_4                 = 202464,

    GUID_VYRA_1                 = 202441,
    GUID_VYRA_2                 = 202442,
    GUID_VYRA_3                 = 202443,
    GUID_VYRA_4                 = 202444
};

class npc_vyragosa_tlpd : public CreatureScript
{
public:
    npc_vyragosa_tlpd() : CreatureScript("npc_vyragosa_tlpd") { }

    struct npc_vyragosa_tlpdAI : public npc_escortAI
    {
        npc_vyragosa_tlpdAI(Creature* creature) : npc_escortAI(creature)
        {
            creature->setActive(true);
            waypointId = 0;

            switch (me->GetGUID().GetCounter())
            {
                case GUID_TLPD_1:
                case GUID_VYRA_1:
                    waypointId = 0;
                    break;
                case GUID_TLPD_2:
                case GUID_VYRA_2:
                    waypointId = 97;
                    break;
                case GUID_TLPD_3:
                case GUID_VYRA_3:
                    waypointId = 157;
                    break;
                case GUID_TLPD_4:
                case GUID_VYRA_4:
                    waypointId = 245;
                    break;
            }
            Start(true, true, ObjectGuid::Empty, 0);
            SetNextWaypoint(waypointId);
        }

        uint32 saveTick, castTimer, waypointId;
        time_t respawn;

        void SetVisibleAndReactState(bool x)
        {
            me->SetVisible(x);
            me->SetReactState(x == true ? REACT_AGGRESSIVE : REACT_PASSIVE);
        }

        void Reset()
        {
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            me->setActive(true);
            respawn = sWorld->getWorldState(WORLDSTATE_TLPD);
            SetVisibleAndReactState(respawn > time(0) ? false : true);
            castTimer = 500;
            saveTick = 1 * MINUTE * IN_MILLISECONDS;

            SetNextWaypoint(waypointId);
        }

        void JustRespawned()
        {
            me->setActive(true);
            waypointId = 0;

            switch (me->GetGUID().GetCounter())
            {
                case GUID_TLPD_1:
                case GUID_VYRA_1:
                    waypointId = 0;
                    break;
                case GUID_TLPD_2:
                case GUID_VYRA_2:
                    waypointId = 97;
                    break;
                case GUID_TLPD_3:
                case GUID_VYRA_3:
                    waypointId = 157;
                    break;
                case GUID_TLPD_4:
                case GUID_VYRA_4:
                    waypointId = 245;
                    break;
            }
            Start(true, true, ObjectGuid::Empty, 0);
            SetNextWaypoint(waypointId);
        }

        void JustDied(Unit* /*victim*/)
        {
            time_t newTime = time(0) + urand(6, 48) * HOUR;
            sWorld->setWorldState(WORLDSTATE_TLPD, newTime);
        }

        void JustReachedHome() { } // overwrite or creature becomes inactive again

        void WaypointReached(uint32 pointId)
        {
            waypointId = pointId;
            switch (pointId)
            {
                case 96:
                    SetNextWaypoint(11, true, false); // path 1 startpoint is 0
                    break;

                case 156:
                    SetNextWaypoint(98, true, false); // path 2 startpoint is 97
                    break;

                case 244:
                    SetNextWaypoint(167, true, false); // path 3 startpoint is 157
                    break;

                case 376:
                    SetNextWaypoint(247, true, false); // path 4 startpoint is 245
                    break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            // make visible if "respawntime" is over
            if (saveTick <= diff)
            {
                if (respawn > time(0))
                {
                    if (me->IsVisible())
                        SetVisibleAndReactState(false);
                }
                else
                {
                    if (!me->IsVisible())
                    {
                        sWorld->setWorldState(WORLDSTATE_TLPD, time_t(0));
                        SetVisibleAndReactState(true);
                    }
                }
                saveTick = 1 * MINUTE * IN_MILLISECONDS;
            }
            else
                saveTick -= diff;

            if (!UpdateVictim())
                return;

            // has target, cast stuff!
            if (castTimer <= diff)
            {
                switch (me->GetEntry())
                {
                    case NPC_VYRAGOSA:
                        DoCastVictim(me->IsWithinMeleeRange(me->GetVictim()) ? SPELL_VYRA_FROST_CLEAVE : SPELL_VYRA_FROST_BREATH);
                        break;
                    case NPC_TLPD:
                        DoCastVictim(urand(0, 1) ? SPELL_TLPD_TIME_SHIFT : SPELL_TLPD_TIME_LAPSE);
                        break;
                    default:
                        return;
                }
                castTimer = 3000 + urand(0, 1500);
            }
            else
                castTimer -= diff;

            if (me->GetVictim() && me->IsVisible())
                DoMeleeAttackIfReady();
        }

    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_vyragosa_tlpdAI(creature);
    }
};

/*######
## Quest 12915: Mending Fences
## spell_hurl_boulder
######*/

enum MendingFences
{
    NPC_EARTHEN_IRONBANE = 29927
};

class spell_hurl_boulder : public SpellScriptLoader
{
    public:
        spell_hurl_boulder() : SpellScriptLoader("spell_hurl_boulder") { }

        class spell_hurl_boulder_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hurl_boulder_SpellScript);
               
            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* unit = GetHitUnit())
                {
                    if (Unit* caster = GetCaster())
                    {
                        for (uint8 i = 0; i < 5; i++)
                        {
                            if (Creature* dwarf = caster->SummonCreature(NPC_EARTHEN_IRONBANE, unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 60000))
                            {
                                unit->EngageWithTarget(dwarf);
                            }
                        }
                    }
                }
            }
            
            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_hurl_boulder_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hurl_boulder_SpellScript();
        }
};

/*######
## Quest 12985: Forging a Head
## spell_salvage_corpse
######*/

enum ForgingAHead
{
    NPC_DEAD_IRON_GIANTS     = 29914,
    NPC_STORMFORGED_AMBUSHER = 30208,
    ITEM_STORMFORGED_EYE     = 42423    
};

class spell_salvage_corpse : public SpellScriptLoader
{
    public:
        spell_salvage_corpse() : SpellScriptLoader("spell_salvage_corpse") { }

        class spell_salvage_corpse_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_salvage_corpse_SpellScript);
               
            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Creature* giant = caster->FindNearestCreature(NPC_DEAD_IRON_GIANTS, 10.0f))
                    {
                        switch (urand(0, 1))
                        {
                            case 0:
                                if (Player* player = caster->ToPlayer())
                                {
                                    player->AddItem(ITEM_STORMFORGED_EYE, urand(1, 2));
                                }
                                break;
                            case 1:
                                for (uint8 i = 0; i < urand(3, 4); i++)
                                {
                                    if (Creature* ambusher = caster->SummonCreature(NPC_STORMFORGED_AMBUSHER, giant->GetPositionX(), giant->GetPositionY(), giant->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 300000))
                                    {
                                        ambusher->AI()->AttackStart(caster);
                                    }
                                }
                                break;
                        }
                        giant->DespawnOrUnsummon();
                    }
                }
            }

            SpellCastResult CheckCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->FindNearestCreature(NPC_DEAD_IRON_GIANTS, 10.0f))
                    {
                        return SPELL_CAST_OK;
                    }
                }
                return SPELL_FAILED_BAD_TARGETS;
            }
            
            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_salvage_corpse_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast += SpellCheckCastFn(spell_salvage_corpse_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_salvage_corpse_SpellScript();
        }
};

/*######
## Quest 13010: Krolmir, Hammer of Storms
## npc_king_jokkum_vehicle
######*/

enum KrolmirQuest
{
    NPC_KING_JOKKUM_VEHICLE = 30331,
    NPC_THORIM              = 30390,

    SAY_JOKKUM_WP_START     = 0,
    SAY_JOKKUM_CALL_THORIM  = 1,
    SAY_JOKKUM_DIALOG_1     = 2,
    SAY_JOKKUM_DIALOG_2     = 3,
    SAY_JOKKUM_DIALOG_3     = 4,
    SAY_JOKKUM_DIALOG_4     = 5,
    SAY_JOKKUM_DIALOG_5     = 6,
    SAY_JOKKUM_DIALOG_6     = 7,
    SAY_JOKKUM_DIALOG_7     = 8,
    SAY_THORIM_DIALOG_1     = 0,
    SAY_THORIM_DIALOG_2     = 1,
    SAY_THORIM_DIALOG_3     = 2,
    SAY_THORIM_DIALOG_4     = 3,

    FINAL_WAYPOINT               = 23,

    SPELL_SUMMON_VERANUS         = 56649,
    SPELL_JOKKUM_KILL_CREDIT     = 56545,
    SPELL_REVEAL_KROLMIR         = 56660
};

const Position JOKKUM_RUN_AWAY_POSITION = {7736.73f, -3226.5f, 861.458f, 2.735f};

class npc_king_jokkum_vehicle : public CreatureScript
{
    public:
        npc_king_jokkum_vehicle() : CreatureScript("npc_king_jokkum_vehicle") { }

        struct npc_king_jokkum_vehicleAI : public npc_escortAI
        {
            npc_king_jokkum_vehicleAI(Creature* creature) : npc_escortAI(creature) { }
          
            void Reset()
            {
                Action       = 0;
                ActionTimer  = 0;
                PlayerGUID.Clear();
                ThorimGUID.Clear();

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            }

            void UpdateAI(uint32 diff)
            {
                npc_escortAI::UpdateAI(diff);

                if (ActionTimer <= diff)
                {
                    switch (Action)
                    {
                        case 0: // Player enters Vehicle
                            break;
                        case 1: // Waypoint Movement
                            break;
                        case 2: // Call for Thorim
                            Talk(SAY_JOKKUM_CALL_THORIM);
                            // Despawn existing Thorim
                            if (Creature* thorim = me->FindNearestCreature(NPC_THORIM, 200.0f))
                                thorim->DespawnOrUnsummon();
                            NextAction(1.5 * IN_MILLISECONDS);
                            break;
                        case 3: // Summon Thorim
                            DoCast(SPELL_SUMMON_VERANUS);
                            // Wait until Thorim dismounts from Veranus
                            NextAction();
                            break;
                        case 4: // Thorim kneels down and says dialog text 1 - - He Sets Data 1 1 on me
                            break;
                        case 5: // Jokkum dialog text 1
                            if (Player* player = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                                Talk(SAY_JOKKUM_DIALOG_1, player);
                            NextAction();
                            break;
                        case 6: // Thorim dialog text 2
                            if (Creature* thorim = ObjectAccessor::GetCreature(*me, ThorimGUID))
                                thorim->AI()->Talk(SAY_THORIM_DIALOG_2);                            
                            NextAction();
                            break;
                        case 7: // Thorim dialog text 3
                            if (Creature* thorim = ObjectAccessor::GetCreature(*me, ThorimGUID))
                                thorim->AI()->Talk(SAY_THORIM_DIALOG_3);                            
                            NextAction();
                            break;
                        case 8: // Jokkum dialog text 2
                            Talk(SAY_JOKKUM_DIALOG_2);
                            NextAction();
                            break;
                        case 9: // Jokkum dialog text 3 and Thorim stands up
                            Talk(SAY_JOKKUM_DIALOG_3);
                            if (Creature* thorim = ObjectAccessor::GetCreature(*me, ThorimGUID))
                                thorim->RemoveAllAuras();
                            NextAction();
                            break;
                        case 10: // Jokkum dialog text 4
                            Talk(SAY_JOKKUM_DIALOG_4);
                            NextAction();
                            break;
                        case 11: // Jokkum dialog text 5 + summon Krolmir
                            Talk(SAY_JOKKUM_DIALOG_5);
                            if (Player* player = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                                DoCast(player, SPELL_REVEAL_KROLMIR, true);
                            NextAction();
                            break;
                        case 12: // Thorim dialog text 4
                            if (Creature* thorim = ObjectAccessor::GetCreature(*me, ThorimGUID))
                            {
                                thorim->AI()->Talk(SAY_THORIM_DIALOG_4);
                            }
                            NextAction();
                            break;
                        case 13: // Jokkum dialog text 6
                            Talk(SAY_JOKKUM_DIALOG_6);
                            NextAction();
                            break;
                        case 14: // Jokkum dialog text 7
                            Talk(SAY_JOKKUM_DIALOG_7);
                            NextAction(3 * IN_MILLISECONDS);
                            break;
                        case 15: // Jokkum run away + reset Thorims flags
                            if (Creature* thorim = ObjectAccessor::GetCreature(*me, ThorimGUID))
                                thorim->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                            me->GetMotionMaster()->MovePoint(1, JOKKUM_RUN_AWAY_POSITION);
                            NextAction();
                            break;
                        case 16:
                            me->DisappearAndDie();
                            break;
                    }
                }
                else
                {
                    ActionTimer -= diff;
                }
            }

            void NextAction(uint32 time = 5 * IN_MILLISECONDS)
            {
                Action++;
                ActionTimer = time;
            }

            void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
            {
                if (who->GetTypeId() == TYPEID_PLAYER)
                {
                    if (apply) 
                    {
                        Start(false, true, who->GetGUID());
                        DoCast(who, SPELL_JOKKUM_KILL_CREDIT, true);
                        me->SetImmuneToAll(true, false);
                    }      
                }
            }

            void WaypointReached(uint32 waypointId)
            {
                if (waypointId == FINAL_WAYPOINT)
                {
                    if (Player* player = me->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        if (Creature* jokkum = player->SummonCreature(NPC_KING_JOKKUM_VEHICLE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 300000))
                        {                           
                            jokkum->SetOwnerGUID(player->GetGUID());
                            jokkum->GetAI()->SetData(2, 1);
                        }
                    }
                }
            }

            void SetData(uint32 type, uint32 data)
            {
                if (type == 1 && data == 1)
                {
                    if (Creature* thorim = me->FindNearestCreature(NPC_THORIM, 100.0f, true))
                        ThorimGUID = thorim->GetGUID();
                    Action = 5;
                }

                else if (type == 2 && data == 1) 
                {
                    Action = 2;
                }

            }

            void AttackStart(Unit* /*who*/) {}
            void EnterCombat(Unit* /*who*/) {}
            void EnterEvadeMode(EvadeReason /*why*/) {}
            void JustDied(Unit* /*killer*/) {}
            void OnCharmed(bool /*apply*/) {}

            private:
                ObjectGuid PlayerGUID;
                ObjectGuid ThorimGUID;
                uint32 Action;
                uint32 ActionTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_king_jokkum_vehicleAI(creature);
        }
};

/*######
## Quest 12977: Blowing Hodir's Horn
## spell_blow_hodirs_horn
######*/

enum BlowingHodirsHorn
{
    NPC_RESTLESS_FROSTBORN_WARRIOR = 30135,
    NPC_RESTLESS_FROSTBORN_GHOST   = 30144,
    NPC_NIFFELEM_FOREFATHER        = 29974,
    NPC_FROST_GIANT_GHOST_KC       = 30138,
    NPC_FROST_DWARF_GHOST_KC       = 30139
};

class spell_blow_hodirs_horn : public SpellScriptLoader
{
    public:
        spell_blow_hodirs_horn() : SpellScriptLoader("spell_blow_hodirs_horn") { }

        class spell_blow_hodirs_horn_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blow_hodirs_horn_SpellScript);
               
            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* player = caster->ToPlayer())
                    {
                        if (Unit* target = GetHitUnit())
                        {
                            if (Creature* creature = target->ToCreature())
                            {
                                switch (creature->GetEntry())
                                {
                                    case NPC_RESTLESS_FROSTBORN_WARRIOR:
                                    case NPC_RESTLESS_FROSTBORN_GHOST:
                                        player->KilledMonsterCredit(NPC_FROST_DWARF_GHOST_KC);
                                        break;
                                    case NPC_NIFFELEM_FOREFATHER:
                                        player->KilledMonsterCredit(NPC_FROST_GIANT_GHOST_KC);
                                        break;
                                }
                                creature->DespawnOrUnsummon();
                            }
                        }
                    }
                }
            }
            
            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_blow_hodirs_horn_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_blow_hodirs_horn_SpellScript();
        }
};

/*######
## Quest 12994: Spy Hunter
## npc_ethereal_frostworg
######*/

enum QuestSpyHunter
{
    NPC_STORMFORGED_INFILTRATOR   = 30222,
    NPC_CORPSE_OF_THE_FALLEN_WORG = 32569
};

class npc_ethereal_frostworg : public CreatureScript
{
    public:
        npc_ethereal_frostworg() : CreatureScript("npc_ethereal_frostworg") { }

        struct npc_ethereal_frostworgAI : public ScriptedAI
        {
            npc_ethereal_frostworgAI(Creature* creature) : ScriptedAI(creature) {}

            bool RandomWalk;
            uint32 WalkTimer;
            uint32 WPTimer;
            ObjectGuid InfiltratorGUID;

            void InitializeAI()
            {
                RandomWalk = true;
                WalkTimer = 30000;
                WPTimer = 5000;
                InfiltratorGUID.Clear();
            }

            void Reset()
            {
                if (Creature* fallenworg = me->FindNearestCreature(NPC_CORPSE_OF_THE_FALLEN_WORG, 100.0f))
                {
                    Position nearpos;
                    fallenworg->GetNearPosition(nearpos, urand(20, 50), frand(0.0f, 6.28f));
                    me->GetMotionMaster()->MovePoint(1, nearpos);
                }   
            }

            void UpdateAI(uint32 diff)
            {
                if (RandomWalk)
                {
                    if (WPTimer <= diff)
                    {
                        if (Creature* fallenworg = me->FindNearestCreature(NPC_CORPSE_OF_THE_FALLEN_WORG, 100.0f))
                        {
                            Position nearpos;
                            fallenworg->GetNearPosition(nearpos, urand(20, 50), frand(0.0f, 6.28f));
                            me->GetMotionMaster()->MovePoint(1, nearpos);
                        }  
                        WPTimer = 5000;
                    }
                    else
                    {
                        WPTimer -= diff;
                    }

                    if (WalkTimer <= diff)
                    {
                        RandomWalk = false;
                        me->GetMotionMaster()->MovementExpired();
                        me->GetMotionMaster()->MoveIdle();
                        if (Creature* infiltrator = me->SummonCreature(NPC_STORMFORGED_INFILTRATOR, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN, 60000))
                        {
                            InfiltratorGUID = infiltrator->GetGUID();
                        }
                    }
                    else
                    {
                        WalkTimer -= diff;
                    }
                }

                if (InfiltratorGUID)
                {
                    if (Creature* infiltrator = ObjectAccessor::GetCreature(*me, InfiltratorGUID))
                    {
                        if (infiltrator->isDead())
                        {
                            me->DespawnOrUnsummon();
                        }
                    }
                }

                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ethereal_frostworgAI(creature);
        }
};

/*######
## Quest 13003: Thrusting Hodir's Spear
## npc_wild_wyrm
## spell_dodge_claws
## spell_wild_wyrm_grip
## spell_jaws_of_death
######*/

enum ThrustingHodirsSpear
{
    AREA_DUN_NIFFELEM = 4438,
    SPELL_SPEAR_OF_HODIR = 56671,
    NPC_WILD_WYRM = 30275,
    NPC_WILD_WYRM_KC_BUNNY = 30415,
    SPELL_GRIP = 56689,
    SPELL_DODGE_CLAWS = 56704,
    SPELL_JAWS_OF_DEATH = 56692,
    SPELL_THRUST_SPEAR = 56690,
    SPELL_MIGHTY_SPEAR_THRUST = 60586,
    SPELL_FATAL_STRIKE = 60587,
    SPELL_PRY_JAWS_OPEN = 56706,
    SPELL_PARACHUTE = 44795,
    EVENT_CLAW_WARNING = 1,
    EVENT_CLAW = 2,
    EVENT_CHECK_SEATS = 3,
    EVENT_MOVE_RANDOM = 4,
    EVENT_AREA_CHECK = 5,
    SAY_WARNING = 0,
    SAY_DODGED = 1,
    SAY_MISSED = 2
};

Position PositionOverDunNiffelem = {7317.16f, -2769.71f, 1310.28f, 3.34f};

class npc_wild_wyrm : public CreatureScript
{
    public:
        npc_wild_wyrm() : CreatureScript("npc_wild_wyrm") { }

        struct npc_wild_wyrmAI : public VehicleAI
        {
            npc_wild_wyrmAI(Creature* creature) : VehicleAI(creature) {}

            EventMap events;
            uint32 Phase;

            void Reset()
            {
                Phase = 0;
                events.Reset();
                me->SetHealth(me->GetMaxHealth());
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);
                if (Vehicle* vehicle = me->GetVehicleKit())
                {
                    switch (Phase)
                    {
                        case 1:
                            // Check if player has lost grip
                            if (!me->HasAura(SPELL_GRIP))
                            {
                                if (Unit* passenger = vehicle->GetPassenger(0))
                                {
                                    passenger->ExitVehicle();
                                }
                            }

                            // remove Jaws-Open aura, when it is on the wyrm in phase 1 for some reason
                            me->RemoveAura(SPELL_PRY_JAWS_OPEN);

                            switch (events.ExecuteEvent())
                            {
                                case EVENT_CLAW_WARNING:
                                    if (Unit* passenger = vehicle->GetPassenger(0))
                                    {
                                        Talk(SAY_WARNING, passenger);
                                    }
                                    events.ScheduleEvent(EVENT_CLAW, 1500, 1);
                                    break;
                                case EVENT_CLAW:
                                    if (Unit* passenger = vehicle->GetPassenger(0))
                                    {
                                        if (passenger->HasAura(SPELL_DODGE_CLAWS))
                                        {
                                            Talk(SAY_DODGED, passenger);
                                        }
                                        else
                                        {
                                            me->DealDamage(passenger, passenger->GetMaxHealth()/10);
                                        }
                                    }
                                    events.ScheduleEvent(EVENT_CLAW_WARNING, urand(5000, 10000), 1);
                                    break;
                                case EVENT_CHECK_SEATS:
                                    if (vehicle->HasEmptySeat(0))
                                    {
                                        Reset();
                                        EnterEvadeMode();
                                        return;
                                    }
                                    events.ScheduleEvent(EVENT_CHECK_SEATS, 1000, 1);
                                    break;
                                case EVENT_MOVE_RANDOM:
                                    MoveRandom();
                                    break;
                                case EVENT_AREA_CHECK:
                                    if (me->GetAreaId() == AREA_DUN_NIFFELEM)
                                    {
                                        for (uint8 i = 0; i < 5; i++)
                                        {
                                            me->RemoveAuraFromStack(SPELL_GRIP);
                                        }
                                    }
                                    events.ScheduleEvent(EVENT_AREA_CHECK, 1000, 1);
                                    break;
                            }

                            if (me->GetHealthPct() <= 20.0f)
                            {
                                // Entering phase 2
                                Phase = 2;
                                events.Reset();
                                if (Unit* passenger = vehicle->GetPassenger(0))
                                {
                                    passenger->ChangeSeat(1);
                                    passenger->AddAura(SPELL_JAWS_OF_DEATH, passenger);
                                    events.ScheduleEvent(EVENT_CHECK_SEATS, 1000, 2);
                                    events.ScheduleEvent(EVENT_MOVE_RANDOM, 10000, 2);
                                }
                            }
                            break;
                        case 2:
                            switch (events.ExecuteEvent())
                            {
                                case EVENT_CHECK_SEATS:
                                    if (vehicle->HasEmptySeat(1))
                                    {
                                        Reset();
                                        EnterEvadeMode();
                                        return;
                                    }
                                    events.ScheduleEvent(EVENT_CHECK_SEATS, 1000, 2);
                                    break;
                                case EVENT_MOVE_RANDOM:
                                    MoveRandom();
                                    break;
                            }
                            break;
                    }
                }
            }

            void MoveRandom()
            {
                me->GetMotionMaster()->MovePoint(0, PositionOverDunNiffelem.GetPositionX() + frand(-200.0f, 200.0f), PositionOverDunNiffelem.GetPositionY() + frand(-200.0f, 200.0f), PositionOverDunNiffelem.GetPositionZ());
                events.ScheduleEvent(EVENT_MOVE_RANDOM, 10000);
            }

            void DamageTaken(Unit* /*who*/, uint32 &damage)
            {
                // Prevent all damage in phase 2, the wyrm can only be killed with fatal strike
                if (Phase == 2)
                {
                    damage = 0;
                }
            }

            void SpellHit(Unit* caster, const SpellInfo* spell)
            {
                if (Phase == 0 && spell->Id == SPELL_SPEAR_OF_HODIR)
                {
                    if (Vehicle* vehicle = me->GetVehicleKit())
                    {
                        Phase = 1;
                        caster->EnterVehicle(me, 0);
                        for (uint8 i = 0; i < 60; i++)
                        {
                            me->AddAura(SPELL_GRIP, me);
                        }
                        me->GetMotionMaster()->MovePoint(0, PositionOverDunNiffelem);
                        events.ScheduleEvent(EVENT_CLAW_WARNING, urand(5000, 10000), 1);
                        events.ScheduleEvent(EVENT_CHECK_SEATS, 1000, 1);
                        events.ScheduleEvent(EVENT_MOVE_RANDOM, 10000);
                        events.ScheduleEvent(EVENT_AREA_CHECK, 1000, 1);
                    }
                }
                else if (Phase == 1)
                {
                    if (spell->Id == SPELL_THRUST_SPEAR)
                    {
                        for (uint8 i = 0; i < 5; i++)
                        {
                            me->RemoveAuraFromStack(SPELL_GRIP);
                        }
                    }
                    else if (spell->Id == SPELL_MIGHTY_SPEAR_THRUST)
                    {
                        for (uint8 i = 0; i < 15; i++)
                        {
                            me->RemoveAuraFromStack(SPELL_GRIP);
                        }
                    }
                }
                else if (Phase == 2)
                {
                    if (spell->Id == SPELL_FATAL_STRIKE)
                    {
                        if (Vehicle* vehicle = me->GetVehicleKit())
                        {
                            if (Unit* passenger = vehicle->GetPassenger(1))
                            {
                                if (urand(1, 20) <= me->GetAuraCount(SPELL_PRY_JAWS_OPEN))
                                {
                                    if (Player* player = passenger->ToPlayer())
                                    {
                                        player->RemoveAura(SPELL_JAWS_OF_DEATH);
                                        player->KilledMonsterCredit(NPC_WILD_WYRM_KC_BUNNY);
                                        player->CastSpell(player, SPELL_PARACHUTE);
                                        me->KillSelf();
                                    }
                                }
                                else
                                {
                                    if (Unit* passenger = vehicle->GetPassenger(1))
                                    {
                                        Talk(SAY_MISSED, passenger);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_wild_wyrmAI(creature);
        }
};

class spell_dodge_claws : public SpellScriptLoader
{
public:
    spell_dodge_claws() : SpellScriptLoader("spell_dodge_claws") { }

    class spell_dodge_claws_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dodge_claws_SpellScript);

        void ApplyAura()
        {
            if (Unit* caster = GetCaster())
            {
                caster->AddAura(SPELL_DODGE_CLAWS, caster);
            }
        }

        void Register()
        {
            AfterCast += SpellCastFn(spell_dodge_claws_SpellScript::ApplyAura);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_dodge_claws_SpellScript();
    }
};

class spell_wild_wyrm_grip : public SpellScriptLoader
{
public:
    spell_wild_wyrm_grip() : SpellScriptLoader("spell_wild_wyrm_grip") { }

    class spell_wild_wyrm_grip_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_wild_wyrm_grip_SpellScript);

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                for (uint8 i = 0; i < 10; i++)
                {
                    caster->AddAura(SPELL_GRIP, caster);
                }
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_wild_wyrm_grip_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_wild_wyrm_grip_SpellScript();
    }
};

class spell_jaws_of_death : public SpellScriptLoader
{
    public:
        spell_jaws_of_death() : SpellScriptLoader("spell_jaws_of_death") { }

        class spell_jaws_of_deathAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_jaws_of_deathAuraScript);
            
            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (Unit* target = GetTarget())
                {
                    target->DealDamage(target, target->GetMaxHealth()*0.03);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_jaws_of_deathAuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_jaws_of_deathAuraScript();
        }
};

enum max
{
    GOSSIP_MENU_ITEM                = 10144,
    SPELL_LOANER_MOUNT_HORDE        = 60128,
    SPELL_LOANER_MOUNT_ALLIANCE     = 60126,
    SPELL_COLD_EATHER_FLYING        = 54197
};

class npc_honest_max : public CreatureScript
{
public:
    npc_honest_max() : CreatureScript("npc_honest_max") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (!player->HasSpell(SPELL_COLD_EATHER_FLYING) && player->getLevel() >= 77) //player have not cold weather flying and is lvl 77+
            player->AddGossipItem(GOSSIP_MENU_ITEM);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->SendCloseGossip();
        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            if (player->GetTeamId() == TEAM_ALLIANCE)
                player->CastSpell(player, SPELL_LOANER_MOUNT_ALLIANCE, false);
            else if (player->GetTeamId() == TEAM_HORDE)
                player->CastSpell(player, SPELL_LOANER_MOUNT_HORDE, false);
        }
        return true;
    }
};


enum SparksocketAACannon
{
    SPELL_HOSTILE_AIRSPACE = 54433
};

class npc_sparksocket_aa_cannon : public CreatureScript
{
public:
    npc_sparksocket_aa_cannon() : CreatureScript("npc_sparksocket_aa_cannon") { }

    struct npc_sparksocket_aa_cannonAI : public ScriptedAI
    {
        npc_sparksocket_aa_cannonAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI()
        {
            ValidTargetTimer = 2 * IN_MILLISECONDS;
        }

        void CheckPlayerList()
        {
            if (Player* player = me->FindNearestPlayer(25.0f))
                if (player->IsFlying() && !player->HasAura(SPELL_HOSTILE_AIRSPACE))
                    me->CastSpell(player, SPELL_HOSTILE_AIRSPACE);            
        }

        void UpdateAI(uint32 diff)
        {
            if (ValidTargetTimer <= diff)
            {
                CheckPlayerList();
                ValidTargetTimer = 2 * IN_MILLISECONDS;
            }
            else
                ValidTargetTimer -= diff;
        }
    private:
        uint32 ValidTargetTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_sparksocket_aa_cannonAI(creature);
    }
};

enum BrannMachineMisc
{
    NPC_BRANN = 30405
};

class npc_brann_flying_machine : public CreatureScript
{
public:
    npc_brann_flying_machine() : CreatureScript("npc_brann_flying_machine") { }

    struct npc_brann_flying_machineAI : public SmartAI
    {
        npc_brann_flying_machineAI(Creature* creature) : SmartAI(creature) { }

        void SetData(uint32 type, uint32 data)
        {
            if (type == 1 && data == 1)
            {
                me->GetVehicleKit()->InstallAccessory(NPC_BRANN, 0, true, TEMPSUMMON_DEAD_DESPAWN, 0);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_brann_flying_machineAI(creature);
    }
};

enum FrostbiteMisc
{
    SAY_TRACK_0               = 0,
    SAY_TRACK_1               = 1,
    SAY_TRACK_2               = 2,
    SAY_TRACK_3               = 3,
    SPELL_PERIODIC_FROSTHOUND = 54993,
    NPC_Q_CREDIT              = 29677,
    WAYPOINT_FROSTHOUND       = 2985700
};

class npc_frostbite_horde : public CreatureScript
{
    public:
        npc_frostbite_horde() : CreatureScript("npc_frostbite_horde") { }

        struct npc_frostbite_hordeAI : public VehicleAI
        {
            npc_frostbite_hordeAI(Creature* creature) : VehicleAI(creature) { }

            void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) override
            {
                if (apply && passenger->GetTypeId() == TYPEID_PLAYER)
                {
                    me->GetMotionMaster()->MovePath(WAYPOINT_FROSTHOUND, false);
                    me->SetReactState(REACT_PASSIVE);
                    DoCastSelf(SPELL_PERIODIC_FROSTHOUND, true);
                    Talk(SAY_TRACK_0);
                    Talk(SAY_TRACK_1, passenger);
            }

            if (!apply)
            {
                me->DespawnOrUnsummon(1 * IN_MILLISECONDS);
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != WAYPOINT_MOTION_TYPE)
            return;

            switch (id)
            {
                case 21:
                    if (Player* player = me->FindNearestPlayer(100.0f))
                    {
                        player->KilledMonsterCredit(NPC_Q_CREDIT);
                        Talk(SAY_TRACK_3, player);
                    }
                    Talk(SAY_TRACK_2);
                    me->DespawnOrUnsummon(2 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 /*diff*/) override { } // No reaction to enemies
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_frostbite_hordeAI(creature);
    }
};

enum OathboundWarderMisc
{
    // Spells
    SPELL_LAVA_ERUPTION = 56491,
    SPELL_ROOT          = 56425,
    SPELL_EARTH_SHIELD  = 56451,
    SPELL_EARTH_SHOCK   = 56506,

    // Events
    EVENT_LAVA_ERUPTION = 1,
    EVENT_ROOT,
    EVENT_EARTH_SHOCK,
};

class npc_pet_oathbound_warder : public CreatureScript
{
    public:
        npc_pet_oathbound_warder() : CreatureScript("npc_pet_oathbound_warder") { }

        struct npc_pet_oathbound_warderAI : public ScriptedAI
        {
            npc_pet_oathbound_warderAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _events.Reset();               
            }

            void EnterCombat(Unit* /*who*/) override
            {
                DoCastSelf(SPELL_EARTH_SHIELD);
                _events.ScheduleEvent(EVENT_EARTH_SHOCK,   urand(1000, 3000));
                _events.ScheduleEvent(EVENT_LAVA_ERUPTION, urand(5000, 20000));
                _events.ScheduleEvent(EVENT_ROOT,          urand(8000, 12000));
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr)
                    if (Unit* target = player->GetSelectedUnit())
                        if (me->IsValidAttackTarget(target))
                            AttackStart(target);

                _events.Update(diff);

                if (me->GetVictim()->HasBreakableByDamageCrowdControlAura(me))
                {
                    me->InterruptNonMeleeSpells(false);
                    return;
                }

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_EARTH_SHOCK:
                            DoCastVictim(SPELL_EARTH_SHOCK);
                            _events.ScheduleEvent(EVENT_EARTH_SHOCK, urand(5000, 8000));
                            break;
                        case EVENT_LAVA_ERUPTION:
                            DoCastVictim(SPELL_LAVA_ERUPTION);
                            _events.ScheduleEvent(EVENT_LAVA_ERUPTION, urand(10000, 12000));
                            break;
                        case EVENT_ROOT:
                            DoCastVictim(SPELL_ROOT);
                            _events.ScheduleEvent(EVENT_ROOT, urand(8000, 12000));
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
        return new npc_pet_oathbound_warderAI(creature);
    }
};

enum TerritorialTrespassMisc
{
    GO_SMALL_PROTO_DRAKE_EGG = 192538
};

class spell_place_stolen_eggs : public SpellScriptLoader
{
    public:
        spell_place_stolen_eggs() : SpellScriptLoader("spell_place_stolen_eggs") { }

        class spell_place_stolen_eggs_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_place_stolen_eggs_SpellScript);

            void HandleAfterCast()
            {
                GetCaster()->SummonGameObject(GO_SMALL_PROTO_DRAKE_EGG, 7081.91f, -906.408f, 1065.91f, 0.95993f, 0, 0, 0, 0, 300 * IN_MILLISECONDS);
                GetCaster()->SummonGameObject(GO_SMALL_PROTO_DRAKE_EGG, 7080.19f, -912.123f, 1066.74f, -0.541051f, 0, 0, 0, 0, 300 * IN_MILLISECONDS);
                GetCaster()->SummonGameObject(GO_SMALL_PROTO_DRAKE_EGG, 7085.54f, -912.668f, 1066.48f, 1.81514f, 0, 0, 0, 0, 300 * IN_MILLISECONDS);
                GetCaster()->SummonGameObject(GO_SMALL_PROTO_DRAKE_EGG, 7090.95f, -908.667f, 1065.04f, -0.994837f, 0, 0, 0, 0, 300 * IN_MILLISECONDS);
                GetCaster()->SummonGameObject(GO_SMALL_PROTO_DRAKE_EGG, 7082.19f, -916.648f, 1068.39f, 2.23402f, 0, 0, 0, 0, 300 * IN_MILLISECONDS);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_place_stolen_eggs_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_place_stolen_eggs_SpellScript();
        }
};

enum StormPeakWyrmMisc
{
    SPELL_RIDING_BRANNS_MACHINE = 56603
};

class npc_stormpeak_wyrm : public CreatureScript
{
    public:
        npc_stormpeak_wyrm() : CreatureScript("npc_stormpeak_wyrm") { }
    
        struct npc_stormpeak_wyrmAI : public SmartAI
        {
            npc_stormpeak_wyrmAI(Creature* creature) : SmartAI(creature) { }
    
            bool CanAIAttack(Unit const* target) const override
            {
                return !target->HasAura(SPELL_RIDING_BRANNS_MACHINE);
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_stormpeak_wyrmAI(creature);
        }
};

void AddSC_storm_peaks_rg()
{
    new spell_landmine_knockback_achievement_aura();
    new spell_bouldercrags_bomb();
    new npc_overseer_narvir();
    new npc_vyragosa_tlpd();
    new npc_hyldsmeet_warbear();
    new spell_hurl_boulder();
    new spell_salvage_corpse();
    new npc_king_jokkum_vehicle();
    new spell_blow_hodirs_horn();
    new npc_ethereal_frostworg();
    new npc_wild_wyrm();
    new spell_dodge_claws();
    new spell_wild_wyrm_grip();
    new spell_jaws_of_death();
    new npc_honest_max();
    new npc_sparksocket_aa_cannon();
    new npc_brann_flying_machine();
    new npc_frostbite_horde();
    new npc_pet_oathbound_warder();
    new spell_place_stolen_eggs();
    new npc_stormpeak_wyrm();
}
