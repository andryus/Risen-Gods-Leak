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

/*
    Ulduar AreaTriggers
    - 5366: iwo unter der Map
    - 5369: Eisenstau Repairstation links
    - 5381: Entrance trigger
    - 5388: Vorm Lorekeeper
    - 5390: Kologarn runterfall trigger
    - 5399: Kologarn Start trigger???
    - 5400: Algalon vor T�r
    - 5401: Algalon vor T�r
    - 5402: Mimiron Tram Runterfall trigger
    - 5414: Eisenstau Freya Abfahrt vorne
    - 5414: Eisenstau Mimiron Auffahrt vorne
    - 5416: Eisenstau Hodir Br�cke
    - 5417: Eisenstau Thorim Zugang
    - 5420: Teleport Bassislager
    - 5423: Eisenstau Repairstation rechts
    - 5426: Eisenstau iwo in der Mitte oO
    - 5427: Ignis Rampe oben
    - 5428: Eisenstau iwo vorne
    - 5432: Mimiron Tram Schiene
    - 5433: Algalon unter der mitte
    - 5442: Eisenstau Hinten
    - 5443: Eisenstau ganz vorne
*/

#include "ulduar.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellInfo.h"

enum trashSpells
{
    // npc_ice_turrets
    SPELL_ICE_NOVA                  = 64920,

    // npc_rune_etched_sentry
    SPELL_FLAMING_RUNE              = 64852,

    // npc_storm_tempered_keeper
    SPELL_FORKED_LIGHTNING          = 63541,
    SPELL_SEPARATION_ANXIETY        = 63539,
    SPELL_SUMMON_CHARGED_SPHERE     = 63527,
    SPELL_VENGEFUL_SURGE            = 63630,
    SPELL_SUPERCHARGED              = 63528,
    SPELL_STORM_WAVE                = 63533,

    // npc_arachnopod_destroyer
    SPELL_FLAME_SPRAY               = 64717,
    SPELL_MACHINE_GUN               = 64776,

    // npc_clockwork_mechanic
    SPELL_ICE_TURRET                = 64966,

    // npc_snowmount_trigger
    SPELL_CHARGE_EFFECT             = 74399,
};

enum Creatures
{
    NPC_STORM_TEMPERED_KEEPER_ONE   = 33722,
    NPC_STORM_TEMPERED_KEEPER_TWO   = 33699,
    NPC_CHARGED_SPHERE              = 33715,
    NPC_SNOW_MOUND_8                = 34151,
    NPC_SNOW_MOUND_6                = 34150,
    NPC_SNOW_MOUND_4                = 34146,

    //Misc
    NPC_WINTER_JORMUNGAR            = 34137,
};

class npc_ulduar_storm_tempered_keeper : public CreatureScript
{
    public:
        npc_ulduar_storm_tempered_keeper() : CreatureScript("npc_ulduar_storm_tempered_keeper") { }

        struct npc_ulduar_storm_tempered_keeperAI : public ScriptedAI
        {
            npc_ulduar_storm_tempered_keeperAI(Creature* creature) : ScriptedAI(creature)
            {
                PartnerGUID.Clear();
            }

            uint32 ForkedLightningTimer;
            uint32 CheckDistanceTimer;
            uint32 SpawnSphereTimer;
            bool Enraged;
            ObjectGuid PartnerGUID;

            void Reset()
            {
                ForkedLightningTimer =  7*IN_MILLISECONDS;
                CheckDistanceTimer   =  6*IN_MILLISECONDS;
                SpawnSphereTimer     = 10*IN_MILLISECONDS;
                Enraged = false;

                if (Creature* partner = ObjectAccessor::GetCreature(*me, PartnerGUID))
                    if (!partner->IsAlive())
                    {
                        partner->Respawn(true);
                        partner->GetMotionMaster()->MoveTargetedHome();
                    }
            }

            void EnterCombat(Unit* who)
            {
                uint32 partnerEntry = NPC_STORM_TEMPERED_KEEPER_ONE;
                if (me->GetEntry() == NPC_STORM_TEMPERED_KEEPER_ONE)
                    partnerEntry = NPC_STORM_TEMPERED_KEEPER_TWO;

                if (Creature* partner = me->FindNearestCreature(partnerEntry, 40.0f))
                {
                    PartnerGUID = partner->GetGUID();
                    partner->AI()->AttackStart(who);
                }
            }

            void JustDied(Unit* /*who*/)
            {
                if (Creature* partner = ObjectAccessor::GetCreature(*me, PartnerGUID))
                    partner->CastSpell(partner, SPELL_VENGEFUL_SURGE, true);
            }

            void SpellHit(Unit* /*caster*/, const SpellInfo* spell)
            {
                if (spell->Id == SPELL_VENGEFUL_SURGE && !Enraged)
                    Enraged = true;
            }

