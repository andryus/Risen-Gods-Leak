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

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "PassiveAI.h"
#include "Player.h"
#include "CreatureTextMgr.h"
#include "GameEventMgr.h"

//
//  Emerald Dragon NPCs and IDs (kept here for reference)
//

enum EmeraldDragonNPC
{
    NPC_DREAM_FOG                   = 15224,
    DRAGON_YSONDRE                  = 14887,
    DRAGON_LETHON                   = 14888,
    DRAGON_EMERISS                  = 14889,
    DRAGON_TAERAR                   = 14890,
    NPC_CONTROLLER                  = 1010804,
    NPC_NIGHMARE_CREATURE           = 1010805,
    NPC_DREAMER                     = 1010806,
    NPC_DREAM_SPORE                 = 1010806,
};

//
// Emerald Dragon Spells (used for the dragons)
//

enum EmeraldDragonSpells
{
    SPELL_TAIL_SWEEP                = 15847,    // tail sweep - slap everything behind dragon (2 seconds interval)
    SPELL_SUMMON_PLAYER             = 24776,    // teleport highest threat player in front of dragon if wandering off
    SPELL_DREAM_FOG                 = 24777,    // auraspell for Dream Fog NPC (15224)
    SPELL_SLEEP                     = 24778,    // sleep triggerspell (used for Dream Fog)
    SPELL_SEEPING_FOG_LEFT          = 24813,    // dream fog - summon left
    SPELL_SEEPING_FOG_RIGHT         = 24814,    // dream fog - summon right
    SPELL_NOXIOUS_BREATH            = 24818,
    SPELL_MARK_OF_NATURE            = 25040,    // Mark of Nature trigger (applied on target death - 15 minutes of being suspectible to Aura Of Nature)
    SPELL_MARK_OF_NATURE_AURA       = 25041,    // Mark of Nature (passive marker-test, ticks every 10 seconds from boss, triggers spellID 25042 (scripted)
    SPELL_AURA_OF_NATURE            = 25043,    // Stun for 2 minutes (used when SPELL_MARK_OF_NATURE exists on the target)
    SPELL_ENTANGLING_ROOTS_VISUAL   = 51367,
    SPELL_SLEEP_PERMANENT           = 34664,
    SPELL_DREAM_VISUAL              = 27614,
    SPELL_SLEEP_FADE                = 64394,
    SPELL_CAMERA_SHAKE              = 24203,
    SPELL_PROCESS                   = 530,
};

//
// Emerald Dragon Eventlists (shared and specials)
//

enum Events
{
    // General events for all dragons
    EVENT_SEEPING_FOG = 1,
    EVENT_NOXIOUS_BREATH,
    EVENT_TAIL_SWEEP,
    EVENT_START_DREAM_PHASE,
    EVENT_SUMMON_SPORE,
    EVENT_UPDATE_MAX_HEALTH,

    // Ysondre
    EVENT_LIGHTNING_WAVE,
    EVENT_SUMMON_DRUID_SPIRITS,

    // Lethon
    EVENT_SHADOW_BOLT_WHIRL,

    // Emeriss
    EVENT_VOLATILE_INFECTION,
    EVENT_CORRUPTION_OF_EARTH,

    // Taerar
    EVENT_ARCANE_BLAST,
    EVENT_BELLOWING_ROAR,
};

enum ControllerActions
{
    ACTION_START_PLAYER     = 1,
    ACTION_START            = 2,
    ACTION_RESPAWN          = 3,
    ACTION_DESPAWN          = 4
};

/*
* ---
* --- Emerald Dragons : Base AI-structure used for all the Emerald dragons
* ---
*/

const uint32 MAP_ASZHARA_CRATER = 37;
const Position DREAM_ORIGIN = { -630.93f , -259.42f , 352.58f };

struct emerald_dragon_eventAI : public WorldBossAI
{
    emerald_dragon_eventAI(Creature* creature) : WorldBossAI(creature), controllerGUID()
    {
        DoCastSelf(SPELL_ENTANGLING_ROOTS_VISUAL, true);
    }

    void Reset() override
    {
        WorldBossAI::Reset();
        UpdateMaxHealth();
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        me->SetReactState(REACT_AGGRESSIVE);
        DoCastSelf(SPELL_MARK_OF_NATURE_AURA, true);
        events.ScheduleEvent(EVENT_TAIL_SWEEP, 4000);
        events.ScheduleEvent(EVENT_NOXIOUS_BREATH, urand(7500, 15000));
        events.ScheduleEvent(EVENT_SEEPING_FOG, urand(12500, 20000));
        events.ScheduleEvent(EVENT_UPDATE_MAX_HEALTH, 0);
    }

    void UpdateMaxHealth()
    {
        uint32 currentMax = me->GetMaxHealth();
        uint32 targetMax = me->GetCreateHealth() * float(std::max(5u, std::min(40u, me->GetMap()->GetPlayersCountExceptGMs()))) / 40.f;
        if (currentMax != targetMax)
        {
            float healthPct = me->GetHealthPct();
            me->SetMaxHealth(targetMax);
            me->SetHealth(CalculatePct(targetMax, healthPct));
        }
    }

