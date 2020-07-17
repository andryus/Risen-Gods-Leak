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
#include "CombatAI.h"
#include "SpellScript.h"
#include "Group.h"

/*######
# npc_greater_felfire_diemetradon
######*/

enum GreaterFelfireDiemetradon
{
    NPC_ARCANOSCORP     = 21909,
    SPELL_FLAMING_WOUND = 37941,
    SPELL_FEL_FIREBALL  = 37945,
    SPELL_TAG           = 37851
};

class npc_greater_felfire_diemetradon : public CreatureScript
{
public:
    npc_greater_felfire_diemetradon() : CreatureScript("npc_greater_felfire_diemetradon") { }

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_greater_felfire_diemetradonAI(c);
    }

    struct npc_greater_felfire_diemetradonAI : public ScriptedAI
    {
        npc_greater_felfire_diemetradonAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 fireball_timer;
        uint32 tagged_timer;
        bool is_tagged;

        void Reset()
        {
            fireball_timer = 3000;
        }

        void EnterCombat(Unit* /*who*/)
        {
            fireball_timer = 3000;
        }

        void InitializeAI()
        {
            is_tagged = false;
            tagged_timer = 0;
        }

        void UpdateAI(uint32 diff)
        {
            if (me->GetVictim())
            {
                if (me->GetVictim()->GetTypeId() == TYPEID_UNIT && me->GetVictim()->GetEntry() == NPC_ARCANOSCORP)
                {
                    EnterEvadeMode();
                    return;
                }

                if (!me->HasAura(SPELL_FLAMING_WOUND))
                {
                    DoCastSelf(SPELL_FLAMING_WOUND);
                }

                if (fireball_timer <= diff)
                {
                    DoCastVictim(SPELL_FEL_FIREBALL);
                    fireball_timer = 6000;
                }
                else
                {
                    fireball_timer -= diff;
                }

                DoMeleeAttackIfReady();
            }

            if (is_tagged)
            {
                if (!me->HasAura(SPELL_TAG))
                {
                    me->AddAura(SPELL_TAG, me);
                }

                if (tagged_timer <= diff)
                {
                    is_tagged = false;
                }
                else
                {
                    tagged_timer -= diff;
                }
            }

            if (me->GetDistance(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ()) > 70.0f)
            {
                EnterEvadeMode();
            }
        }

        void SpellHit(Unit* caster, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_TAG)
            {
                if (!is_tagged)
                {
                    if (caster->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        caster->GetCharmerOrOwnerPlayerOrPlayerItself()->KilledMonsterCredit(NPC_ARCANOSCORP);
                    }
                }

                is_tagged = true;
                tagged_timer = 100000;
            }
        }
    };
};

/*######
# npc_arcano_scorp
######*/

class npc_arcano_scorp : public CreatureScript
{
public:
    npc_arcano_scorp() : CreatureScript("npc_arcano_scorp") { }

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_arcano_scorpAI(c);
    }

    struct npc_arcano_scorpAI : public ScriptedAI
    {
        npc_arcano_scorpAI(Creature* creature) : ScriptedAI(creature) {}

        void UpdateAI(uint32 /*diff*/)
        {
            if (me->GetVictim())
            {
                if (me->GetVictim()->GetTypeId() == TYPEID_PLAYER)
                {
                    EnterEvadeMode();
                }
            }
        }
    };
};

/*#####
# npc_zuluhed_demon_portal
#####*/

enum zuluhedCreature {
    ARCUBUS_DESTROYER = 22338
};

class npc_zuluhed_demon_portal : public CreatureScript
{
    public : npc_zuluhed_demon_portal() : CreatureScript("npc_zuluhed_demon_portal") { }

    struct npc_zuluhed_demon_portalAI : public ScriptedAI
    {
        npc_zuluhed_demon_portalAI(Creature* creature) : ScriptedAI(creature)
        {
            spawnTimer = 10000;
            canSpawn = true;
        }

        uint32 spawnTimer;
        bool canSpawn;

        void UpdateAI(uint32 diff)
        {
            if (canSpawn)
            {
                if (spawnTimer <= diff)
                {
                    me->SummonCreature(ARCUBUS_DESTROYER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 300000);
                    canSpawn = false;
                } else
                    spawnTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_zuluhed_demon_portalAI(creature);
    }
};

/*#####
# go_zuluhed_chains
######*/

enum ZuluhedQuest {
    ZULUHED_THE_WRACKED = 10866
};

class go_zuluhed_chains : public GameObjectScript
{
public:
    go_zuluhed_chains() : GameObjectScript("go_zuluhed_chains") { }

    void OnLootStateChanged(GameObject* /*go*/, uint32 state, Unit* unit)
    {
        if (state == GO_ACTIVATED)
        {
            if (Player* player = unit->ToPlayer())
            {
                if (Group* group = player->GetGroup())
                {
                    for (GroupReference* groupRef = group->GetFirstMember(); groupRef != NULL; groupRef = groupRef->next())
                        if (Player* member = groupRef->GetSource())
                            member->CompleteQuest(ZULUHED_THE_WRACKED);
                }
                else
                {
                    player->CompleteQuest(ZULUHED_THE_WRACKED);
                }
            }
        }
    }
};

/*########
## Netherwing Quest Dragonmaw Race 1-6
######*/

enum Dragonmaw_Race1
{
    QUEST_MURG_MUCKJAW          = 11064,
    NPC_MURG_TARGET             = 23356,
    SPELL_MURG_PUMKIN           = 40890,
    SPELL_MURG_KNOCKDOWN        = 40905,
    //Script Texts
    MURG_SAY_START              = 0,
    MURG_SAY_END                = 1
};

class npc_murg_muckjaw : public CreatureScript
{
public:
    npc_murg_muckjaw() : CreatureScript("npc_murg_muckjaw") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        npc_murg_muckjawAI* EscortAI = CAST_AI(npc_murg_muckjaw::npc_murg_muckjawAI, creature->AI());
        if (!EscortAI)
            return false;