            void JustSummoned(Creature* summon)
            {
                summon->AI()->SetGuidData(PartnerGUID, 0);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (CheckDistanceTimer <= diff)
                {
                    if (Creature* partner = ObjectAccessor::GetCreature(*me, PartnerGUID))
                        if (me->GetDistance(partner) > 50.0f)
                            DoCastSelf(SPELL_SEPARATION_ANXIETY);
                    CheckDistanceTimer = 7000;
                }
                else
                    CheckDistanceTimer -= diff;

                if (SpawnSphereTimer <= diff)
                {
                    if (!Enraged)
                        DoCast(SPELL_SUMMON_CHARGED_SPHERE);

                    SpawnSphereTimer = 10000;
                }
                else
                    SpawnSphereTimer -= diff;

                if (ForkedLightningTimer <= diff)
                {
                    DoCast(SPELL_FORKED_LIGHTNING);
                    ForkedLightningTimer = 7000;
                }
                else
                    ForkedLightningTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ulduar_storm_tempered_keeperAI(creature);
        }
};

class npc_ulduar_charged_sphere : public CreatureScript
{
    public:
        npc_ulduar_charged_sphere() : CreatureScript("npc_ulduar_charged_sphere") { }

        struct npc_ulduar_charged_sphereAI : public ScriptedAI
        {
            npc_ulduar_charged_sphereAI(Creature* creature) : ScriptedAI(creature) {}

            bool KillMe;
            uint32 KillTimer;
            ObjectGuid TargetGUID;

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                KillMe = false;
                TargetGUID.Clear();
            }

            void SetGuidData(ObjectGuid guid, uint32 /*type*/) override
            {
                TargetGUID = guid;
                if (Creature* creature = ObjectAccessor::GetCreature(*me, guid))
                    me->GetMotionMaster()->MoveChase(creature);
            }

            void UpdateAI(uint32 diff)
            {
                if (!TargetGUID)
                    return;

                if (!KillMe)
                {
                    if (Creature* partner = ObjectAccessor::GetCreature(*me, TargetGUID))
                        if (me->GetDistance(partner) < 1.5f)
                        {
                            DoCast(partner, SPELL_SUPERCHARGED, true);
                            KillTimer = 2000;
                            KillMe = true;
                        }
                }
                else
                {
                    if (KillTimer <= diff)
                        me->DisappearAndDie();
                    else
                        KillTimer -= diff;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ulduar_charged_sphereAI(creature);
        }
};

class npc_ulduar_snow_mound : public CreatureScript
{
    public:
        npc_ulduar_snow_mound() : CreatureScript("npc_ulduar_snow_mound") { }

        struct npc_ulduar_snow_moundAI : public ScriptedAI
        {
            npc_ulduar_snow_moundAI(Creature* creature) : ScriptedAI(creature)
            {
                Spawns = 0;
                Activated = false;
            }

            uint32 SpawnTimer;
            uint8 Spawns;
            bool Activated;

