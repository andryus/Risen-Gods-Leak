/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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
#include "SpellAuraEffects.h"
#include "event.h"
#include "Vehicle.h"
#include "DBCStores.h"

enum TheTurkinator
{
    SPELL_KILL_COUNTER_VISUAL = 62015,
    SPELL_KILL_COUNTER_VISUAL_MAX = 62021,
};

#define THE_THUKINATOR_10 "Turkey Hunter!"
#define THE_THUKINATOR_20 "Turkey Domination!"
#define THE_THUKINATOR_30 "Turkey Slaughter!"
#define THE_THUKINATOR_40 "TURKEY TRIUMPH!"

class spell_gen_turkey_tracker : public SpellScriptLoader
{
    public:
        spell_gen_turkey_tracker() : SpellScriptLoader("spell_gen_turkey_tracker") {}

        class spell_gen_turkey_tracker_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_turkey_tracker_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_KILL_COUNTER_VISUAL))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_KILL_COUNTER_VISUAL_MAX))
                    return false;
                return true;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Player* target = GetHitPlayer();
                if (!target)
                    return;

                if (Aura* aura = GetCaster()->ToPlayer()->GetAura(GetSpellInfo()->Id))
                {
                    switch (aura->GetStackAmount())
                    {
                        case 10:
                            target->MonsterTextEmote(THE_THUKINATOR_10, 0, true);
                            GetCaster()->CastSpell(target, SPELL_KILL_COUNTER_VISUAL, true);
                            break;
                        case 20:
                            target->MonsterTextEmote(THE_THUKINATOR_20, 0, true);
                            GetCaster()->CastSpell(target, SPELL_KILL_COUNTER_VISUAL, true);
                            break;
                        case 30:
                            target->MonsterTextEmote(THE_THUKINATOR_30, 0, true);
                            GetCaster()->CastSpell(target, SPELL_KILL_COUNTER_VISUAL, true);
                            break;
                        case 40:
                            target->MonsterTextEmote(THE_THUKINATOR_40, 0, true);
                            GetCaster()->CastSpell(target, SPELL_KILL_COUNTER_VISUAL, true);
                            GetCaster()->CastSpell(target, SPELL_KILL_COUNTER_VISUAL_MAX, true); // Achievement Credit
                            target->RemoveAura(SPELL_KILL_COUNTER_VISUAL_MAX);
                            break;
                        default:
                            break;
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_gen_turkey_tracker_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_turkey_tracker_SpellScript();
        }
};


enum FeastSpells
{
    SPELL_SERVING_OF_PIE          =    61805,
    SPELL_SERVING_OF_CRANBERRIES  =    61804,
    SPELL_SERVING_OF_POTATO       =    61808,
    SPELL_SERVING_OF_TURKEY       =    61807,
    SPELL_SERVING_OF_STUFFING     =    61806,

    SPELL_SPIRIT_OF_SHARING       =    61849,

    SPELL_FEAST_ON_PIE            =    61787,
    SPELL_FEAST_ON_CRANBERRIES    =    61785,
    SPELL_FEAST_ON_POTATO         =    61786,
    SPELL_FEAST_ON_TURKEY         =    61784,
    SPELL_FEAST_ON_STUFFING       =    61788,

    // Well Fed
    SPELL_WELL_FED_AP                   = 65414,
    SPELL_WELL_FED_ZM                   = 65412,
    SPELL_WELL_FED_HIT                  = 65416,
    SPELL_WELL_FED_HASTE                = 65410,
    SPELL_WELL_FED_SPIRIT               = 65415
};

class spell_gen_feast_on : public SpellScriptLoader
{
    public:
        spell_gen_feast_on() : SpellScriptLoader("spell_gen_feast_on") { }

        class spell_gen_feast_on_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_feast_on_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {

                Unit* caster = GetCaster();
                if (caster->IsVehicle())
                    if (Unit* player = caster->GetVehicleKit()->GetPassenger(0))
                    {
                        switch (GetSpellInfo()->Id)
                        {
                            case SPELL_FEAST_ON_CRANBERRIES:
                                player->CastSpell(player, SPELL_SERVING_OF_CRANBERRIES, true);
                                if (player->GetAuraCount(SPELL_SERVING_OF_CRANBERRIES) == 5)
                                    player->CastSpell(player, SPELL_WELL_FED_ZM, true);
                                break;
                            case SPELL_FEAST_ON_PIE:
                                player->CastSpell(player, SPELL_SERVING_OF_PIE, true);
                                if (player->GetAuraCount(SPELL_SERVING_OF_PIE) == 5)
                                    player->CastSpell(player, SPELL_WELL_FED_SPIRIT, true);
                                break;
                            case SPELL_FEAST_ON_POTATO:
                                player->CastSpell(player, SPELL_SERVING_OF_POTATO, true);
                                if (player->GetAuraCount(SPELL_SERVING_OF_POTATO) == 5)
                                    player->CastSpell(player, SPELL_WELL_FED_HASTE, true);
                                break;
                            case SPELL_FEAST_ON_TURKEY:
                                player->CastSpell(player, SPELL_SERVING_OF_TURKEY, true);
                                if (player->GetAuraCount(SPELL_SERVING_OF_TURKEY) == 5)
                                    player->CastSpell(player, SPELL_WELL_FED_AP, true);
                                break;
                            case SPELL_FEAST_ON_STUFFING:
                                player->CastSpell(player, SPELL_SERVING_OF_STUFFING, true);
                                if (player->GetAuraCount(SPELL_SERVING_OF_STUFFING) == 5)
                                    player->CastSpell(player, SPELL_WELL_FED_HIT, true);
                                break;
                        }

                        if ( (player->GetAuraCount(SPELL_SERVING_OF_CRANBERRIES) +
                            player->GetAuraCount(SPELL_SERVING_OF_PIE) +
                            player->GetAuraCount(SPELL_SERVING_OF_POTATO) +
                            player->GetAuraCount(SPELL_SERVING_OF_TURKEY) +
                            player->GetAuraCount(SPELL_SERVING_OF_STUFFING)) == 25) //means we are adding the last stack for spirit of sharing (5 stacks of each), correct logic?
                        {
                            player->RemoveAura(SPELL_SERVING_OF_CRANBERRIES);
                            player->RemoveAura(SPELL_SERVING_OF_POTATO);
                            player->RemoveAura(SPELL_SERVING_OF_PIE);
                            player->RemoveAura(SPELL_SERVING_OF_TURKEY);
                            player->RemoveAura(SPELL_SERVING_OF_STUFFING);
                            player->CastSpell(player, SPELL_SPIRIT_OF_SHARING, true);
                        }
                    }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_gen_feast_on_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_feast_on_SpellScript();
        }
};

enum PassSpells
{
    SPELL_PASS_THE_TURKEY       =   66250,
    SPELL_PASS_THE_STUFFING     =   66259,
    SPELL_PASS_THE_PIE          =   66260,
    SPELL_PASS_THE_CRANBERRIES  =   66261,
    SPELL_PASS_THE_POTATOES     =   66262,

    SPELL_THROW_STUFFING        =   61927,
    SPELL_THROW_PIE             =   61926,
    SPELL_THROW_CRANBERRIES     =   61925,
    SPELL_THROW_TURKEY          =   61928,
    SPELL_THROW_POTATOES        =   61929,

    SPELL_ACHIEVEMENT_PASS_TURKEY       =   66373,
    SPELL_ACHIEVEMENT_PASS_STUFFING     =   66375,
    SPELL_ACHIEVEMENT_PASS_PIE          =   66374,
    SPELL_ACHIEVEMENT_PASS_CRANBERRIES  =   66372,
    SPELL_ACHIEVEMENT_PASS_POTATOES     =   66376, //we could link this in DB btw.
};


class spell_pb_pass_food : public SpellScriptLoader
{
    public:
        spell_pb_pass_food() : SpellScriptLoader("spell_pb_pass_food") { }