    void JustSummoned(Creature* summon) override
    {
        if (summon->getLevel() < 80)
            summon->SetLevel(80);

        if (summon->GetEntry() == 15260)
        {
            summon->SetMaxHealth(20040);
            summon->SetHealth(summon->GetMaxHealth());
            summon->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 450.f);
            summon->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 650.f);
            summon->UpdateDamagePhysical(BASE_ATTACK);
        }

        if (summon->GetEntry() == NPC_NIGHMARE_CREATURE)
        {
            summons.Summon(summon);
            for (auto itr = dreamPlayers.begin(); itr != dreamPlayers.end(); itr++)
                if (auto creature = sObjectAccessor->GetCreature(*me, itr->second))
                    summon->AI()->AttackStart(creature);
        }
        else
            WorldBossAI::JustSummoned(summon);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        me->SetVisible(false);
        WorldBossAI::EnterEvadeMode();
    }

    void JustReachedHome() override
    {
        me->SetVisible(true);
        if (auto controller = sObjectAccessor->GetCreature(*me, controllerGUID))
        {
            controller->AI()->SetData(0, ACTION_RESPAWN);
            me->SetImmuneToAll(true);
            DoCastSelf(SPELL_ENTANGLING_ROOTS_VISUAL, true);
        }
    }

    void SetGuidData(ObjectGuid guid, uint32 /*dataId*/) override
    {
        controllerGUID = guid;
    }

    // Execute and reschedule base events shared between all Emerald Dragons
    virtual void ExecuteEvent(uint32 eventId)
    {
        switch (eventId)
        {
        case EVENT_SEEPING_FOG:
            // Seeping Fog appears only as "pairs", and only ONE pair at any given time!
            // Despawntime is 2 minutes, so reschedule it for new cast after 2 minutes + a minor "random time" (30 seconds at max)
            DoCastSelf(roll_chance_i(50) ? SPELL_SEEPING_FOG_LEFT : SPELL_SEEPING_FOG_RIGHT, true);
            events.ScheduleEvent(EVENT_SUMMON_SPORE, urand(120000, 130000));
            break;
        case EVENT_SUMMON_SPORE:
            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.f, true))
            {
                if (Creature* spore = me->SummonCreature(NPC_DREAM_SPORE, *target))
                {
                    spore->AI()->AttackStart(target);
                    spore->GetThreatManager().addThreat(target, 1000000.f);
                }
            }
            events.ScheduleEvent(EVENT_SEEPING_FOG, 35 * IN_MILLISECONDS);
            break;
        case EVENT_NOXIOUS_BREATH:
            // Noxious Breath is cast on random intervals, no less than 7.5 seconds between
            // DoCastSelf(SPELL_NOXIOUS_BREATH);
            events.ScheduleEvent(EVENT_NOXIOUS_BREATH, urand(7500, 15000));
            break;
        case EVENT_TAIL_SWEEP:
            // Tail Sweep is cast every two seconds, no matter what goes on in front of the dragon
            {
                auto targets = me->GetThreatManager().getThreatList();
                for (auto i : targets)
                {
                    Unit* target = i->getTarget();
                    if (!target)
                        continue;

                    if (!me->HasInArc(static_cast<float>(M_PI) * 1.5f, target) && me->IsWithinMeleeRange(target))
                        target->KnockbackFrom(me->GetPositionX(), me->GetPositionY(), 40.f, 10.f);
                }
            }
            events.ScheduleEvent(EVENT_TAIL_SWEEP, 2000);
            break;
        case EVENT_UPDATE_MAX_HEALTH:
            UpdateMaxHealth();
            events.ScheduleEvent(EVENT_UPDATE_MAX_HEALTH, 2000);
            break;
        case EVENT_START_DREAM_PHASE:
            {
                auto targets = me->GetThreatManager().getThreatList();
                std::vector<Player*> healers;
                std::vector<Player*> selectedPlayers;
                uint32 playerCount = 0;
                for (auto i : targets)
                {
                    Unit* target = i->getTarget();
                    if (!target)
                        continue;

                    Player* plr = target->ToPlayer();
                    if (!plr)
                        continue;

                    if (plr->HasAura(SPELL_SLEEP_PERMANENT))
                        continue;

                    if (plr->IsHealer())
                        healers.push_back(plr);
                    else if (++playerCount % 2 == 0)
                            selectedPlayers.push_back(plr);
                }

                for (auto plr : healers)
                    if (++playerCount % 2 == 0)
                        selectedPlayers.push_back(plr);

                float x, y, z;
                DREAM_ORIGIN.GetPosition(x, y, z);

                std::vector<Unit*> dreamerUnits;

                for (auto plr : selectedPlayers)
                {
                    if (auto creature = plr->SummonCreature(NPC_DREAMER, *plr))
                    {
                        plr->AddAura(SPELL_SLEEP_PERMANENT, plr);
                        plr->CastSpell(creature, SPELL_PROCESS, true);
                        plr->AddAura(57507, creature);
                        creature->AddAura(38457, creature);
                        dreamPlayers[plr->GetGUID()] = creature->GetGUID();
                        Position realTargetPos(DREAM_ORIGIN);
                        creature->MovePosition(realTargetPos, 40.f * (float)rand_norm(), (float)rand_norm() * static_cast<float>(2 * M_PI));
                        creature->NearTeleportTo(realTargetPos.GetPositionX(), realTargetPos.GetPositionY(), realTargetPos.GetPositionZ(), 0.f);
                        summons.Summon(creature);
                        dreamerUnits.push_back(creature);
                    }
                }

                uint32 ghostCount = selectedPlayers.size() / 2 + (selectedPlayers.size() % 2 == 0 ? 0 : 1);

                for (uint32 i = 0; i < ghostCount; i++)
                {
                    if (auto shadow = me->SummonCreature(NPC_NIGHMARE_CREATURE, x, y, z, 0.f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000))
                    {
                        shadow->AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);
                        Position realTargetPos;
                        shadow->GetRandomNearPosition(realTargetPos, 40.f);
                        shadow->NearTeleportTo(realTargetPos.GetPositionX(), realTargetPos.GetPositionY(), realTargetPos.GetPositionZ(), 0.f);
                        shadowGUIDs.push_back(shadow->GetGUID());
                        for (auto dreamer : dreamerUnits)
                            shadow->AI()->AttackStart(dreamer);
                        shadow->AI()->AttackStart(Trinity::Containers::SelectRandomContainerElement(dreamerUnits));
                    }
                }

                break;
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        shadowGUIDs.erase(std::remove_if(shadowGUIDs.begin(), shadowGUIDs.end(), [this](ObjectGuid guid)
        {
            if (auto creature = sObjectAccessor->GetCreature(*me, guid))
                return creature->isDead();
            return true;
        }), shadowGUIDs.end());

        if (shadowGUIDs.empty())
        {
            for (auto itr : dreamPlayers)
            {
                if (auto player = sObjectAccessor->GetPlayer(*me, itr.first))
                {
                    player->RemoveAurasDueToSpell(SPELL_SLEEP_PERMANENT);
                    player->CastSpell(player, SPELL_SLEEP_FADE, true);
                }
                if (auto creature = sObjectAccessor->GetCreature(*me, itr.second))
                    creature->DespawnOrUnsummon();
            }
            dreamPlayers.clear();
        }

        for (auto itr = dreamPlayers.begin(); itr != dreamPlayers.end();)
        {
            bool alive = false;
            if (auto creature = sObjectAccessor->GetCreature(*me, itr->second))
            {
                alive = creature->IsAlive() && creature->IsCharmed();
                if (!creature->IsWithinDist3d(&DREAM_ORIGIN, 60.f) && creature->IsWithinDist3d(&DREAM_ORIGIN, 80.f))
                    alive = false;
            }
            if (!alive)
            {
                if (auto player = sObjectAccessor->GetPlayer(*me, itr->first))
                    me->Kill(player);
                if (auto creature = sObjectAccessor->GetCreature(*me, itr->second))
                    creature->DespawnOrUnsummon();
                itr = dreamPlayers.erase(itr);
            }
            else
                itr++;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteEvent(eventId);

        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override
    {
        for (Player& player : me->GetMap()->GetPlayers())
            player.KilledMonsterCredit(me->GetEntry());

        if (auto controller = sObjectAccessor->GetCreature(*me, controllerGUID))
            controller->AI()->SetData(0, ACTION_DESPAWN);

        me->SetRespawnDelay(DAY);
        const uint32 ED_ENTRIES[4] = { 1010796, 1010797, 1010798, 1010799 };
        const uint32 CONTROLLER_ENTRY = 1010804;
        for (uint32 i = 0; i < 4; i++)
        {
            if (i != me->GetEntry())
                if (auto dragon = me->FindNearestCreature(ED_ENTRIES[i], 250.f))
                    if (auto controller = dragon->FindNearestCreature(CONTROLLER_ENTRY, 30.f))
                    {
                        controller->AI()->SetData(0, ACTION_START);
                        break;
                    }
        }

        for (auto itr : dreamPlayers)
        {
            if (auto player = sObjectAccessor->GetPlayer(*me, itr.first))
            {
                player->RemoveAurasDueToSpell(SPELL_SLEEP_PERMANENT);
                player->CastSpell(player, SPELL_SLEEP_FADE, true);
            }
            if (auto creature = sObjectAccessor->GetCreature(*me, itr.second))
                creature->DespawnOrUnsummon();
        }
        dreamPlayers.clear();
    }
    private:
        std::map<ObjectGuid, ObjectGuid> dreamPlayers;
        std::vector<ObjectGuid> shadowGUIDs;
        ObjectGuid controllerGUID;
};

