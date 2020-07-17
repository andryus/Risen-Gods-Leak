#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "CombatAI.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "CreatureTextMgr.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "Vehicle.h"
#include "SpellHistory.h"

/*######
## npc_olympia_chromie
######*/

Position chromieSpawn[2] = { { 5788.5f, 393.5f, 672.4f, 1.08f }, { 5829.7f, 514.43f, 657.75f, 4.9788f } };
Position chromieMovePos[3] = { 
    { 5807.89f, 427.175f, 658.79f, 1.039f },
    { 5810.89f, 431.961f, 658.78f, 1.117f },
    { 5775.55f, 355.311f, 623.05f, 4.841f } };

enum
{
    SPELL_PHASE_SPELL       = 61043,
    SPELL_GHOST_EFFECT      = 22650,
    SPELL_KNOCKDOWN         = 13360,

    DISPLAYID_DRAGON_FORM   = 24782,

    GO_LOST_NOTE            = 411523,

    GOSSIP_CHROMIE_DEFAULT  = 60207,
    GOSSIP_CHROMIE_EVENT    = 60208,

    QUEST_TIME_IS_RELATIVE  = 110177,

    NPC_CHROMIE             = 1010753,
    NPC_THE_GNOME           = 1010752,
};

class npc_olympia_chromie : public CreatureScript
{
    public:
        npc_olympia_chromie() : CreatureScript("npc_olympia_chromie") { }
        
        struct npc_olympia_chromieAI : public ScriptedAI
        {
            npc_olympia_chromieAI(Creature* creature) : stepTimer(0), stepCount(0), lastStart(0), chromieGUID(), theGnomeGUID(), ScriptedAI(creature) { }

            void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
            {
                questCredit.insert(guid);
                if (!stepTimer && !stepCount)
                {
                    stepTimer = 4 * IN_MILLISECONDS;
                    if (auto chromie = me->SummonCreature(NPC_CHROMIE, chromieSpawn[0]))
                    {
                        chromieGUID = chromie->GetGUID();
                        me->AddAura(SPELL_GHOST_EFFECT, chromie);
                        chromie->GetMotionMaster()->MovePoint(0, chromieSpawn[0]);
                        chromie->SetDisplayId(DISPLAYID_DRAGON_FORM);
                        chromie->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                        chromie->SetPhaseMask(2, true);
                        if (auto go = me->FindNearestGameObject(GO_LOST_NOTE, 100.f))
                            go->SetPhaseMask(1, true);
                    }
                }
            }

            uint32 GetData(uint32 param) const override
            {
                if (param == 0)
                    return stepCount < 2;
                
                return 0;
            }

            void UpdateAI(uint32 diff) override
            {
                if (stepTimer)
                {
                    if (stepTimer <= diff)
                    {
                        Creature* chromie = ObjectAccessor::GetCreature(*me, chromieGUID);
                        Creature* theGnome = ObjectAccessor::GetCreature(*me, theGnomeGUID);
                        switch (stepCount++)
                        {
                            case 0:
                                if (chromie)
                                    chromie->GetMotionMaster()->MovePoint(0, chromieMovePos[0]);
                                stepTimer = 7 * IN_MILLISECONDS;
                                break;
                            case 1:
                                if (chromie)
                                {
                                    chromie->RestoreDisplayId();
                                    chromie->GetMotionMaster()->MovePoint(0, chromieMovePos[1]);
                                }
                                if (auto theGnome = me->SummonCreature(NPC_THE_GNOME, chromieSpawn[1]))
                                {
                                    theGnomeGUID = theGnome->GetGUID();
                                    me->AddAura(SPELL_GHOST_EFFECT, theGnome);
                                    theGnome->GetMotionMaster()->MovePoint(0, chromieMovePos[0]);
                                    theGnome->SetPhaseMask(2, true);
                                }
                                stepTimer = 6 * IN_MILLISECONDS;
                                break;
                            case 2:
                                if (theGnome)
                                    sCreatureTextMgr->SendChat(theGnome, 0);
                                stepTimer = 4 * IN_MILLISECONDS;
                                break;
                            case 3:
                                if (chromie)
                                    chromie->CastSpell(chromie, SPELL_KNOCKDOWN, true);
                                stepTimer = 2 * IN_MILLISECONDS;
                                break;
                            case 4:
                                if (theGnome)
                                {
                                    theGnome->GetMotionMaster()->MoveJump(chromieMovePos[2], 20.f, 60.f);
                                    sCreatureTextMgr->SendChat(theGnome, 1);
                                }
                                if (auto go = me->FindNearestGameObject(GO_LOST_NOTE, 100.f))
                                    go->SetPhaseMask(3, true);
                                stepTimer = 8 * IN_MILLISECONDS;
                                break;
                            case 5:
                                stepTimer = 0;
                                stepCount = 0;
                                for (auto itr : questCredit)
                                    if (auto plr = ObjectAccessor::GetPlayer(*me, itr))
                                    {
                                        plr->RemoveAurasDueToSpell(SPELL_PHASE_SPELL);
                                        plr->AreaExploredOrEventHappens(QUEST_TIME_IS_RELATIVE);
                                    }
                                questCredit.clear();
                                if (theGnome)
                                    theGnome->DespawnOrUnsummon();
                                if (chromie)
                                    chromie->DespawnOrUnsummon();
                                break;
                        }
                    }
                    else
                        stepTimer -= diff;
                }
            }

            ObjectGuid chromieGUID;
            ObjectGuid theGnomeGUID;
            std::unordered_set<ObjectGuid> questCredit;
            uint32 stepCount;
            uint32 stepTimer;
            uint32 lastStart;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_chromieAI(creature);
        }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            QuestStatus status = player->GetQuestStatus(QUEST_TIME_IS_RELATIVE);
            if (status != QUEST_STATUS_INCOMPLETE || creature->AI()->GetData(0) == 0)
            {
                player->PrepareGossipMenu(creature, GOSSIP_CHROMIE_DEFAULT, true);
                player->SendPreparedGossip(creature);
            }
            else
            {
                player->PrepareGossipMenu(creature, GOSSIP_CHROMIE_EVENT);
                player->SendPreparedGossip(creature);
            }
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            uint32 menuId = player->PlayerTalkClass->GetGossipMenu().GetMenuId();
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            creature->AI()->SetGuidData(player->GetGUID());
            player->CastSpell(player, SPELL_PHASE_SPELL, true);

            return false;
        }
};

/*######
## npc_olympia_edna
######*/

uint32 const ednaIntroPathSize = 5;
G3D::Vector3 const ednaIntroPath[ednaIntroPathSize] = {
    { 8469.596680f, 1039.947388f, 551.516479f },
    { 8471.014648f, 1025.613159f, 547.353027f },
    { 8456.032227f, 1004.149475f, 544.674072f },
    { 8446.006836f, 995.578003f, 544.674072f },
    { 8431.98f, 990.373f, 544.674f } };

uint32 const ednaCyclePathSize = 14;
G3D::Vector3 ednaCyclePath[ednaCyclePathSize] = {
    // { 8431.98f, 990.373f, 544.674f },
    { 8420.26f, 989.068f, 544.674f },
    { 8412.71f, 983.535f, 544.674f },
    { 8409.81f, 973.664f, 544.674f },
    { 8409.98f, 945.63f, 544.674f },
    { 8410.04f, 918.133f, 544.674f },
    { 8414.73f, 906.774f, 544.674f },
    { 8426.82f, 904.383f, 544.674f },
    { 8438.14f, 904.951f, 544.674f },
    { 8442.12f, 909.003f, 544.674f },
    { 8448.24f, 920.349f, 544.674f },
    { 8448.45f, 933.472f, 544.674f },
    { 8448.61f, 955.472f, 544.674f },
    { 8448.07f, 981.602f, 544.674f },
    { 8441.95f, 990.272f, 544.674f } };

enum
{
    SPELL_EDNA_THRUST       = 62544,
    SPELL_EDNA_CHARGE       = 62960,
    SPELL_ENDA_DEFEND       = 62552,
    SPELL_ENDA_REFESH       = 64077,
    SPELL_ENDA_DUEL         = 62863,
    SPELL_LANCE_EQUIPPED    = 62853,
    SPELL_SUMMON_TOUNAMENT_MOUNT = 63663,

    QUEST_TARGET_PRACTICE   = 110182,
};

class npc_olympia_edna : public CreatureScript
{
    public:
        npc_olympia_edna() : CreatureScript("npc_olympia_edna") { }
        
        struct npc_olympia_ednaAI : public VehicleAI
        {
            npc_olympia_ednaAI(Creature* creature) : cylicMovement(false), VehicleAI(creature) { }

            bool cylicMovement;

            void IsSummonedBy(Unit* summoner) override
            { 
                if (auto plr = summoner->ToPlayer())
                {
                    QuestStatus status = plr->GetQuestStatus(QUEST_TARGET_PRACTICE);
                    if (status != QUEST_STATUS_REWARDED)
                    {
                        me->GetSpellHistory()->AddCooldown(SPELL_EDNA_THRUST, 0, std::chrono::seconds(DAY));
                        me->GetSpellHistory()->AddCooldown(SPELL_EDNA_CHARGE, 0, std::chrono::seconds(DAY));
                        me->GetSpellHistory()->AddCooldown(SPELL_ENDA_DEFEND, 0, std::chrono::seconds(DAY));
                        me->GetSpellHistory()->AddCooldown(SPELL_ENDA_REFESH, 0, std::chrono::seconds(DAY));
                        me->GetSpellHistory()->AddCooldown(SPELL_ENDA_DUEL, 0, std::chrono::seconds(DAY));
                        me->SetWalk(false);

                        cylicMovement = true;

                        Movement::PointsArray path(ednaIntroPath, ednaIntroPath + ednaIntroPathSize);

                        Movement::MoveSplineInit init(me);
                        init.SetSmooth();
                        init.MovebyPath(path);
                        init.Launch();
                    }
                }
            }

            void OnCharmed(bool /*apply*/) override { }

            void UpdateAI(uint32 diff) override
            {
                if (cylicMovement && me->movespline->Finalized())
                {
                    Movement::PointsArray path(ednaCyclePath, ednaCyclePath + ednaCyclePathSize);

                    Movement::MoveSplineInit init(me);
                    init.SetSmooth();
                    init.SetCyclic();
                    init.MovebyPath(path);
                    init.Launch();
                }

                VehicleAI::UpdateAI(diff);
            }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_ednaAI(creature);
        }
};

class spell_olympia_summon_edna : public SpellScriptLoader
{
    public:
        spell_olympia_summon_edna() : SpellScriptLoader("spell_olympia_summon_edna") { }

        class spell_gen_summon_tournament_mount_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_summon_tournament_mount_SpellScript);

            SpellCastResult CheckIfLanceEquiped()
            {
                if (GetCaster()->IsInDisallowedMountForm())
                    GetCaster()->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);

                if (!GetCaster()->HasAura(SPELL_LANCE_EQUIPPED))
                {
                    if (auto plr = GetCaster()->ToPlayer())
                        Spell::SendCastResult(plr, sSpellMgr->GetSpellInfo(SPELL_SUMMON_TOUNAMENT_MOUNT), 1, SPELL_FAILED_CUSTOM_ERROR, SPELL_CUSTOM_ERROR_MUST_HAVE_LANCE_EQUIPPED);
                    SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_MUST_HAVE_LANCE_EQUIPPED);
                    return SPELL_FAILED_CUSTOM_ERROR;
                }

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_gen_summon_tournament_mount_SpellScript::CheckIfLanceEquiped);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_summon_tournament_mount_SpellScript();
        }
};


/*######
## player_olympia_tournament
######*/

enum
{
    NPC_TOURNAMENT_CREDIT   = 1010782,
};

class player_olympia_tournament : public PlayerScript
{
public:
    player_olympia_tournament() : PlayerScript("player_olympia_tournament") {}

    void OnDuelEnd(Player* winner, Player* /*loser*/, DuelCompleteType type) override
    {
        if (type == DUEL_WON && winner && winner->GetVehicle() && winner->GetVehicle()->GetCreatureEntry() == 1010765)
            winner->KilledMonsterCredit(NPC_TOURNAMENT_CREDIT);
    }
};

/*######
## npc_olympia_linken
######*/

enum
{
    QUEST_FIND_EDNA     = 110181,
    NPC_EDNA_1          = 1010763,
};

class npc_olympia_linken : public CreatureScript
{
    public:
        npc_olympia_linken() : CreatureScript("npc_olympia_linken") { }
        
        struct npc_olympia_linkenAI : public ScriptedAI
        {
            npc_olympia_linkenAI(Creature* creature) : ScriptedAI(creature) { }