        class spell_pb_pass_food_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pb_pass_food_SpellScript);


            void DoAchievementSpells(uint32 pass_spellid, uint32 acm_spellid/*, Player* player*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (!caster->IsVehicle())
                        return;

                    if (Player* player = caster->GetVehicleKit()->GetPassenger(0)->ToPlayer())
                        if (Unit* target = player->GetSelectedPlayer())
                            if (Vehicle* vehicle = target->GetVehicle())
                                if ((player->isInFront(target, atan2(2.3f, 2.79f)) || player->isInFront(target, atan2(5.4f, 1.483F))) && vehicle->GetVehicleInfo()->m_ID == 321 && (player->GetVehicle()->GetCreatureEntry() != target->GetVehicle()->GetCreatureEntry())){
                                    player->CastSpell(target, pass_spellid, true);
                                    player->CastSpell(target, acm_spellid, true);
                                }
                }
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                        switch (GetSpellInfo()->Id)
                        {
                            case SPELL_PASS_THE_TURKEY:
                                DoAchievementSpells(SPELL_ACHIEVEMENT_PASS_TURKEY, SPELL_THROW_TURKEY);
                                break;
                            case SPELL_PASS_THE_STUFFING:
                                DoAchievementSpells(SPELL_ACHIEVEMENT_PASS_STUFFING, SPELL_THROW_STUFFING);
                                break;
                            case SPELL_PASS_THE_PIE:
                                DoAchievementSpells(SPELL_ACHIEVEMENT_PASS_PIE, SPELL_THROW_PIE);
                                break;
                            case SPELL_PASS_THE_CRANBERRIES:
                                DoAchievementSpells(SPELL_ACHIEVEMENT_PASS_CRANBERRIES, SPELL_THROW_CRANBERRIES);
                                break;
                            case SPELL_PASS_THE_POTATOES:
                                DoAchievementSpells(SPELL_ACHIEVEMENT_PASS_POTATOES, SPELL_THROW_POTATOES);
                                break;
                        }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pb_pass_food_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pb_pass_food_SpellScript();
        }
};

#define SPELL_PILGRIM_HAT       66304
#define SPELL_PILGRIM_ATTIRE    66303

class achievement_terokkar_turkey_time : public AchievementCriteriaScript
{
    public:
        achievement_terokkar_turkey_time() : AchievementCriteriaScript("achievement_terokkar_turkey_time") { }

        bool OnCheck(Player* player, Unit* /*target*/)
        {
                if (player->HasAura(SPELL_PILGRIM_HAT) && player->HasAura(SPELL_PILGRIM_ATTIRE))
                return true;

            return false;
        }
};

enum BountifulFeast
{
    // Bountiful Feast
    SPELL_BOUNTIFUL_FEAST_DRINK          = 66041,
    SPELL_BOUNTIFUL_FEAST_FOOD           = 66478,
    SPELL_BOUNTIFUL_FEAST_REFRESHMENT    = 66622,
};

class spell_gen_bountiful_feast : public SpellScriptLoader
{
    public:
        spell_gen_bountiful_feast() : SpellScriptLoader("spell_gen_bountiful_feast") { }

        class spell_gen_bountiful_feast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_bountiful_feast_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                caster->CastSpell(caster, SPELL_BOUNTIFUL_FEAST_DRINK, true);
                caster->CastSpell(caster, SPELL_BOUNTIFUL_FEAST_FOOD, true);
                caster->CastSpell(caster, SPELL_BOUNTIFUL_FEAST_REFRESHMENT, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_gen_bountiful_feast_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_bountiful_feast_SpellScript();
        }
};

// Item: Turkey Caller
enum LonelyTurkey
{
    SPELL_STINKER_BROKEN_HEART  = 62004,
};

class npc_lonely_turkey : public CreatureScript
{
    public:
        npc_lonely_turkey() : CreatureScript("npc_lonely_turkey") { }

        struct npc_lonely_turkeyAI : public ScriptedAI
        {
            npc_lonely_turkeyAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset()
            {
                if (me->IsSummon())
                    if (Unit* owner = me->ToTempSummon()->GetSummoner())
                        me->GetMotionMaster()->MovePoint(0, owner->GetPositionX() + 25 * cos(owner->GetOrientation()), owner->GetPositionY() + 25 * cos(owner->GetOrientation()), owner->GetPositionZ());

                _StinkerBrokenHeartTimer = 3.5 * IN_MILLISECONDS;
            }

            void UpdateAI(uint32 diff)
            {
                if (_StinkerBrokenHeartTimer <= diff)
                {
                    DoCast(SPELL_STINKER_BROKEN_HEART);
                    me->setFaction(31);
                }
                _StinkerBrokenHeartTimer -= diff;
            }
        private:
            uint32 _StinkerBrokenHeartTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lonely_turkeyAI(creature);
        }
};



