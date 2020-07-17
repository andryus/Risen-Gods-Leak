/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

/*
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "npc_pet_gen_".
 */

 /* ContentData
 npc_pet_gen_baby_blizzard_bear     100%    Baby Blizzard Bear sits down occasionally
 npc_pet_gen_egbert                 100%    Egbert run's around
 npc_pet_gen_pandaren_monk          100%    Pandaren Monk drinks and bows with you
 npc_pet_gen_mojo                   100%    Mojo follows you when you kiss it
 EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "PetAI.h"
#include "PassiveAI.h"
#include "Pet.h"
#include "CreatureTextMgr.h"
#include "GameEventMgr.h"

enum Mojo
{
    SAY_MOJO                = 0,

    SPELL_FEELING_FROGGY    = 43906,
    SPELL_SEDUCTION_VISUAL  = 43919
};

class npc_pet_gen_mojo : public CreatureScript
{
    public:
        npc_pet_gen_mojo() : CreatureScript("npc_pet_gen_mojo") { }

        struct npc_pet_gen_mojoAI : public ScriptedAI
        {
            npc_pet_gen_mojoAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _victimGUID.Clear();

                if (Unit* owner = me->GetOwner())
                    me->GetMotionMaster()->MoveFollow(owner, 0.0f, 0.0f);
            }

            void EnterCombat(Unit* /*who*/) override { }
            void UpdateAI(uint32 /*diff*/) override { }

            void ReceiveEmote(Player* player, uint32 emote) override
            {
                me->HandleEmoteCommand(emote);
                Unit* owner = me->GetOwner();
                if (emote != TEXT_EMOTE_KISS || !owner || owner->GetTypeId() != TYPEID_PLAYER ||
                    owner->ToPlayer()->GetTeam() != player->GetTeam())
                {
                    return;
                }

                Talk(SAY_MOJO, player);

                if (_victimGUID)
                    if (Player* victim = ObjectAccessor::GetPlayer(*me, _victimGUID))
                        victim->RemoveAura(SPELL_FEELING_FROGGY);

                _victimGUID = player->GetGUID();

                player->CastSpell(player, SPELL_FEELING_FROGGY, true);
                DoCastSelf(SPELL_SEDUCTION_VISUAL, true);
                me->GetMotionMaster()->MoveFollow(player, 0.0f, 0.0f);
            }

        private:
            ObjectGuid _victimGUID;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_gen_mojoAI(creature);
        }
};

class npc_pet_gen_fetch_ball : public CreatureScript
{
    public:
        npc_pet_gen_fetch_ball() : CreatureScript("npc_pet_gen_fetch_ball") { }

        struct npc_pet_gen_fetch_ballAI : public NullCreatureAI
        {
            npc_pet_gen_fetch_ballAI(Creature *c) : NullCreatureAI(c) { }

            void IsSummonedBy(Unit* summoner) override
            {
                if (!summoner)
                    return;

                me->SetOwnerGUID(summoner->GetGUID());
                checkTimer = 0;
                targetGUID.Clear();
                DoCastSelf(48649 /*SPELL_PET_TOY_FETCH_BALL_COME_HERE*/, true);
            }

            void SpellHitTarget(Unit* target, const SpellInfo* spellInfo) override
            {
                if (spellInfo->Id == 48649 /*SPELL_PET_TOY_FETCH_BALL_COME_HERE*/)
                {
                    target->GetMotionMaster()->MovePoint(50, me->GetHomePosition());
                    targetGUID = target->GetGUID();
                }
            }

            void UpdateAI(uint32 diff) override
            {
                checkTimer += diff;
                if (checkTimer >= 1000)
                {
                    checkTimer = 0;
                    if (Creature* target = ObjectAccessor::GetCreature(*me, targetGUID))
                        if (me->GetDistance2d(target) < 2.0f)
                        {
                            target->OnEvade();
                            target->CastSpell(target, 48708 /*SPELL_PET_TOY_FETCH_BALL_HAS_BALL*/, true);
                            me->DespawnOrUnsummon(1);
                        }
                }
            }

        private:

            uint32 checkTimer;
            ObjectGuid targetGUID;
        };