            void MoveInLineOfSight(Unit* who) override 
            {
                if (Creature* creature = who->ToCreature())
                {
                    if (creature->GetEntry() == NPC_EDNA_1 && creature->IsWithinDist(me, 5.f))
                    {
                        if (auto player = creature->GetCharmerOrOwnerPlayerOrPlayerItself())
                        {
                            player->AreaExploredOrEventHappens(QUEST_FIND_EDNA);
                            creature->DespawnOrUnsummon();
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_linkenAI(creature);
        }
};

/*######
## npc_olympia_spawn_helper
######*/

enum
{
    QUEST_PARTY             = 110178,
    QUEST_DARK_WINGS        = 110179,

    NPC_THE_GNOME_BOSS      = 1010755,
};

class npc_olympia_spawn_helper : public CreatureScript
{
    public:
        npc_olympia_spawn_helper() : CreatureScript("npc_olympia_spawn_helper") { }
        
        struct npc_olympia_spawn_helperAI : public ScriptedAI
        {
            npc_olympia_spawn_helperAI(Creature* creature) : summonedGnome(), ScriptedAI(creature) { }

            void Reset() override
            {
                checkTimer = urand(0, 10 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                if (checkTimer <= diff)
                {
                    checkTimer += 10 * IN_MILLISECONDS;
                    if (ObjectAccessor::GetCreature(*me, summonedGnome))
                        return;
                    if (auto plr = me->FindNearestPlayer(40.f))
                    {
                        QuestStatus status = plr->GetQuestStatus(QUEST_PARTY);
                        QuestStatus status2 = plr->GetQuestStatus(QUEST_DARK_WINGS);
                        if (((status == QUEST_STATUS_INCOMPLETE) || (status == QUEST_STATUS_COMPLETE) || (status == QUEST_STATUS_REWARDED))
                            && (status2 == QUEST_STATUS_NONE || status2 == QUEST_STATUS_INCOMPLETE) && !me->IsWithinLOSInMap(plr) && !plr->IsFlying() && !plr->IsInCombat())
                        {
                            Position pos;
                            me->GetNearPosition(pos, 10.0f, M_PI_F + frand(-M_PI_F / 16.0f, M_PI_F / 16.0f));
                            if (auto summon = me->SummonCreature(NPC_THE_GNOME_BOSS, pos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120 * IN_MILLISECONDS))
                            {
                                summonedGnome = summon->GetGUID();
                                summon->AI()->SetGuidData(plr->GetGUID());
                                summon->GetMotionMaster()->MovePoint(0, *me);
                            }
                        }
                    }
                }
                else
                    checkTimer -= diff;
            }

            ObjectGuid summonedGnome;
            uint32 checkTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_spawn_helperAI(creature);
        }
};

/*######
## npc_olympia_vudimir_platin_boss
######*/

enum
{
    SPELL_IMMUNE_TO_ALL      = 29230,
    SPELL_DAZE               = 1604,
    SPELL_SUMMON_EL_POLLO    = 23056,
    SPELL_VUDIMIR_CHARGE     = 56106,
    SPELL_FEAR               = 19408,

    EVENT_VUDIMIR_CHARGE     = 1,
    EVENT_FEAR               = 2,

    NPC_VUDIMIR_PLATIN_ENTRY = 1010756,
    NPC_PET_BEAR             = 1010757,
    NPC_EL_POLLO             = 1010758,
};

class npc_olympia_vudimir_platin_boss : public CreatureScript
{
    public:
        npc_olympia_vudimir_platin_boss() : CreatureScript("npc_olympia_vudimir_platin_boss") { }
        
        struct npc_olympia_vudimir_platin_bossAI : public ScriptedAI
        {
            npc_olympia_vudimir_platin_bossAI(Creature* creature) : ownerGuid(), talkCount(0), talkTimer(5 * IN_MILLISECONDS), ScriptedAI(creature)
            { 
                me->GetPosition(&spawn);
            }

            uint32 petMoveTimer;
            float petPos;
            uint32 chasePetTimer;
            uint32 talkTimer;
            uint32 talkCount;
            uint32 phase;
            ObjectGuid ownerGuid;
            ObjectGuid petGuid;
            ObjectGuid chickenGuid;
            Position spawn;
            EventMap events;

            void Reset() override
            {
                chasePetTimer = 5 * IN_MILLISECONDS;
                petMoveTimer = 3 * IN_MILLISECONDS;
                phase = 0;
                events.Reset();
                events.ScheduleEvent(SPELL_VUDIMIR_CHARGE, 5 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FEAR, 25 * IN_MILLISECONDS);
            }

            void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
            {
                ownerGuid = guid;
            }

            void DamageTaken(Unit* attacker, uint32& /*damage*/) override 
            { 
                if (phase == 2)
                {
                    attacker->CastSpell(me, SPELL_DAZE, true);
                    if (auto plr = ObjectAccessor::GetPlayer(*me, ownerGuid))
                        plr->GroupEventHappens(QUEST_DARK_WINGS, plr);
                    me->SetImmuneToAll(true);
                    phase = 3;
                    me->DespawnOrUnsummon(5 * IN_MILLISECONDS);
                }
            }

            void JustSummoned(Creature* summon) override
            {
                if (summon->GetEntry() == NPC_EL_POLLO)
                {
                    if (me->GetVictim())
                        summon->AI()->AttackStart(me->GetVictim());
                    chickenGuid = summon->GetGUID();
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (talkTimer)
                {
                    if (talkTimer <= diff)
                    {
                        Talk(talkCount++);
                        talkTimer = (5 + talkCount) * IN_MILLISECONDS;
                        if (talkCount == 1)
                        {
                            Position pos;
                            me->GetNearPosition(pos, 2.f, PET_FOLLOW_ANGLE);
                            if (auto pet = me->SummonCreature(NPC_PET_BEAR, pos))
                            {
                                pet->AI()->SetGuidData(me->GetGUID());
                                petGuid = pet->GetGUID();
                            }
                            me->Dismount();
                        }
                        if (talkCount >= 4)
                        {
                            if (auto plr = ObjectAccessor::GetPlayer(*me, ownerGuid))
                                plr->GroupEventHappens(QUEST_PARTY, plr);
                            me->UpdateEntry(NPC_VUDIMIR_PLATIN_ENTRY);
                            talkTimer = 0;
                        }
                    }
                    else
                        talkTimer -= diff;
                }

                if (!UpdateVictim())
                    return;

                switch (phase)
                {
                    case 0:
                        if (me->GetHealthPct() < 50.f)
                        {
                            phase = 1;
                            petPos = 0;
                            me->SendClearTarget();
                            DoCastSelf(SPELL_IMMUNE_TO_ALL, true);
                            DoCastSelf(SPELL_SUMMON_EL_POLLO, true);
                            if (auto pet = ObjectAccessor::GetCreature(*me, petGuid))
                            {
                                SetCombatMovement(false);
                                me->GetMotionMaster()->MoveFollow(pet, PET_FOLLOW_DIST, 0.f);
                            }
                        }
                        events.Update(diff);
                        if (uint32 e = events.ExecuteEvent())
                        {
                            switch (e)
                            {
                                case SPELL_VUDIMIR_CHARGE:
                                    if (Unit* target = SelectTarget(SELECT_TARGET_MAXDISTANCE, 0))
                                    {
                                        if (me->GetDistance(target) > 5.f)
                                            DoCast(target, SPELL_VUDIMIR_CHARGE, true);
                                    }
                                    events.ScheduleEvent(SPELL_VUDIMIR_CHARGE, 5 * IN_MILLISECONDS);
                                    break;
                                case EVENT_FEAR:
                                    events.RescheduleEvent(SPELL_VUDIMIR_CHARGE, 13 * IN_MILLISECONDS);
                                    DoCastAOE(SPELL_FEAR, true);
                                    ResetThreatList();
                                    events.ScheduleEvent(EVENT_FEAR, 30 * IN_MILLISECONDS);
                                    break;
                            }
                        }
                        DoMeleeAttackIfReady();
                        break;
                    case 1:
                    {
                        bool nextPhase = false;

                        if (chasePetTimer <= diff)
                        {
                            chasePetTimer = 20 * IN_MILLISECONDS;
                            Talk(0);
                        }
                        else
                            chasePetTimer -= diff;

                        if (petMoveTimer <= diff)
                        {
                            petMoveTimer = 2 * IN_MILLISECONDS;
                            if (auto pet = ObjectAccessor::GetCreature(*me, petGuid))
                            {
                                Position target(spawn);
                                me->MovePosition(target, 20.0f, petPos - me->GetOrientation());
                                petPos += M_PI_F / 4.0f;
                                pet->GetMotionMaster()->MovePoint(0, target);
                            }
                        }
                        else
                            petMoveTimer -= diff;
                            if (auto chicken = ObjectAccessor::GetCreature(*me, chickenGuid))
                            {
                                if (chicken->isDead())
                                    nextPhase = true;
                                else if (!chicken->IsInCombat())
                                {
                                    chicken->DespawnOrUnsummon();
                                    EnterEvadeMode();
                                }
                            }
                            else
                                nextPhase = true;
                        
                            if (nextPhase)
                            {
                                me->RemoveAurasDueToSpell(SPELL_IMMUNE_TO_ALL);
                                SetCombatMovement(true);
                                me->GetMotionMaster()->MoveChase(me->GetVictim());
                                phase = 2;
                            }
                            break;
                        }
                    case 2:
                        DoMeleeAttackIfReady();
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_vudimir_platin_bossAI(creature);
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            uint32 menuId = player->PlayerTalkClass->GetGossipMenu().GetMenuId();
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            creature->setFaction(FACTION_HOSTILE);
            creature->AI()->AttackStart(player);

            return false;
        }
};

/*######
## npc_olympia_vudimir_pet
######*/

class npc_olympia_vudimir_pet : public CreatureScript
{
    public:
        npc_olympia_vudimir_pet() : CreatureScript("npc_olympia_vudimir_pet") { }
        
        struct npc_olympia_vudimir_petAI : public ScriptedAI
        {
            npc_olympia_vudimir_petAI(Creature* creature) : ownerGuid(), ScriptedAI(creature) { }

            ObjectGuid ownerGuid;

            void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
            {
                ownerGuid = guid;
            }

            void UpdateAI(uint32 diff) override
            {
                if (!ObjectAccessor::GetCreature(*me, ownerGuid))
                    me->DespawnOrUnsummon();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_vudimir_petAI(creature);
        }
};

/*######
## spell_event_call_infernal_destroyer
######*/

class spell_event_call_infernal_destroyer : public SpellScriptLoader
{
    public:
        spell_event_call_infernal_destroyer() : SpellScriptLoader("spell_event_call_infernal_destroyer") { }

        class spell_event_call_infernal_destroyer_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_event_call_infernal_destroyer_SpellScript)

        public:
            spell_event_call_infernal_destroyer_SpellScript() : SpellScript() { }

            void ReplaceSummon(SpellEffIndex /*effIndex*/)
            {
                if (GetCaster()->GetEntry() != NPC_VUDIMIR_PLATIN_ENTRY)
                    return;

                PreventHitDefaultEffect(EFFECT_0);

                GetCaster()->SummonCreature(NPC_EL_POLLO, *GetCaster(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30 * IN_MILLISECONDS);
            }

            void Register() override
            {
                OnEffectHit += SpellEffectFn(spell_event_call_infernal_destroyer_SpellScript::ReplaceSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_event_call_infernal_destroyer_SpellScript();
        }
};

/*######
## npc_olympia_reyn
######*/

enum
{
    SOUND_CHICKEN = 1024,
    SOUND_HOWLING = 9036,
};

class WolfHowlingEvent : public BasicEvent
{
    public:
        WolfHowlingEvent(Player& owner) : BasicEvent(), _owner(owner) { }

        bool Execute(uint64 /*eventTime*/, uint32 /*diff*/)
        {
            _owner.PlayDirectSound(SOUND_HOWLING, &_owner);
            return true;
        }

    private:
        Player& _owner;
};

class npc_olympia_reyn : public CreatureScript
{
    public:
        npc_olympia_reyn() : CreatureScript("npc_olympia_reyn") { }

        bool OnQuestReward(Player* player, Creature* /*creature*/, Quest const* /*quest*/, uint32 /*opt*/) 
        { 
            player->PlayDirectSound(SOUND_CHICKEN, player);
            player->m_Events.AddEvent(new WolfHowlingEvent(*player), player->m_Events.CalculateTime(1 * IN_MILLISECONDS));
            return false; 
        }
};

/*######
## npc_olympia_baron_baroness
######*/

enum 
{
    QUEST_OLYMPIA_DOPE          = 110171,
    QUEST_OLYMPIA_END_P2        = 110172,
    QUEST_OLYMPIA_USE_THE_FORCE = 110175,

    SPELL_RESSURECT             = 24341,
    SPELL_INSTANT_KILL          = 40450,

    SAY_OLYMPIA_END_P2          = 0,

    GOSSIP_BARON_DEFAULT        = 60199,
    GOSSIP_BARONESS_DEFAULT     = 60200,
    GOSSIP_QUEST_STEP_1         = 60203,
    GOSSIP_QUEST_STEP_2         = 60204,
};

class DelayedRessurectEvent : public BasicEvent
{
    public:
        DelayedRessurectEvent(Creature& owner, ObjectGuid target) : BasicEvent(), _owner(owner), _target(target) { }

        bool Execute(uint64 /*eventTime*/, uint32 /*diff*/)
        {
            if (auto target = ObjectAccessor::GetUnit(_owner, _target))
            {
                if (!_owner.isMoving())
                    _owner.SetFacingToObject(target);
                _owner.CastSpell(target, SPELL_RESSURECT, true);
            }
            return true;
        }

    private:
        ObjectGuid _target;
        Creature& _owner;
};

class npc_olympia_baron_baroness : public CreatureScript
{
    public:
        npc_olympia_baron_baroness() : CreatureScript("npc_olympia_baron_baroness") { }

        bool OnGossipHello(Player* player, Creature* creature) override 
        {
            QuestStatus status = player->GetQuestStatus(QUEST_OLYMPIA_DOPE);
            if (status != QUEST_STATUS_INCOMPLETE)
            {
                player->PrepareGossipMenu(creature, (creature->getGender() == GENDER_FEMALE ? GOSSIP_BARONESS_DEFAULT : GOSSIP_BARON_DEFAULT), true);
                player->SendPreparedGossip(creature);
            }
            else
            {
                player->PrepareGossipMenu(creature, GOSSIP_QUEST_STEP_1);
                player->SendPreparedGossip(creature);
            }
            return true; 
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            uint32 menuId = player->PlayerTalkClass->GetGossipMenu().GetMenuId();
            player->PlayerTalkClass->ClearMenus();
            switch (menuId)
            {
                case GOSSIP_QUEST_STEP_1:
                {
                    player->CLOSE_GOSSIP_MENU();

                    player->PrepareGossipMenu(creature, GOSSIP_QUEST_STEP_2);
                    player->SendPreparedGossip(creature);

                    return true;
                }
                case GOSSIP_QUEST_STEP_2:
                {
                    player->CLOSE_GOSSIP_MENU();
                    player->AreaExploredOrEventHappens(QUEST_OLYMPIA_DOPE);

                    return true;
                }
            } 

            return false;
        }

        bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 /*opt*/) 
        {
            if (quest->GetQuestId() == QUEST_OLYMPIA_USE_THE_FORCE)
            {
                Position pos;
                player->GetNearPosition(pos, 3.f, M_PI_F);
                if (auto summon = player->SummonCreature(NPC_THE_GNOME, pos, TEMPSUMMON_TIMED_DESPAWN, 10 * IN_MILLISECONDS))
                {
                    summon->SetFacingToObject(player);
                    summon->AI()->SetGuidData(player->GetGUID());
                }
            }
            return false; 
        }
};

/*######
## npc_olympia_vudimir_platin
######*/

enum
{
    NPC_OLYMPIA_BARON           = 1010729,
    NPC_OLYMPIA_BARONESS        = 1010730,
};

Position moveTarget = { 5820.4f, 543.9f, 651.1f, 4.4100f };

class npc_olympia_vudimir_platin : public CreatureScript
{
    public:
        npc_olympia_vudimir_platin() : CreatureScript("npc_olympia_vudimir_platin") { }
        
        struct npc_olympia_vudimir_platinAI : public ScriptedAI
        {
            npc_olympia_vudimir_platinAI(Creature* creature) : ownerGuid(), attackTimer(1000), moveTimer(0), ScriptedAI(creature) { }

            uint32 moveTimer;
            uint32 attackTimer;
            ObjectGuid ownerGuid;

            void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
            {
                ownerGuid = guid;
            }

            ObjectGuid GetGuidData(uint32 id) const override
            {
                return ownerGuid;
            }

            void UpdateAI(uint32 diff) override
            {
                if (auto player = ObjectAccessor::GetPlayer(*me, ownerGuid))
                {
                    if (attackTimer)
                    {
                        if (attackTimer <= diff)
                        {
                            attackTimer = 0;
                            moveTimer = 1000;
                            DoCast(player, SPELL_INSTANT_KILL, true);
                            Creature* baron = me->FindNearestCreature(NPC_OLYMPIA_BARON, 100.f);
                            Creature* baroness = me->FindNearestCreature(NPC_OLYMPIA_BARONESS, 100.f);
                            if (baroness && (!baron || me->GetDistance(baron) > me->GetDistance(baroness)))
                                baron = baroness;
                            if (baron)
                                baron->m_Events.AddEvent(new DelayedRessurectEvent(*baron, player->GetGUID()), baron->m_Events.CalculateTime(3 * IN_MILLISECONDS));
                        }
                        else
                            attackTimer -= diff;
                    }

                    if (moveTimer)
                    {
                        if (moveTimer <= diff)
                        {
                            moveTimer = 0;
                            Position target;
                            me->GetRandomPoint(moveTarget, 10.f, target);
                            me->GetMotionMaster()->MovePoint(0, target);
                        }
                        else
                            moveTimer -= diff;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_vudimir_platinAI(creature);
        }
};

/*######
## npc_olympia_race_start
######*/

Position raceStartA = { -6350.952f, -3908.0f, -62.356f, 6.2368f };
Position raceStartB = { -6350.952f, -3895.0f, -62.356f, 6.2368f };
Position raceEndA = { -6200.0f, -3912.0f, -60.27f, 6.2368f };
Position raceEndB = { -6200.0f, -3892.0f, -60.27f, 6.2368f };

enum
{
    NPC_RUNNER                  = 1010733,

    GOSSIP_CAN_START            = 60201,
    GOSSIP_CANNOT_START         = 60202,
    
    QUEST_OLYMPIA_RACE          = 110168,

    SPELL_MAGE_BLINK            = 1953,
    SPELL_WARLOCK_DEMONIC_CIRCLE = 48020,

    SAY_PREPARE_RACE            = 0,
    SAY_RACE_START_1            = 1,
    SAY_RACE_START_2            = 2,
    SAY_RACE_START_3            = 3,
    SAY_RACE_DISQUALIFIED_SPELL = 4,
    SAY_RACE_DISQUALIFIED_MOUNT = 5,
    SAY_RACE_START_ANNOUNCE     = 6,
    SAY_RACE_DISQUALIFIED_POS   = 7,
    SAY_RACE_START_5            = 8,
    SAY_RACE_IN_PROGRESS        = 9,
};

class npc_olympia_race_start : public CreatureScript
{
    public:
        npc_olympia_race_start() : CreatureScript("npc_olympia_race_start") { }

        struct npc_olympia_race_startAI : public ScriptedAI
        {
            npc_olympia_race_startAI(Creature* creature) : countdown(0), lastStart(0), ScriptedAI(creature) {
            }

            void DoAction(int32 /*param*/) override
            { 
                if (!countdown && GetMSTimeDiffToNow(lastStart) > 35 * IN_MILLISECONDS)
                {
                    Talk(SAY_RACE_START_ANNOUNCE);
                    talkCount = 0;
                    countdown = 13 * IN_MILLISECONDS;
                    lastStart = getMSTime();
                }
            }

            uint32 GetData(uint32 param) const override
            {
                if (param == 0)
                    return countdown;
                
                return 0;
            }

            void JustSummoned(Creature* summon) override
            {
                auto playerGuid = summon->AI()->GetGuidData();
                for (auto itr : runners)
                {
                    if (auto oldRunner = ObjectAccessor::GetCreature(*me, itr))
                        oldRunner->AI()->SetGuidData(playerGuid, 2);
                }
                runners.insert(summon->GetGUID());
            }

            void UpdateAI(uint32 diff) override
            {
                if (countdown)
                {
                    if (countdown <= diff)
                    {
                        countdown = 0;
                        Talk(SAY_RACE_START_3);
                        for (auto it = runners.begin(); it != runners.end();)
                        {
                            auto runner = ObjectAccessor::GetCreature(*me, *it);
                            if (runner)
                            {
                                runner->AI()->DoAction(0);
                                ++it;
                            }
                            else
                                it = runners.erase(it);
                        }
                    }
                    else
                    {
                        countdown -= diff;
                        if (talkCount == 0 && countdown < 4 * IN_MILLISECONDS)
                        {
                            talkCount = 1;
                            Talk(SAY_RACE_START_1);
                        }
                        if (talkCount == 1 && countdown < 2 * IN_MILLISECONDS)
                        {
                            talkCount = 2;
                            Talk(SAY_RACE_START_2);
                        }
                    }
                }
            }

            std::unordered_set<ObjectGuid> runners;
            uint32 talkCount;
            uint32 countdown;
            uint32 lastStart;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_race_startAI(creature);
        }

        bool OnGossipHello(Player* player, Creature* creature) override 
        {
            QuestStatus status = player->GetQuestStatus(QUEST_OLYMPIA_RACE);
            if (status == QUEST_STATUS_INCOMPLETE)
            {
                player->PrepareGossipMenu(creature, GOSSIP_CAN_START);
                player->SendPreparedGossip(creature);
            }
            else
            {
                player->PrepareGossipMenu(creature, GOSSIP_CANNOT_START, true);
                player->SendPreparedGossip(creature);
            }
            return true; 
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            player->PlayerTalkClass->ClearMenus();
            if (action == 1)
            {
                player->CLOSE_GOSSIP_MENU();

                if (auto runner = player->SummonCreature(NPC_RUNNER, raceStartA.GetPositionX(), 
                    frand(raceStartA.GetPositionY(), raceStartB.GetPositionY()), raceStartA.GetPositionZ(), raceStartA.GetOrientation()))
                {
                    runner->AI()->SetGuidData(player->GetGUID(), 0);
                    runner->AI()->SetGuidData(creature->GetGUID(), 1);
                    creature->AI()->JustSummoned(runner);
                }
                sCreatureTextMgr->SendChat(creature, SAY_PREPARE_RACE, player);

                return true;
            }

            return false;
        }
};

class npc_olympia_runner : public CreatureScript
{
    public:
        npc_olympia_runner() : CreatureScript("npc_olympia_runner") { }
        
        struct npc_olympia_runnerAI : public ScriptedAI
        {
            npc_olympia_runnerAI(Creature* creature) : phase(0), ownerGuid(), announcerGuid(), talkTimer(0), talkCounter(0), ScriptedAI(creature) { }

            std::vector<ObjectGuid> spectatorList;
            std::vector<ObjectGuid>::iterator nextSpeaker;
            uint32 talkTimer;
            uint32 talkCounter;
            uint32 phase;
            ObjectGuid ownerGuid;
            ObjectGuid announcerGuid;

            void SetGuidData(ObjectGuid guid, uint32 id) override
            {
                if (id == 0)
                    ownerGuid = guid;
                else if (id == 1)
                    announcerGuid = guid;
                else if (id == 2 && guid == ownerGuid)
                    me->DespawnOrUnsummon();
            }

            ObjectGuid GetGuidData(uint32 id) const override
            {
                return ownerGuid;
            }

            bool IsWithinStartArea(Unit* check)
            {
                return std::abs(check->GetPositionX() - raceStartA.GetPositionX()) < 4.0f
                    && check->GetPositionY() > raceStartA.GetPositionY() && check->GetPositionY() < raceStartB.GetPositionY();
            }

            bool IsWithinRaceArea(Unit* check)
            {
                return check->GetPositionY() > raceEndA.GetPositionY() && check->GetPositionY() < raceEndB.GetPositionY();
            }

            bool HasRaceWon(Unit* check)
            {
                return check->GetPositionX() > raceEndA.GetPositionX();
            }

            void DoAction(int32 /*param*/) override
            {
                if (phase == 1)
                {
                    phase = 2;
                    me->SetWalk(false);
                    float relativeY = (me->GetPositionY() - raceStartA.GetPositionY()) / (raceStartB.GetPositionY() - raceStartA.GetPositionY());
                    me->GetMotionMaster()->MovePoint(1, raceEndA.GetPositionX() + 15.f,
                        raceEndA.GetPositionY() + relativeY * (raceEndB.GetPositionY() - raceEndA.GetPositionY()),
                        raceEndB.GetPositionZ(), true);
                    if (auto player = ObjectAccessor::GetPlayer(*me, ownerGuid))
                    {
                        player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
                        player->GetSpellHistory()->ResetCooldown(SPELL_MAGE_BLINK);
                        player->GetSpellHistory()->ResetCooldown(SPELL_WARLOCK_DEMONIC_CIRCLE);
                    }
                }
            }

            void FailQuest(Player* player)
            {
                player->FailQuest(QUEST_OLYMPIA_RACE);
                phase = 3;
                me->DespawnOrUnsummon(10 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!ownerGuid)
                    me->DespawnOrUnsummon();
                else if (auto player = ObjectAccessor::GetPlayer(*me, ownerGuid))
                {
                    switch (phase)
                    {
                        case 0:
                        {
                            if (IsWithinStartArea(player))
                            {
                                if (auto announcer = ObjectAccessor::GetCreature(*me, announcerGuid))
                                {
                                    announcer->AI()->DoAction(0);
                                    if (announcer->AI()->GetData(0))
                                        sCreatureTextMgr->SendChat(announcer, SAY_RACE_START_5, player);
                                    else
                                        sCreatureTextMgr->SendChat(announcer, SAY_RACE_IN_PROGRESS, player);
                                    phase = 1;
                                }
                                player->RemoveAurasByType(SPELL_AURA_MOUNTED);
                                player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
                            }
                            break;
                        }
                        case 1:
                        {
                            if (!IsWithinStartArea(player))
                            {
                                phase = 0;
                                if (auto announcer = ObjectAccessor::GetCreature(*me, announcerGuid))
                                    sCreatureTextMgr->SendChat(announcer, SAY_PREPARE_RACE, player);
                            }
                            break;
                        }
                        case 2:
                        {
                            if (!IsWithinRaceArea(player))
                            {
                                if (auto announcer = ObjectAccessor::GetCreature(*me, announcerGuid))
                                    sCreatureTextMgr->SendChat(announcer, SAY_RACE_DISQUALIFIED_POS, player);
                                FailQuest(player);
                            }
                            else if (player->GetSpellHistory()->HasCooldown(SPELL_MAGE_BLINK) || player->GetSpellHistory()->HasCooldown(SPELL_WARLOCK_DEMONIC_CIRCLE)
                                || player->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))
                            {
                                if (auto announcer = ObjectAccessor::GetCreature(*me, announcerGuid))
                                    sCreatureTextMgr->SendChat(announcer, SAY_RACE_DISQUALIFIED_SPELL, player);
                                FailQuest(player);
                            }
                            else if (player->IsMounted())
                            {
                                if (auto announcer = ObjectAccessor::GetCreature(*me, announcerGuid))
                                    sCreatureTextMgr->SendChat(announcer, SAY_RACE_DISQUALIFIED_MOUNT, player);
                                FailQuest(player);
                            }
                            else if (HasRaceWon(player))
                            {
                                player->AreaExploredOrEventHappens(QUEST_OLYMPIA_RACE);
                                phase = 4;
                                std::list<Creature*> spectators;
                                player->GetCreatureListWithEntryInGrid(spectators, 0, 70.f);
                                for (auto itr : spectators)
                                    spectatorList.push_back(itr->GetGUID());
                                nextSpeaker = spectatorList.end();
                                talkTimer = 4 * IN_MILLISECONDS;
                                me->DespawnOrUnsummon(25 * IN_MILLISECONDS);
                            }
                            else if (HasRaceWon(me))
                                FailQuest(player);
                            break;
                        }
                        case 4:
                        {
                            if (talkCounter >= 3)
                                return;
                            if (talkTimer <= diff)
                            {
                                if (nextSpeaker == spectatorList.end())
                                    nextSpeaker = spectatorList.begin();
                                if (nextSpeaker != spectatorList.end())
                                {
                                    if (auto speaker = ObjectAccessor::GetCreature(*me, *nextSpeaker))
                                        sCreatureTextMgr->SendChat(speaker, talkCounter, player);
                                    nextSpeaker++;
                                }
                                talkCounter++;
                                talkTimer = 8 * IN_MILLISECONDS;
                            }
                            else
                                talkTimer -= diff;
                            break;
                        }
                    }
                }
                else
                    me->DespawnOrUnsummon();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_runnerAI(creature);
        }
};

Position bossElementalTargetPos = { 5676.132813f, 352.172821f, 159.143021f, 0.788695f };

enum {
    SPELL_REFLECTION_FIRST = 100009,
    SPELL_REFLECTION_LAST = 100013,
    SPELL_REFLECTION_VISUAL = 13567,
    SPELL_PETRIFYING_BREATH = 62030,
    SPELL_TRANSPARENCY = 44816,
    SPELL_MORTAL_WOUND = 54378,

    EVENT_CHANGE_REFLECTION = 1,
    EVENT_MORTAL_WOUND = 2,
    EVENT_SELF_HEAL = 3,
    EVENT_UPDATE_IN_COMBAT_LIST = 4,
};

struct OlympiaBaseAI : public ScriptedAI
{
        OlympiaBaseAI(Creature* creature, SpellSchools alternateMeleeMask) : alternateMeleeMask(alternateMeleeMask), eventMovement(false), isFinalBoss(false), canMelee(true), ScriptedAI(creature)
        { 
            me->setActive(true);
        }

        void Reset() override
        {
            questCredit.clear();

            me->SetReactState(REACT_DEFENSIVE);
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 1.f);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 20.f);

            petrifyingBreath = 6 * IN_MILLISECONDS;
            lastReflection = 0;
            commonEvents.Reset();
            commonEvents.ScheduleEvent(EVENT_CHANGE_REFLECTION, 30 * IN_MILLISECONDS);
            commonEvents.ScheduleEvent(EVENT_MORTAL_WOUND, 6 * IN_MILLISECONDS);
            commonEvents.ScheduleEvent(EVENT_SELF_HEAL, 15 * IN_MILLISECONDS);
            commonEvents.ScheduleEvent(EVENT_UPDATE_IN_COMBAT_LIST, 10 * IN_MILLISECONDS);

        }

        void DamageTaken(Unit* doneBy, uint32& damage) override
        {
            if (damage >= me->GetHealth() && !isFinalBoss)
            {
                damage = me->GetHealth() - 1;

                if (!me->IsInEvadeMode())
                    EnterEvadeMode();
                if (!eventMovement)
                {
                    Talk(0);
                    Talk(1);
                    me->AddAura(SPELL_TRANSPARENCY, me);
                }
                eventMovement = true;
                me->SetImmuneToAll(true);
            }
        }

        void UpdateOOC(uint32 diff)
        {
            if (eventMovement && !me->isMoving() && !me->IsInEvadeMode())
            {
                const float MAX_MOVE_DIST = 100.f;

                Position target = bossElementalTargetPos;
                me->MovePosition(target, 20.f, bossElementalTargetPos.GetOrientation() + (me->GetEntry() % 4) * M_PI / 2.f - me->GetOrientation());
                if (me->GetDistance(target) > MAX_MOVE_DIST)
                {
                    float angle = me->GetAngle(&target);
                    float x = me->GetPositionX() + MAX_MOVE_DIST * std::cos(angle);
                    float y = me->GetPositionY() + MAX_MOVE_DIST * std::sin(angle);
                    float z = me->GetPositionZ();
                    me->UpdateAllowedPositionZ(x, y, z);
                    me->GetMotionMaster()->MovePoint(0, x, y, z);
                }
                else if (me->GetDistance(target) > 0)
                    me->GetMotionMaster()->MovePoint(0, target);
                else if (me->GetOrientation() != me->GetAngle(&bossElementalTargetPos))
                    me->SetFacingTo(me->GetAngle(&bossElementalTargetPos));
            }
        }

        void UpdateAI(uint32 diff) override
        {
            commonEvents.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = commonEvents.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHANGE_REFLECTION:

                        if (lastReflection)
                            me->RemoveAurasDueToSpell(lastReflection);
                        lastReflection = urand(SPELL_REFLECTION_FIRST, SPELL_REFLECTION_LAST);
                        DoCastSelf(lastReflection, true);
                        DoCastSelf(SPELL_REFLECTION_VISUAL, true);
                        commonEvents.ScheduleEvent(EVENT_CHANGE_REFLECTION, 30 * IN_MILLISECONDS);
                        break;
                    case EVENT_MORTAL_WOUND:
                        DoCastVictim(SPELL_MORTAL_WOUND);
                        commonEvents.ScheduleEvent(EVENT_MORTAL_WOUND, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_SELF_HEAL:
                    {
                        uint32 heal = me->CountPctFromMaxHealth(1);
                        me->ModifyHealth(CalculatePct(heal, 100 + me->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HEALING_PCT)));
                        commonEvents.ScheduleEvent(EVENT_SELF_HEAL, 10 * IN_MILLISECONDS);
                        break;
                    }
                    case EVENT_UPDATE_IN_COMBAT_LIST:
                        if (!isFinalBoss)
                            break;
                        ThreatContainer::StorageType const& threatlist = me->GetThreatManager().getThreatList();
                        for (auto itr : threatlist)
                            questCredit.insert(itr->getTarget()->GetGUID());
                        break;
                }
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            Unit* victim = me->GetVictim();

            if (!me->IsWithinMeleeRange(victim))
            {
                if (petrifyingBreath <= diff)
                {
                    petrifyingBreath = 3 * IN_MILLISECONDS;
                    DoCastVictim(SPELL_PETRIFYING_BREATH);
                }
                else
                    petrifyingBreath -= diff;
                return;
            }
            else if (petrifyingBreath + diff >= 6 * IN_MILLISECONDS)
                petrifyingBreath = 6 * IN_MILLISECONDS;
            else
                petrifyingBreath = petrifyingBreath + diff;
        
            if (canMelee && me->isAttackReady())
            {
                me->AttackerStateUpdate(victim, BASE_ATTACK, false, false);
                me->SetMeleeDamageSchool(alternateMeleeMask);
                me->AttackerStateUpdate(victim, BASE_ATTACK, false, false);
                me->SetMeleeDamageSchool(SPELL_SCHOOL_NORMAL);
                me->resetAttackTimer();
            }
        }

    protected:
        std::unordered_set<ObjectGuid> questCredit;
        bool eventMovement;
        bool isFinalBoss;
        bool canMelee;
        SpellSchools alternateMeleeMask;
    private:
        EventMap commonEvents;
        uint32 lastReflection;
        uint32 petrifyingBreath;
};

enum OlympiaWaterBoss
{
    SPELL_FROST_BEACON = 70126,
    SPELL_FROSTBOLT_VOLLEY = 71889,
    SPELL_VILE_SLUDGE = 45573,
    SPELL_ICE_TOMB_DUMMY = 69675,

    SPELL_SPIRIT_OF_REDEMPPTION = 27827,

    EVENT_FROST_BEACON = 1,
    EVENT_FROSTBOLT_VOLLEY = 2,
    EVENT_VILE_SLUDGE = 3
};

class FrostBeaconSelector
{
    public:
        FrostBeaconSelector(Unit* source) : _source(source) { }

        bool operator()(Unit* target) const
        {
            return target->GetTypeId() == TYPEID_PLAYER &&
                target != _source->GetVictim() &&
                !target->HasAura(SPELL_FROST_BEACON) &&
                !target->HasAura(SPELL_SPIRIT_OF_REDEMPPTION);
        }

    private:
        Unit* _source;
};

/*######
## boss_olympia_water_boss
######*/
class boss_olympia_water_boss : public CreatureScript
{
    public:
        boss_olympia_water_boss() : CreatureScript("boss_olympia_water_boss") { }

        struct boss_olympia_water_bossAI : public OlympiaBaseAI
        {
            boss_olympia_water_bossAI(Creature* creature) : OlympiaBaseAI(creature, SPELL_SCHOOL_FROST) { }

            void Reset() override
            {
                OlympiaBaseAI::Reset();

                events.Reset();
                events.ScheduleEvent(EVENT_FROST_BEACON, 30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FROSTBOLT_VOLLEY, 12 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_VILE_SLUDGE, 10 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                {
                    UpdateOOC(diff);
                    return;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FROST_BEACON:
                            for (int i = 0; i < 2; i++)
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, FrostBeaconSelector(me)))
                            {
                                DoCast(target, SPELL_ICE_TOMB_DUMMY, true);
                                DoCast(target, SPELL_FROST_BEACON, true);
                            }
                            events.ScheduleEvent(EVENT_FROST_BEACON, 30 * IN_MILLISECONDS);
                            break;
                        case EVENT_FROSTBOLT_VOLLEY:
                            DoCastAOE(SPELL_FROSTBOLT_VOLLEY, true);
                            events.ScheduleEvent(EVENT_FROSTBOLT_VOLLEY, 12 * IN_MILLISECONDS);
                            break;
                        case EVENT_VILE_SLUDGE:
                        {
                            std::list<Unit*> targets;
                            //SelectTargetList(targets, 15, SELECT_TARGET_MINDISTANCE, 100, true);
                            for (std::list<Unit*>::const_iterator i = targets.begin(); i != targets.end(); ++i)
                                DoCast((*i), SPELL_VILE_SLUDGE, true);
                            events.ScheduleEvent(EVENT_VILE_SLUDGE, 30 * IN_MILLISECONDS);
                            break;
                        }
                    }
                }

                OlympiaBaseAI::UpdateAI(diff);
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_olympia_water_bossAI(creature);
        }
};

enum OlympiaFireBoss
{
    SPELL_FIRE_BOMB = 66313,
    SPELL_METEOR_SLASH = 45150,
    SPELL_CONFLAGRATION = 74456,

    EVENT_METEOR_SLASH = 1,
    EVENT_CONFLAGRATION = 2,
    EVENT_FIRE_BOMB = 3,
};

/*######
## boss_olympia_fire_boss
######*/
class boss_olympia_fire_boss : public CreatureScript
{
    public:
        boss_olympia_fire_boss() : CreatureScript("boss_olympia_fire_boss") { }

        struct boss_olympia_fire_bossAI : public OlympiaBaseAI
        {
            boss_olympia_fire_bossAI(Creature* creature) : OlympiaBaseAI(creature, SPELL_SCHOOL_FIRE) { }

            void Reset() override
            {
                OlympiaBaseAI::Reset();

                events.Reset();
                events.ScheduleEvent(EVENT_METEOR_SLASH, 14 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CONFLAGRATION, 10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FIRE_BOMB, 15 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                {
                    UpdateOOC(diff);
                    return;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_METEOR_SLASH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                DoCast(target, SPELL_METEOR_SLASH);
                            events.ScheduleEvent(EVENT_METEOR_SLASH, 35 * IN_MILLISECONDS);
                            break;
                        case EVENT_CONFLAGRATION:
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_MAXTHREAT, 0))
                                DoCast(target, SPELL_CONFLAGRATION, true);
                            std::list<Unit*> targets;
                            //SelectTargetList(targets, 2, SELECT_TARGET_RANDOM, 100, true);
                            for (auto i : targets)
                                DoCast(i, SPELL_CONFLAGRATION, true);
                            events.ScheduleEvent(EVENT_CONFLAGRATION, 35 * IN_MILLISECONDS);
                            break;
                        }
                        case EVENT_FIRE_BOMB:
                        {
                            std::list<Unit*> targets;
                            //SelectTargetList(targets, 4, SELECT_TARGET_RANDOM, 100, true);
                            for (auto i : targets)
                                DoCast(i, SPELL_FIRE_BOMB, true);
                            events.ScheduleEvent(EVENT_FIRE_BOMB, 10 * IN_MILLISECONDS);
                            break;
                        }
                    }
                }
                OlympiaBaseAI::UpdateAI(diff);
            }
            
        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_olympia_fire_bossAI(creature);
        }
};

enum OlympiaAirBoss
{
    SPELL_CHAIN_LIGHTNING = 59517,
    SPELL_THOR_LIGHTNING = 64390,
    SPELL_CHARGE = 56458,
    SPELL_SHIELD = 55019,
    SPELL_STARLIGHT = 62807,

    EVENT_SHIELD = 1,
    EVENT_CHAIN_LIGHTNING = 2,
    EVENT_STARLIGHT = 3
};

/*######
## boss_olympia_air_boss
######*/
class boss_olympia_air_boss : public CreatureScript
{
    public:
        boss_olympia_air_boss() : CreatureScript("boss_olympia_air_boss") { }

        struct boss_olympia_air_bossAI : public OlympiaBaseAI
        {
            boss_olympia_air_bossAI(Creature* creature) : OlympiaBaseAI(creature, SPELL_SCHOOL_NATURE) { }

            void Reset() override
            {
                OlympiaBaseAI::Reset();

                lightningCnt = 0;

                events.Reset();
                events.ScheduleEvent(EVENT_SHIELD, 35 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 6 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_STARLIGHT, 25 * IN_MILLISECONDS);
            }

            void SpellHitTarget(Unit* /*target*/, const SpellInfo* spellInfo) override
            {
                if (spellInfo->Id == SPELL_CHAIN_LIGHTNING)
                    DoCastSelf(SPELL_CHARGE, true);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                {
                    UpdateOOC(diff);
                    return;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHIELD:
                            if (auto shieldAura = me->AddAura(SPELL_SHIELD, me))
                            {
                                shieldAura->SetMaxDuration(20 * IN_MILLISECONDS);
                                shieldAura->RefreshDuration();
                                if (auto aurEff = shieldAura->GetEffect(EFFECT_0))
                                    aurEff->SetAmount(500000);
                            }
                            events.ScheduleEvent(EVENT_SHIELD, 20 * IN_MILLISECONDS);
                            break;
                        case EVENT_CHAIN_LIGHTNING:
                            if (lightningCnt >= 10)
                            {
                                DoCastVictim(SPELL_THOR_LIGHTNING, true);
                                lightningCnt = 0;
                            }
                            else
                            {
                                DoCastVictim(SPELL_CHAIN_LIGHTNING, true);
                                lightningCnt++;
                            }
                            events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 6 * IN_MILLISECONDS);
                            break;
                        case EVENT_STARLIGHT:
                        {
                            std::list<Unit*> targets;
                            //SelectTargetList(targets, 2, SELECT_TARGET_RANDOM, 100, true);
                            for (auto i : targets)
                                i->CastSpell(i, SPELL_STARLIGHT, true);
                            events.ScheduleEvent(EVENT_STARLIGHT, 60 * IN_MILLISECONDS);
                            break;
                        }
                    }
                }

                OlympiaBaseAI::UpdateAI(diff);
            }

        private:
            EventMap events;
            int lightningCnt;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_olympia_air_bossAI(creature);
        }
};

enum OlympiaEarthBoss
{
    SPELL_THROW_SARONITE = 68788,
    SPELL_MYSTIC_BUFFET = 70128,
    SPELL_MYSTIC_BUFFET_TRIGGER = 70127,
    SPELL_PLAGUE_STENCH = 71805,
    SPELL_ROOTS_IRONBRANCH_SUMMON = 62283,
    SPELL_ROOTS_IRONBRANCH_DOT = 62283,

    GO_SARONITE_ROCK = 196485,

    EVENT_ROOTS_IRONBRANCH = 1,
    EVENT_THROW_SARONITE = 2
};

class NotInMeleeRangeSelector
{
    public:
        NotInMeleeRangeSelector(Unit* source) : _source(source) { }

        bool operator()(Unit* target) const
        {
            return !target->IsWithinMeleeRange(_source);
        }

    private:
        Unit* _source;
};

/*######
## boss_olympia_earth_boss
######*/
class boss_olympia_earth_boss : public CreatureScript
{
    public:
        boss_olympia_earth_boss() : CreatureScript("boss_olympia_earth_boss") { }

        struct boss_olympia_earth_bossAI : public OlympiaBaseAI
        {
            boss_olympia_earth_bossAI(Creature* creature) : OlympiaBaseAI(creature, SPELL_SCHOOL_NATURE) { }

            void Reset() override
            {
                OlympiaBaseAI::Reset();

                DespawnSaronit();
                me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_MYSTIC_BUFFET_TRIGGER, true);

                events.Reset();
                events.ScheduleEvent(EVENT_ROOTS_IRONBRANCH, 25 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_THROW_SARONITE, 14 * IN_MILLISECONDS);
            }


            void JustDied(Unit* /*killer*/) override
            {
                DespawnSaronit();
                me->RemoveAllGameObjects();
            }

            void EnterCombat(Unit* /*victim*/) override 
            { 
                DoCastSelf(SPELL_MYSTIC_BUFFET, true);
                DoCastSelf(SPELL_PLAGUE_STENCH, true);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                {
                    UpdateOOC(diff);
                    return;
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_THROW_SARONITE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NotInMeleeRangeSelector(me)))
                                DoCast(target, SPELL_THROW_SARONITE, true);
                            events.ScheduleEvent(EVENT_THROW_SARONITE, 14 * IN_MILLISECONDS);
                            break;
                        case EVENT_ROOTS_IRONBRANCH:
                            std::list<Unit*> targets;
                            //SelectTargetList(targets, 10, SELECT_TARGET_RANDOM, 100, true, -SPELL_ROOTS_IRONBRANCH_DOT);
                            for (auto i : targets)
                                i->CastSpell(i, SPELL_ROOTS_IRONBRANCH_SUMMON, true);
                            events.ScheduleEvent(EVENT_ROOTS_IRONBRANCH, 25 * IN_MILLISECONDS);
                            break;
                    }
                }