/*
* ---
* --- Dragonspecific scripts and handling: YSONDRE
* ---
*/

enum YsondreNPC
{
    NPC_DEMENTED_DRUID = 15260,
};

enum YsondreTexts
{
    SAY_YSONDRE_AGGRO = 0,
    SAY_YSONDRE_SUMMON_DRUIDS = 1,
};

enum YsondreSpells
{
    SPELL_LIGHTNING_WAVE = 24819,
    SPELL_SUMMON_DRUID_SPIRITS = 24795,
};

class MeleeRangeSelector
{
    public:
        MeleeRangeSelector(Unit* source, bool inMeleeRange) : _source(source), _range(inMeleeRange) { }

        bool operator()(Unit* target) const
        {
            return _range == target->IsWithinMeleeRange(_source);
        }

    private:
        bool _range;
        Unit* _source;
};

class boss_ysondre_event : public CreatureScript
{
public:
    boss_ysondre_event() : CreatureScript("boss_ysondre_event") { }

    struct boss_ysondreAI : public emerald_dragon_eventAI
    {
        boss_ysondreAI(Creature* creature) : emerald_dragon_eventAI(creature)
        {
        }

        void Reset() override
        {
            melee = true;
            _stage = 1;
            emerald_dragon_eventAI::Reset();
            events.ScheduleEvent(EVENT_LIGHTNING_WAVE, 12000);
        }

        void EnterCombat(Unit* who) override
        {
            Talk(SAY_YSONDRE_AGGRO);
            emerald_dragon_eventAI::EnterCombat(who);
        }

        // Summon druid spirits on 75%, 50% and 25% health
        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) override
        {
            if (!HealthAbovePct(100 - 25 * _stage))
            {
                Talk(SAY_YSONDRE_SUMMON_DRUIDS);

                for (uint8 i = 0; i < 10; ++i)
                    DoCastSelf(SPELL_SUMMON_DRUID_SPIRITS, true);
                ++_stage;
            }
        }

        void ExecuteEvent(uint32 eventId) override
        {
            switch (eventId)
            {
            case EVENT_LIGHTNING_WAVE:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, MeleeRangeSelector(me, melee)))
                    DoCast(target, SPELL_LIGHTNING_WAVE);
                melee = !melee;
                events.ScheduleEvent(EVENT_LIGHTNING_WAVE, 15 * IN_MILLISECONDS);
                break;
            default:
                emerald_dragon_eventAI::ExecuteEvent(eventId);
                break;
            }
        }

    private:
        uint8 _stage;
        bool melee;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_ysondreAI(creature);
    }
};

/*
* ---
* --- Dragonspecific scripts and handling: LETHON
* ---
*
* @todo
* - Spell: Shadow bolt whirl casts needs custom handling (spellscript)
*/

enum LethonTexts
{
    SAY_LETHON_AGGRO = 0,
    SAY_LETHON_DRAW_SPIRIT = 1,
};