        CreatureAI* GetAI(Creature* pCreature) const override
        {
            return new npc_pet_gen_fetch_ballAI(pCreature);
        }
};

enum ToxicWasteling
{
    SPELL_GROWTH = 71854
};

class npc_pet_gen_toxic_wasteling : public CreatureScript
{
    public:
        npc_pet_gen_toxic_wasteling() : CreatureScript("npc_pet_gen_toxic_wasteling") { }

        struct npc_pet_gen_toxic_wastelingAI : public PassiveAI
        {
            npc_pet_gen_toxic_wastelingAI(Creature *c) : PassiveAI(c) { }
            
            void Reset() override
            { 
                checkTimer = 3 * IN_MILLISECONDS;
            }

            void EnterEvadeMode(EvadeReason /*why*/) override { }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == EFFECT_MOTION_TYPE && id == 1)
                    checkTimer = 1;
            }

            void UpdateAI(uint32 diff) override
            {
                if (checkTimer)
                {
                    if (checkTimer == 1)
                        me->GetMotionMaster()->MovementExpired(false);
                    checkTimer += diff;
                    if (checkTimer >= 3 * IN_MILLISECONDS)
                    {
                        if (Unit* owner = me->GetCharmerOrOwner())
                        {
                            me->GetMotionMaster()->Clear(false);
                            me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle(), MOTION_SLOT_ACTIVE);
                        }
                        me->AddAura(SPELL_GROWTH, me);
                        checkTimer = 0;
                    }
                }
            }

        private:
            uint32 checkTimer;
        };

        CreatureAI* GetAI(Creature* pCreature) const override
        {
            return new npc_pet_gen_toxic_wastelingAI(pCreature);
        }
};

enum BabyBlizzardBearMisc
{
    SPELL_BBB_PET_SIT = 61853,
    EVENT_BBB_PET_SIT = 1,
    EVENT_BBB_PET_SIT_INTER = 2
};

class npc_pet_gen_baby_blizzard_bear : public CreatureScript
{
    public:
        npc_pet_gen_baby_blizzard_bear() : CreatureScript("npc_pet_gen_baby_blizzard_bear") {}

        struct npc_pet_gen_baby_blizzard_bearAI : public NullCreatureAI
        {
            npc_pet_gen_baby_blizzard_bearAI(Creature* creature) : NullCreatureAI(creature)
            {
                if (Unit* owner = me->GetCharmerOrOwner())
                    me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle());
                _events.ScheduleEvent(EVENT_BBB_PET_SIT, urand(10, 30) * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                if (Unit* owner = me->GetCharmerOrOwner())
                    if (!me->IsWithinDist(owner, 25.f))
                        me->InterruptSpell(CURRENT_CHANNELED_SPELL);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BBB_PET_SIT:
                            DoCastSelf(SPELL_BBB_PET_SIT, false);
                            _events.ScheduleEvent(EVENT_BBB_PET_SIT_INTER, urand(15, 30) * IN_MILLISECONDS);
                            break;
                        case EVENT_BBB_PET_SIT_INTER:
                            me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                            _events.ScheduleEvent(EVENT_BBB_PET_SIT, urand(10, 30) * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_gen_baby_blizzard_bearAI(creature);
        }
};

enum EgbertMisc
{
    SPELL_EGBERT = 40669,
    EVENT_RETURN = 3
};

class npc_egbert : public CreatureScript
{
    public:
        npc_egbert() : CreatureScript("npc_egbert") {}

        struct npc_egbertAI : public NullCreatureAI
        {
            npc_egbertAI(Creature* creature) : NullCreatureAI(creature)
            {
                if (Unit* owner = me->GetCharmerOrOwner())
                    if (owner->GetMap()->GetEntry()->addon > 1)
                        me->SetCanFly(true);
            }

            void Reset() override
            {
                _events.Reset();
                if (Unit* owner = me->GetCharmerOrOwner())
                    me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle());
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                if (Unit* owner = me->GetCharmerOrOwner())
                {
                    if (!me->IsWithinDist(owner, 40.f))
                    {
                        me->RemoveAura(SPELL_EGBERT);
                        me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle());
                    }
                }