        if (quest->GetQuestId() == QUEST_MURG_MUCKJAW)
        {
            EscortAI->PlayerGUID = player->GetGUID();
            EscortAI->Start(false, true, player->GetGUID());
        }
        return true;
    }

    struct npc_murg_muckjawAI : public npc_escortAI
    {
        npc_murg_muckjawAI(Creature* creature) : npc_escortAI(creature)
        {
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_MURG_KNOCKDOWN, true);
        }

        ObjectGuid PlayerGUID;
        uint32 bombTimer;
        bool StartThrow;

        void Reset()
        {
            PlayerGUID.Clear();
            bombTimer = 1600;
            StartThrow = false;

            me->SetCanFly(false);
            SetEscortPaused(true);
            SetDespawnAtFar(false);
        }

        void JustReachedHome()
        {
            me->Respawn(true);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
            case 0:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 0.5f);
                me->setActive(true);
                Talk(MURG_SAY_START);
                break;
            case 6:
                me->SetCanFly(true);
                me->SetSpeedRate(MOVE_RUN, 1.0f);
                break;
            case 7:
                StartThrow = true;
                me->SetSpeedRate(MOVE_RUN, 3.0f);
                break;
            case 45:
                me->SetSpeedRate(MOVE_RUN, 1.5f);
            case 47:
                StartThrow = false;
                break;
            case 48:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 0.5f);
                if (!PlayerGUID)
                    return;