enum LethonSpells
{
    SPELL_DRAW_SPIRIT               = 24811,
    SPELL_SHADOW_BOLT_WHIRL         = 24834,
    SPELL_DARK_OFFERING             = 24804,
};

enum LethonCreatures
{
    NPC_SPIRIT_SHADE = 15261,
};

class boss_lethon_event : public CreatureScript
{
    public:
        boss_lethon_event() : CreatureScript("boss_lethon_event") { }

        struct boss_lethonAI : public emerald_dragon_eventAI
        {
            boss_lethonAI(Creature* creature) : emerald_dragon_eventAI(creature)
            {
            }

            void Reset() override
            {
                _stage = 1;
                emerald_dragon_eventAI::Reset();
                events.ScheduleEvent(EVENT_SHADOW_BOLT_WHIRL, 10000);
            }

            void EnterCombat(Unit* who) override
            {
                Talk(SAY_LETHON_AGGRO);
                emerald_dragon_eventAI::EnterCombat(who);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) override
            {
                if (!HealthAbovePct(100 - 25 * _stage))
                {
                    Talk(SAY_LETHON_DRAW_SPIRIT);
                    DoCastSelf(SPELL_DRAW_SPIRIT);
                    ++_stage;
                }
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell) override
            {
                if (spell->Id == SPELL_DRAW_SPIRIT && target->GetTypeId() == TYPEID_PLAYER)
                {
                    Position targetPos;
                    target->GetPosition(&targetPos);
                    if (auto shade = me->SummonCreature(NPC_SPIRIT_SHADE, targetPos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50000))
                        shade->SetDisplayId(target->GetDisplayId());
                }
            }

            void UpdateAI(uint32 diff) override
            {
                emerald_dragon_eventAI::UpdateAI(diff);

                if (me->IsInCombat() && !me->HasAura(SPELL_SHADOW_BOLT_WHIRL))
                    me->CastSpell((Unit*)NULL, SPELL_SHADOW_BOLT_WHIRL, false);
            }
        private:
            uint8 _stage;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_lethonAI(creature);
        }
};

/*
* ---
* --- Dragonspecific scripts and handling: EMERISS
* ---
*/

enum EmerissTexts
{
    SAY_EMERISS_AGGRO                   = 0,
    SAY_EMERISS_CAST_CORRUPTION         = 1,
};

enum EmerissSpells
{
    SPELL_PUTRID_MUSHROOM               = 24904,
    SPELL_CORRUPTION_OF_EARTH           = 24910,
    SPELL_VOLATILE_INFECTION            = 24928,
};

class boss_emeriss_event : public CreatureScript
{
public:
    boss_emeriss_event() : CreatureScript("boss_emeriss_event") { }

    struct boss_emerissAI : public emerald_dragon_eventAI
    {
        boss_emerissAI(Creature* creature) : emerald_dragon_eventAI(creature)
        {
        }

        void Reset() override
        {
            _stage = 1;
            emerald_dragon_eventAI::Reset();
            events.ScheduleEvent(EVENT_VOLATILE_INFECTION, 12000);
        }

        void KilledUnit(Unit* who) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER)
                DoCast(who, SPELL_PUTRID_MUSHROOM, true);
            emerald_dragon_eventAI::KilledUnit(who);
        }

        void EnterCombat(Unit* who) override
        {
            Talk(SAY_EMERISS_AGGRO);
            emerald_dragon_eventAI::EnterCombat(who);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) override
        {
            if (!HealthAbovePct(100 - 25 * _stage))
            {
                Talk(SAY_EMERISS_CAST_CORRUPTION);
                DoCastSelf(SPELL_CORRUPTION_OF_EARTH, true);
                ++_stage;
            }
        }

        void ExecuteEvent(uint32 eventId) override
        {
            switch (eventId)
            {
            case EVENT_VOLATILE_INFECTION:
                DoCastVictim(SPELL_VOLATILE_INFECTION);
                events.ScheduleEvent(EVENT_VOLATILE_INFECTION, 120000);
                break;
            default:
                emerald_dragon_eventAI::ExecuteEvent(eventId);
                break;
            }
        }

    private:
        uint8 _stage;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_emerissAI(creature);
    }
};

/*
* ---
* --- Dragonspecific scripts and handling: TAERAR
* ---
*/

enum TaerarTexts
{
    SAY_TAERAR_AGGRO                = 0,
    SAY_TAERAR_SUMMON_SHADES        = 1,
};

enum TaerarSpells
{
    NPC_TAERAR_SHADE                = 15302,
    SPELL_BELLOWING_ROAR            = 22686,
    SPELL_SHADE                     = 24313,
    SPELL_SUMMON_SHADE_1            = 24841,
    SPELL_SUMMON_SHADE_2            = 24842,
    SPELL_SUMMON_SHADE_3            = 24843,
    SPELL_ARCANE_BLAST              = 24857,
};

uint32 const TaerarShadeSpells[] =
{
    SPELL_SUMMON_SHADE_1, SPELL_SUMMON_SHADE_2, SPELL_SUMMON_SHADE_3
};

class boss_taerar_event : public CreatureScript
{
    public:
        boss_taerar_event() : CreatureScript("boss_taerar_event") { }

        struct boss_taerarAI : public emerald_dragon_eventAI
        {
            boss_taerarAI(Creature* creature) : emerald_dragon_eventAI(creature)
            {
            }

            void Reset() override
            {
                me->RemoveAurasDueToSpell(SPELL_SHADE);
                _stage = 1;

                shades.clear();
                _banished = false;
                _banishedTimer = 0;

                emerald_dragon_eventAI::Reset();
                events.ScheduleEvent(EVENT_ARCANE_BLAST, 12000);
                events.ScheduleEvent(EVENT_BELLOWING_ROAR, 30000);
            }

            void EnterCombat(Unit* who) override
            {
                Talk(SAY_TAERAR_AGGRO);
                emerald_dragon_eventAI::EnterCombat(who);
            }