                if (me->HasAura(SPELL_EGBERT))
                    _events.ScheduleEvent(EVENT_RETURN, urand(5, 20) * IN_MILLISECONDS);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_RETURN:
                            me->RemoveAura(SPELL_EGBERT);
                            break;
                        default:
                            break;
                    }
                }
            }
        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_egbertAI(creature);
        }
};

/*#####
# npc_spring_rabbit
#####*/

enum rabbitSpells
{
    SPELL_SPRING_FLING            = 61875,
    SPELL_SPRING_RABBIT_JUMP      = 61724,
    SPELL_SPRING_RABBIT_WANDER    = 61726,
    SPELL_SUMMON_BABY_BUNNY       = 61727,
    SPELL_SPRING_RABBIT_IN_LOVE   = 61728,
    NPC_SPRING_RABBIT             = 32791
};

class npc_spring_rabbit : public CreatureScript
{
    public:
        npc_spring_rabbit() : CreatureScript("npc_spring_rabbit") { }
        
        struct npc_spring_rabbitAI : public ScriptedAI
        {
            npc_spring_rabbitAI(Creature* creature) : ScriptedAI(creature) { }
            
            void Reset() override
            {
                inLove = false;
                rabbitGUID.Clear();
                jumpTimer = urand(5000, 10000);
                bunnyTimer = urand(10000, 20000);
                searchTimer = urand(5000, 10000);
                if (Unit* owner = me->GetOwner())
                    me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
            }

            void EnterCombat(Unit* /*who*/) override { }

            void DoAction(int32 /*param*/) override
            {
                inLove = true;
                if (Unit* owner = me->GetOwner())
                    owner->CastSpell(owner, SPELL_SPRING_FLING, true);
            }

            void UpdateAI(uint32 diff) override
            {
                if (inLove)
                {
                    if (jumpTimer <= diff)
                    {
                        if (Unit* rabbit = ObjectAccessor::GetUnit(*me, rabbitGUID))
                            DoCast(rabbit, SPELL_SPRING_RABBIT_JUMP);
                        jumpTimer = urand(5000, 10000);
                    }
                    else jumpTimer -= diff;

                    if (bunnyTimer <= diff)
                    {
                        DoCast(SPELL_SUMMON_BABY_BUNNY);
                        bunnyTimer = urand(20000, 40000);
                    }
                    else bunnyTimer -= diff;
                }
                else
                {
                    if (searchTimer <= diff)
                    {
                        if (Creature* rabbit = me->FindNearestCreature(NPC_SPRING_RABBIT, 10.0f))
                        {
                            if (rabbit == me || rabbit->HasAura(SPELL_SPRING_RABBIT_IN_LOVE))
                                return;

                            me->AddAura(SPELL_SPRING_RABBIT_IN_LOVE, me);
                            DoAction(1);
                            rabbit->AddAura(SPELL_SPRING_RABBIT_IN_LOVE, rabbit);
                            rabbit->AI()->DoAction(1);
                            rabbit->CastSpell(rabbit, SPELL_SPRING_RABBIT_JUMP, true);
                            rabbitGUID = rabbit->GetGUID();
                        }
                        searchTimer = urand(5000, 10000);
                    }
                    else searchTimer -= diff;
                }
            }

        private:
            bool inLove;
            uint32 jumpTimer;
            uint32 bunnyTimer;
            uint32 searchTimer;
            ObjectGuid rabbitGUID;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_spring_rabbitAI(creature);
        }
};

/*######
## npc_pet_vargoths
######*/

class npc_pet_vargoths : public CreatureScript
{
    public:
        npc_pet_vargoths() : CreatureScript("npc_pet_vargoths") { }

        struct npc_pet_vargothsAI : public ScriptedAI
        {
            npc_pet_vargothsAI(Creature* creature) : ScriptedAI(creature) {}