            void MoveInLineOfSight(Unit* who)
            {
                if (!Activated && who && me->GetDistance2d(who) <= 3.0f && who->ToPlayer() && !who->ToPlayer()->IsGameMaster() && who->IsAlive())
                {
                    Activated = true;
                    SpawnTimer = 500;
                    switch (me->GetEntry())
                    {
                        case NPC_SNOW_MOUND_8:
                            Spawns = 8;
                            break;
                        case NPC_SNOW_MOUND_6:
                            Spawns = 6;
                            break;
                        case NPC_SNOW_MOUND_4:
                            Spawns = 4;
                            break;
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (Spawns)
                {
                    if (SpawnTimer <= diff)
                    {
                        SpawnJormungar();
                        Spawns--;
                        SpawnTimer = urand(600, 1200);
                    }
                    else
                        SpawnTimer -= diff;
                }
            }

            void SpawnJormungar()
            {

                if (Creature* trash = me->SummonCreature(NPC_WINTER_JORMUNGAR, *me, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 2*MINUTE*IN_MILLISECONDS))
                {
                    trash->SetInCombatWithZone();
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        trash->CastSpell(target, SPELL_CHARGE_EFFECT, true);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ulduar_snow_moundAI(creature);
        }
};

#define NPC_ARACHNOPOD_DESTROYER 34183

class npc_ulduar_arachnopod_destroyer_trigger : public CreatureScript
{
    public:
        npc_ulduar_arachnopod_destroyer_trigger() : CreatureScript("npc_ulduar_arachnopod_destroyer_trigger") { }

        struct npc_ulduar_arachnopod_destroyer_triggerAI : public ScriptedAI
        {
            npc_ulduar_arachnopod_destroyer_triggerAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset()
            {
                timer = 500;
            }

            void UpdateAI(uint32 diff)
            {
                if (timer <= diff)
                {
                    if (Creature* target = me->FindNearestCreature(34183, 15.0f, true))
                        me->Kill(target);

                    timer = 500;
                }
                else
                    timer -= diff;
            }

        private:
            uint32 timer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ulduar_arachnopod_destroyer_triggerAI(creature);
        }
};

enum ArchivumConsoleData
{
    QUEST_VALANYR           = 13622,
    QUEST_DISC              = 13604,
    QUEST_DISC_H            = 13817,
    QUEST_PLANETARIUM       = 13607,
    QUEST_PLANETARIUM_H     = 13816,

    SPELL_ROOTS             = 24648, //Visual for Elders
    SPELL_TREMOR            = 64228,

    NPC_BRANN               = 34044,
    NPC_CONSOLE_EFFECT      = 1010604,
    NPC_CONSOLE_HELPER      = 1010605,

    NPC_HODIR_I             = 33879,
    NPC_MIMIRON_I           = 33880,
    NPC_THORIM_I            = 33878,
    NPC_SIF                 = 33877,
    NPC_FREYA_I             = 33876,
    NPC_ELDER_STONEBARK_M   = 33862,
    NPC_ELDER_IRONBENCH_M   = 33861,
    NPC_ELDER_BRIGHTLEAF_M  = 33761,

    GO_CONSOLE              = 194555,

    ACTION_VALANYR          = 1,
    ACTION_DISC             = 2,
    ACTION_PLANETARIUM      = 3,
    ACTION_FREYA            = 4,
    ACTION_HODIR            = 5,
    ACTION_MIMIRON          = 6,
    ACTION_THORIM           = 7,
};

enum ArchivumConsoleText
{
    SAY_ARCHIVUM_VALANYR_ANALYSIS_1  = 0, //speech when player rewards the Val'anyr pre-quest
    SAY_ARCHIVUM_VALANYR_ANALYSIS_2  = 1, //text from Archivum Consoel 1-6
    SAY_ARCHIVUM_VALANYR_ANALYSIS_3  = 2,
    SAY_ARCHIVUM_VALANYR_ANALYSIS_4  = 3,
    SAY_ARCHIVUM_VALANYR_ANALYSIS_5  = 4,
    SAY_ARCHIVUM_VALANYR_ANALYSIS_6  = 5,

    SAY_ARCHIVUM_SYSTEM_1    = 6, //speech with Brann Brozebart when player rewards the archivum data disc quest
    SAY_ARCHIVUM_SYSTEM_2    = 7, //text from Archivum Console 1-10
    SAY_ARCHIVUM_SYSTEM_3    = 8,
    SAY_ARCHIVUM_SYSTEM_4    = 9,
    SAY_ARCHIVUM_SYSTEM_5    = 10,
    SAY_ARCHIVUM_SYSTEM_6    = 11,
    SAY_ARCHIVUM_SYSTEM_7    = 12,
    SAY_ARCHIVUM_SYSTEM_8    = 13,
    SAY_ARCHIVUM_SYSTEM_9    = 14,
    SAY_ARCHIVUM_SYSTEM_10   = 15,
    //text from Brann Brozebart 1-10
    SAY_BRANN_ARCHIVUM_1     = 0,     
    SAY_BRANN_ARCHIVUM_2     = 1,
    SAY_BRANN_ARCHIVUM_3     = 2,
    SAY_BRANN_ARCHIVUM_4     = 3,
    SAY_BRANN_ARCHIVUM_5     = 4,
    SAY_BRANN_ARCHIVUM_6     = 5,
    SAY_BRANN_ARCHIVUM_7     = 6,
    SAY_BRANN_ARCHIVUM_8     = 7,
    SAY_BRANN_ARCHIVUM_9     = 8,
    SAY_BRANN_ARCHIVUM_10    = 9,

    //speech with Brann Brozebart when player rewards the planetarium quest
    //text from Archivum Console 1-6
    SAY_ARCHIVUM_PLANETARIUM_1       = 16, 
    SAY_ARCHIVUM_PLANETARIUM_2       = 17, 
    SAY_ARCHIVUM_PLANETARIUM_3       = 18,
    SAY_ARCHIVUM_PLANETARIUM_4       = 19,
    SAY_ARCHIVUM_PLANETARIUM_5       = 20,
    SAY_ARCHIVUM_PLANETARIUM_6       = 21,
    //text from Brann Brozebart 1-5
    SAY_BRANN_PLANETARIUM_1          = 10, 
    SAY_BRANN_PLANETARIUM_2          = 11,
    SAY_BRANN_PLANETARIUM_3          = 12,
    SAY_BRANN_PLANETARIUM_4          = 13,
    SAY_BRANN_PLANETARIUM_5          = 14,
    //Watcher analysis
    SAY_ANALYSIS_THORIM_1    = 22,
    //Thorim 1-4
    SAY_ANALYSIS_THORIM_2    = 23,    
    SAY_ANALYSIS_THORIM_3    = 24,
    SAY_ANALYSIS_THORIM_4    = 25,
    //Hodir 1-4
    SAY_ANALYSIS_HODIR_1     = 26, 
    SAY_ANALYSIS_HODIR_2     = 27,
    SAY_ANALYSIS_HODIR_3     = 28,
    SAY_ANALYSIS_HODIR_4     = 29,
    SAY_ANALYSIS_HODIR_5     = 30,
    //Freya 1-7
    SAY_ANALYSIS_FREYA_1     = 31, 
    SAY_ANALYSIS_FREYA_2     = 32,
    SAY_ANALYSIS_FREYA_3     = 33,
    SAY_ANALYSIS_FREYA_4     = 34,
    SAY_ANALYSIS_FREYA_5     = 35,
    SAY_ANALYSIS_FREYA_6     = 36,
    SAY_ANALYSIS_FREYA_7     = 37,
    SAY_ANALYSIS_MIMIRON_1   = 38, 
    //Mimiron 1-4
    SAY_ANALYSIS_MIMIRON_2   = 39,
    SAY_ANALYSIS_MIMIRON_3   = 40,
    SAY_ANALYSIS_MIMIRON_4   = 41,
};

Position const ConsoleRight = {1435.223f, 120.386f, 425.498f, 6.0f};
Position const ConsoleMiddle = {1434.919f, 118.88f, 425.567f, 6.0f};
Position const ConsoleLeft = {1434.945f, 117.379f, 425.561f, 6.0f};
Position const ConsoleFront = {1436.158f, 118.853f, 425.284f, 6.0f};
Position const ConsoleFrontLeft = {1436.459f, 117.269f, 425.2160f, 6.0f};

#define GOSSIP_ITEM_FREYA "Watcher Freya status analysis."
#define GOSSIP_ITEM_HODIR "Watcher Hodir status analysis."
#define GOSSIP_ITEM_MIMIRON "Watcher Mimiron status analysis."
#define GOSSIP_ITEM_THORIM "Watcher Thorim status analysis."

class go_archivum_console : public GameObjectScript
{
    public:
        go_archivum_console() : GameObjectScript("go_archivum_console") { }

        bool OnGossipHello(Player* player, GameObject* go)
        {
            if (go->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER)
                player->PrepareQuestMenu(go->GetGUID());

            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_FREYA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_HODIR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_MIMIRON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_THORIM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(go), go->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 action)
        {
            Creature* helper = go->FindNearestCreature(NPC_CONSOLE_HELPER, 15.0f);
            if (!helper)
                return false;

            player->PlayerTalkClass->ClearMenus();
            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF:
                    helper->AI()->DoAction(ACTION_FREYA);
                    break;
                case GOSSIP_ACTION_INFO_DEF+1:
                    helper->AI()->DoAction(ACTION_HODIR);
                    break;
                case GOSSIP_ACTION_INFO_DEF+2:
                    helper->AI()->DoAction(ACTION_MIMIRON);
                    break;
                case GOSSIP_ACTION_INFO_DEF+3:
                    helper->AI()->DoAction(ACTION_THORIM);
                    break;
            }
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        bool OnQuestReward(Player* /*player*/, GameObject* go, Quest const* quest, uint32 /*opt*/)
        {
            Creature* helper = go->FindNearestCreature(NPC_CONSOLE_HELPER, 25.0f);
            if (!helper)
                return false;

            switch (quest->GetQuestId())
            {
                case QUEST_VALANYR:
                    helper->AI()->DoAction(ACTION_VALANYR);
                    break;
                case QUEST_DISC:
                case QUEST_DISC_H:
                    helper->AI()->DoAction(ACTION_DISC);
                    break;
            }
            return true;
        }
};

class npc_prospector : public CreatureScript
{
    public:
        npc_prospector() : CreatureScript("npc_prospector") { }

        bool OnQuestReward(Player* /*player*/, Creature* creature, Quest const* quest, uint32 /*opt*/)
        {
            Creature* prospector = creature->FindNearestCreature(NPC_CONSOLE_HELPER, 25.0f);
            if (!prospector)
                return false;

            switch (quest->GetQuestId())
            {
                case QUEST_PLANETARIUM:
                case QUEST_PLANETARIUM_H:
                    prospector->AI()->DoAction(ACTION_PLANETARIUM);
                    break;
            }
            return false;
        }
};

class npc_console_helper : public CreatureScript
{
    public:
        npc_console_helper() : CreatureScript("npc_console_helper") { }

        struct npc_console_helperAI : public ScriptedAI
        {
            npc_console_helperAI(Creature* creature) : ScriptedAI(creature) { }

            uint8 Step;
            uint32 PhaseTimer;
            ObjectGuid ConsoleGUID;
            ObjectGuid BrannGUID;
            bool EventInfo;
            bool ValanyrStart;
            bool DiscStart;
            bool PlanetariumStart;
            bool FreyaStart;
            bool HodirStart;
            bool MimironStart;
            bool ThorimStart;

            void JumpToNextStep(uint32 timer)
            {
                PhaseTimer = timer;
                ++Step;
            }

            void Reset()
            {
                Step = 0;
                ConsoleGUID.Clear();
                BrannGUID.Clear();
                EventInfo = false;
                ValanyrStart = false;
                DiscStart = false;
                PlanetariumStart = false;
                FreyaStart = false;
                HodirStart = false;
                MimironStart = false;
                ThorimStart = false;
                PhaseTimer = 1000;
            }

            void DoAction(int32 action)
            {
                if (!EventInfo)
                {
                    switch (action)
                    {
                        case ACTION_VALANYR:
                            if (GameObject* go = me->FindNearestGameObject(GO_CONSOLE, 15.0f))
                                ConsoleGUID = go->GetGUID();
                            ValanyrStart = true;
                            break;
                        case ACTION_DISC:
                            if (GameObject* go = me->FindNearestGameObject(GO_CONSOLE, 15.0f))
                                ConsoleGUID = go->GetGUID();
                            if (Creature* brann = me->FindNearestCreature(NPC_BRANN, 25.0f))
                                BrannGUID = brann->GetGUID();
                            DiscStart = true;
                            break;
                        case ACTION_PLANETARIUM:
                            if (GameObject* go = me->FindNearestGameObject(GO_CONSOLE, 15.0f))
                                ConsoleGUID = go->GetGUID();
                            if (Creature* brann = me->FindNearestCreature(NPC_BRANN, 25.0f))
                                BrannGUID = brann->GetGUID();
                            PlanetariumStart = true;
                            break;
                        case ACTION_FREYA:
                            if (GameObject* go = me->FindNearestGameObject(GO_CONSOLE, 15.0f))
                                ConsoleGUID = go->GetGUID();
                            FreyaStart = true;
                            break;
                        case ACTION_HODIR:
                            if (GameObject* go = me->FindNearestGameObject(GO_CONSOLE, 15.0f))
                                ConsoleGUID = go->GetGUID();
                            HodirStart = true;
                            break;
                        case ACTION_MIMIRON:
                            if (GameObject* go = me->FindNearestGameObject(GO_CONSOLE, 15.0f))
                                ConsoleGUID = go->GetGUID();
                            MimironStart = true;
                            break;
                        case ACTION_THORIM:
                            if (GameObject* go = me->FindNearestGameObject(GO_CONSOLE, 15.0f))
                                ConsoleGUID = go->GetGUID();
                            ThorimStart = true;
                            break;
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (ValanyrStart)
                {
                    if (PhaseTimer <= diff)
                    {
                        GameObject* console = GameObject::GetGameObject(*me, ConsoleGUID);
                        if (!console)
                            return;

                        switch (Step)
                        {
                            case 0:
                                Talk(SAY_ARCHIVUM_VALANYR_ANALYSIS_1);
                                EventInfo = true;
                                JumpToNextStep(5000);
                                break;
                            case 1:
                                Talk(SAY_ARCHIVUM_VALANYR_ANALYSIS_2);
                                JumpToNextStep(5000);
                                break;
                            case 2:
                                Talk(SAY_ARCHIVUM_VALANYR_ANALYSIS_3);
                                JumpToNextStep(12000);
                                break;
                            case 3:
                                Talk(SAY_ARCHIVUM_VALANYR_ANALYSIS_4);
                                JumpToNextStep(10500);
                                break;
                            case 4:
                                Talk(SAY_ARCHIVUM_VALANYR_ANALYSIS_5);
                                JumpToNextStep(13500);
                                break;
                            case 5:
                                Talk(SAY_ARCHIVUM_VALANYR_ANALYSIS_6);
                                Reset();
                                break;
                        }
                    }
                    else
                        PhaseTimer -= diff;
                }

                if (DiscStart)
                {
                    if (PhaseTimer <= diff)
                    {
                        GameObject* console = GameObject::GetGameObject(*me, ConsoleGUID);
                        Creature* brann = ObjectAccessor::GetCreature(*me, BrannGUID);
                        if (!console || !brann)
                            return;

                        switch (Step)
                        {
                            case 0:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_1);
                                EventInfo = true;
                                JumpToNextStep(5000);
                                break;
                            case 1:
                                Talk(SAY_ARCHIVUM_SYSTEM_1);
                                JumpToNextStep(4000);
                                break;
                            case 2:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_2);
                                JumpToNextStep(7000);
                                break;
                            case 3:
                                Talk(SAY_ARCHIVUM_SYSTEM_2);
                                JumpToNextStep(7000);
                                break;
                            case 4:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_3);
                                JumpToNextStep(8000);
                                break;
                            case 5:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_4);
                                JumpToNextStep(8000);
                                break;
                            case 6:
                                Talk(SAY_ARCHIVUM_SYSTEM_3);
                                JumpToNextStep(9000);
                                break;
                            case 7:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_5);
                                JumpToNextStep(12000);
                                break;
                            case 8:
                                Talk(SAY_ARCHIVUM_SYSTEM_4);
                                JumpToNextStep(7000);
                                break;
                            case 9:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_6);
                                JumpToNextStep(4000);
                                break;
                            case 10:
                                Talk(SAY_ARCHIVUM_SYSTEM_5);
                                JumpToNextStep(25000);
                                break;
                            case 11:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_7);
                                JumpToNextStep(6000);
                                break;
                            case 12:
                                Talk(SAY_ARCHIVUM_SYSTEM_6);
                                JumpToNextStep(13000);
                                break;
                            case 13:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_8);
                                JumpToNextStep(10000);
                                break;
                            case 14:
                                Talk(SAY_ARCHIVUM_SYSTEM_7);
                                JumpToNextStep(9000);
                                break;
                            case 15:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_9);
                                JumpToNextStep(5000);
                                break;
                            case 16:
                                Talk(SAY_ARCHIVUM_SYSTEM_8);
                                JumpToNextStep(11000);
                                break;
                            case 17:
                                Talk(SAY_ARCHIVUM_SYSTEM_9);
                                JumpToNextStep(4000);
                                break;
                            case 18:
                                Talk(SAY_ARCHIVUM_SYSTEM_10);
                                JumpToNextStep(2000);
                                break;
                            case 19:
                                brann->AI()->Talk(SAY_BRANN_ARCHIVUM_10);
                                Reset();
                                break;
                        }
                    }
                    else
                        PhaseTimer -= diff;
                }