            void JustSummoned(Creature* summon) override
            {
                if (summon->GetEntry() == NPC_TAERAR_SHADE)
                {
                    summon->SetMaxHealth(141120);
                    summon->SetHealth(summon->GetMaxHealth());
                    summon->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 4000.f);
                    summon->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 6000.f);
                    summon->UpdateDamagePhysical(BASE_ATTACK);

                    summons.Summon(summon);
                    shades.insert(summon->GetGUID());
                }
                else
                    emerald_dragon_eventAI::JustSummoned(summon);
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
            {
                auto itr = shades.find(summon->GetGUID());
                if (itr != shades.end())
                    shades.erase(itr);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) override
            {
                // At 75, 50 or 25 percent health, we need to activate the shades and go "banished"
                // Note: _stage holds the amount of times they have been summoned
                if (!_banished && !HealthAbovePct(100 - 25 * _stage))
                {
                    _banished = true;
                    _banishedTimer = 60000;

                    uint32 count = sizeof(TaerarShadeSpells) / sizeof(uint32);
                    for (uint32 i = 0; i < count; ++i)
                        DoCastVictim(TaerarShadeSpells[i], true);

                    me->InterruptNonMeleeSpells(false);
                    DoStopAttack();

                    Talk(SAY_TAERAR_SUMMON_SHADES);

                    DoCast(SPELL_SHADE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_PASSIVE);

                    ++_stage;
                }
            }

            void ExecuteEvent(uint32 eventId) override
            {
                switch (eventId)
                {
                    case EVENT_ARCANE_BLAST:
                        DoCast(SPELL_ARCANE_BLAST);
                        events.ScheduleEvent(EVENT_ARCANE_BLAST, urand(7000, 12000));
                        break;
                    case EVENT_BELLOWING_ROAR:
                        DoCast(SPELL_BELLOWING_ROAR);
                        events.ScheduleEvent(EVENT_BELLOWING_ROAR, urand(20000, 30000));
                        break;
                    default:
                        emerald_dragon_eventAI::ExecuteEvent(eventId);
                        break;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!me->IsInCombat())
                    return;

                for (auto itr : shades)
                    if (auto shade = sObjectAccessor->GetCreature(*me, itr))
                        if (!shade->IsInCombat() && !shade->IsInEvadeMode())
                        {
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true);
                            if (target)
                                shade->AI()->AttackStart(target);
                        }

                if (_banished)
                {
                    // If all three shades are dead, OR it has taken too long, end the current event and get Taerar back into business
                    if (_banishedTimer <= diff || shades.empty())
                    {
                        _banished = false;

                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        me->RemoveAurasDueToSpell(SPELL_SHADE);
                        me->SetReactState(REACT_AGGRESSIVE);
                    }
                    // _banishtimer has not expired, and we still have active shades:
                    else
                        _banishedTimer -= diff;

                    // Update the events before we return (handled under emerald_dragonAI::UpdateAI(diff); if we're not inside this check)
                    events.Update(diff);

                    return;
                }

                emerald_dragon_eventAI::UpdateAI(diff);
            }

        private:
            std::unordered_set<ObjectGuid> shades;
            bool   _banished;                              // used for shades activation testing
            uint32 _banishedTimer;                         // counter for banishment timeout
            uint8  _stage;                                 // check which "shade phase" we're at (75-50-25 percentage counters)
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_taerarAI(creature);
        }
};

class npc_ruby_event_controller : public CreatureScript
{
public:
    npc_ruby_event_controller() : CreatureScript("npc_ruby_event_controller") { }

    struct npc_ruby_event_controllerAI : public ScriptedAI
    {
        npc_ruby_event_controllerAI(Creature* creature) : ScriptedAI(creature)
        {
        }

        void Reset() override
        {
            if (auto ed = GetEmeraldDragon())
                me->SetFacingToObject(ed);
        }


        Creature* GetEmeraldDragon()
        {
            const std::vector<uint32> BOSS_ENTIRES = { 1010796, 1010797, 1010798, 1010799 };

            std::list<Creature*> dragons;
            for (uint32 entry : BOSS_ENTIRES)
                me->GetCreatureListWithEntryInGrid(dragons, entry, 50.f);

            if (dragons.empty())
                return nullptr;
            
            return (*std::min_element(dragons.begin(), dragons.end(), [this](Creature* c1, Creature* c2)
                { return c1->GetHomePosition().GetExactDistSq(&me->GetHomePosition()) <
                         c2->GetHomePosition().GetExactDistSq(&me->GetHomePosition()); }));
        }