            void ReceiveEmote(Player* player, uint32 emote) override
            {
                if (me->IsWithinLOS(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()) && me->IsWithinDistInMap(player, 30.0f))
                {
                    me->SetFacingToObject(player);

                    switch (emote)
                    {
                        case TEXT_EMOTE_DANCE:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                            break;
                        case TEXT_EMOTE_SEXY:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_FLEX);
                            break;
                        case TEXT_EMOTE_WAVE:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                            break;
                        case TEXT_EMOTE_POINT:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                            break;
                        case TEXT_EMOTE_BOW:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
                            break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_vargothsAI(creature);
        }
};

/*######
## npc_vanquished_tentacle
######*/

enum VanquishedTentacleSpells
{
    SPELL_CRUSH = 65038,
    SPELL_ACIDIC_POISON = 65035,
    SPELL_CONSTRICTING_REND = 65033,

};

enum VanquishedTentacle
{
    Constrictor,
    Corruptor,
    Crusher
};


class npc_vanquished_tentacle : public CreatureScript
{
    public:
        npc_vanquished_tentacle() : CreatureScript("npc_vanquished_tentacle") { }

        struct npc_vanquished_tentacleAI : public ScriptedAI
        {
            npc_vanquished_tentacleAI(Creature* creature) : ScriptedAI(creature) {}
            
            void Reset() override
            {
                m_uiSpellTimer = 0;                                // 0 Sekunden-Start Timer

                me->RestoreFaction();

                // Reset-Timer f�llen, jenachdem welche Creatur erschaffen wurde
                if (me->GetEntry() == 34266) {
                    // Constrictor
                    m_uiSpellResetTimer = 4400;
                    self = Constrictor;
                }
                else if (me->GetEntry() == 34265) {
                    // Corrupter
                    m_uiSpellResetTimer = 2100;
                    // Base-Attack-Timer auf 20 Sekunden setzen, da nur etwa 5% Meele
                    me->SetAttackTime(BASE_ATTACK, 20000);
                    me->setAttackTimer(BASE_ATTACK, 20000);
                    self = Corruptor;
                }
                else {
                    // Crusher (34264)
                    m_uiSpellResetTimer = 5900;
                    self = Crusher;
                }
            }


            void UpdateAI(uint32 uiDiff) override
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                //Spell Attack
                if (m_uiSpellTimer <= uiDiff)
                {
                    // Spell-Casten jenachdem welche Creatur man ist
                    if (self == Constrictor)
                        DoCastVictim(SPELL_CONSTRICTING_REND);
                    else if (self == Corruptor)
                        DoCastVictim(SPELL_ACIDIC_POISON);
                    else
                        DoCastVictim(SPELL_CRUSH);


                    // Cast-Timer zur�cksetzen
                    m_uiSpellTimer = m_uiSpellResetTimer;
                }
                else
                    m_uiSpellTimer -= uiDiff;

                DoMeleeAttackIfReady();
            }

        private:
            uint32 m_uiSpellTimer;
            uint32 m_uiSpellResetTimer;
            VanquishedTentacle self;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_vanquished_tentacleAI(creature);
        }
};

/*#####
## npc_lil_xt
#####*/

enum XTSpells
{
    SPELL_XT_TANTRUM                   = 76092,
    SPELL_XT_STRIKE                    = 76096,             // unused
    SPELL_XT_STRETCH                   = 76114,
    SPELL_XT_SLEEP                     = 76116,
    NPC_LIL_XT                         = 40703,
    GO_TRAIN                           = 193963,            // already defined

    SOUND_XT_THINKIBROKEIT             = 17923,
    SOUND_XT_DOESNTBENDTHATWAY         = 17924,
    SOUND_XT_NONONONONONONO            = 17925,
    SOUND_XT_SOTIRED                   = 17926,
    SOUND_XT_READYTOPLAY               = 17927,
    SOUND_XT_NEWPRESENTS               = 18016,
    SOUND_XT_LIGHTBOMB                 = 18017,            // unused
    SOUND_XT_NONONO_2                  = 18018             // unused
};

class npc_lil_xt : public CreatureScript
{
    public:
        npc_lil_xt() : CreatureScript("npc_lil_xt") { }

        struct npc_lil_xtAI : public CritterAI
        {
            npc_lil_xtAI(Creature* creature) : CritterAI(creature) { }
            
            void Reset() override
            {
                asleep = false;
                busy = false;
                state = 0;
                stretchTimer = urand(10000, 15000);
                sleepTimer = urand(100000, 120000);
                searchTimer = urand(2000, 4000);
            }

