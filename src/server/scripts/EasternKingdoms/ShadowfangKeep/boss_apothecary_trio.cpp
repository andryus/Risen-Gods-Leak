/*
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

#include "shadowfang_keep.h"
#include "LFGMgr.h"
#include "Group.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"

/*######
## apothecary hummel
######*/

enum Hummel
{
    NPC_CRAZED_APOTHECARY            = 36568,
    NPC_VIAL_BUNNY                   = 36530,

    SAY_AGGRO_1                      = 0, // Did they bother to tell you who I am and why Iam doing this?
    SAY_AGGRO_2                      = 1, // ...or are they just using you like they do everybody else?
    SAY_AGGRO_3                      = 2, // But what does it matter. It is time for this to end.
    SAY_CALL_BAXTER                  = 3, // Baxter! Get in here and help! NOW!
    SAY_CALL_FRYE                    = 4, // It is time, Frye! Attack!
    SAY_SUMMON_ADDS                  = 5, // Apothecaries! Give your life for The Crown!
    SAY_DEATH_BAXTER                 = 0, // It has been the greatest honor of my life to server with you, Hummel.
    SAY_DEATH_FRYE                   = 0, // Great. We're nto gutless, we're incompetent.

    SPELL_ALLURING_PERFUME           = 68589,
    SPELL_ALLURING_PERFUME_SPRAY     = 68607,
    SPELL_IRRESISTIBLE_COLOGNE       = 68946,
    SPELL_IRRESISTIBLE_COLOGNE_SPRAY = 68948,

    SPELL_TABLE_APPEAR               = 69216,
    SPELL_SUMMON_TABLE               = 69218,
    SPELL_CHAIN_REACTION             = 68821,
    SPELL_UNSTABLE_REACTION          = 68957,

    // frye
    SPELL_THROW_PERFUME              = 68799,
    SPELL_THROW_COLOGNE              = 68841,
    SPELL_ALLURING_PERFUME_SPILL     = 68798,
    SPELL_ALLURING_PERFUME_DAMAGE    = 68927,
    SPELL_IRRESISTIBLE_COLOGNE_SPILL = 68614,
    SPELL_IRRESISTIBLE_DAMAGE        = 68934,
};

enum Action
{
    START_INTRO,
    START_FIGHT,
    APOTHECARY_DIED,
    SPAWN_CRAZED
};

enum Phase
{
    PHASE_NORMAL,
    PHASE_INTRO
};

static Position Loc[]=
{
    // spawn points
    {-215.776443f, 2242.365479f, 79.769257f, 0.0f},
    {-169.500702f, 2219.286377f, 80.613045f, 0.0f},
    {-200.056641f, 2152.635010f, 79.763107f, 0.0f},
    {-238.448242f, 2165.165283f, 89.582985f, 0.0f},
    // moveto points
    {-210.784164f, 2219.004150f, 79.761803f, 0.0f},
    {-198.453400f, 2208.410889f, 79.762474f, 0.0f},
    {-208.469910f, 2167.971924f, 79.764969f, 0.0f},
    {-228.251511f, 2187.282471f, 79.762840f, 0.0f}
};

#define GOSSIP_ITEM_START "Begin the battle."