                if (PlanetariumStart)
                {
                    if (PhaseTimer <= diff)
                    {
                        GameObject* console = GameObject::GetGameObject(*me, ConsoleGUID);
                        Creature* brann = ObjectAccessor::GetCreature(*me, BrannGUID);
                        if (!console || !brann)
                            return;

                        switch (Step)
                        {
                            case 0:
                                brann->AI()->Talk(SAY_BRANN_PLANETARIUM_1);
                                EventInfo = true;
                                JumpToNextStep(4000);
                                break;
                            case 1:
                                Talk(SAY_ARCHIVUM_PLANETARIUM_1);
                                JumpToNextStep(6000);
                                break;
                            case 2:
                                brann->AI()->Talk(SAY_BRANN_PLANETARIUM_2);
                                JumpToNextStep(6000);
                                break;
                            case 3:
                                Talk(SAY_ARCHIVUM_PLANETARIUM_2);
                                JumpToNextStep(16000);
                                break;
                            case 4:
                                brann->AI()->Talk(SAY_BRANN_PLANETARIUM_3);
                                JumpToNextStep(7000);
                                break;
                            case 5:
                                Talk(SAY_ARCHIVUM_PLANETARIUM_3);
                                JumpToNextStep(5000);
                                break;
                            case 6:
                                Talk(SAY_ARCHIVUM_PLANETARIUM_4);
                                JumpToNextStep(6000);
                                break;
                            case 7:
                                brann->AI()->Talk(SAY_BRANN_PLANETARIUM_4);
                                JumpToNextStep(5000);
                                break;
                            case 8:
                                Talk(SAY_ARCHIVUM_PLANETARIUM_5);
                                JumpToNextStep(7000);
                                break;
                            case 9:
                                Talk(SAY_ARCHIVUM_PLANETARIUM_6);
                                JumpToNextStep(11000);
                                break;
                            case 10:
                                brann->AI()->Talk(SAY_BRANN_PLANETARIUM_5);
                                Reset();
                                break;
                        }
                    }
                    else
                        PhaseTimer -= diff;
                }