        void SetData(uint32 /*id*/, uint32 value) override 
        { 
            switch (value)
            {
                case ACTION_START:
                case ACTION_START_PLAYER:
                {
                    if (auto dragon = GetEmeraldDragon())
                    {
                        druidGUIDs.clear();
                        const uint32 NPC_DREAMWARDEN = 1010803;

                        std::list<Creature*> druids;
                        me->GetCreatureListWithEntryInGrid(druids, NPC_DREAMWARDEN, 50.f);
                        for (auto itr : druids)
                        {
                            druidGUIDs.push_back(itr->GetGUID());
                            itr->SetImmuneToAll(false);
                        }
                        me->SetImmuneToAll(false);

                        dragon->AI()->SetGuidData(me->GetGUID());
                        dragon->SetImmuneToAll(false);
                        dragon->AI()->AttackStart(me);
                        dragon->RemoveAurasDueToSpell(SPELL_ENTANGLING_ROOTS_VISUAL);
                        if (value == ACTION_START)
                            dragon->AI()->Talk(2);
                        dragon->SetInCombatWithZone();
                    }
                    break;
                }
                case ACTION_DESPAWN:
                {
                    me->SetRespawnDelay(DAY);
                    me->DespawnOrUnsummon();

                    for (auto itr : druidGUIDs)
                        if (auto druid = sObjectAccessor->GetCreature(*me, itr))
                        {
                            druid->SetRespawnDelay(DAY);
                            druid->DespawnOrUnsummon();
                        }

                    break;
                }
                case ACTION_RESPAWN:
                {
                    me->SetRespawnAtHome(true);
                    me->Respawn(true);

                    for (auto itr : druidGUIDs)
                        if (auto druid = sObjectAccessor->GetCreature(*me, itr))
                        {
                            druid->SetRespawnAtHome(true);
                            druid->Respawn(true);
                        }


                    break;
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!me->IsInCombat())
            {
                if (!me->HasUnitState(UNIT_STATE_CASTING))
                {
                    const uint32 NATURE_CHANNELING = 13236;
                    DoCast(NATURE_CHANNELING);
                }
                return;
            }

            DoMeleeAttackIfReady();
        }

    private:
        std::vector<ObjectGuid> druidGUIDs;
    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 /*action*/) override
    {
        creature->AI()->SetData(0, ACTION_START_PLAYER);
        player->CLOSE_GOSSIP_MENU();

        return false; 
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ruby_event_controllerAI(creature);
    }
};

enum
{
    QUEST_REQUIRED_A            = 110213,
    QUEST_REQUIRED_H            = 110215,
    QUEST_FOLLOW_UP_A           = 110214,
    QUEST_FOLLOW_UP_H           = 110216,

    NPC_CITY_GUARD_A            = 1010790,
    NPC_CITY_GUARD_H            = 1010791,
};

class npc_ruby_opnening_tavern_trigger : public CreatureScript
{
    public:
        npc_ruby_opnening_tavern_trigger() : CreatureScript("npc_ruby_opnening_tavern_trigger") { }

        struct npc_ruby_opnening_tavern_triggerAI : public ScriptedAI
        {
            npc_ruby_opnening_tavern_triggerAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override { }

            void EnterCombat(Unit* /*who*/) override { }

            void MoveInLineOfSight(Unit* who) override
            {
                if (Player* plr = who->ToPlayer())
                {
                    uint32 required = plr->GetTeam() == ALLIANCE ? QUEST_REQUIRED_A : QUEST_REQUIRED_H;
                    uint32 follow = plr->GetTeam() == ALLIANCE ? QUEST_FOLLOW_UP_A : QUEST_FOLLOW_UP_H;
                    uint32 guardEntry = plr->GetTeam() == ALLIANCE ? NPC_CITY_GUARD_A : NPC_CITY_GUARD_H;
                    if (me->IsWithinDist(plr, 25.f) && plr->GetQuestStatus(required) == QUEST_STATUS_COMPLETE && plr->GetQuestStatus(follow) == QUEST_STATUS_NONE)
                    {
                        auto itr = spawnTime.find(plr->GetGUID().GetRawValue());
                        if (itr != spawnTime.end() && GetMSTimeDiffToNow(itr->second) < 2 * MINUTE * IN_MILLISECONDS)
                            return;
                        spawnTime[plr->GetGUID().GetRawValue()] = getMSTime();

                        float angle = me->GetAngle(plr);
                        float x, y, z;
                        me->GetNearPoint(me, x, y, z, DEFAULT_COMBAT_REACH, 10.f, angle);

                        if (auto summon = plr->SummonCreature(guardEntry, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN, 2 * MINUTE * IN_MILLISECONDS))
                        {
                            summon->GetMotionMaster()->MoveFollow(plr, 0, 0);
                            summon->AI()->Talk(3, plr);
                        }
                    }
                }

                ScriptedAI::MoveInLineOfSight(who);
            }

            std::map<uint64, uint32> spawnTime;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ruby_opnening_tavern_triggerAI(creature);
        }
};

class npc_ruby_event_hydraxis : public CreatureScript
{
    public:
        npc_ruby_event_hydraxis() : CreatureScript("npc_ruby_event_hydraxis") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            InstanceScript* instance = creature->GetInstanceScript();
            if (!instance)
                return true;

            if (instance->GetData(3) == DONE || instance->GetData(9) == DONE || instance->GetData(17) == DONE || instance->GetData(5) == DONE || instance->GetData(10) == DONE)
            {
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                return true;
            }

            return false;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 /*action*/) override
        {
            player->PlayerTalkClass->ClearMenus();

            InstanceScript* instance = creature->GetInstanceScript();
            if (!instance)
                return true;

            instance->SetData(1000, 1);
            player->CLOSE_GOSSIP_MENU();
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            return true; 
        }
};

class npc_ruby_event_malygos : public CreatureScript
{
    public:
        npc_ruby_event_malygos() : CreatureScript("npc_ruby_event_malygos") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            InstanceScript* instance = creature->GetInstanceScript();
            if (!instance)
                return true;

            if (instance->GetBossState(0) == DONE) // BOSS_RAZORGORE
            {
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                return true;
            }

            return false;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 /*action*/) override
        {
            uint32 menuId = player->PlayerTalkClass->GetGossipMenu().GetMenuId();
            player->PlayerTalkClass->ClearMenus();

            switch (menuId)
            {
                case 60218:
                    player->PrepareGossipMenu(creature, 60219);
                    player->SendPreparedGossip(creature);
                    break;
                case 60219:
                {
                    InstanceScript* instance = creature->GetInstanceScript();
                    if (!instance)
                        return true;

                    instance->SetData(1000, 1);
                    player->CLOSE_GOSSIP_MENU();
                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                }
            }

            return true; 
        }
};


enum MinigobData
{
    MAP_SSC                 = 548,

    EVENT_NEXT_STEP         = 1,

    GO_FAKE_PORTAL          = 411529,

    SPELL_TELEPORT_VISUAL   = 64446,
    SPELL_TELEPORT_DALARAN  = 53141,
    SPELL_POLYMORPH         = 61834,
    SPELL_PHASING_AURA      = 100022,
    SPELL_FLAME_BREATH      = 68970,