class npc_apothecary_hummel : public CreatureScript
{
public:
    npc_apothecary_hummel() : CreatureScript("npc_apothecary_hummel") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        InstanceScript* instanceScript = creature->GetInstanceScript();
        if (!instanceScript)
            return true;

        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (instanceScript->GetData(TYPE_BATTLE) ==  IN_PROGRESS)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_START, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 /*opt*/)
    {
        InstanceScript* instanceScript = creature->GetInstanceScript();
        if (!instanceScript)
            return true;

        if (quest->GetQuestId() == 14488)
        {
            instanceScript->SetData(TYPE_BATTLE, IN_PROGRESS);
            creature->AI()->DoAction(START_INTRO);
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        InstanceScript* instanceScript = creature->GetInstanceScript();
        if (!instanceScript)
            return true;

        if (action == GOSSIP_ACTION_INFO_DEF + 1)
            creature->AI()->DoAction(START_INTRO);

        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    struct npc_apothecary_hummelAI : public ScriptedAI
    {
        npc_apothecary_hummelAI(Creature* creature) : ScriptedAI(creature), _summons(me)
        {
            _instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            me->RestoreFaction();
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            _phase = PHASE_NORMAL;
            _step = 0;
            _deadCount = 0;
            _stepTimer = 1500;
            _aggroTimer = 5000;
            _sprayTimer = urand(4000, 7000);
            _chainReactionTimer = urand(10000, 25000);
            _firstCrazed = false;

            me->SetCorpseDelay(900); // delay despawn while still fighting baxter or frye
            _summons.DespawnAll();

            if (!_instance)
                return;

            _instance->SetData(TYPE_CROWN, NOT_STARTED);

            if (Creature* baxter = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(DATA_BAXTER))))
            {
                if (baxter->IsAlive())
                    baxter->AI()->EnterEvadeMode();
                else
                    baxter->Respawn();
            }

            if (Creature* frye = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(DATA_FRYE))))
            {
                if (frye->IsAlive())
                    frye->AI()->EnterEvadeMode();
                else
                    frye->Respawn();
            }

            if (GameObject* door = _instance->instance->GetGameObject(ObjectGuid(_instance->GetGuidData(DATA_DOOR))))
                _instance->HandleGameObject(ObjectGuid::Empty, true, door);
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
            case START_INTRO:
                {
                    if (Creature* baxter = ObjectAccessor::GetCreature(*me, _instance ? ObjectGuid(_instance->GetGuidData(DATA_BAXTER)) : ObjectGuid::Empty))
                        baxter->AI()->DoAction(START_INTRO);
                    if (Creature* frye = ObjectAccessor::GetCreature(*me, _instance ? ObjectGuid(_instance->GetGuidData(DATA_FRYE)) : ObjectGuid::Empty))
                        frye->AI()->DoAction(START_INTRO);

                    _phase = PHASE_INTRO;
                    me->SetReactState(REACT_PASSIVE);
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    DoZoneInCombat(me);
                    break;
                }
            case START_FIGHT:
                {
                    _phase = PHASE_NORMAL;
                    me->setFaction(FACTION_HOSTILE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    DoZoneInCombat(me);
                    _instance->SetData(TYPE_CROWN, IN_PROGRESS);
                    break;
                }
            case APOTHECARY_DIED:
                {
                    ++_deadCount;
                    if (_deadCount > 2) // all 3 apothecarys dead, set lootable
                    {
                        _summons.DespawnAll();
                        me->SetCorpseDelay(90); // set delay
                        me->setDeathState(JUST_DIED); // update delay
                        _instance->SetData(TYPE_CROWN, DONE);
                        me->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);

                        // HACKFIX: Complete LFG Dungeon
                        Map* map = me->GetMap();
                        if (map && map->IsDungeon())
                        {
                            for (Player& player : map->GetPlayers())
                            {
                                if (player.IsGameMaster() || !player.GetGroup())
                                    continue;

                                sLFGMgr->FinishDungeon(player.GetGroup()->GetGUID(), 288);
                                return;
                            }
                        }

                    }
                    else
                    {
                        if (me->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE))
                            me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                    }
                    break;
                }
            case SPAWN_CRAZED:
                {
                    uint8 i = urand(0, 3);
                    if (Creature* crazed = me->SummonCreature(NPC_CRAZED_APOTHECARY, Loc[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3*IN_MILLISECONDS))
                    {
                        crazed->setFaction(FACTION_HOSTILE);
                        crazed->SetReactState(REACT_PASSIVE);
                        crazed->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        crazed->GetMotionMaster()->MovePoint(1, Loc[i + 4]);
                    }

                    if (!_firstCrazed)
                    {
                        Talk(SAY_SUMMON_ADDS);
                        _firstCrazed = true;
                    }
                    break;
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (_phase == PHASE_INTRO)
            {
                if (_stepTimer <= diff)
                {
                    ++_step;
                    switch (_step)
                    {
                    case 1:
                        {
                            Talk(SAY_AGGRO_1);
                            _stepTimer = 4000;
                            break;
                        }
                    case 2:
                        {
                            Talk(SAY_AGGRO_2);
                            _stepTimer = 5500;
                            break;
                        }
                    case 3:
                        {
                            Talk(SAY_AGGRO_3);
                            _stepTimer = 1000;
                            break;
                        }
                    case 4:
                        {
                            DoAction(START_FIGHT);
                            break;
                        }
                    }
                }
                else
                    _stepTimer -= diff;
            }
            else // PHASE_NORMAL
            {
                if (!UpdateVictim())
                    return;

                if (_aggroTimer <= diff)
                {
                    DoZoneInCombat(me);
                    _aggroTimer = 5000;
                }
                else
                    _aggroTimer -= diff;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (_chainReactionTimer <= diff)
                {
                    DoCastSelf(SPELL_TABLE_APPEAR, true);
                    DoCastSelf(SPELL_SUMMON_TABLE, true);
                    DoCast(SPELL_CHAIN_REACTION);
                    _chainReactionTimer = urand(15000, 25000);
                }
                else
                    _chainReactionTimer -= diff;

                if (_sprayTimer <= diff)
                {
                    DoCastVictim(SPELL_ALLURING_PERFUME_SPRAY);
                    _sprayTimer = urand(8000, 15000);
                }
                else
                    _sprayTimer -= diff;

                DoMeleeAttackIfReady();
            }
        }

        void JustSummoned(Creature* summon)
        {
            _summons.Summon(summon);
        }

        void JustDied(Unit* /*killer*/)
        {
            DoAction(APOTHECARY_DIED);
        }

    private:
        InstanceScript* _instance;
        SummonList _summons;
        uint8 _deadCount;
        uint8 _phase;
        uint8 _step;
        uint32 _aggroTimer;
        uint32 _stepTimer;
        uint32 _sprayTimer;
        uint32 _chainReactionTimer;
        bool _firstCrazed;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_apothecary_hummelAI(creature);
    }
};