                if (HodirStart)
                {
                    if (PhaseTimer <= diff)
                    {
                        GameObject* console = GameObject::GetGameObject(*me, ConsoleGUID);
                        if (!console)
                            return;

                        switch (Step)
                        {
                            case 0:
                                Talk(SAY_ANALYSIS_HODIR_1);
                                JumpToNextStep(5000);
                                break;
                            case 1:
                                Talk(SAY_ANALYSIS_HODIR_2);
                                me->SummonCreature(NPC_HODIR_I, ConsoleFront, TEMPSUMMON_TIMED_DESPAWN, 25000);
                                JumpToNextStep(5000);
                                break;
                            case 2:
                                Talk(SAY_ANALYSIS_HODIR_3);
                                JumpToNextStep(8000);
                                break;
                            case 3:
                                Talk(SAY_ANALYSIS_HODIR_4);
                                JumpToNextStep(7000);
                                break;
                            case 4:
                                Talk(SAY_ANALYSIS_HODIR_5);
                                Reset();
                                break;
                        }
                    }
                    else PhaseTimer -= diff;
                }

                if (MimironStart)
                {
                    if (PhaseTimer <= diff)
                    {
                        GameObject* console = GameObject::GetGameObject(*me, ConsoleGUID);
                        if (!console)
                            return;

                        switch (Step)
                        {
                            case 0:
                                Talk(SAY_ANALYSIS_MIMIRON_1);
                                JumpToNextStep(5000);
                                break;
                            case 1:
                                Talk(SAY_ANALYSIS_MIMIRON_2);
                                me->SummonCreature(NPC_MIMIRON_I, ConsoleFront, TEMPSUMMON_TIMED_DESPAWN, 24000);
                                JumpToNextStep(9000);
                                break;
                            case 2:
                                Talk(SAY_ANALYSIS_MIMIRON_3);
                                JumpToNextStep(8000);
                                break;
                            case 3:
                                Talk(SAY_ANALYSIS_MIMIRON_4);
                                Reset();
                                break;
                        }
                    }
                    else
                        PhaseTimer -= diff;
                }