            void UpdateAI(uint32 diff) override
            {
                if (me->HasAura(SPELL_XT_SLEEP) || me->HasAura(SPELL_XT_STRETCH))
                {
                    if (wakeTimer >= diff)
                        wakeTimer -= diff;
                    return;
                }

                if (searchTimer <= diff)
                {
                    if (GameObject* xtPetVictim = me->FindNearestGameObject(GO_TRAIN, 10.0f))
                    {
                        busy = true;
                        switch (state)
                        {
                            case 0:
                                me->SetFacingToObject(xtPetVictim);
                                me->PlayDirectSound(SOUND_XT_NEWPRESENTS, NULL);
                                searchTimer = 6000;
                                state++;
                                return;
                            case 1:
                                me->GetMotionMaster()->MovePoint(0, xtPetVictim->GetPositionX(), xtPetVictim->GetPositionY(), xtPetVictim->GetPositionZ());
                                searchTimer = 2000;
                                state++;
                                return;
                            case 2:
                                DoCastSelf(SPELL_XT_TANTRUM);
                                searchTimer = 5000;
                                state++;
                                return;
                            case 3:
                                xtPetVictim->Delete();
                                if (Unit* owner = me->GetOwner())
                                    me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                                me->PlayDirectSound(RAND(SOUND_XT_THINKIBROKEIT, SOUND_XT_DOESNTBENDTHATWAY), NULL);
                                searchTimer = urand(2000, 4000);
                                busy = false;
                                state = 0;
                                return;
                            default:
                                state = 0;
                        }
                    }
                    else
                    {
                        state = 0;
                        busy = false;
                    }
                }
                else searchTimer -= diff;

                if (busy)
                    return;

                if (stretchTimer <= diff)
                {
                    DoCastSelf(SPELL_XT_STRETCH);
                    stretchTimer = urand(30000, 40000);
                }
                else stretchTimer -= diff;

                if (sleepTimer <= diff)
                {
                    me->PlayDirectSound(SOUND_XT_SOTIRED, NULL);
                    DoCastSelf(SPELL_XT_SLEEP);
                    wakeTimer = 16000;
                    asleep = true;
                    sleepTimer = urand(100000, 120000);
                }
                else sleepTimer -= diff;

                if (asleep && wakeTimer <= diff)
                {
                    me->PlayDirectSound(SOUND_XT_READYTOPLAY, NULL);
                    asleep = false;
                }
            }

        private:
            bool asleep;
            bool busy;
            uint32 stretchTimer;
            uint32 sleepTimer;
            uint32 wakeTimer;
            uint32 searchTimer;
            uint32 state;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lil_xtAI(creature);
    }
};

enum SylvanasLamenterMisc
{
    EVENT_PLAY_MUSIC            = 1,
    SPELL_HIGHBORNE_AURA        = 37090,
    SOUND_HIGHBORNE             = 15095
};

class npc_sylvanas_lamenter : public CreatureScript
{
    public:
        npc_sylvanas_lamenter() : CreatureScript("npc_sylvanas_lamenter") { }

        struct npc_sylvanas_lamenterAI : public ScriptedAI
        {
            npc_sylvanas_lamenterAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_PLAY_MUSIC, 1 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_PLAY_MUSIC:
                            if (Player* player = me->FindNearestPlayer(10.0f))
                                me->GetMotionMaster()->MoveCharge(me->GetPositionX(), me->GetPositionY(), player->GetPositionZ() + 4.5f, 1.0f);
                            DoCastSelf(SPELL_HIGHBORNE_AURA, true);
                            me->PlayDirectSound(SOUND_HIGHBORNE);
                            break;
                        default:
                            break;
                    }
                }
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_sylvanas_lamenterAI(creature);
        }
};

class npc_imp_in_a_ball : public CreatureScript
{
    private:
        enum
        {
            SAY_RANDOM,
            EVENT_TALK = 1,
        };

public:
    npc_imp_in_a_ball() : CreatureScript("npc_imp_in_a_ball") { }

    struct npc_imp_in_a_ballAI : public ScriptedAI
    {
        npc_imp_in_a_ballAI(Creature* creature) : ScriptedAI(creature)
        {
            summonerGUID.Clear();
        }