//int Chairs[5] =    {/*NPC_STUFFING_CHAIR*/ 34819, /*NPC_POTATO_CHAIR*/ 34824, /*NPC_CRANBERRY_CHAIR*/ 34823, /*NPC_TURKEY_CHAIR*/ 34812, /*NPC_PIE_CHAIR*/ 34822};

/*
class npc_bountyful_table : public CreatureScript
{
public:
    npc_bountyful_table() : CreatureScript("npc_bountyful_table") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_bountyful_tableAI(pCreature);
    }

    struct npc_bountyful_tableAI : public ScriptedAI
    {
        npc_bountyful_tableAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            SpawnChairs();
        }

        void Reset()
        {
            me->AddUnitState(UNIT_STATE_ROOT);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void SpawnChairs()
        {
            float div_o = (2*M_PI)/5;

            for (int i = 0; i <= 4; ++i)
            {
                Position pos;
                me->GetNearPoint(me, pos.m_positionX, pos.m_positionY, pos.m_positionZ, 0.0f, 2, div_o*i+0.6f);
                pos.m_positionX = pos.GetPositionX() -0.85f; //center of the table is not as seen on the model

                if (Creature * chair = me->SummonCreature(Chairs[i], pos))
                {
                    chair->SetFacing(chair->GetAngle(me)-0.5f);
                    chair->SetSpeed(MOVE_WALK, 0.0f, true);
                    chair->AddUnitState(UNIT_STATE_ROOT);
                    chair->AddUnitState(UNIT_STAT_CANNOT_TURN);
                }
            }

        }

    };
};
*/

#define GOB_CAMP_FIRE    29784

class ncpet_plumb_turkey : public CreatureScript
{
public:
    ncpet_plumb_turkey() : CreatureScript("ncpet_plumb_turkey") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new ncpet_plumb_turkeyAI(pCreature);
    }

    struct ncpet_plumb_turkeyAI : public ScriptedAI
    {
        ncpet_plumb_turkeyAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void Reset()
        {
            fireCheckTimer = 2000;
            foundFire = false;
            jumpTimer = 2000;
            fireGUID.Clear();
            die = false;
            dieTimer = 3000;
        }

        void UpdateAI(uint32 diff)
        {
            if (!foundFire)
            {
                if (fireCheckTimer <= diff)
                {
                    if (GameObject* fire = me->FindNearestGameObject(GOB_CAMP_FIRE, 5.0f))
                    {
                        me->MonsterTextEmote("Plump Turkey senses his destiny!", 0, 0);
                        me->AddUnitState(UNIT_STATE_ROOT);
                        me->SetFacingTo(me->GetAngle(fire));
                        fireGUID = fire->GetGUID();
                        foundFire = true;
                        fireCheckTimer = 0;
                    }
                }
                else fireCheckTimer -= diff;
            }

            if (foundFire && !die)
            {
                if (jumpTimer <= diff)
                {
                    if (GameObject* fire = ObjectAccessor::GetGameObject(*me, fireGUID))
                    {
                        me->GetMotionMaster()->MoveJump(*fire, 1.0f, 1.0f);
                        die = true;
                    }
                }
                else jumpTimer -= diff;
            }

            if (foundFire && die)
            {
                if (dieTimer <= diff)
                {
                    if (GameObject* fire = ObjectAccessor::GetGameObject(*me, fireGUID))
                        me->UpdatePosition(fire->GetPositionX(), fire->GetPositionY(), fire->GetPositionZ(), 0.0f, true);

                    me->DealDamage(me, 1000);
                    me->DespawnOrUnsummon(5000);
                }
                else dieTimer -= diff;
            }
        }

    private:
        uint32 fireCheckTimer;
        uint32 jumpTimer;
        uint32 dieTimer;
        bool foundFire;
        bool die;
        ObjectGuid fireGUID;
    };
};

void AddSC_event_pilgrims_bounty()
{
    new spell_gen_feast_on();
    new spell_pb_pass_food();
    new spell_gen_turkey_tracker();
    new achievement_terokkar_turkey_time();

    new spell_gen_bountiful_feast();

//    new npc_bountyful_table();
    new ncpet_plumb_turkey();
}