                OlympiaBaseAI::UpdateAI(diff);
            }

        private:
            EventMap events;

            void DespawnSaronit()
            {
                std::list<GameObject*> SaronitList;
                me->GetGameObjectListWithEntryInGrid(SaronitList, GO_SARONITE_ROCK, 300.0f);

                for (std::list<GameObject*>::const_iterator itr = SaronitList.begin(); itr != SaronitList.end(); ++itr)
                    if (GameObject* saronit = (*itr))
                        saronit->Delete();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_olympia_earth_bossAI(creature);
        }
};

enum
{
    WORLDSTATE_BOSS = 30002,

    PHASE_WATER = 0,
    PHASE_FIRE = 2,
    PHASE_EARTH = 4,
    PHASE_AIR = 6,

    NPC_SHADOWTRAP = 39137,

    EVENT_SHADOWTRAP = 1,

    SPELL_TARGET_INDICATOR = 43313,
    SPELL_SHADOWBOLT_VOLLEY = 57942,
    SPELL_SHADOWTRAP = 73539,
    SPELL_SHADOW_SHIELD = 65874,

    QUEST_SAVE_AZEROTH = 110186,
    QUEST_SAVE_AZEROTH_2 = 110187,

    PHASE_COUNT = 8,

    RESPAWN_TIME = 3 * HOUR,
};

