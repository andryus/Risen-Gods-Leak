/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
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

/* ScriptData
SDName: Boss_Headless_Horseman
SD%Complete:
SDComment:
SDCategory: Scarlet Monastery
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellMgr.h"
#include "scarlet_monastery.h"
#include "LFGMgr.h"
#include "Player.h"
#include "Group.h"
#include "SpellInfo.h"
#include "WorldSession.h"

enum Yells
{
    SAY_ENTRANCE                = 0,
    SAY_REJOINED                = 1,
    SAY_CONFLAGRATION           = 2,
    SAY_SPROUTING_PUMPKINS      = 3,
    SAY_DEATH                   = 4,

    SAY_LOST_HEAD               = 0,
    SAY_PLAYER_DEATH            = 1
};

enum Actions
{
    ACTION_SWITCH_PHASE         = 1
};

enum Entries
{
    NPC_HORSEMAN                = 23682,
    NPC_HEAD                    = 23775,
    NPC_PULSING_PUMPKIN         = 23694,
    NPC_PUMPKIN_FIEND           = 23545,
    NPC_SIR_THOMAS              = 23904,
};

enum Spells
{
    SPELL_SUMMON_PUMPKIN        = 52236, // 8s trigger 42394

    SPELL_IMMUNED               = 42556,
    SPELL_BODY_REGEN            = 42403, // regenerate health plus model change to unhorsed
    SPELL_CONFUSED              = 43105,
    SPELL_HEAL_BODY             = 43306, // heal to 100%
    SPELL_CLEAVE                = 42587,
    SPELL_WHIRLWIND             = 43116,
    SPELL_CONFLAGRATION         = 42380,

    SPELL_FLYING_HEAD           = 42399, // flying head visual
    SPELL_HEAD                  = 42413, // horseman head visual
    SPELL_HEAD_LANDS            = 42400,
    SPELL_CREATE_PUMPKIN_TREATS = 42754,
    SPELL_RHYME_BIG             = 42909,

    SPELL_BURNING               = 42971,
};

enum quests
{
    QUEST_CALL_THE_HEADLESS_HORSEMAN          = 11392,
};

uint32 randomLaugh[]            = {11965, 11975, 11976};

static Position flightPos[]=
{
    {1764.957f, 1347.432f, 18.7f, 6.029f},
    {1774.625f, 1345.035f, 20.8f, 6.081f},
    {1789.114f, 1341.439f, 26.8f, 0.198f},
    {1798.446f, 1345.865f, 30.8f, 1.781f},
    {1791.671f, 1360.825f, 30.1f, 2.766f},
    {1777.449f, 1364.652f, 25.1f, 2.911f},
    {1770.126f, 1361.402f, 20.7f, 4.093f},
    {1772.743f, 1354.941f, 18.4f, 5.841f}
};

static char const* Text[2][5]=
{
    {
        "Horseman rise...",
        "Your time is nigh...",
        "You felt death once...",
        "Now, know demise!"
    },
    {
        "Erhebe dich, Reiter...",
        "Deine Zeit ist gekommen...",
        "Heut' Nacht sei dein Leben...",
        "Auf ewig genommen!"
    }
};

#define GOSSIP_OPTION "Call the Headless Horseman."

class boss_headless_horseman : public CreatureScript
{
    public:
        boss_headless_horseman() : CreatureScript("boss_headless_horseman") { }

        struct boss_headless_horsemanAI : public ScriptedAI
        {
            boss_headless_horsemanAI(Creature* creature) : ScriptedAI(creature), _summons(me) { }
            
            void Reset() override
            {
                _summons.DespawnAll();

                me->SetVisible(false);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_REMOVE_CLIENT_CONTROL | UNIT_FLAG_NOT_SELECTABLE);

                me->SetUnitMovementFlags(MOVEMENTFLAG_NONE);
                me->SetDisableGravity(false);
                me->SetSpeedRate(MOVE_RUN, 1.0f);
                me->SetCorpseDelay(60);

                wpCount    = 0;
                introCount = 0;
                wpReached  = false;
                phase      = 0;

                introTimer   = 1 * IN_MILLISECONDS;
                laughTimer   = 5 * IN_MILLISECONDS;
                cleaveTimer  = 3 * IN_MILLISECONDS;
                summonTimer  = 1 * IN_MILLISECONDS;
                conflagTimer = 4 * IN_MILLISECONDS;

                DoCastSelf(SPELL_HEAD, true);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE || id != wpCount)
                    return;

                if (id < 7)
                {
                    ++wpCount;
                    wpReached = true;
                }
                else // start fighting
                {
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_REMOVE_CLIENT_CONTROL | UNIT_FLAG_NOT_SELECTABLE);
                    me->SetDisableGravity(true);
                    DoZoneInCombat(me);

                    if (me->GetVictim())
                        me->GetMotionMaster()->MoveChase(me->GetVictim());

                    Talk(SAY_ENTRANCE);
                }
            }

            void EnterCombat(Unit* /*who*/) override
            {
                DoZoneInCombat(me);
            }

            void KilledUnit(Unit* victim) override
            {
                if (!victim->ToPlayer())
                    return;

                Talk(SAY_PLAYER_DEATH);
            }

            void JustSummoned(Creature* summon) override
            {
                _summons.Summon(summon);
                summon->SetInCombatWithZone();

                Talk(SAY_SPROUTING_PUMPKINS);
            }

            void JustDied(Unit* /*killer*/) override
            {
                Talk(SAY_DEATH);
                _summons.DespawnAll();

                Map* map = me->GetMap();
                if (map && map->IsDungeon())
                {
                    for (Player& player : map->GetPlayers()) 
                    {
                        if (player.IsGameMaster() || !player.GetGroup())
                            continue;

                        sLFGMgr->FinishDungeon(player.GetGroup()->GetGUID(), 285);
                        return;
                    }
                }

                DoCastSelf(SPELL_BURNING, true);
                me->SummonCreature(NPC_SIR_THOMAS, 1762.863f, 1345.217f, 17.9f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 60*IN_MILLISECONDS);
            }

            void DamageTaken(Unit* /*attacker*/, uint32 &damage) override
            {
                if (phase > 3)
                {
                    me->RemoveAllAuras();
                    return;
                }

                if (me->HasAura(SPELL_BODY_REGEN))
                {
                    damage = 0;
                    return;
                }


                if (damage >= me->GetHealth())
                {
                    damage = me->GetHealth() - 1;

                    DoCastSelf(SPELL_IMMUNED, true);
                    DoCastSelf(SPELL_BODY_REGEN, true);
                    DoCastSelf(SPELL_CONFUSED, true);

                    Position randomPos;
                    me->GetRandomNearPosition(randomPos, 20.0f);

                    if (Creature* head = me->SummonCreature(NPC_HEAD, randomPos))
                    {
                        head->AI()->SetData(0, phase);

                        switch (phase)
                        {
                            case 2: 
                                head->SetHealth(uint32(head->GetMaxHealth() * 2 / 3)); 
                                break;
                            case 3: 
                                head->SetHealth(uint32(head->GetMaxHealth() / 3)); 
                                break;
                            default:
                                break;
                        }
                    }

                    me->RemoveAurasDueToSpell(SPELL_HEAD);
                }
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_SWITCH_PHASE:
                        me->RemoveAllAuras();
                        DoCastSelf(SPELL_HEAL_BODY, true);
                        DoCastSelf(SPELL_HEAD, true);
                        ++phase;
                        if (phase > 3)
                            me->DealDamage(me, me->GetHealth());
                        else
                            Talk(SAY_REJOINED);
                        break;
                    default:
                        break;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                if (phase == 0)
                {
                    if (introTimer <= diff)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            if (Player* player = target->ToPlayer())
                            {
                                if (introCount < 3)
                                {
                                    if (player->GetSession()->GetSessionDbLocaleIndex() == 0)
                                        player->Say(Text[0][introCount], 0);
                                    else
                                        player->Say(Text[1][introCount], 0);

                                } else {
                                    DoCastSelf(SPELL_RHYME_BIG, true);

                                    if (player->GetSession()->GetSessionDbLocaleIndex() == 0)
                                        player->Say(Text[0][introCount], 0);
                                    else
                                        player->Say(Text[1][introCount], 0);

                                    player->HandleEmoteCommand(ANIM_EMOTE_SHOUT);
                                    phase = 1;
                                    wpReached = true;
                                    me->SetVisible(true);
                                }
                            }
                        introTimer = 3 * IN_MILLISECONDS;
                        ++introCount;
                    }
                    else
                        introTimer -= diff;

                    return;
                }

                if (wpReached)
                {
                    wpReached = false;
                    me->GetMotionMaster()->MovePoint(wpCount, flightPos[wpCount]);
                }

                if (me->HasAura(SPELL_BODY_REGEN))
                {
                    if (me->IsFullHealth())
                    {
                        me->RemoveAllAuras();
                        DoCastSelf(SPELL_HEAD, true);

                        if (Creature* head = me->FindNearestCreature(NPC_HEAD, 250.0f, true))
                        {
                            head->SetFullHealth();
                            head->RemoveAllAuras();
                            head->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            head->DespawnOrUnsummon(3 * IN_MILLISECONDS);
                            head->CastSpell(me, SPELL_FLYING_HEAD, true);
                        }
                    }
                    else if (!me->HasAura(SPELL_WHIRLWIND) && me->GetHealthPct() > 10.0f)
                        DoCastSelf(SPELL_WHIRLWIND, true);

                    return;
                }

                if (laughTimer <= diff)
                {
                    DoPlaySoundToSet(me, randomLaugh[rand32()%3]);
                    laughTimer = urand(11, 22) * IN_MILLISECONDS;
                }
                else
                    laughTimer -= diff;

                if (me->HasReactState(REACT_PASSIVE))
                    return;

                if (cleaveTimer <= diff)
                {
                    DoCastVictim(SPELL_CLEAVE);
                    cleaveTimer = urand(2, 6) *IN_MILLISECONDS;
                }
                else
                    cleaveTimer -= diff;

                switch (phase)
                {
                    case 2:
                        if (conflagTimer <= diff)
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 30.0f, true))
                            {
                                DoCast(target, SPELL_CONFLAGRATION);
                                Talk(SAY_CONFLAGRATION);
                            }
                            conflagTimer = urand(8, 12) * IN_MILLISECONDS;
                        }
                        else
                            conflagTimer -= diff;
                        break;
                    case 3:
                        if (summonTimer <= diff)
                        {
                            DoCastSelf(SPELL_SUMMON_PUMPKIN, true);
                            summonTimer = 15 * IN_MILLISECONDS;
                        }
                        else
                            summonTimer -= diff;
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }

            private:
                SummonList _summons;
                uint8 phase;
                uint8 wpCount;
                uint8 introCount;
                uint32 introTimer;
                uint32 laughTimer;
                uint32 cleaveTimer;
                uint32 summonTimer;
                uint32 conflagTimer;
                bool wpReached;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_headless_horsemanAI(creature);
        }
};