                if (ThorimStart)
                {
                    if (PhaseTimer <= diff)
                    {
                        GameObject* console = GameObject::GetGameObject(*me, ConsoleGUID);
                        if (!console)
                            return;

                        switch (Step)
                        {
                            case 0:
                                Talk(SAY_ANALYSIS_THORIM_1);
                                JumpToNextStep(5000);
                                break;
                            case 1:
                                Talk(SAY_ANALYSIS_THORIM_2);
                                me->SummonCreature(NPC_THORIM_I, ConsoleFront, TEMPSUMMON_TIMED_DESPAWN, 32000);
                                me->SummonCreature(NPC_SIF, ConsoleRight, TEMPSUMMON_TIMED_DESPAWN, 32000);
                                JumpToNextStep(7000);
                                break;
                            case 2:
                                Talk(SAY_ANALYSIS_THORIM_3);
                                JumpToNextStep(8000);
                                break;
                            case 3:
                                Talk(SAY_ANALYSIS_THORIM_4);
                                Reset();
                                break;
                        }
                    }
                    else
                        PhaseTimer -= diff;
                }

                if (FreyaStart)
                {
                    if (PhaseTimer <= diff)
                    {
                        GameObject* console = GameObject::GetGameObject(*me, ConsoleGUID);
                        if (!console)
                            return;

                        switch (Step)
                        {
                            case 0:
                                Talk(SAY_ANALYSIS_FREYA_1);
                                JumpToNextStep(5000);
                                break;
                            case 1:
                                Talk(SAY_ANALYSIS_FREYA_2);
                                me->SummonCreature(NPC_FREYA_I, ConsoleFront, TEMPSUMMON_TIMED_DESPAWN, 49000);
                                me->SummonCreature(NPC_ELDER_BRIGHTLEAF_M, ConsoleMiddle, TEMPSUMMON_TIMED_DESPAWN, 37000);
                                me->SummonCreature(NPC_ELDER_IRONBENCH_M, ConsoleLeft, TEMPSUMMON_TIMED_DESPAWN, 37000);
                                me->SummonCreature(NPC_ELDER_STONEBARK_M, ConsoleRight, TEMPSUMMON_TIMED_DESPAWN, 37000);
                                JumpToNextStep(5000);
                                break;
                            case 2:
                                Talk(SAY_ANALYSIS_FREYA_3);
                                JumpToNextStep(8000);
                                break;
                            case 3:
                                Talk(SAY_ANALYSIS_FREYA_4);
                                if (Creature* ElderS = me->FindNearestCreature(NPC_ELDER_STONEBARK, 15.0f))
                                    ElderS->CastSpell(me,SPELL_TREMOR, true);
                                JumpToNextStep(7000);
                                break;
                            case 4:
                                Talk(SAY_ANALYSIS_FREYA_5);
                                if (Creature* root = me->SummonCreature(NPC_CONSOLE_EFFECT, ConsoleFrontLeft, TEMPSUMMON_TIMED_DESPAWN, 4000))
                                    me->AddAura(SPELL_ROOTS, root);
                                JumpToNextStep(7000);
                                break;
                            case 5:
                                Talk(SAY_ANALYSIS_FREYA_6);
                                JumpToNextStep(10000);
                                break;
                            case 6:
                                Talk(SAY_ANALYSIS_FREYA_7);
                                Reset();
                                break;
                        }
                    }
                    else
                        PhaseTimer -= diff;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_console_helperAI(creature);
        }
};

class go_dalaran_portal : public GameObjectScript
{
    public:
        go_dalaran_portal() : GameObjectScript("go_dalaran_portal") { }