        void IsSummonedBy(Unit* summoner) override
        {
            if (summoner->GetTypeId() == TYPEID_PLAYER)
            {
                summonerGUID = summoner->GetGUID();
                events.ScheduleEvent(EVENT_TALK, 3000);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            if (events.ExecuteEvent() == EVENT_TALK)
            {
                if (Player* owner = ObjectAccessor::GetPlayer(*me, summonerGUID))
                {
                    sCreatureTextMgr->SendChat(me, SAY_RANDOM, owner,
                        owner->GetGroup() ? CHAT_MSG_MONSTER_PARTY : CHAT_MSG_MONSTER_WHISPER, LANG_ADDON, TEXT_RANGE_NORMAL);
                }
            }
        }

    private:
        EventMap events;
        ObjectGuid summonerGUID;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_imp_in_a_ballAI(creature);
    }
};

enum PandarenMonkMisc
{
    SPELL_PANDAREN_MONK = 69800,
    EVENT_FOCUS         = 1,
    EVENT_EMOTE         = 2,
    EVENT_FOLLOW        = 3,
    EVENT_DRINK         = 4
};

class npc_pandaren_monk : public CreatureScript
{
    public:
        npc_pandaren_monk() : CreatureScript("npc_pandaren_monk") {}

        struct npc_pandaren_monkAI : public NullCreatureAI
        {
            npc_pandaren_monkAI(Creature* creature) : NullCreatureAI(creature) { }

            void Reset() override
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_FOCUS, 1000);
            }

            void ReceiveEmote(Player* /*player*/, uint32 emote) override           
            {
                me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                me->StopMoving();

                switch (emote)
                {
                    case TEXT_EMOTE_BOW:
                        _events.ScheduleEvent(EVENT_FOCUS, 1000);
                        break;
                    case TEXT_EMOTE_DRINK:
                        _events.ScheduleEvent(EVENT_DRINK, 1000);
                        break;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                if (Unit* owner = me->GetCharmerOrOwner())
                    if (!me->IsWithinDist(owner, 30.f))
                        me->InterruptSpell(CURRENT_CHANNELED_SPELL);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FOCUS:
                            if (Unit* owner = me->GetCharmerOrOwner())
                                me->SetFacingToObject(owner);
                            _events.ScheduleEvent(EVENT_EMOTE, 1000);
                            break;
                        case EVENT_EMOTE:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
                            _events.ScheduleEvent(EVENT_FOLLOW, 1000);
                            break;
                        case EVENT_FOLLOW:
                            if (Unit* owner = me->GetCharmerOrOwner())
                                me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                            break;
                        case EVENT_DRINK:
                            DoCastSelf(SPELL_PANDAREN_MONK, false);
                            break;
                        default:
                            break;
                    }
                }
            }
        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pandaren_monkAI(creature);
        }
};

/*####
## npc_brewfest_reveler
####*/

enum BrewfestReveler
{
    SPELL_BREWFEST_TOAST = 41586
};

class npc_brewfest_reveler : public CreatureScript
{
    public:
        npc_brewfest_reveler() : CreatureScript("npc_brewfest_reveler") { }

        struct npc_brewfest_revelerAI : public ScriptedAI
        {
            npc_brewfest_revelerAI(Creature* creature) : ScriptedAI(creature) { }

            void ReceiveEmote(Player* player, uint32 emote) override
            {
                if (!IsHolidayActive(HOLIDAY_BREWFEST))
                    return;

                if (emote == TEXT_EMOTE_DANCE)
                    me->CastSpell(player, SPELL_BREWFEST_TOAST, false);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_brewfest_revelerAI(creature);
        }
};

void AddSC_generic_pet_scripts()
{
    new npc_pet_gen_baby_blizzard_bear();
    new npc_pet_gen_mojo();
    new npc_pet_gen_fetch_ball();
    new npc_pet_gen_toxic_wasteling();
    new npc_egbert();
    new npc_spring_rabbit();
    new npc_pet_vargoths();
    new npc_vanquished_tentacle();
    new npc_lil_xt();
    new npc_sylvanas_lamenter();
    new npc_imp_in_a_ball();
    new npc_pandaren_monk();
    new npc_brewfest_reveler();
}