/*######
## boss_olympia_shadow_boss
######*/
class boss_olympia_shadow_boss : public CreatureScript
{
    public:
        boss_olympia_shadow_boss() : CreatureScript("boss_olympia_shadow_boss") { }

        struct boss_olympia_shadow_bossAI : public OlympiaBaseAI
        {
            boss_olympia_shadow_bossAI(Creature* creature) : OlympiaBaseAI(creature, SPELL_SCHOOL_SHADOW) 
            {
                isFinalBoss = true;
            }

            void Reset() override
            {
                phaseId = urand(0, PHASE_COUNT - 1);
                phaseId = (phaseId + phaseId % 2) % PHASE_COUNT;
                phaseTimer = 10 * IN_MILLISECONDS;

                events.Reset();

                DespawnSaronit();

                OlympiaBaseAI::Reset();
            }

            void JustDied(Unit* /*killer*/) override
            {
                DespawnSaronit();
                me->RemoveAllGameObjects();

                for (auto itr : questCredit)
                    if (auto plr = sObjectAccessor->GetPlayer(*me, itr))
                    {
                        plr->AreaExploredOrEventHappens(QUEST_SAVE_AZEROTH);
                        plr->AreaExploredOrEventHappens(QUEST_SAVE_AZEROTH_2);
                    }

                sWorld->setWorldState(WORLDSTATE_BOSS, time(nullptr) + RESPAWN_TIME);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (phaseTimer <= diff)
                {
                    phaseId = (phaseId + 1) % PHASE_COUNT;
                    phaseTimer = ((phaseId % 2 == 1) ? 30 : 90) * IN_MILLISECONDS;
                    events.Reset();
                    if (phaseId % 2 == 1)
                    {
                        DespawnSaronit();
                        canMelee = false;
                        std::list<Creature*> shadowtraps;
                        me->GetAllMinionsByEntry(shadowtraps, NPC_SHADOWTRAP);
                        events.ScheduleEvent(EVENT_SHADOWTRAP, 5 * IN_MILLISECONDS);

                        shadowtrapTargets.clear();
                        std::list<Unit*> targets;
                        //SelectTargetList(targets, 3, SELECT_TARGET_RANDOM, 100, true);
                        for (auto i : targets)
                        {
                            shadowtrapTargets.push_back(i->GetGUID());
                            if (auto aura = me->AddAura(SPELL_TARGET_INDICATOR, i))
                            {
                                aura->SetMaxDuration(30 * IN_MILLISECONDS);
                                aura->RefreshDuration();
                            }
                        }

                        if (auto shieldAura = me->AddAura(SPELL_SHADOW_SHIELD, me))
                        {
                            shieldAura->SetMaxDuration(120 * IN_MILLISECONDS);
                            shieldAura->RefreshDuration();
                            if (auto aurEff = shieldAura->GetEffect(EFFECT_0))
                                aurEff->SetAmount(1000000);
                        }

                        DoCastAOE(SPELL_SHADOWBOLT_VOLLEY, true);
                    }
                    else
                    {
                        canMelee = true;
                        switch (phaseId)
                        {
                            case PHASE_WATER:
                                events.ScheduleEvent(EVENT_FROST_BEACON, 30 * IN_MILLISECONDS);
                                events.ScheduleEvent(EVENT_FROSTBOLT_VOLLEY, 12 * IN_MILLISECONDS);
                                events.ScheduleEvent(EVENT_VILE_SLUDGE, 10 * IN_MILLISECONDS);
                                alternateMeleeMask = SPELL_SCHOOL_FROST;
                                break;
                            case PHASE_FIRE:
                                events.ScheduleEvent(EVENT_METEOR_SLASH, 14 * IN_MILLISECONDS);
                                events.ScheduleEvent(EVENT_CONFLAGRATION, 10 * IN_MILLISECONDS);
                                events.ScheduleEvent(EVENT_FIRE_BOMB, 15 * IN_MILLISECONDS);
                                alternateMeleeMask = SPELL_SCHOOL_FIRE;
                                break;
                            case PHASE_EARTH:
                                events.ScheduleEvent(EVENT_ROOTS_IRONBRANCH, 25 * IN_MILLISECONDS);
                                events.ScheduleEvent(EVENT_THROW_SARONITE, 14 * IN_MILLISECONDS);
                                alternateMeleeMask = SPELL_SCHOOL_NATURE;
                                break;
                            case PHASE_AIR:
                                events.ScheduleEvent(EVENT_SHIELD, 35 * IN_MILLISECONDS);
                                events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 6 * IN_MILLISECONDS);
                                events.ScheduleEvent(EVENT_STARLIGHT, 25 * IN_MILLISECONDS);
                                alternateMeleeMask = SPELL_SCHOOL_NATURE;
                                break;
                        }
                    }
                }
                else
                    phaseTimer -= diff;