                if (Unit* Player = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    Player->ToPlayer()->CompleteQuest(QUEST_MURG_MUCKJAW);
                Talk(MURG_SAY_END);
                me->setActive(false);
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();
                break;
            }
        }

        void UpdateAI (uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (StartThrow)
            {
                if (bombTimer <= diff)
                {
                    if (!PlayerGUID)
                        return;

                    if (Unit* target = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    {
                        if (!target->IsFlying())
                            return;

                        if (Creature* trigger = me->SummonCreature(NPC_MURG_TARGET, target->GetPositionX() + (irand(-25, 25) / 10.0f),
                                                                   target->GetPositionY() + (irand(-25, 25) / 10.0f), target->GetPositionZ() + (irand(-25, 25) / 10.0f), 0,
                                                                   TEMPSUMMON_TIMED_DESPAWN, 8000))
                        {
                            me->CastSpell(trigger, SPELL_MURG_PUMKIN, true);
                        }
                    }
                    bombTimer = 1600;
                }
                else bombTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_murg_muckjawAI(creature);
    }
};

enum Dragonmaw_Race2
{
    QUEST_TROPE_BELCHER         = 11067,
    NPC_TROPE_TARGET            = 23357,
    SPELL_TROPE_SLIME           = 40909,
    SPELL_TROPE_KNOCKDOWN       = 40900,

    //Script Texts
    TROPE_SAY_START              = 0,
    TROPE_SAY_END                = 1,
};

class npc_trope_belcher : public CreatureScript
{
public:
    npc_trope_belcher() : CreatureScript("npc_trope_belcher") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        npc_trope_belcherAI* EscortAI = CAST_AI(npc_trope_belcher::npc_trope_belcherAI, creature->AI());
        if (!EscortAI)
            return false;

        if (quest->GetQuestId() == QUEST_TROPE_BELCHER)
        {
            EscortAI->PlayerGUID = player->GetGUID();
            EscortAI->Start(false, true, player->GetGUID());
        }
        return true;
    }

    struct npc_trope_belcherAI : public npc_escortAI
    {
        npc_trope_belcherAI(Creature* creature) : npc_escortAI(creature)
        {
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_TROPE_KNOCKDOWN, true);
        }

        ObjectGuid PlayerGUID;
        uint32 bombTimer;
        bool StartThrow;

        void Reset()
        {
            PlayerGUID.Clear();
            bombTimer = urand(750, 1250);
            StartThrow = false;

            me->SetCanFly(false);
            SetEscortPaused(true);
            SetDespawnAtFar(false);
        }

        void JustReachedHome()
        {
            me->Respawn(true);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
            case 0:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 0.5f);
                Talk(TROPE_SAY_START);
                me->setActive(true);
                break;
            case 8:
                me->SetCanFly(true);
                me->SetSpeedRate(MOVE_RUN, 1.0f);
                break;
            case 9:
                StartThrow = true;
                break;
            case 10:
                me->SetSpeedRate(MOVE_RUN, 3.3f);
                break;                   
            case 47:
                StartThrow = false;
                me->SetSpeedRate(MOVE_RUN, 1.5f);
                break;
            case 49:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 0.5f);
                break;
            case 51:
                if (!PlayerGUID)
                    return;

                if (Unit* Player = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    Player->ToPlayer()->CompleteQuest(QUEST_TROPE_BELCHER);
                Talk(TROPE_SAY_END);
                me->setActive(false);
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();
                break;
            }
        }

        void UpdateAI (uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (StartThrow)
            {
                if (bombTimer <= diff)
                {
                    if (!PlayerGUID)
                        return;

                    if (Unit* target = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    {
                        if (!target->IsFlying())
                            return;

                        if (Creature* trigger = me->SummonCreature(NPC_TROPE_TARGET, target->GetPositionX() + (irand(-25, 25) / 10.0f),
                                                                   target->GetPositionY() + (irand(-25, 25) / 10.0f), target->GetPositionZ() + (irand(-25, 25) / 10.0f), 0,
                                                                   TEMPSUMMON_TIMED_DESPAWN, 8000))
                        {
                            me->CastSpell(trigger, SPELL_TROPE_SLIME, true);
                        }
                    }
                    bombTimer = urand(750, 1250);
                }
                else bombTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_trope_belcherAI(creature);
    }
};

enum Dragonmaw_Race3
{
    QUEST_CORLOK                = 11068,
    NPC_CORLOK_TARGET           = 23358,
    SPELL_CORLOK_SKULL          = 40894,
    SPELL_CORLOK_KNOCKDOWN      = 40900,
    //Script Texts
    CORLOK_SAY_START            = 0,
    CORLOK_SAY_END              = 1
};

class npc_corlok : public CreatureScript
{
public:
    npc_corlok() : CreatureScript("npc_corlok") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        npc_corlokAI* EscortAI = CAST_AI(npc_corlok::npc_corlokAI, creature->AI());
        if (!EscortAI)
            return false;

        if (quest->GetQuestId() == QUEST_CORLOK)
        {
            EscortAI->PlayerGUID = player->GetGUID();
            EscortAI->Start(false, true, player->GetGUID());
        }
        return true;
    }

    struct npc_corlokAI : public npc_escortAI
    {
        npc_corlokAI(Creature* creature) : npc_escortAI(creature)
        {
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_CORLOK_KNOCKDOWN, true);
        }

        ObjectGuid PlayerGUID;
        uint32 bombTimer;
        bool StartThrow;

        void Reset()
        {
            PlayerGUID.Clear();
            bombTimer = urand(500, 2000);
            StartThrow = false;

            me->SetCanFly(false);
            SetEscortPaused(true);
            SetDespawnAtFar(false);
        }

        void JustReachedHome()
        {
            me->Respawn(true);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 0:
                    me->SetCanFly(false);
                    me->SetSpeedRate(MOVE_RUN, 0.5f);
                    Talk(CORLOK_SAY_START);
                    me->setActive(true);
                    break;
                case 11:
                    me->SetCanFly(true);
                    me->SetSpeedRate(MOVE_RUN, 1.0f);
                    break;
                case 12:
                    me->SetSpeedRate(MOVE_RUN, 3.5f);
                    break;     
                case 13:
                    StartThrow = true;
                    break;              
                case 80:
                    StartThrow = false;
                    me->SetSpeedRate(MOVE_RUN, 1.5f);
                    break;
                case 81:
                    me->SetCanFly(false);
                    me->SetSpeedRate(MOVE_RUN, 1.0f);
                    break;
                case 84:
                    me->SetSpeedRate(MOVE_RUN, 0.5f);
                    break;
                case 85:
                    if (!PlayerGUID)
                        return;

                    if (Unit* Player = ObjectAccessor::GetUnit(*me, PlayerGUID))
                        Player->ToPlayer()->CompleteQuest(QUEST_CORLOK);
                    Talk(CORLOK_SAY_END);
                    me->setActive(false);
                    break;
                case 89:
                    me->GetMotionMaster()->MoveTargetedHome();
                    Reset();
                    break;
            }
        }

        void UpdateAI (uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (StartThrow)
            {
                if (bombTimer <= diff)
                {
                    if (!PlayerGUID)
                        return;

                    if (Unit* target = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    {
                        if (!target->IsFlying())
                            return;

                        if (Creature* trigger = me->SummonCreature(NPC_CORLOK_TARGET, target->GetPositionX() + (irand(-25, 25) / 10.0f),
                                                                   target->GetPositionY() + (irand(-25, 25) / 10.0f), target->GetPositionZ() + (irand(-25, 25) / 10.0f), 0,
                                                                   TEMPSUMMON_TIMED_DESPAWN, 8000))
                        {
                            me->CastSpell(trigger, SPELL_CORLOK_SKULL, true);
                        }
                    }
                    bombTimer = urand(500, 2000);
                }
                else bombTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_corlokAI(creature);
    }
};

enum Dragonmaw_Race4
{
    QUEST_ICHMAN                = 11069,
    NPC_ICHMAN_TARGET           = 23359,
    SPELL_ICHMAN_FIRE           = 40928,
    SPELL_ICHMAN_KNOCKDOWN      = 40929,
    //Script Texts
    ICHMAN_SAY_START            = 0,
    ICHMAN_SAY_END              = 1
};

class npc_ichman : public CreatureScript
{
public:
    npc_ichman() : CreatureScript("npc_ichman") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        npc_ichmanAI* EscortAI = CAST_AI(npc_ichman::npc_ichmanAI, creature->AI());
        if (!EscortAI)
            return false;

        if (quest->GetQuestId() == QUEST_ICHMAN)
        {
            EscortAI->PlayerGUID = player->GetGUID();
            EscortAI->Start(false, true, player->GetGUID());
        }
        return true;
    }

    struct npc_ichmanAI : public npc_escortAI
    {
        npc_ichmanAI(Creature* creature) : npc_escortAI(creature) { }

        ObjectGuid PlayerGUID;
        uint32 bombTimer;
        bool StartThrow;

        void Reset()
        {
            PlayerGUID.Clear();
            bombTimer = urand(750, 1250);
            StartThrow = false;

            me->SetCanFly(false);
            SetEscortPaused(true);
            SetDespawnAtFar(false);
        }

        void JustReachedHome()
        {
            me->Respawn(true);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
            case 0:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 0.5f);
                Talk(ICHMAN_SAY_START);
                me->setActive(true);
                break;
            case 6:
                me->SetCanFly(true);
                me->SetSpeedRate(MOVE_RUN, 1.5f);
                break;
            case 8:
                StartThrow = true;
                me->SetSpeedRate(MOVE_RUN, 3.6f);
                break;                  
            case 182:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 1.5f);
                break;
            case 183:
                StartThrow = false;
                break;
            case 186:
                me->SetSpeedRate(MOVE_RUN, 0.5f);
                break;
            case 187:
                if (!PlayerGUID)
                    return;

                if (Unit* Player = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    Player->ToPlayer()->CompleteQuest(QUEST_ICHMAN);
                Talk(ICHMAN_SAY_END);
                me->setActive(false);
                break;
            case 192:
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();
                break;
            }
        }

        void UpdateAI (uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (StartThrow)
            {
                if (bombTimer <= diff)
                {
                    if (!PlayerGUID)
                        return;

                    if (Unit* target = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    {
                        if (!target->IsFlying())
                            return;

                        if (Creature* trigger = me->SummonCreature(NPC_ICHMAN_TARGET, target->GetPositionX() + (irand(-25, 25) / 10.0f),
                                                                   target->GetPositionY() + (irand(-25, 25) / 10.0f), target->GetPositionZ() + (irand(-25, 25) / 10.0f), 0,
                                                                   TEMPSUMMON_TIMED_DESPAWN, 8000))
                        {
                            me->CastSpell(trigger, SPELL_ICHMAN_FIRE, true);
                        }
                    }
                    bombTimer = urand(750, 1250);
                }
                else bombTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ichmanAI(creature);
    }
};

enum Dragonmaw_Race5
{
    QUEST_MULVERICK             = 11070,
    NPC_MULVERICK_TARGET        = 23360,
    SPELL_MULVERICK_LIGHT       = 40930,
    //Script Texts
    MULVERICK_SAY_START         = 0,
    MULVERICK_SAY_END           = 1
};

class npc_mulverick : public CreatureScript
{
public:
    npc_mulverick() : CreatureScript("npc_mulverick") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        npc_mulverickAI* EscortAI = CAST_AI(npc_mulverick::npc_mulverickAI, creature->AI());
        if (!EscortAI)
            return false;

        if (quest->GetQuestId() == QUEST_MULVERICK)
        {
            EscortAI->PlayerGUID = player->GetGUID();
            EscortAI->Start(false, true, player->GetGUID());
        }
        return true;
    }

    struct npc_mulverickAI : public npc_escortAI
    {
        npc_mulverickAI(Creature* creature) : npc_escortAI(creature)  {  }

        ObjectGuid PlayerGUID;
        uint32 bombTimer;
        uint32 bombTimer2;
        bool StartThrow;

        void Reset()
        {
            PlayerGUID.Clear();
            bombTimer = urand(750, 1250);
            bombTimer2 = 10000;
            StartThrow = false;

            me->SetCanFly(false);
            SetEscortPaused(true);
            SetDespawnAtFar(false);
        }

        void JustReachedHome()
        {
            me->Respawn(true);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
            case 0:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 0.5f);
                Talk(MULVERICK_SAY_START);
                me->setActive(true);
                break;
            case 6:
                me->SetSpeedRate(MOVE_RUN, 1.0f);
                break;
            case 7:
                me->SetCanFly(true);
                me->SetSpeedRate(MOVE_RUN, 3.6f);
                break;
            case 8:
                StartThrow = true;
                break;
            case 231:
                StartThrow = false;
                me->SetSpeedRate(MOVE_RUN, 1.5f);
                break;
            case 234:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 1.0f);
                break;
            case 239:
                if (!PlayerGUID)
                    return;

                if (Unit* Player = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    Player->ToPlayer()->CompleteQuest(QUEST_MULVERICK);
                Talk(MULVERICK_SAY_END);
                me->setActive(false);
                me->SetSpeedRate(MOVE_RUN, 0.5f);
                break;
            case 243:
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();
                break;
            }
        }

        void UpdateAI (uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (StartThrow)
            {
                if (bombTimer <= diff)
                {
                    if (!PlayerGUID)
                        return;

                    if (Unit* target = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    {
                        if (!target->IsFlying())
                            return;

                        if (Creature* trigger = me->SummonCreature(NPC_MURG_TARGET, target->GetPositionX() + (irand(-25, 25) / 10.0f),
                                                                   target->GetPositionY() + (irand(-25, 25) / 10.0f), target->GetPositionZ() + (irand(-25, 25) / 10.0f), 0,
                                                                   TEMPSUMMON_TIMED_DESPAWN, 8000))
                        {
                            me->CastSpell(trigger, SPELL_MULVERICK_LIGHT, true);
                        }
                    }
                    bombTimer = urand(750, 1500);
                }
                else bombTimer -= diff;


                if (bombTimer2 <= diff)
                {
                    bombTimer = 100;
                    bombTimer2 = 10000;
                }
                else bombTimer2 -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_mulverickAI(creature);
    }
}; 

enum Dragonmaw_Race6
{
    QUEST_SKYSHATTER            = 11071,
    NPC_SKYSHATTER_TARGET       = 23361,
    SPELL_SKYSHATTER_COMET      = 40945,
    SPELL_SKYSHATTER_KNOCKDOWN  = 41064,
    //Script Texts
    SKYSHATTER_SAY_BEGINN              = 0, //"beginn"
    SKYSHATTER_SAY_START               = 1, //take off
    SKYSHATTER_SAY_END                 = 2  //end
};

class npc_skyshatter : public CreatureScript
{
public:
    npc_skyshatter() : CreatureScript("npc_skyshatter") { }

    bool OnQuestAccept(Player* player, Creature* creature, const Quest* quest)
    {
        npc_skyshatterAI* EscortAI = CAST_AI(npc_skyshatter::npc_skyshatterAI, creature->AI());
        if (!EscortAI)
            return false;
        if (quest->GetQuestId() == QUEST_SKYSHATTER)
        {
            EscortAI->PlayerGUID = player->GetGUID();
            EscortAI->Start(false, true, player->GetGUID());
        }
        return true;
    }

    struct npc_skyshatterAI : public npc_escortAI
    {
        npc_skyshatterAI(Creature* creature) : npc_escortAI(creature)  {  }

        ObjectGuid PlayerGUID;
        uint32 bombTimer;
        uint32 checkMTimer;
        bool StartThrow;
        bool CheckMounted;

        void Reset()
        {
            PlayerGUID.Clear();
            bombTimer = urand(150, 750);
            StartThrow = false;

            me->SetCanFly(false);
            SetEscortPaused(true);
            SetDespawnAtFar(false);
        }

        void JustReachedHome()
        {
            me->Respawn(true);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
            case 0:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 0.5f);
                Talk(SKYSHATTER_SAY_BEGINN);
                me->setActive(true);
                break;
            case 7:
                me->SetCanFly(true);
                me->SetSpeedRate(MOVE_RUN, 1.0f);
                Talk(SKYSHATTER_SAY_START);
                break;
            case 8:
                StartThrow = true;
                break;
            case 9:
                me->SetSpeedRate(MOVE_RUN, 4.0f);
                break;                   
            case 184:
                StartThrow = false;
                me->SetSpeedRate(MOVE_RUN, 1.5f);
                break;
            case 185:
                me->SetCanFly(false);
                me->SetSpeedRate(MOVE_RUN, 1.2f);
                break;
            case 187:
                if (!PlayerGUID)
                    return;

                if (Unit* Player = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    Player->ToPlayer()->CompleteQuest(QUEST_SKYSHATTER);
                Talk(SKYSHATTER_SAY_END);
                me->setActive(false);
                break;
            case 188:
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();
                break;
            }
        }

        void UpdateAI (uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (StartThrow)
            {
                if (bombTimer <= diff)
                {
                    if (!PlayerGUID)
                        return;

                    if (Unit* target = ObjectAccessor::GetUnit(*me, PlayerGUID))
                    {
                        if (!target->IsFlying())
                            return;

                        if (Creature* trigger = me->SummonCreature(NPC_SKYSHATTER_TARGET, target->GetPositionX() + (irand(-25, 25) / 10.0f),
                                                                   target->GetPositionY() + (irand(-25, 25) / 10.0f), target->GetPositionZ() + (irand(-25, 25) / 10.0f), 0,
                                                                   TEMPSUMMON_TIMED_DESPAWN, 8000))
                        {
                            me->CastSpell(trigger, SPELL_SKYSHATTER_COMET, true);
                        }
                        bombTimer = urand(150, 750);
                    }
                }
                else bombTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_skyshatterAI(creature);
    }
};

enum TriggerMisc
{
    GO_SPELL_FOCUS              = 184750,
    NPC_DARK_CONCLAVE_RITUALIST = 22138
};

class npc_invis_arakkoa_target : public CreatureScript
{
public:
    npc_invis_arakkoa_target() : CreatureScript("npc_invis_arakkoa_target") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_invis_arakkoa_targetAI (creature);
    }

    struct npc_invis_arakkoa_targetAI : public ScriptedAI
    {
        npc_invis_arakkoa_targetAI(Creature* creature) : ScriptedAI(creature){}

        uint32 checkTimer;

        void Reset()
        {
            checkTimer = 4*IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff)
        {
            if (checkTimer <= diff)
            {
                if (!me->FindNearestCreature(NPC_DARK_CONCLAVE_RITUALIST, 1000.0f))
                {
                    me->SummonGameObject(GO_SPELL_FOCUS, -4192.24f, 2005.37f, 76.79f, 0.80f, 0, 0, 0, 0, 300);
                    me->DespawnOrUnsummon();
                }
                
                checkTimer = 4*IN_MILLISECONDS;
            }
            else
            checkTimer -= diff;
         }    
    };
};

/*####
 ## npc_commander_hobb
 ## Quest: The Deadliest Trap Ever Laid: Scyer
 ####*/

enum Hobb_Arcus_Misc
{
    QUEST_DEADLIEST_TRAP           = 11097,
    QUEST_DEADLIEST_TRAP_ALDOR     = 11101,
    SAY_BEGIN                      = 0,
    SAY_ATTACK                     = 1,
    SAY_END                        = 2,

    NPC_DRAGONMAW_SKYBREAKER_ONE   = 22274, // unused
    NPC_DRAGONMAW_SKYBREAKER_TWO   = 23440, // scyer
    NPC_DRAGONMAW_SKYBREAKER_THREE = 23441, // aldor
    NPC_SANCTUM_DEFENDER           = 23435,
    NPC_ALTAR_DEFENDER             = 23453,
    NPC_COMMANDER_HOPP             = 23434,
    NPC_COMMANDER_ARCUS            = 23452,

    SPELL_AIMED_SHOT               = 38370,
    SPELL_MULI_SHOT                = 41448,
    SPELL_SHOOT                    = 41440,

    ACTION_START                   = 0,
    EVENT_START_WP                 = 1,
    EVENT_SPAWN_GUARD              = 2,
    EVENT_SAY_ATTACK               = 3,
    EVENT_SPAWN_ONE                = 4,
    EVENT_SPAWN_TWO                = 5,
    EVENT_SPAWN_THREE              = 6,
    EVENT_SPAWN_FOUR               = 7,
    EVENT_SPAWN_FIVE               = 8,
    EVENT_DESPAWN                  = 9,    
};

Position const GuardsPos[10] =
{
    {-4086.503662f, 1065.057617f, 31.350611f, 4.766143f},
    {-4080.797363f, 1065.430054f, 31.190235f, 4.532879f},
    {-4080.992676f, 1071.929810f, 31.664232f, 4.705667f},
    {-4084.207031f, 1069.164673f, 31.715506f, 4.784990f},
    {-4076.614990f, 1072.137939f, 31.158127f, 4.879239f},
    {-4075.368408f, 1068.156616f, 30.809317f, 5.020610f},
    {-4073.453857f, 1070.877197f, 30.736773f, 5.064565f},
    {-4071.126953f, 1076.464844f, 31.704157f, 5.229500f},
    {-4067.632324f, 1071.246094f, 30.546251f, 5.317856f},
    {-4065.033936f, 1075.178711f, 31.172022f, 5.423888f},
};

Position const SkybreakerPos[8] =
{   
    {-4081.807801f, 1024.324f, 67.8690f, 1.59697f},
    {-4081.474120f, 1023.633f, 42.3268f, 1.82071f},
    {-4071.804101f, 1021.981f, 53.3124f, 1.77602f},
    {-4061.251709f, 1027.822f, 67.6852f, 2.17461f},
    {-4046.750488f, 1042.870f, 64.7339f, 2.21702f},
    {-4046.908936f, 1043.038f, 43.9972f, 2.16990f},
    {-4033.487790f, 1051.842f, 49.5804f, 2.47463f},
};

class npc_commander_hobb : public CreatureScript
{
    public:
        npc_commander_hobb() : CreatureScript("npc_commander_hobb") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) 
        {
            if (quest->GetQuestId() == QUEST_DEADLIEST_TRAP)
            {
                creature->AI()->Talk(SAY_BEGIN, player);
                creature->AI()->DoAction(ACTION_START);
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            }
        return true;
        }
    
    struct npc_commander_hobbAI : public ScriptedAI
    {
        npc_commander_hobbAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 AimedShootTimer;
        uint32 MultiShootTimer;
        uint32 ShootTimer;

        void Reset()
        {
            AimedShootTimer =  3 * IN_MILLISECONDS;
            MultiShootTimer =  5 * IN_MILLISECONDS;
            ShootTimer      =  8 * IN_MILLISECONDS;
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_START)
            {
                events.ScheduleEvent(EVENT_START_WP, 2*IN_MILLISECONDS);
            }
        }

        void SpawnSkybreaker()
        {
            for (int i = 0; i < 8; ++i)
                me->SummonCreature(NPC_DRAGONMAW_SKYBREAKER_TWO, SkybreakerPos[i], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000);
        }

        void CompleteQuest()
        {
            if (Map *map = me->GetMap())
            {
                for (Player& player : map->GetPlayers())
                {
                    if (me->IsInRange(&player, 0.0f, 100.0f))
                        player.CompleteQuest(QUEST_DEADLIEST_TRAP);
                }
            }
        }

        void DespawnSummons()
        {
            std::list<Creature*> SummonList;
            me->GetCreatureListWithEntryInGrid(SummonList, NPC_DRAGONMAW_SKYBREAKER_TWO, 1000.0f);
            me->GetCreatureListWithEntryInGrid(SummonList, NPC_SANCTUM_DEFENDER, 1000.0f);
            if (!SummonList.empty())
                for (std::list<Creature*>::iterator itr = SummonList.begin(); itr != SummonList.end(); itr++)
                    (*itr)->DespawnOrUnsummon();
        }

        void JustDied(Unit* /*victim*/)
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            events.Reset();
            DespawnSummons();
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 100.f);
            for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
            {
                Player* player = *pitr;
                if (player->GetQuestStatus(QUEST_DEADLIEST_TRAP) == QUEST_STATUS_INCOMPLETE)
                {
                    player->FailQuest(QUEST_DEADLIEST_TRAP);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
                         
            events.Update(diff);
            
            switch (events.ExecuteEvent())
            {
                case EVENT_START_WP:
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->GetMotionMaster()->MovePath(NPC_COMMANDER_HOPP * 10, false);
                    events.ScheduleEvent(EVENT_SPAWN_GUARD, 3*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_GUARD:
                    for (int i = 0; i < 10; ++i)
                        me->SummonCreature(NPC_SANCTUM_DEFENDER, GuardsPos[i], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 360000);
                    events.ScheduleEvent(EVENT_SAY_ATTACK, 11*IN_MILLISECONDS);
                    break;
                case EVENT_SAY_ATTACK: // note: probably the spawn for these creatures must be done completely different!
                    Talk(SAY_ATTACK);
                    events.ScheduleEvent(EVENT_SPAWN_ONE, 8*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_ONE:
                    SpawnSkybreaker();
                    events.ScheduleEvent(EVENT_SPAWN_TWO, 40*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_TWO:
                    SpawnSkybreaker();
                    events.ScheduleEvent(EVENT_SPAWN_THREE, 40*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_THREE:
                    SpawnSkybreaker();
                    events.ScheduleEvent(EVENT_SPAWN_FOUR, 40*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_FOUR:
                    SpawnSkybreaker();
                    events.ScheduleEvent(EVENT_SPAWN_FIVE, 40*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_FIVE:
                    SpawnSkybreaker();
                    events.ScheduleEvent(EVENT_DESPAWN, 40*IN_MILLISECONDS);
                    break;
                case EVENT_DESPAWN:
                    DespawnSummons();
                    Talk(SAY_END);
                    me->SetReactState(REACT_PASSIVE);
                    CompleteQuest();
                    me->GetMotionMaster()->MovePath(NPC_COMMANDER_HOPP * 100, false);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    break;
                default:
                    break;
            }

            if (me->GetVictim())
            {
                if (AimedShootTimer <= diff)
                {
                    DoCastVictim(SPELL_AIMED_SHOT, true);
                    AimedShootTimer = urand(8, 10)*IN_MILLISECONDS;
                }
                else AimedShootTimer -= diff;

                if (MultiShootTimer <= diff)
                {
                    DoCastVictim(SPELL_MULI_SHOT, true);
                    MultiShootTimer = urand(6, 8)*IN_MILLISECONDS;
                }
                else AimedShootTimer -= diff;

                if (ShootTimer <= diff)
                {
                    DoCastVictim(SPELL_SHOOT, true);
                    ShootTimer = urand(3, 7)*IN_MILLISECONDS;
                }
                else ShootTimer -= diff;
                
                DoMeleeAttackIfReady();
            }
        }
    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_commander_hobbAI(creature);
    }
};

/*####
 ## npc_commander_arcus
 ## Quest: The Deadliest Trap Ever Laid : Aldor
 ####*/

 Position const GuardsPosTwo[10] =
{
    {-3079.332031f, 698.072632f, -15.892089f, 3.587279f},
    {-3083.029785f, 695.118835f, -16.861736f, 3.445909f}, 
    {-3076.401367f, 692.130859f, -15.883623f, 3.282942f},
    {-3081.257813f, 689.463562f, -16.722704f, 3.341846f},
    {-3075.532227f, 686.624451f, -15.745694f, 3.377576f},
    {-3080.453613f, 683.843628f, -15.785131f, 3.483605f}, 
    {-3074.416260f, 681.507629f, -14.804823f, 3.579350f}, 
    {-3078.669922f, 678.660156f, -14.266019f, 3.544007f},
    {-3072.313232f, 677.393494f, -13.875944f, 3.677919f},
    {-3077.119629f, 673.290161f, -13.252112f, 3.189008f},
};

Position const SkybreakerPosTwo[8] =
{   
    {-3129.323242f, 687.449341f, 0.100033f, 5.999167f}, 
    {-3127.754639f, 694.050232f, 0.397313f, 5.999167f}, 
    {-3126.013428f, 701.703735f, -0.392249f, 5.969713f}, 
    {-3123.968994f, 709.590698f, 0.050460f, 6.048253f},  
    {-3134.563721f, 709.723389f, 2.950210f, 6.107155f}, 
    {-3136.214355f, 700.318604f, 2.783471f, 6.154277f},
    {-3137.747070f, 692.380493f, 3.364385f, 6.160165f}, 
    {-3139.252441f, 685.337463f, 3.570346f, 6.232813f}, 
};

class npc_commander_arcus : public CreatureScript
{
    public:
        npc_commander_arcus() : CreatureScript("npc_commander_arcus") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) 
        {
            if (quest->GetQuestId() == QUEST_DEADLIEST_TRAP_ALDOR)
            {
                creature->AI()->Talk(SAY_BEGIN, player);
                creature->AI()->DoAction(ACTION_START);
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            }
        return true;
        }
    
    struct npc_commander_arcusAI : public ScriptedAI
    {
        npc_commander_arcusAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 AimedShootTimer;
        uint32 MultiShootTimer;
        uint32 ShootTimer;

        void Reset()
        {
            AimedShootTimer =  3 * IN_MILLISECONDS;
            MultiShootTimer =  5 * IN_MILLISECONDS;
            ShootTimer      =  8 * IN_MILLISECONDS;
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_START)
            {
                events.ScheduleEvent(EVENT_START_WP, 2*IN_MILLISECONDS);
            }
        }

        void SpawnSkybreakerTwo()
        {
            for (int i = 0; i < 8; ++i)
                me->SummonCreature(NPC_DRAGONMAW_SKYBREAKER_THREE, SkybreakerPosTwo[i], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000);
        }

        void CompleteQuest()
        {
            if (Map *map = me->GetMap())
            {
                for (Player& player : map->GetPlayers())
                {
                    if (me->IsInRange(&player, 0.0f, 100.0f))
                        player.CompleteQuest(QUEST_DEADLIEST_TRAP_ALDOR);
                }
            }
        }

        void DespawnSummons()
        {
            std::list<Creature*> SummonList;
            me->GetCreatureListWithEntryInGrid(SummonList, NPC_DRAGONMAW_SKYBREAKER_THREE, 1000.0f);
            me->GetCreatureListWithEntryInGrid(SummonList, NPC_ALTAR_DEFENDER, 1000.0f);
            if (!SummonList.empty())
                for (std::list<Creature*>::iterator itr = SummonList.begin(); itr != SummonList.end(); itr++)
                    (*itr)->DespawnOrUnsummon();
        }

        void JustDied(Unit* /*victim*/)
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            events.Reset();
            DespawnSummons();
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 100.f);
            for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
            {
                Player* player = *pitr;
                if (player->GetQuestStatus(QUEST_DEADLIEST_TRAP_ALDOR) == QUEST_STATUS_INCOMPLETE)
                {
                    player->FailQuest(QUEST_DEADLIEST_TRAP_ALDOR);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
                         
            events.Update(diff);
            
            switch (events.ExecuteEvent())
            {
                case EVENT_START_WP:
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->GetMotionMaster()->MovePath(NPC_COMMANDER_ARCUS * 10, false);
                    events.ScheduleEvent(EVENT_SPAWN_GUARD, 8*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_GUARD:
                    for (int i = 0; i < 10; ++i)
                        me->SummonCreature(NPC_ALTAR_DEFENDER, GuardsPosTwo[i], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 360000);
                    events.ScheduleEvent(EVENT_SAY_ATTACK, 13*IN_MILLISECONDS);
                    break;
                case EVENT_SAY_ATTACK: // note: probably the spawn for these creatures must be done completely different!
                    Talk(SAY_ATTACK);
                    events.ScheduleEvent(EVENT_SPAWN_ONE, 16*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_ONE:
                    SpawnSkybreakerTwo();
                    events.ScheduleEvent(EVENT_SPAWN_TWO, 40*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_TWO:
                    SpawnSkybreakerTwo();
                    events.ScheduleEvent(EVENT_SPAWN_THREE, 40*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_THREE:
                    SpawnSkybreakerTwo();
                    events.ScheduleEvent(EVENT_SPAWN_FOUR, 40*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_FOUR:
                    SpawnSkybreakerTwo();
                    events.ScheduleEvent(EVENT_SPAWN_FIVE, 40*IN_MILLISECONDS);
                    break;
                case EVENT_SPAWN_FIVE:
                    SpawnSkybreakerTwo();
                    events.ScheduleEvent(EVENT_DESPAWN, 40*IN_MILLISECONDS);
                    break;
                case EVENT_DESPAWN:
                    DespawnSummons();
                    Talk(SAY_END);
                    me->SetReactState(REACT_PASSIVE);
                    CompleteQuest();
                    me->GetMotionMaster()->MovePath(NPC_COMMANDER_ARCUS * 100, false);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    break;
                default:
                    break;
            }

            if (me->GetVictim())
            {
                if (AimedShootTimer <= diff)
                {
                    DoCastVictim(SPELL_AIMED_SHOT, true);
                    AimedShootTimer = urand(8, 10)*IN_MILLISECONDS;
                }
                else AimedShootTimer -= diff;

                if (MultiShootTimer <= diff)
                {
                    DoCastVictim(SPELL_MULI_SHOT, true);
                    MultiShootTimer = urand(6, 8)*IN_MILLISECONDS;
                }
                else AimedShootTimer -= diff;

                if (ShootTimer <= diff)
                {
                    DoCastVictim(SPELL_SHOOT, true);
                    ShootTimer = urand(3, 7)*IN_MILLISECONDS;
                }
                else ShootTimer -= diff;
                
                DoMeleeAttackIfReady();
            }
        }
    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_commander_arcusAI(creature);
    }
};

enum TheFelAndTheFuriousMisc
{
    NPC_KILLCREDIT_REAVER = 21959
};

class npc_fel_reaver : public CreatureScript
{
    public:
        npc_fel_reaver() : CreatureScript("npc_fel_reaver") { }
    
        struct npc_fel_reaverAI : public VehicleAI
        {
            npc_fel_reaverAI(Creature* creature) : VehicleAI(creature) { }

            void OnCharmed(bool apply) override
            {
                if (!apply)
                {
                    me->DespawnOrUnsummon();
                    me->SetRespawnTime(1);
                    me->SetCorpseDelay(1);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new  npc_fel_reaverAI(creature);
        }
};

class spell_random_rocket_missile : public SpellScriptLoader
{
    public:
        spell_random_rocket_missile() : SpellScriptLoader("spell_random_rocket_missile") { }

        class spell_random_rocket_missile_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_random_rocket_missile_SpellScript);

            void Activate(SpellEffIndex index)
            {
                PreventHitDefaultEffect(index);
                GetHitGObj()->UseDoorOrButton();
                if (Unit* caster = GetCaster())
                    if (Player* player = caster->FindNearestPlayer(1000.0f))
                        player->KilledMonsterCredit(NPC_KILLCREDIT_REAVER);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_random_rocket_missile_SpellScript::Activate, EFFECT_0, SPELL_EFFECT_ACTIVATE_OBJECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_random_rocket_missile_SpellScript();
        }
};

enum SpellDisguiseMisc
{
	SPELL_STUN = 23775
};

class spell_crate_disguise : public SpellScriptLoader
{
    public:
		spell_crate_disguise() : SpellScriptLoader("spell_crate_disguise") { }

		class spell_crate_disguise_AuraScript : public AuraScript
		{
			PrepareAuraScript(spell_crate_disguise_AuraScript);
			
			void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
			{
				GetTarget()->AddAura(SPELL_STUN, GetTarget());
			}

			void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
			{
				GetTarget()->RemoveAurasDueToSpell(SPELL_STUN);
			}

			void Register()
			{
				OnEffectApply += AuraEffectApplyFn(spell_crate_disguise_AuraScript::HandleOnEffectApply, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
				OnEffectRemove += AuraEffectRemoveFn(spell_crate_disguise_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
			}
		};

		AuraScript* GetAuraScript() const override
		{
			return new spell_crate_disguise_AuraScript();
		}
};

void AddSC_shadowmoon_valley_rg()
{
    new npc_greater_felfire_diemetradon();
    new npc_arcano_scorp();
    new npc_zuluhed_demon_portal();
    new go_zuluhed_chains();
    new npc_murg_muckjaw();
    new npc_trope_belcher();
    new npc_corlok();
    new npc_ichman();
    new npc_mulverick();
    new npc_skyshatter();
    new npc_invis_arakkoa_target();
    new npc_commander_hobb();
    new npc_commander_arcus();
    new npc_fel_reaver();
    new spell_random_rocket_missile();
	new spell_crate_disguise();
}