        bool OnGossipHello(Player* player, GameObject* /*go*/)
        {
            if (!player)
                return false;

            player->TeleportTo(571, 5802.30f, 610.908f, 658.425f,1.566f,0);

            return false;
        }
};

class go_call_tram : public GameObjectScript
{
public:
    go_call_tram() : GameObjectScript("go_call_tram") { }

    bool OnGossipHello(Player* /*pPlayer*/, GameObject* pGo)
    {
        InstanceScript* pInstance = pGo->GetInstanceScript();

        if (!pInstance)
            return false;

        switch (pGo->GetEntry())
        {
            case 194914:
            case 194438:
                pInstance->SetData(DATA_CALL_TRAM, 0);
                break;
            case 194912:
            case 194437:
                pInstance->SetData(DATA_CALL_TRAM, 1);
                break;
        }
        return true;
    }
};

enum GuardianLasher
{
    SPELL_GUARDIAN_PHEROMONES = 63007,
    SPELL_GUARDIANS_LASH_10   = 63047,
    SPELL_GUARDIANS_LASH_25   = 63550
};

class npc_guardian_lasher : public CreatureScript
{
    public:
        npc_guardian_lasher() : CreatureScript("npc_guardian_lasher") { }

        struct npc_guardian_lasherAI : public ScriptedAI
        {
            npc_guardian_lasherAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 lash_timer;
            uint32 checkTimer;

            void Reset()
            {
                lash_timer = urand(3000, 6000);
                checkTimer = 1*IN_MILLISECONDS;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (checkTimer <= diff)
                {
                    if (me->FindNearestCreature(NPC_FOREST_SWARMER, 20.0f, true))
                    {
                        if (!me->HasAura(SPELL_GUARDIAN_PHEROMONES))
                            DoCastSelf(SPELL_GUARDIAN_PHEROMONES);
                    }
                    else
                        me->RemoveAura(SPELL_GUARDIAN_PHEROMONES);

                    checkTimer = 1*IN_MILLISECONDS;
                }
                else
                    checkTimer -= diff;

                if (lash_timer <= diff)
                {
                    if (me->GetVictim())
                    {
                        DoCastVictim(RAID_MODE(SPELL_GUARDIANS_LASH_10,SPELL_GUARDIANS_LASH_25));
                        lash_timer = urand(8000, 12000);
                    }
                }
                else
                {
                    lash_timer -= diff;
                }

                DoMeleeAttackIfReady();

            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_guardian_lasherAI(creature);
        }
};

enum ForestSwarmer
{
    SPELL_POLLINATE           = 63059,
};

class npc_forest_swarmer : public CreatureScript
{
    public:
        npc_forest_swarmer() : CreatureScript("npc_forest_swarmer") { }

        struct npc_forest_swarmerAI : public ScriptedAI
        {
            npc_forest_swarmerAI(Creature* creature) : ScriptedAI(creature) {}

            ObjectGuid lasher_guid;
            uint32 pollinate_timer;
            uint32 teleport_timer;

            void Reset()
            {
                lasher_guid.Clear();
                pollinate_timer = urand(3000, 4000);
                teleport_timer = urand(5000, 7000);
            }