                if (phaseId % 2 == 1)
                {
                    while (uint32 eventId = events.ExecuteEvent())
                    {
                        switch (eventId)
                        {
                            case EVENT_SHADOWTRAP:
                            {
                                for (auto i : shadowtrapTargets)
                                    if (auto target = ObjectAccessor::GetUnit(*me, i))
                                        DoCast(target, SPELL_SHADOWTRAP, true);
                                events.ScheduleEvent(EVENT_SHADOWTRAP, 3 * IN_MILLISECONDS);
                            }
                        }
                    }
                }
                else
                {
                    switch (phaseId)
                    {
                        case PHASE_WATER:
                            while (uint32 eventId = events.ExecuteEvent())
                            {
                                switch (eventId)
                                {
                                    case EVENT_FROST_BEACON:
                                        for (int i = 0; i < 2; i++)
                                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, FrostBeaconSelector(me)))
                                        {
                                            DoCast(target, SPELL_ICE_TOMB_DUMMY, true);
                                            DoCast(target, SPELL_FROST_BEACON, true);
                                        }
                                        events.ScheduleEvent(EVENT_FROST_BEACON, 30 * IN_MILLISECONDS);
                                        break;
                                    case EVENT_FROSTBOLT_VOLLEY:
                                        DoCastAOE(SPELL_FROSTBOLT_VOLLEY, true);
                                        events.ScheduleEvent(EVENT_FROSTBOLT_VOLLEY, 12 * IN_MILLISECONDS);
                                        break;
                                    case EVENT_VILE_SLUDGE:
                                    {
                                        std::list<Unit*> targets;
                                        //SelectTargetList(targets, 15, SELECT_TARGET_MINDISTANCE, 100, true);
                                        for (std::list<Unit*>::const_iterator i = targets.begin(); i != targets.end(); ++i)
                                            DoCast((*i), SPELL_VILE_SLUDGE, true);
                                        events.ScheduleEvent(EVENT_VILE_SLUDGE, 30 * IN_MILLISECONDS);
                                        break;
                                    }
                                }
                            }
                            break;
                        case PHASE_FIRE:
                            while (uint32 eventId = events.ExecuteEvent())
                            {
                                switch (eventId)
                                {
                                    case EVENT_METEOR_SLASH:
                                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                            DoCast(target, SPELL_METEOR_SLASH);
                                        events.ScheduleEvent(EVENT_METEOR_SLASH, 35 * IN_MILLISECONDS);
                                        break;
                                    case EVENT_CONFLAGRATION:
                                    {
                                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                            DoCast(target, SPELL_CONFLAGRATION, true);
                                        std::list<Unit*> targets;
                                        //SelectTargetList(targets, 2, SELECT_TARGET_RANDOM, 100, true);
                                        for (auto i : targets)
                                            DoCast(i, SPELL_CONFLAGRATION, true);
                                        events.ScheduleEvent(EVENT_CONFLAGRATION, 35 * IN_MILLISECONDS);
                                        break;
                                    }
                                    case EVENT_FIRE_BOMB:
                                    {
                                        std::list<Unit*> targets;
                                        //SelectTargetList(targets, 10, SELECT_TARGET_RANDOM, 100, true);
                                        for (auto i : targets)
                                            DoCast(i, SPELL_FIRE_BOMB, true);
                                        events.ScheduleEvent(EVENT_FIRE_BOMB, 10 * IN_MILLISECONDS);
                                        break;
                                    }
                                }
                            }
                            break;
                        case PHASE_EARTH:
                            while (uint32 eventId = events.ExecuteEvent())
                            {
                                switch (eventId)
                                {
                                    case EVENT_THROW_SARONITE:
                                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NotInMeleeRangeSelector(me)))
                                            DoCast(target, SPELL_THROW_SARONITE, true);
                                        events.ScheduleEvent(EVENT_THROW_SARONITE, 14 * IN_MILLISECONDS);
                                        break;
                                    case EVENT_ROOTS_IRONBRANCH:
                                        std::list<Unit*> targets;
                                        //SelectTargetList(targets, 10, SELECT_TARGET_RANDOM, 100, true, -SPELL_ROOTS_IRONBRANCH_DOT);
                                        for (auto i : targets)
                                            i->CastSpell(i, SPELL_ROOTS_IRONBRANCH_SUMMON, true);
                                        events.ScheduleEvent(EVENT_ROOTS_IRONBRANCH, 25 * IN_MILLISECONDS);
                                        break;
                                }
                            }
                            break;
                        case PHASE_AIR:
                            while (uint32 eventId = events.ExecuteEvent())
                            {
                                switch (eventId)
                                {
                                    case EVENT_SHIELD:
                                        if (auto shieldAura = me->AddAura(SPELL_SHIELD, me))
                                        {
                                            shieldAura->SetMaxDuration(20 * IN_MILLISECONDS);
                                            shieldAura->RefreshDuration();
                                            if (auto aurEff = shieldAura->GetEffect(EFFECT_0))
                                                aurEff->SetAmount(500000);
                                        }
                                        events.ScheduleEvent(EVENT_SHIELD, 20 * IN_MILLISECONDS);
                                        break;
                                    case EVENT_CHAIN_LIGHTNING:
                                        if (lightningCnt >= 10)
                                        {
                                            DoCastVictim(SPELL_THOR_LIGHTNING, true);
                                            lightningCnt = 0;
                                        }
                                        else
                                        {
                                            DoCastVictim(SPELL_CHAIN_LIGHTNING, true);
                                            lightningCnt++;
                                        }
                                        events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 6 * IN_MILLISECONDS);
                                        break;
                                    case EVENT_STARLIGHT:
                                    {
                                        std::list<Unit*> targets;
                                        //SelectTargetList(targets, 2, SELECT_TARGET_RANDOM, 100, true);
                                        for (auto i : targets)
                                            i->CastSpell(i, SPELL_STARLIGHT, true);
                                        events.ScheduleEvent(EVENT_STARLIGHT, 60 * IN_MILLISECONDS);
                                        break;
                                    }
                                }
                            }
                            break;
                    }
                }

                OlympiaBaseAI::UpdateAI(diff);
            }

        private:
            std::vector<ObjectGuid> shadowtrapTargets;
            uint32 phaseId;
            uint32 phaseTimer;
            int lightningCnt;
            EventMap events;

            void DespawnSaronit()
            {
                std::list<GameObject*> SaronitList;
                me->GetGameObjectListWithEntryInGrid(SaronitList, GO_SARONITE_ROCK, 300.0f);

                for (std::list<GameObject*>::const_iterator itr = SaronitList.begin(); itr != SaronitList.end(); ++itr)
                    if (GameObject* saronit = (*itr))
                        saronit->Delete();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_olympia_shadow_bossAI(creature);
        }
};