class npc_horseman_head : public CreatureScript
{
    public:
        npc_horseman_head() : CreatureScript("npc_head") { }

        struct npc_horseman_headAI : public ScriptedAI
        {
            npc_horseman_headAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetDisplayId(21908);
                me->SetReactState(REACT_PASSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED | UNIT_FLAG_NOT_SELECTABLE);
                me->GetMotionMaster()->MoveRandom(30.0f);
                DoCastSelf(SPELL_HEAD, true);
                DoCastSelf(SPELL_HEAD_LANDS, true);
                Talk(SAY_LOST_HEAD);
            }

            void SetData(uint32 /*type*/, uint32 data) override
            {
                _phase = data;
            }

            void DamageTaken(Unit* /*attacker*/, uint32 &damage) override
            {
                int32 healthPct;

                switch (_phase)
                {
                    case 1: healthPct = 66; 
                        break;
                    case 2: healthPct = 33; 
                        break;
                    default: 
                        healthPct = 1; 
                        break;
                }

                if (me->HealthBelowPctDamaged(healthPct, damage) || damage >= me->GetHealth())
                {
                    damage = 0;
                    me->RemoveAllAuras();
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    me->DespawnOrUnsummon(3 * IN_MILLISECONDS);

                    if (me->ToTempSummon())
                        if (Unit* horseman = me->ToTempSummon()->GetSummoner())
                        {
                            if (_phase < 3)
                                DoCast(horseman, SPELL_FLYING_HEAD, true);
                            horseman->ToCreature()->AI()->DoAction(ACTION_SWITCH_PHASE);
                        }
                }
            }

        private:
            uint8 _phase;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_horseman_headAI(creature);
        }
};

class go_loosely_turned_soil : public GameObjectScript
{
    public:
        go_loosely_turned_soil() : GameObjectScript("go_loosely_turned_soil") { }
            
        bool OnQuestReward(Player* player, GameObject* go, Quest const* Quest, uint32 /*opt*/)
        {
            if (Quest->GetQuestId() == QUEST_CALL_THE_HEADLESS_HORSEMAN)
            {
                if (!go->FindNearestCreature(NPC_HORSEMAN, 100.0f))
                    if (Unit* horseman = go->SummonCreature(NPC_HORSEMAN, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), 0))
                    {
                        horseman->SetDisableGravity(false);
                        horseman->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
                        horseman->SetSpeedRate(MOVE_RUN, 1.0f);
                        horseman->ToCreature()->AI()->AttackStart(player);
                        horseman->GetThreatManager().AddThreat(player, 1000.f);
                    }
            }
            return true;
        }
};

void AddSC_boss_headless_horseman()
{
    new boss_headless_horseman();
    new npc_horseman_head();
    new go_loosely_turned_soil();
}