    NPC_HALION              = 1010818,
};

class npc_ruby_minigob : public CreatureScript
{
    public:
        npc_ruby_minigob() : CreatureScript("npc_ruby_minigob") { }

        struct npc_ruby_minigobAI : public ScriptedAI
        {
            npc_ruby_minigobAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override 
            {
                events.Reset();
                step = 0;
                
                me->SetVisible(false);
                events.ScheduleEvent(EVENT_NEXT_STEP, ((me->GetMapId() != MAP_SSC) ? 5 : 12) * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 next = events.ExecuteEvent())
                {
                    switch (next)
                    {
                        case EVENT_NEXT_STEP:
                            if (me->GetMapId() == MAP_SSC)
                            {
                                switch (step)
                                {
                                    case 0:
                                        me->SetVisible(true);
                                        DoCastSelf(SPELL_TELEPORT_VISUAL);
                                        events.ScheduleEvent(EVENT_NEXT_STEP, 2 * IN_MILLISECONDS);
                                        break;
                                    case 3:
                                    {
                                        for (Player& player : me->GetMap()->GetPlayers())
                                            DoCast(&player, SPELL_POLYMORPH, true);
                                        events.ScheduleEvent(EVENT_NEXT_STEP, 10 * IN_MILLISECONDS);
                                        break;
                                    }
                                    case 5:
                                        me->SummonGameObject(GO_FAKE_PORTAL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.f, 0.f, 0.f, 0.f, 0.f, HOUR * IN_MILLISECONDS);
                                        me->SetVisible(false);
                                        me->DespawnOrUnsummon(HOUR * IN_MILLISECONDS);
                                        break;
                                    default:
                                        Talk(step, 0);
                                        uint32 delay = 20 * IN_MILLISECONDS;
                                        if (step == 2)
                                            delay = 4 * IN_MILLISECONDS;
                                        events.ScheduleEvent(EVENT_NEXT_STEP, delay);
                                        break;
                                }
                            }
                            else
                            {
                                switch (step)
                                {
                                    case 0:
                                    case 1:
                                        if (Creature* halion = me->FindNearestCreature(NPC_HALION, 100.f))
                                            sCreatureTextMgr->SendChat(halion, step, nullptr);
                                        events.ScheduleEvent(EVENT_NEXT_STEP, (step == 0 ? 10 : 7) * IN_MILLISECONDS);
                                        break;
                                    case 2:
                                        me->SetVisible(true);
                                        DoCastSelf(SPELL_TELEPORT_VISUAL);
                                        events.ScheduleEvent(EVENT_NEXT_STEP, 1 * IN_MILLISECONDS);
                                        break;
                                    case 3:
                                        Talk(5, 0);
                                        events.ScheduleEvent(EVENT_NEXT_STEP, 2 * IN_MILLISECONDS);
                                        break;
                                    case 4:
                                        if (Creature* halion = me->FindNearestCreature(NPC_HALION, 100.f))
                                            halion->CastSpell(halion, SPELL_FLAME_BREATH);
                                        events.ScheduleEvent(EVENT_NEXT_STEP, 3 * IN_MILLISECONDS);
                                        break;
                                    case 5:
                                        {
                                            for (Player& player : me->GetMap()->GetPlayers())
                                                player.RemoveAurasDueToSpell(SPELL_PHASING_AURA);
                                        }
                                        events.ScheduleEvent(EVENT_NEXT_STEP, 1 * IN_MILLISECONDS);
                                        break;
                                    default:
                                        break;
                                }
                            }
                            step++;
                            if (me->GetMapId() != MAP_SSC && step > 5)
                            {
                                me->SetVisible(false);
                                step = 0;
                            }
                            break;
                    }
                }
            }

            EventMap events;
            uint32 step;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ruby_minigobAI(creature);
        }
};

class spell_ruby_event_phase : public SpellScriptLoader
{
    public:
        spell_ruby_event_phase() : SpellScriptLoader("spell_ruby_event_phase") { }

        class spell_ruby_event_phase_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ruby_event_phase_AuraScript);

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->CastSpell(target, SPELL_TELEPORT_DALARAN, true);
            }

            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_ruby_event_phase_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_MOD_INVISIBILITY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_ruby_event_phase_AuraScript();
        }
};

class go_ruby_event_portal : public GameObjectScript
{
    public:
        go_ruby_event_portal() : GameObjectScript("go_ruby_event_portal") { }

        bool OnGossipHello(Player* /*player*/, GameObject* /*go*/) override
        {
            return false;
        }
};

enum
{
    EVENT_CHECK_PLAYERS = 1,
};

class instance_event_aszhara_crater : public InstanceMapScript
{
    public:
        instance_event_aszhara_crater() : InstanceMapScript("instance_event_aszhara_crater", 37) { }

        struct instance_event_aszhara_crater_InstanceMapScript : public InstanceScript
        {
            instance_event_aszhara_crater_InstanceMapScript(Map* map) : InstanceScript(map) 
            { 
                events.ScheduleEvent(EVENT_CHECK_PLAYERS, 5 * IN_MILLISECONDS);
            }

            void Update(uint32 diff) 
            { 
                events.Update(diff);

                while (uint32 event = events.ExecuteEvent())
                {
                    switch (event)
                    {
                        case EVENT_CHECK_PLAYERS:
                        {
                            for (Player& player : instance->GetPlayers())
                                if (!player.IsGameMaster())
                                {
                                    if (!sGameEventMgr->IsActiveEvent(116))
                                        player.TeleportTo(player.m_homebindMapId, player.m_homebindX, player.m_homebindY, player.m_homebindZ, player.GetOrientation());
                                    else if (player.GetPositionX() < 650.f && player.GetQuestStatus(110206) == QUEST_STATUS_NONE)
                                        player.CastSpell(&player, 100019);
                                }
                            events.ScheduleEvent(EVENT_CHECK_PLAYERS, 5 * IN_MILLISECONDS);
                            break;
                        }
                    }
                }
            }