enum OlympiaElementSpawnEvents
{
    EVENT_SPAWN_WEAK_ELEMENTAL = 1,
    EVENT_SPAWN_ELITE_ELEMENTAL = 2,
    EVENT_SPAWN_BOSS_ELEMENTAL = 3,
    EVENT_RECHECK_SPAWN = 4
};

enum OlympiaWeakElementalEvents
{
    EVENT_ELEMENTAL_HAZARD = 1,
};

enum OlympiaWeakElementalSpells //rarely used by weak elementals, more often if elemental power rises
{
    SPELL_FIRE_RAIN = 37279,
    SPELL_EARTH_FURY = 54193, //throws enemys in the air, 10m range
    SPELL_FROST_ARMOR = 15784,
    SPELL_STORM_STRIKE = 56352,
};

enum OlympiaElementNpcs
{
    EVENT_NPC_WEAK_ELEMENTAL_WATER = 1010770,
    EVENT_NPC_WEAK_ELEMENTAL_FIRE = 1010771,
    EVENT_NPC_WEAK_ELEMENTAL_WIND = 1010772,
    EVENT_NPC_WEAK_ELEMENTAL_EARTH = 1010773,
    EVENT_NPC_ELITE_ELEMENTAL_WATER = 1010778,
    EVENT_NPC_ELITE_ELEMENTAL_FIRE = 1010779,
    EVENT_NPC_ELITE_ELEMENTAL_WIND = 1010780,
    EVENT_NPC_ELITE_ELEMENTAL_EARTH = 1010781,
    EVENT_NPC_BOSS_ELEMENTAL_WATER = 1010728,
    EVENT_NPC_BOSS_ELEMENTAL_FIRE = 1010725,
    EVENT_NPC_BOSS_ELEMENTAL_WIND = 1010726,
    EVENT_NPC_BOSS_ELEMENTAL_EARTH = 1010727,
};

struct ElementalHazard
{
    ElementalHazard(uint32 eventId, uint32 spellId, uint32 time, float baseChance) : eventId(eventId), spellId(spellId), time(time), baseChance(baseChance)
    {}
    ElementalHazard() : eventId(0), spellId(0), time(0), baseChance(0)
    {}

    uint32 eventId;
    uint32 spellId;
    uint32 time; 
    float baseChance;
};

#define SPAWN_RANGE 50.0f

struct OlympiaElementalSpawnerAI : public CreatureAI
{
    OlympiaElementalSpawnerAI(Creature* creature, uint32 weakElementalId, uint32 eliteElementalId, uint32 bossElementalId) : weakElementalId(weakElementalId), eliteElementalId(eliteElementalId), bossElementalId(bossElementalId), CreatureAI(creature) { }

    bool CanSpawn()
    {
        auto state = sWorld->getWorldState(WORLDSTATE_BOSS);
        return state != 1 && state < static_cast<decltype(state)>(time(nullptr));
    }

    void Reset() override
    {
        spawnEvents.Reset();
        elementalPower = 1;
        if (CanSpawn())
        {
            for (int i = 0; i < 30; i++)
                spawnEvents.ScheduleEvent(EVENT_SPAWN_WEAK_ELEMENTAL, (i + 1) * 6 * IN_MILLISECONDS);
            inBossPhase = false;
        }
        else
        {
            spawnEvents.ScheduleEvent(EVENT_RECHECK_SPAWN, 10 * IN_MILLISECONDS);
            inBossPhase = true;
        }
    }

    void SummonedCreatureDies(Creature* deadElemental, Unit* killer) override {
        if (deadElemental->GetEntry() == weakElementalId)
        {
            weakElementals.remove(deadElemental->GetGUID());
            if (elementalPower < 200)
            {
                spawnEvents.ScheduleEvent(EVENT_SPAWN_WEAK_ELEMENTAL, 10 * IN_MILLISECONDS);
                RaiseElementalPower(1);
            }
        }

        if (deadElemental->GetEntry() == eliteElementalId)
        {
            eliteElementals.remove(deadElemental->GetGUID());
            if (eliteElementals.empty() && elementalPower > 230) //atleast 4 elite must be dead (the 4 spawning ones)
                StartBossPhase();
			if (!inBossPhase)
			{
				spawnEvents.ScheduleEvent(EVENT_SPAWN_ELITE_ELEMENTAL, 30 * IN_MILLISECONDS);
				RaiseElementalPower(10);
			}
        }

        if (deadElemental->GetEntry() == bossElementalId)
            spawnEvents.ScheduleEvent(EVENT_RECHECK_SPAWN, RESPAWN_TIME * IN_MILLISECONDS);
    }

    void StartEliteElementalPhase()
    {
        for (int i = 0; i < 4; i++)
        {
            spawnEvents.ScheduleEvent(EVENT_SPAWN_ELITE_ELEMENTAL, (i + 1) * 15 * IN_MILLISECONDS);
        }
    }

    void StartBossPhase()
    {
        if (inBossPhase)
            return;
        spawnEvents.Reset();
        spawnEvents.ScheduleEvent(EVENT_SPAWN_BOSS_ELEMENTAL, 30 * IN_MILLISECONDS);
        inBossPhase = true;
    }

    void RaiseElementalPower(uint32 powerGain)
    {
        elementalPower+= powerGain;
        if (elementalPower % 5 == 0)
        {
            for (auto weakElementalGUID : weakElementals)
            {
                auto weakElemental = sObjectAccessor->GetCreature(*me, weakElementalGUID);
                if (!weakElemental)
                    continue;
                Aura* buffAura = weakElemental->GetAura(100020);
                if (!buffAura)
                    buffAura = weakElemental->AddAura(100020, weakElemental);
                if(buffAura->GetStackAmount() < 40)
                    buffAura->ModStackAmount(1);
            }
        }
        if (elementalPower == 200)
        {
            StartEliteElementalPhase();
        }
        if (elementalPower == 400)
        {
            StartBossPhase();
        }
    }

    void JustSummoned(Creature* elemental)
    {
        if (elemental->GetEntry() == weakElementalId)
        {
            Aura* buffAura = elemental->GetAura(100020);
            if (!buffAura)
                buffAura = elemental->AddAura(100020, elemental);
            buffAura->ModStackAmount(std::min((uint8) 40, (uint8)(elementalPower / 5)));
        }

        if (elemental->GetEntry() != bossElementalId)
        {
            if (!me->IsWithinLOSInMap(elemental))
            {
                Position target;
                me->GetRandomNearPosition(target, 10.f);
                elemental->Relocate(target);
            }
            elemental->GetMotionMaster()->MoveRandom(5.f);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        spawnEvents.Update(diff);

        while (uint32 eventId = spawnEvents.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_SPAWN_WEAK_ELEMENTAL:
            {
                Creature* summon = DoSummon(weakElementalId, me, SPAWN_RANGE);
                weakElementals.push_back(summon->GetGUID());
            }
                break;
            case EVENT_SPAWN_ELITE_ELEMENTAL:
            {
                Creature* summon = DoSummon(eliteElementalId, me, SPAWN_RANGE);
                eliteElementals.push_back(summon->GetGUID());
            }
            break;
            case EVENT_SPAWN_BOSS_ELEMENTAL:
            {
                Creature* summon = DoSummon(bossElementalId, me);
            }
            break;
            case EVENT_RECHECK_SPAWN:
                Reset();
                break;
            }
        }
    }
protected:
    std::list<ObjectGuid> weakElementals;
    std::list<ObjectGuid> eliteElementals;
    uint32 weakElementalId;
    uint32 eliteElementalId;
    uint32 bossElementalId;
    uint32 elementalPower;

private:
    EventMap spawnEvents;
    bool inBossPhase;

};

class npc_olympia_water_spawner : public CreatureScript
{
public:
    npc_olympia_water_spawner() : CreatureScript("npc_olympia_water_spawner") {}