/*######
## apothecary baxter
######*/

class npc_apothecary_baxter : public CreatureScript
{
public:
    npc_apothecary_baxter() : CreatureScript("npc_apothecary_baxter") { }

    struct npc_apothecary_baxterAI : public ScriptedAI
    {
        npc_apothecary_baxterAI(Creature* creature) : ScriptedAI(creature)
        {
            _instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            me->RestoreFaction();
            _aggroTimer = 5000;
            _waitTimer = 20000;
            _sprayTimer = urand(4000, 7000);
            _chainReactionTimer = urand (10000, 25000);
            _phase = PHASE_NORMAL;

            if (Creature* hummel = ObjectAccessor::GetCreature(*me, _instance ? ObjectGuid(_instance->GetGuidData(DATA_HUMMEL)) : ObjectGuid::Empty))
            {
                if (hummel->IsAlive())
                    hummel->AI()->EnterEvadeMode();
                else
                    hummel->Respawn();
            }
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
            case START_INTRO:
                {
                    me->GetMotionMaster()->MovePoint(0, -223.92f, 2208.784f, 79.7640f);
                    _phase = PHASE_INTRO;
                    break;
                }
            case START_FIGHT:
                {
                    if (Creature* hummel = ObjectAccessor::GetCreature(*me, _instance ? ObjectGuid(_instance->GetGuidData(DATA_HUMMEL)) : ObjectGuid::Empty))
                        hummel->AI()->Talk(SAY_CALL_BAXTER);

                    _phase = PHASE_NORMAL;
                    me->setFaction(FACTION_HOSTILE);
                    DoZoneInCombat(me);
                    break;
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (_phase == PHASE_INTRO)
            {
                if (_waitTimer <= diff)
                {
                    DoAction(START_FIGHT);
                }
                else
                    _waitTimer -= diff;
            }
            else // PHASE_NORMAL
            {
                if (!UpdateVictim())
                    return;

                if (_aggroTimer <= diff)
                {
                    DoZoneInCombat(me);
                    _aggroTimer = 5000;
                }
                else
                    _aggroTimer -= diff;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (_chainReactionTimer <= diff)
                {
                    DoCastSelf(SPELL_TABLE_APPEAR, true);
                    DoCastSelf(SPELL_SUMMON_TABLE, true);
                    DoCast(SPELL_CHAIN_REACTION);
                    _chainReactionTimer = urand(15000, 25000);
                }
                else
                    _chainReactionTimer -= diff;

                if (_sprayTimer <= diff)
                {
                    DoCastVictim(SPELL_IRRESISTIBLE_COLOGNE_SPRAY);
                    _sprayTimer = urand(8000, 15000);
                }
                else
                    _sprayTimer -= diff;

                DoMeleeAttackIfReady();
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH_BAXTER);
            if (Creature* hummel = ObjectAccessor::GetCreature(*me, _instance ? ObjectGuid(_instance->GetGuidData(DATA_HUMMEL)) : ObjectGuid::Empty))
                hummel->AI()->DoAction(APOTHECARY_DIED);
        }

    private:
        InstanceScript* _instance;
        uint32 _chainReactionTimer;
        uint32 _sprayTimer;
        uint32 _aggroTimer;
        uint32 _waitTimer;
        uint8 _phase;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_apothecary_baxterAI(creature);
    }
};

/*######
## apothecary frye
######*/

class npc_apothecary_frye : public CreatureScript
{
public:
    npc_apothecary_frye() : CreatureScript("npc_apothecary_frye") { }