        protected:
            EventMap events;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_event_aszhara_crater_InstanceMapScript(map);
        }
};

class spell_ruby_event_scaling : public SpellScriptLoader
{
    public:
        spell_ruby_event_scaling(char const* name, float multiplier) : SpellScriptLoader(name), _multiplier(multiplier) { }

        class spell_ruby_event_scaling_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ruby_event_scaling_SpellScript)

        public:
            spell_ruby_event_scaling_SpellScript(float multiplier) : SpellScript(), _multiplier(multiplier) { }

            void RecalculateDamage()
            {
                if (GetCaster()->GetMapId() != MAP_ASZHARA_CRATER)
                    return;

                SetHitDamage(GetHitDamage() * _multiplier);
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_ruby_event_scaling_SpellScript::RecalculateDamage);
            }

        private:
            float _multiplier;
        };

        class spell_ruby_event_scaling_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ruby_event_scaling_AuraScript);

        public:
            spell_ruby_event_scaling_AuraScript(float multiplier) : AuraScript(), _multiplier(multiplier) { }

            void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (GetOwner()->GetMapId() != MAP_ASZHARA_CRATER)
                    return;

                amount = int32(amount * _multiplier);
            }

            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_ruby_event_scaling_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_ruby_event_scaling_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_ruby_event_scaling_AuraScript::CalculateAmount, EFFECT_2, SPELL_AURA_PERIODIC_DAMAGE);
            }

        private:
            float _multiplier;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_ruby_event_scaling_AuraScript(_multiplier);
        }

        SpellScript* GetSpellScript() const override
        {
            return new spell_ruby_event_scaling_SpellScript(_multiplier);
        }

    private:
        float _multiplier;
};

enum
{
    EVENT_POISON_BOLT_VOLLEY         = 1,
    EVENT_FUNGAL_CREEP               = 2,
    EVENT_WARNING                    = 3,

    SPELL_POISON_BOLT_VOLLEY         = 29293,
    SPELL_FUNGAL_CREEP               = 29232,
};

class npc_ruby_event_spore : public CreatureScript
{
    public:
        npc_ruby_event_spore() : CreatureScript("npc_ruby_event_spore") { }

        struct npc_ruby_event_sporeAI : public ScriptedAI
        {
            npc_ruby_event_sporeAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void Reset() override
            {
                say = true;
                events.Reset();
                events.ScheduleEvent(EVENT_POISON_BOLT_VOLLEY, 4 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WARNING, 20 * IN_MILLISECONDS);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage) override
            {
                damage = 0;
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (say)
                {
                    say = false;
                    Talk(0, me->GetVictim());
                }

                if (uint32 ev = events.ExecuteEvent())
                {
                    switch (ev)
                    {
                        case EVENT_POISON_BOLT_VOLLEY:
                            DoCastAOE(SPELL_POISON_BOLT_VOLLEY, true);
                            events.ScheduleEvent(EVENT_POISON_BOLT_VOLLEY, 2 * IN_MILLISECONDS);
                            break;
                        case EVENT_WARNING:
                            events.ScheduleEvent(EVENT_FUNGAL_CREEP, 10 * IN_MILLISECONDS);
                            events.CancelEvent(EVENT_POISON_BOLT_VOLLEY);
                            Talk(1);
                            break;
                        case EVENT_FUNGAL_CREEP:
                            {
                                for (Player& player : me->GetMap()->GetPlayers())
                                {
                                    if (!me->IsWithinDist(&player, 15.f))
                                        continue;

                                    player.AddAura(SPELL_FUNGAL_CREEP, &player);
                                }
                            }
                            me->DespawnOrUnsummon();
                            break;
                    }
                    
                }
            }

        private:
            bool say;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ruby_event_sporeAI(creature);
        }
};

class spell_ruby_event_timewalking_npc : public SpellScriptLoader
{
    public:
        spell_ruby_event_timewalking_npc() : SpellScriptLoader("spell_ruby_event_timewalking_npc") { }

        class spell_ruby_event_timewalking_npc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ruby_event_timewalking_npc_AuraScript);

            void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                    if (Player* owner = target->GetCharmerOrOwnerPlayerOrPlayerItself())
                        target->RemoveAurasDueToSpell(GetId());
            }

            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_ruby_event_timewalking_npc_AuraScript::HandleOnEffectApply, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_ruby_event_timewalking_npc_AuraScript();
        }
};

constexpr bool IsRubyOpeningActive = false;

void AddSC_custom_ruby_opening()
{
    if constexpr(IsRubyOpeningActive)
    {
        new boss_ysondre_event();
        new boss_taerar_event();
        new boss_emeriss_event();
        new boss_lethon_event();
        new npc_ruby_event_controller();
        new npc_ruby_opnening_tavern_trigger();
        new npc_ruby_event_hydraxis();
        new npc_ruby_event_malygos();
        new npc_ruby_minigob();
        new go_ruby_event_portal();
        new spell_ruby_event_phase();
        new spell_ruby_event_scaling("spell_ruby_event_03_0_multiplier", 3.0f);
        new spell_ruby_event_scaling("spell_ruby_event_04_0_multiplier", 4.0f);
        new spell_ruby_event_scaling("spell_ruby_event_04_5_multiplier", 4.5f);
        new spell_ruby_event_scaling("spell_ruby_event_05_0_multiplier", 5.0f);
        new spell_ruby_event_scaling("spell_ruby_event_10_0_multiplier", 10.f);
        new spell_ruby_event_scaling("spell_ruby_event_12_0_multiplier", 12.f);
        new spell_ruby_event_scaling("spell_ruby_event_17_0_multiplier", 17.f);
        new spell_ruby_event_scaling("spell_ruby_event_30_0_multiplier", 30.f);
        new instance_event_aszhara_crater();
        new spell_ruby_event_timewalking_npc();
        new npc_ruby_event_spore();
    }
}