    struct npc_olympia_water_spawnerAI : public OlympiaElementalSpawnerAI
    {
        npc_olympia_water_spawnerAI(Creature* creature) : OlympiaElementalSpawnerAI(creature, EVENT_NPC_WEAK_ELEMENTAL_WATER, EVENT_NPC_ELITE_ELEMENTAL_WATER, EVENT_NPC_BOSS_ELEMENTAL_WATER)
        {}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_water_spawnerAI(creature);
    }
};

class npc_olympia_fire_spawner : public CreatureScript
{
public:
    npc_olympia_fire_spawner() : CreatureScript("npc_olympia_fire_spawner") {}

    struct npc_olympia_fire_spawnerAI : public OlympiaElementalSpawnerAI
    {
        npc_olympia_fire_spawnerAI(Creature* creature) : OlympiaElementalSpawnerAI(creature, EVENT_NPC_WEAK_ELEMENTAL_FIRE, EVENT_NPC_ELITE_ELEMENTAL_FIRE, EVENT_NPC_BOSS_ELEMENTAL_FIRE)
        {}

        void UpdateAI(uint32 diff) override
        {
            OlympiaElementalSpawnerAI::UpdateAI(diff);
        }
     
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_fire_spawnerAI(creature);
    }
};

class npc_olympia_wind_spawner : public CreatureScript
{
public:
    npc_olympia_wind_spawner() : CreatureScript("npc_olympia_wind_spawner") {}

    struct npc_olympia_wind_spawnerAI : public OlympiaElementalSpawnerAI
    {
        npc_olympia_wind_spawnerAI(Creature* creature) : OlympiaElementalSpawnerAI(creature, EVENT_NPC_WEAK_ELEMENTAL_WIND, EVENT_NPC_ELITE_ELEMENTAL_WIND, EVENT_NPC_BOSS_ELEMENTAL_WIND)
        {}

        void UpdateAI(uint32 diff) override
        {
            OlympiaElementalSpawnerAI::UpdateAI(diff);
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_wind_spawnerAI(creature);
    }
};

class npc_olympia_earth_spawner : public CreatureScript
{
public:
    npc_olympia_earth_spawner() : CreatureScript("npc_olympia_earth_spawner") {}

    struct npc_olympia_earth_spawnerAI : public OlympiaElementalSpawnerAI
    {
        npc_olympia_earth_spawnerAI(Creature* creature) : OlympiaElementalSpawnerAI(creature, EVENT_NPC_WEAK_ELEMENTAL_EARTH, EVENT_NPC_ELITE_ELEMENTAL_EARTH, EVENT_NPC_BOSS_ELEMENTAL_EARTH)
        {}

        void UpdateAI(uint32 diff) override
        {
            OlympiaElementalSpawnerAI::UpdateAI(diff);
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_earth_spawnerAI(creature);
    }
};

struct OlympiaWeakElementalAI : public CreatureAI
{
    OlympiaWeakElementalAI(Creature* creature, uint32 hazardSpellId, float baseChance) : hazardSpellId(hazardSpellId), baseChance(baseChance),CreatureAI(creature)
    {
        Reset();
    }

    void Reset()
    {
        spellEvents.Reset();
    }

    void EnterCombat(Unit* target)
    {
        spellEvents.ScheduleEvent(EVENT_ELEMENTAL_HAZARD, 5 * IN_MILLISECONDS);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        spellEvents.Update(diff);

        while (uint32 eventId = spellEvents.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_ELEMENTAL_HAZARD:
                AuraApplication* elementalBuff = me->GetAuraApplication(100020);
                if (elementalBuff)
                {
                    uint32 stackAmount = elementalBuff->GetBase()->GetStackAmount();
                    if (roll_chance_f(baseChance*stackAmount))
                        DoCastVictim(hazardSpellId, true);
                    spellEvents.ScheduleEvent(EVENT_ELEMENTAL_HAZARD, 5 * IN_MILLISECONDS);
                }
                break;
            }
        }

        DoMeleeAttackIfReady();

    }
protected:
    uint32 hazardSpellId;
    float baseChance;

private:
    EventMap spellEvents;

};

class npc_olympia_weak_fire_elemental : public CreatureScript
{
public:
    npc_olympia_weak_fire_elemental() : CreatureScript("npc_olympia_weak_fire_elemental") {}

    struct npc_olympia_weak_fire_elementalAI : public OlympiaWeakElementalAI
    {
        npc_olympia_weak_fire_elementalAI(Creature* creature) : OlympiaWeakElementalAI(creature, SPELL_FIRE_RAIN, 0.8f)
        {}

        void UpdateAI(uint32 diff) override
        {
            OlympiaWeakElementalAI::UpdateAI(diff);
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_weak_fire_elementalAI(creature);
    }
};

class npc_olympia_weak_water_elemental : public CreatureScript
{
public:
    npc_olympia_weak_water_elemental() : CreatureScript("npc_olympia_weak_water_elemental") {}

    struct npc_olympia_weak_water_elementalAI : public OlympiaWeakElementalAI
    {
        npc_olympia_weak_water_elementalAI(Creature* creature) : OlympiaWeakElementalAI(creature, SPELL_FROST_ARMOR, 1.25f)
        {}

        void UpdateAI(uint32 diff) override
        {
            OlympiaWeakElementalAI::UpdateAI(diff);
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_weak_water_elementalAI(creature);
    }
};

class npc_olympia_weak_wind_elemental : public CreatureScript
{
public:
    npc_olympia_weak_wind_elemental() : CreatureScript("npc_olympia_weak_wind_elemental") {}

    struct npc_olympia_weak_wind_elementalAI : public OlympiaWeakElementalAI
    {
        npc_olympia_weak_wind_elementalAI(Creature* creature) : OlympiaWeakElementalAI(creature, SPELL_STORM_STRIKE, 5.0f)
        {}

        void UpdateAI(uint32 diff) override
        {
            OlympiaWeakElementalAI::UpdateAI(diff);
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_weak_wind_elementalAI(creature);
    }
};

class npc_olympia_weak_earth_elemental : public CreatureScript
{
public:
    npc_olympia_weak_earth_elemental() : CreatureScript("npc_olympia_weak_earth_elemental") {}

    struct npc_olympia_weak_earth_elementalAI : public OlympiaWeakElementalAI
    {
        npc_olympia_weak_earth_elementalAI(Creature* creature) : OlympiaWeakElementalAI(creature, SPELL_EARTH_FURY, 0.75f)
        {}

        void UpdateAI(uint32 diff) override
        {
            OlympiaWeakElementalAI::UpdateAI(diff);
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_weak_earth_elementalAI(creature);
    }
};

enum OlympiaEliteElementalSpells //rarely used by weak elementals, more often if elemental power rises
{
    // water elite
    SPELL_FROST_VOLLEY = 58532,
    SPELL_FROST_SHOCK = 41116,
    SPELL_FROST_ATTACK = 39087, //throws enemys in the air, 10m range
    SPELL_RIPTIDE = 75367,

    SPELL_FIRE_VOLLEY = 38836,
    SPELL_FLAME_CINDER = 66684,
    SPELL_RAGE = 19451,

    SPELL_THUNDER_BLADE = 55866, //additional 15% damage meele strike, which hits up to 2 extra targets
    SPELL_WIND_SHEAR = 52870,

    SPELL_STONE_FISTS = 62344,
    SPELL_REFLECTION = 31554,
    SPELL_THUNDERING_STOMP = 60925
};

enum OlympiaEliteElementalEvents //rarely used by weak elementals, more often if elemental power rises
{
    // water elite
    EVENT_FROST_VOLLEY = 1,
    EVENT_FROST_SHOCK,
    EVENT_FROST_ATTACK,
    EVENT_RIPTIDE,

    EVENT_FIRE_VOLLEY,
    EVENT_FLAME_CINDER,
    EVENT_RAGE,

    EVENT_ELITE_CHAIN_LIGHTNING,
    EVENT_WIND_SHEAR, //additional 15% damage meele strike, which hits up to 2 extra targets

    EVENT_STONE_FISTS,
    EVENT_REFLECTION,
    EVENT_THUNDERING_STOMP
};

class npc_olympia_elite_water_elemental : CreatureScript
{
public:
    npc_olympia_elite_water_elemental() : CreatureScript("npc_olympia_elite_water_elemental") {}

    struct npc_olympia_elite_water_elementalAI : public CreatureAI
    {
        npc_olympia_elite_water_elementalAI(Creature* creature) : CreatureAI(creature)
        {}

        void reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* target)
        {
            events.ScheduleEvent(EVENT_FROST_VOLLEY, irand(8 * IN_MILLISECONDS, 12 * IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_FROST_SHOCK, irand(6 * IN_MILLISECONDS, 8 * IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_FROST_ATTACK, irand(6 * IN_MILLISECONDS, 8 * IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_RIPTIDE, irand(20 * IN_MILLISECONDS, 25 * IN_MILLISECONDS));
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_FROST_VOLLEY:
                    DoCastVictim(SPELL_FROST_VOLLEY);
                    events.ScheduleEvent(EVENT_FROST_VOLLEY, irand(8 * IN_MILLISECONDS, 12 * IN_MILLISECONDS));
                    break;
                case EVENT_FROST_SHOCK:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, false))
                        DoCast(target, SPELL_FROST_SHOCK);
                    events.ScheduleEvent(EVENT_FROST_SHOCK, irand(6 * IN_MILLISECONDS, 8 * IN_MILLISECONDS));
                    break;
                case EVENT_FROST_ATTACK:
                    DoCastVictim(SPELL_FROST_ATTACK);
                    events.ScheduleEvent(EVENT_FROST_ATTACK, irand(6 * IN_MILLISECONDS, 8 * IN_MILLISECONDS));
                    break;
                case EVENT_RIPTIDE:
                    DoCastSelf(SPELL_RIPTIDE);
                    events.ScheduleEvent(EVENT_RIPTIDE, irand(20 * IN_MILLISECONDS, 25 * IN_MILLISECONDS));
                    break;
                }

            }
            DoMeleeAttackIfReady();
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_elite_water_elementalAI(creature);
    }
};

class npc_olympia_elite_fire_elemental : CreatureScript
{
public:
    npc_olympia_elite_fire_elemental() : CreatureScript("npc_olympia_elite_fire_elemental") {}

    struct npc_olympia_elite_fire_elementalAI : public CreatureAI
    {
        npc_olympia_elite_fire_elementalAI(Creature* creature) : CreatureAI(creature)
        {}

        void reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* target)
        {
            events.ScheduleEvent(EVENT_FIRE_VOLLEY, irand(8 * IN_MILLISECONDS, 12 * IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_FLAME_CINDER, irand(10 * IN_MILLISECONDS, 14 * IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_RAGE, irand(20 * IN_MILLISECONDS, 25 * IN_MILLISECONDS));
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_FIRE_VOLLEY:
                    DoCastVictim(SPELL_FIRE_VOLLEY);
                    events.ScheduleEvent(EVENT_FIRE_VOLLEY, irand(8 * IN_MILLISECONDS, 12 * IN_MILLISECONDS));
                    break;
                case EVENT_FLAME_CINDER:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, false))
                        DoCast(target, SPELL_FLAME_CINDER);
                    events.ScheduleEvent(EVENT_FLAME_CINDER, irand(7 * IN_MILLISECONDS, 8 * IN_MILLISECONDS));
                    break;
                case EVENT_RAGE:
                    DoCastSelf(SPELL_RAGE);
                    events.ScheduleEvent(EVENT_RAGE, irand(20 * IN_MILLISECONDS, 25 * IN_MILLISECONDS));
                    break;
                }

            }
            DoMeleeAttackIfReady();
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_elite_fire_elementalAI(creature);
    }
};

class npc_olympia_elite_wind_elemental : CreatureScript
{
public:
    npc_olympia_elite_wind_elemental() : CreatureScript("npc_olympia_elite_wind_elemental") {}

    struct npc_olympia_elite_wind_elementalAI : public CreatureAI
    {
        npc_olympia_elite_wind_elementalAI(Creature* creature) : CreatureAI(creature)
        {}

        void reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* target)
        {
            events.ScheduleEvent(EVENT_ELITE_CHAIN_LIGHTNING, irand(8 * IN_MILLISECONDS, 12 * IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_WIND_SHEAR, irand(7 * IN_MILLISECONDS, 10 * IN_MILLISECONDS));
            DoCastVictim(SPELL_THUNDER_BLADE);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_ELITE_CHAIN_LIGHTNING:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, false))
                        DoCast(target, SPELL_CHAIN_LIGHTNING);
                    events.ScheduleEvent(EVENT_ELITE_CHAIN_LIGHTNING, irand(14 * IN_MILLISECONDS, 18 * IN_MILLISECONDS));
                    break;
                case EVENT_WIND_SHEAR:
                    DoCastVictim(SPELL_WIND_SHEAR);
                    events.ScheduleEvent(EVENT_WIND_SHEAR, irand(12 * IN_MILLISECONDS, 14 * IN_MILLISECONDS));
                    break;
                }

            }
            DoMeleeAttackIfReady();
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_elite_wind_elementalAI(creature);
    }
};

class npc_olympia_elite_earth_elemental : CreatureScript
{
public:
    npc_olympia_elite_earth_elemental() : CreatureScript("npc_olympia_elite_earth_elemental") {}