    struct npc_apothecary_fryeAI : public ScriptedAI
    {
        npc_apothecary_fryeAI(Creature* creature) : ScriptedAI(creature)
        {
            _instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            me->RestoreFaction();
            _aggroTimer = 5000;
            _waitTimer = 28000;
            _throwTimer = urand(2000, 4000);
            _targetSwitchTimer = urand(1000, 2000);
            _phase = PHASE_NORMAL;

            if (Creature* hummel = ObjectAccessor::GetCreature(*me, _instance ? ObjectGuid(_instance->GetGuidData(DATA_HUMMEL)) : ObjectGuid::Empty))
            {
                if (hummel->IsAlive())
                    hummel->AI()->EnterEvadeMode();
                else
                    hummel->Respawn();
            }
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
            case START_INTRO:
                {
                    me->GetMotionMaster()->MovePoint(0, -196.16f, 2199.833f, 79.7640f);
                    _phase = PHASE_INTRO;
                    break;
                }
            case START_FIGHT:
                {
                    if (Creature* hummel = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance ? ObjectGuid(_instance->GetGuidData(DATA_HUMMEL)) : ObjectGuid::Empty)))
                        hummel->AI()->Talk(SAY_CALL_FRYE);

                    _phase = PHASE_NORMAL;
                    me->setFaction(FACTION_HOSTILE);
                    DoZoneInCombat(me);
                    break;
                }
            }
        }

        void SummonBunny(Unit* target, bool perfume)
        {
            if (!target)
                return;

            if (Creature* bunny = me->SummonCreature(NPC_VIAL_BUNNY, *target, TEMPSUMMON_TIMED_DESPAWN, 25*IN_MILLISECONDS))
            {
                bunny->setFaction(FACTION_HOSTILE);
                bunny->SetReactState(REACT_PASSIVE);
                bunny->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL | UNIT_FLAG_NON_ATTACKABLE |UNIT_FLAG_NOT_SELECTABLE);
                bunny->CastSpell(bunny, perfume ? SPELL_ALLURING_PERFUME_SPILL : SPELL_IRRESISTIBLE_COLOGNE_SPILL, true, NULL, NULL, me->GetGUID());
            }
        }



        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            switch (spell->Id)
            {
            case SPELL_THROW_PERFUME:
                SummonBunny(target, true);
                break;
            case SPELL_THROW_COLOGNE:
                SummonBunny(target, false);
                break;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (_phase == PHASE_INTRO)
            {
                if (_waitTimer <= diff)
                {
                    DoAction(START_FIGHT);
                }
                else
                    _waitTimer -= diff;
            }
            else // PHASE_NORMAL
            {
                if (!UpdateVictim())
                    return;

                if (_throwTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        DoCast(target, urand(0, 1) ? SPELL_THROW_PERFUME : SPELL_THROW_COLOGNE);
                    _throwTimer = urand(5000, 7500);
                }
                else
                    _throwTimer -= diff;

                if (_targetSwitchTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true))
                    {
                        me->GetThreatManager().ModifyThreatByPercent(me->GetVictim(), -100);
                        me->GetThreatManager().AddThreat(target, 9999999.9f);
                    }
                    _targetSwitchTimer = urand(5000, 10000);
                }
                else
                    _targetSwitchTimer -= diff;

                if (_aggroTimer <= diff)
                {
                    DoZoneInCombat(me);
                    _aggroTimer = 5000;
                }
                else
                    _aggroTimer -= diff;

                DoMeleeAttackIfReady();
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH_FRYE);
            if (Creature* hummel = ObjectAccessor::GetCreature(*me, _instance ? ObjectGuid(_instance->GetGuidData(DATA_HUMMEL)) : ObjectGuid::Empty))
                hummel->AI()->DoAction(APOTHECARY_DIED);
        }

    private:
        InstanceScript* _instance;
        uint32 _targetSwitchTimer;
        uint32 _throwTimer;
        uint32 _aggroTimer;
        uint32 _waitTimer;
        uint8 _phase;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_apothecary_fryeAI(creature);
    }
};