            void EnterCombat(Unit* /*who*/)
            {
                if (Creature* lasher = me->FindNearestCreature(NPC_GUARDIAN_LASHER, 60.0f))
                {
                    lasher_guid = lasher->GetGUID();
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (Creature* lasher = ObjectAccessor::GetCreature(*me, lasher_guid))
                {
                    if (lasher->IsAlive())
                    {
                        if (pollinate_timer <= diff)
                        {
                            if (me->GetDistance(lasher) <= 20.0f)
                            {
                                DoCast(lasher, SPELL_POLLINATE);
                                pollinate_timer = urand(6000, 8000);
                            }
                            else
                            {
                                pollinate_timer = 1000;
                            }
                        }
                        else
                        {
                            pollinate_timer -= diff;
                        }

                        if (teleport_timer <= diff)
                        {
                            if (!me->HasUnitState(UNIT_STATE_CASTING))
                            {
                                me->NearTeleportTo(lasher->GetPositionX(), lasher->GetPositionY(), lasher->GetPositionZ(), 0);
                                ResetThreatList();
                                teleport_timer = urand(12000, 15000);
                            }
                            else
                            {
                                teleport_timer = 1000;
                            }
                        }
                        else
                        {
                            teleport_timer -= diff;
                        }
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_forest_swarmerAI(creature);
        }
};

enum BoomerXP500
{
    SPELL_PROMIMITY_BOMB_EXPLODE  =  30844
};

class npc_boomer_xp500 : public CreatureScript
{
    public:
        npc_boomer_xp500() : CreatureScript("npc_boomer_xp500") { }

        struct npc_boomer_xp500AI : public ScriptedAI
        {
            npc_boomer_xp500AI(Creature* creature) : ScriptedAI(creature) {}

            void UpdateAI(uint32 /*diff*/)
            {
                if (!UpdateVictim())
                    return;

                if (me->GetDistance(me->GetVictim()) <= 1.0f)
                {
                    int32 dmg = (int32)urand(20000, 22000);
                    me->CastCustomSpell(me, SPELL_PROMIMITY_BOMB_EXPLODE, &dmg, 0, 0, true);
                    me->KillSelf();
                    me->DespawnOrUnsummon(750);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_boomer_xp500AI(creature);
        }
};

class npc_arachnopod_destroyer : public CreatureScript
{
    public:
        npc_arachnopod_destroyer() : CreatureScript("npc_arachnopod_destroyer") { }

        struct npc_arachnopod_destroyerAI : public ScriptedAI
        {
            npc_arachnopod_destroyerAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 flamesprayTimer;
            uint32 machinegunTimer;
            uint32 friendfaction;
            bool novehicle;

            void InitializeAI()
            {
                novehicle = true;
                me->setFaction(16);
                me->SetFlag(IMMUNITY_MECHANIC, MECHANIC_BANDAGE);
            }

            void Reset()
            {
                flamesprayTimer = 1000;
                machinegunTimer = 8000;
            }

            void UpdateAI(uint32 diff)
            {
                if (novehicle)
                {
                    if (!UpdateVictim())
                        return;

                    if (flamesprayTimer <= diff)
                    {
                        DoCastVictim(SPELL_FLAME_SPRAY);
                        flamesprayTimer = 12000;
                    }
                    else
                        flamesprayTimer -= diff;

                    if (machinegunTimer <= diff)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f))
                            DoCast(target, SPELL_MACHINE_GUN);
                        machinegunTimer = 9000;
                    }
                    else
                        machinegunTimer -= diff;

                    DoMeleeAttackIfReady();
                }
                else
                {
                    me->setFaction(friendfaction);
                }

            }

            void DamageTaken(Unit* who, uint32& /*damage*/)
            {
                if (!who)
                    return;

                if (novehicle)
                {
                    if (me->GetHealthPct() <= 20)
                    {
                        friendfaction = who->getFaction();
                        me->setFaction(friendfaction);
                        me->setRegeneratingHealth(false);
                        me->RemoveFlag(IMMUNITY_MECHANIC, MECHANIC_BANDAGE);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                        _EnterEvadeMode();
                        me->SetLastDamagedTime(0);
                        novehicle = false;
                    }
                }
            }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_arachnopod_destroyerAI(creature);
        }
};

enum teleportSpell
{
    SPELL_TELEPORT_VISUAL   = 51347,
};

class AreaTrigger_at_kologarn_up_teleporter : public AreaTriggerScript
{
    public:
        AreaTrigger_at_kologarn_up_teleporter() : AreaTriggerScript("AreaTrigger_at_kologarn_up_teleporter") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/)
        {
            if (!player->IsAlive())
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
            {
                if (instance->GetBossState(BOSS_KOLOGARN) == DONE)
                    player->TeleportTo(603, 1859.45f, -24.1f, 448.9f, 3.27f);
                else
                    player->TeleportTo(603, 1747.86f, -24.01f, 448.80f, 6.278f);
            }

            player->CastSpell(player, SPELL_TELEPORT_VISUAL, true);
            return true;
        }
};

class AreaTrigger_at_mimiron_tram_up_teleporter : public AreaTriggerScript
{
    public:
        AreaTrigger_at_mimiron_tram_up_teleporter() : AreaTriggerScript("AreaTrigger_at_mimiron_tram_up_teleporter") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/)
        {
            if (!player->IsAlive())
                return false;

            player->TeleportTo(603, 2264.340f, 297.98f, 419.285f, 0.009f);

            player->CastSpell(player, SPELL_TELEPORT_VISUAL, true);
            return true;
        }
};

void AddSC_ulduar_trash()
{
    new npc_ulduar_storm_tempered_keeper();
    new npc_ulduar_charged_sphere();
    new npc_ulduar_snow_mound();
    new npc_ulduar_arachnopod_destroyer_trigger();

    new npc_prospector();
    new npc_console_helper();
    new npc_guardian_lasher();
    new npc_forest_swarmer();
    new npc_boomer_xp500();
    new npc_arachnopod_destroyer();
    new go_archivum_console();

    new go_dalaran_portal();
    new go_call_tram();

    new AreaTrigger_at_kologarn_up_teleporter();
    new AreaTrigger_at_mimiron_tram_up_teleporter();
}