    struct npc_olympia_elite_earth_elementalAI : public CreatureAI
    {
        npc_olympia_elite_earth_elementalAI(Creature* creature) : CreatureAI(creature)
        {}

        void reset()
        {
            events.Reset();
        }

        void EnterCombat(Unit* target)
        {
            events.ScheduleEvent(EVENT_THUNDERING_STOMP, irand(8 * IN_MILLISECONDS, 12 * IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_STONE_FISTS, irand(18 * IN_MILLISECONDS, 22 * IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_REFLECTION, irand(5 * IN_MILLISECONDS, 6 * IN_MILLISECONDS));
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_THUNDERING_STOMP:
                    DoCastVictim(SPELL_THUNDERING_STOMP);
                    events.ScheduleEvent(EVENT_THUNDERING_STOMP, irand(16 * IN_MILLISECONDS, 18 * IN_MILLISECONDS));
                    break;
                case EVENT_REFLECTION:
                    DoCastSelf(SPELL_REFLECTION);
                    events.ScheduleEvent(EVENT_FROST_SHOCK, irand(20 * IN_MILLISECONDS, 22 * IN_MILLISECONDS));
                    break;
                case EVENT_STONE_FISTS:
                    DoCastSelf(SPELL_STONE_FISTS);
                    events.ScheduleEvent(EVENT_STONE_FISTS, irand(18 * IN_MILLISECONDS, 22 * IN_MILLISECONDS));
                    break;
                }

            }
            DoMeleeAttackIfReady();
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_olympia_elite_earth_elementalAI(creature);
    }
};

/*######
## npc_olympia_siegbert
######*/

enum 
{
    NPC_L70ETC_START            = 23623,
    NPC_L70ETC_END              = 23626,

    SPELL_INVISIBILITY          = 16380,
    SPELL_VISUAL_1              = 42499,
    SPELL_VISUAL_2              = 42500,
    SPELL_VISUAL_3              = 42501,

    MUSIC_L70ETC                = 11803,

    EVENT_CHECK_RESTART         = 1,
    EVENT_SPECIAL_EFFECT        = 2,
};

class npc_olympia_siegbert : public CreatureScript
{
    public:
        npc_olympia_siegbert() : CreatureScript("npc_olympia_siegbert") { }

        struct npc_olympia_siegbertAI : public ScriptedAI
        {
            npc_olympia_siegbertAI(Creature* creature) : ScriptedAI(creature) { }

            void MoveInLineOfSight(Unit* who) override
            {
                if (auto plr = who->ToPlayer())
                {
                    auto lasttime = playerDelay.find(plr->GetGUID().GetRawValue());
                    if (lasttime != playerDelay.end() && lasttime->second > time(nullptr))
                        return;
                    playerDelay[plr->GetGUID().GetRawValue()] = time(nullptr) + 4 * MINUTE + 45;
                    plr->PlayDirectSound(MUSIC_L70ETC);
                }
            }

            void Reset() override
            {
                events.Reset();
                events.ScheduleEvent(EVENT_CHECK_RESTART, 10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SPECIAL_EFFECT, 4 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 e = events.ExecuteEvent())
                {
                    switch (e)
                    {
                        case EVENT_CHECK_RESTART:
                            events.ScheduleEvent(EVENT_CHECK_RESTART, 10 * IN_MILLISECONDS);
                            if (auto check = me->FindNearestCreature(NPC_L70ETC_START, 50.f))
                            {
                                if (check->HasAura(SPELL_INVISIBILITY))
                                {
                                    for (uint32 i = NPC_L70ETC_START; i <= NPC_L70ETC_END; i++)
                                        if (auto band = me->FindNearestCreature(i, 50.f))
                                        {
                                            band->RemoveAllAuras();
                                            band->AI()->Reset();
                                        }
                                }
                            }
                            break;
                        case EVENT_SPECIAL_EFFECT:
                            events.ScheduleEvent(EVENT_SPECIAL_EFFECT, urand(10 * IN_MILLISECONDS, 30 * IN_MILLISECONDS));
                            if (auto target = me->FindNearestCreature(urand(NPC_L70ETC_START, NPC_L70ETC_END), 50.f))
                                target->CastSpell(target, RAND<uint32>(SPELL_VISUAL_1, SPELL_VISUAL_2, SPELL_VISUAL_3), true);
                            break;
                    }
                }

                ScriptedAI::UpdateAI(diff);
            }

        private:
            std::unordered_map<uint64, time_t> playerDelay;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_siegbertAI(creature);
        }

        bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 /*opt*/) 
        {
            Position pos;
            player->GetRandomNearPosition(pos, 10.f);
            if (auto summon = player->SummonCreature(EVENT_NPC_WEAK_ELEMENTAL_FIRE, pos, TEMPSUMMON_TIMED_DESPAWN, 60 * IN_MILLISECONDS))
                summon->AI()->AttackStart(player);

            return false; 
        }
};


/*######
## npc_olympia_millhouse
######*/

enum
{
    NPC_AKYROXIS                = 1010736,

    SPELL_TELEPORT_VISUAL       = 7791,
    SPELL_ARANCE_EXLOSION_VISUAL = 34656,
    SPELL_ATTACK_VISUAL         = 45537,
    SPELL_SUICIDE               = 7,

    EVENT_CHECK_BOSSES          = 1,
    EVENT_TELEPORT              = 2,
    EVENT_TALK                  = 3,
    EVENT_MERGE_ELEMENTALS      = 4,
    EVENT_SUMMON_FINAL_BOSS     = 5,
    EVENT_ATTACK_BOSS           = 6,
    EVENT_KNOCK_BACK            = 7,
    EVENT_HIDE                  = 8
};

class npc_olympia_millhouse : public CreatureScript
{
    public:
        npc_olympia_millhouse() : CreatureScript("npc_olympia_millhouse") { }

        struct npc_olympia_millhouseAI : public ScriptedAI
        {
            npc_olympia_millhouseAI(Creature* creature) : canStartEvent(false), eventHasStarted(false), ScriptedAI(creature) { }

            void MoveInLineOfSight(Unit* who) override
            {
                if (who->GetTypeId() == TYPEID_PLAYER && me->IsWithinDistInMap(who, 30.f) && canStartEvent && !eventHasStarted)
                {
                    eventHasStarted = true;
                    events.ScheduleEvent(EVENT_TELEPORT, 400);
                    events.ScheduleEvent(EVENT_TALK, 3 * IN_MILLISECONDS);
                    me->SetVisible(true);
                }
            }

            void Reset() override
            {
                events.Reset();
                canStartEvent = false;
                eventHasStarted = false;
                talkCount = 0;
                events.ScheduleEvent(EVENT_CHECK_BOSSES, 0);
                me->SetVisible(false);
                if (sWorld->getWorldState(WORLDSTATE_BOSS) == 1)
                {
                    DoSummon(NPC_AKYROXIS, bossElementalTargetPos, 30 * IN_MILLISECONDS, TEMPSUMMON_DEAD_DESPAWN);
                    canStartEvent = false;
                    eventHasStarted = true;
                }
            }

            void SummonedCreatureDies(Creature* /*who*/, Unit* /*killer*/) override
            {
                sWorld->setWorldState(WORLDSTATE_BOSS, time(nullptr) + RESPAWN_TIME);
                for (uint32 i = EVENT_NPC_BOSS_ELEMENTAL_FIRE; i <= EVENT_NPC_BOSS_ELEMENTAL_WATER; i++)
                    if (auto boss = me->FindNearestCreature(i, 50.f))
                        boss->CastSpell(boss, SPELL_SUICIDE, true);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 e = events.ExecuteEvent())
                {
                    switch (e)
                    {
                        case EVENT_CHECK_BOSSES:
                            events.ScheduleEvent(EVENT_CHECK_BOSSES, 10 * IN_MILLISECONDS);
                            canStartEvent = true;
                            for (uint32 i = EVENT_NPC_BOSS_ELEMENTAL_FIRE; i <= EVENT_NPC_BOSS_ELEMENTAL_WATER; i++)
                                if (!me->FindNearestCreature(i, 50.f))
                                    canStartEvent = false;
                            break;
                        case EVENT_TELEPORT:
                            DoCastSelf(SPELL_TELEPORT_VISUAL, true);
                            break;
                        case EVENT_TALK:
                            if (talkCount == 2)
                            {
                                if (auto boss = me->FindNearestCreature(NPC_AKYROXIS, 50.f))
                                {
                                    sCreatureTextMgr->SendChat(boss, 10);
                                    sCreatureTextMgr->SendChat(boss, 1);
                                }
                            }
                            else
                                Talk(talkCount);
                            switch (talkCount++)
                            {
                                case 0:
                                    events.ScheduleEvent(EVENT_TALK, 12 * IN_MILLISECONDS);
                                    break;
                                case 1:
                                    events.ScheduleEvent(EVENT_MERGE_ELEMENTALS, 8 * IN_MILLISECONDS);
                                    break;
                                case 2:
                                    events.ScheduleEvent(EVENT_TALK, 10 * IN_MILLISECONDS);
                                    break;
                                case 3:
                                    events.ScheduleEvent(EVENT_ATTACK_BOSS, 5 * IN_MILLISECONDS);
                                    break;
                            }
                            break;
                        case EVENT_MERGE_ELEMENTALS:
                            for (uint32 i = EVENT_NPC_BOSS_ELEMENTAL_FIRE; i <= EVENT_NPC_BOSS_ELEMENTAL_WATER; i++)
                                if (auto boss = me->FindNearestCreature(i, 50.f))
                                    boss->GetMotionMaster()->MovePoint(0, bossElementalTargetPos);
                            events.ScheduleEvent(EVENT_SUMMON_FINAL_BOSS, 1 * IN_MILLISECONDS);
                            break;
                        case EVENT_SUMMON_FINAL_BOSS:
                            sWorld->setWorldState(WORLDSTATE_BOSS, 1);
                            for (uint32 i = EVENT_NPC_BOSS_ELEMENTAL_FIRE; i <= EVENT_NPC_BOSS_ELEMENTAL_WATER; i++)
                                if (auto boss = me->FindNearestCreature(i, 50.f))
                                    boss->SetVisible(false);
                            if (auto boss = DoSummon(NPC_AKYROXIS, bossElementalTargetPos, 30 * IN_MILLISECONDS, TEMPSUMMON_DEAD_DESPAWN))
                            {
                                boss->CastSpell(boss, SPELL_ARANCE_EXLOSION_VISUAL, true);
                                boss->SetFacingToObject(me);
                                boss->SetImmuneToNPC(true);
                            }
                            events.ScheduleEvent(EVENT_TALK, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_ATTACK_BOSS:
                            if (auto boss = me->FindNearestCreature(NPC_AKYROXIS, 50.f))
                                DoCast(boss, SPELL_ATTACK_VISUAL, true);
                            events.ScheduleEvent(EVENT_KNOCK_BACK, 5 * IN_MILLISECONDS);
                            break;
                        case EVENT_KNOCK_BACK:
                            if (auto boss = me->FindNearestCreature(NPC_AKYROXIS, 50.f))
                            {
                                me->GetMotionMaster()->MoveKnockbackFrom(boss->GetPositionX(), boss->GetPositionY(), 100.f, 50.f);
                                boss->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK_UNARMED);
                                sCreatureTextMgr->SendChat(boss, 0);
                            }
                            events.ScheduleEvent(EVENT_HIDE, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_HIDE:
                            if (auto boss = me->FindNearestCreature(NPC_AKYROXIS, 50.f))
                            {
                                boss->SetImmuneToNPC(false);
                                if (auto target = boss->FindNearestPlayer(50.f))
                                    boss->AI()->AttackStart(target);
                            }
                            me->SetVisible(false);
                            me->GetMotionMaster()->MoveTargetedHome();
                            break;
                    }
                }

                ScriptedAI::UpdateAI(diff);
            }

        private:
            bool canStartEvent;
            bool eventHasStarted;
            uint32 talkCount;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_olympia_millhouseAI(creature);
        }
};

void AddSC_custom_olympia()
{
    new npc_olympia_millhouse();
    new npc_olympia_siegbert();
    new spell_olympia_summon_edna();
    new player_olympia_tournament();  
    new npc_olympia_edna();
    new npc_olympia_linken();
    new npc_olympia_baron_baroness();
    new spell_event_call_infernal_destroyer();
    new npc_olympia_chromie();
    new npc_olympia_spawn_helper();
    new npc_olympia_vudimir_platin();
    new npc_olympia_vudimir_platin_boss();
    new npc_olympia_vudimir_pet();
    new npc_olympia_runner();
    new npc_olympia_race_start();
    new boss_olympia_water_boss();
    new boss_olympia_fire_boss();
    new boss_olympia_air_boss();
    new boss_olympia_earth_boss();
    new boss_olympia_shadow_boss();
    new npc_olympia_reyn();
    new npc_olympia_water_spawner();
    new npc_olympia_fire_spawner();
    new npc_olympia_wind_spawner();
    new npc_olympia_earth_spawner();
    new npc_olympia_weak_water_elemental();
    new npc_olympia_weak_fire_elemental();
    new npc_olympia_weak_wind_elemental();
    new npc_olympia_weak_earth_elemental();
    new npc_olympia_elite_water_elemental();
    new npc_olympia_elite_fire_elemental();
    new npc_olympia_elite_wind_elemental();
    new npc_olympia_elite_earth_elemental();
}