/*######
## npc_crazed_apothecary
######*/

class npc_crazed_apothecary : public CreatureScript
{
public:
    npc_crazed_apothecary() : CreatureScript("npc_crazed_apothecary") { }

    struct npc_crazed_apothecaryAI : public ScriptedAI
    {
        npc_crazed_apothecaryAI(Creature* creature) : ScriptedAI(creature) { }

        void MovementInform(uint32 type, uint32 /*id*/)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            DoZoneInCombat(me);

            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
            {
                _explodeTimer = urand (2500, 5000);
                me->GetMotionMaster()->MoveFollow(target, 0.0f, float(2*M_PI*rand_norm()));
            }
            else
                me->DespawnOrUnsummon();
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (_explodeTimer <= diff)
            {
                DoCastSelf(SPELL_UNSTABLE_REACTION);
            }
            else
                _explodeTimer -= diff;
        }

    private:
        uint32 _explodeTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_crazed_apothecaryAI(creature);
    }
};

class spell_concentrated_alluring_perfume : public SpellScriptLoader
{
public:
    spell_concentrated_alluring_perfume() : SpellScriptLoader("spell_concentrated_alluring_perfume") { }

    class spell_concentrated_alluring_perfume_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_concentrated_alluring_perfume_AuraScript);

        void HandleDummyTick(AuraEffect const* /*aurEff*/)
        {
            Unit* target = GetTarget();
            if (target->GetTypeId() == TYPEID_PLAYER)
                if(Unit* caster = GetCaster())
                    caster->CastSpell(target, SPELL_ALLURING_PERFUME_DAMAGE, true);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_concentrated_alluring_perfume_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_concentrated_alluring_perfume_AuraScript();
    }
};

class spell_irrestible_cologne_spill : public SpellScriptLoader
{
public:
    spell_irrestible_cologne_spill() : SpellScriptLoader("spell_irrestible_cologne_spill") { }

    class spell_irrestible_cologne_spill_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_irrestible_cologne_spill_AuraScript);

        void HandleDummyTick(AuraEffect const* /*aurEff*/)
        {
            Unit* target = GetTarget();
            if (target->GetTypeId() == TYPEID_PLAYER)
                if(Unit* caster = GetCaster())
                    caster->CastSpell(target, SPELL_IRRESISTIBLE_DAMAGE, true);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_irrestible_cologne_spill_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_irrestible_cologne_spill_AuraScript();
    }
};

void AddSC_boss_apothecary_trio()
{
    new npc_apothecary_hummel();
    new npc_apothecary_baxter();
    new npc_apothecary_frye();
    new npc_crazed_apothecary();
    new spell_concentrated_alluring_perfume();
    new spell_irrestible_cologne_spill();
}