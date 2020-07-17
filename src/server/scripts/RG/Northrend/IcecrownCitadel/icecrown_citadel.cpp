/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "PassiveAI.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Transport.h"
#include "TransportMgr.h"
#include "SpellAuraEffects.h"
#include "SmartAI.h"
#include "Group.h"
#include "icecrown_citadel.h"
#include "ScriptedGossip.h"
#include "Chat.h"

// Weekly quest support
// * Deprogramming                (DONE)
// * Securing the Ramparts        (DONE)
// * Residue Rendezvous           (DONE)
// * Blood Quickening             (DONE)
// * Respite for a Tormented Soul

enum Texts
{
    // Highlord Tirion Fordring (at Light's Hammer)
    SAY_TIRION_INTRO_1              = 0,
    SAY_TIRION_INTRO_2              = 1,
    SAY_TIRION_INTRO_3              = 2,
    SAY_TIRION_INTRO_4              = 3,
    SAY_TIRION_INTRO_H_5            = 4,
    SAY_TIRION_INTRO_A_5            = 5,

    // The Lich King (at Light's Hammer)
    SAY_LK_INTRO_1                  = 0,
    SAY_LK_INTRO_2                  = 1,
    SAY_LK_INTRO_3                  = 2,
    SAY_LK_INTRO_4                  = 3,
    SAY_LK_INTRO_5                  = 4,

    // Highlord Bolvar Fordragon (at Light's Hammer)
    SAY_BOLVAR_INTRO_1              = 0,

    // High Overlord Saurfang (at Light's Hammer)
    SAY_SAURFANG_INTRO_1            = 15,
    SAY_SAURFANG_INTRO_2            = 16,
    SAY_SAURFANG_INTRO_3            = 17,
    SAY_SAURFANG_INTRO_4            = 18,

    // Muradin Bronzebeard (at Light's Hammer)
    SAY_MURADIN_INTRO_1             = 13,
    SAY_MURADIN_INTRO_2             = 14,
    SAY_MURADIN_INTRO_3             = 15,

    // Deathbound Ward
    SAY_TRAP_ACTIVATE               = 0,

    // Rotting Frost Giant
    EMOTE_DEATH_PLAGUE_WARNING      = 0,

    // Sister Svalna
    SAY_SVALNA_KILL_CAPTAIN         = 1, // happens when she kills a captain
    SAY_SVALNA_KILL                 = 4,
    SAY_SVALNA_CAPTAIN_DEATH        = 5, // happens when a captain resurrected by her dies
    SAY_SVALNA_DEATH                = 6,
    EMOTE_SVALNA_IMPALE             = 7,
    EMOTE_SVALNA_BROKEN_SHIELD      = 8,

    SAY_CROK_INTRO_1                = 0, // Ready your arms, my Argent Brothers. The Vrykul will protect the Frost Queen with their lives.
    SAY_ARNATH_INTRO_2              = 5, // Even dying here beats spending another day collecting reagents for that madman, Finklestein.
    SAY_CROK_INTRO_3                = 1, // Enough idle banter! Our champions have arrived - support them as we push our way through the hall!
    SAY_SVALNA_EVENT_START          = 0, // You may have once fought beside me, Crok, but now you are nothing more than a traitor. Come, your second death approaches!
    SAY_CROK_COMBAT_WP_0            = 2, // Draw them back to us, and we'll assist you.
    SAY_CROK_COMBAT_WP_1            = 3, // Quickly, push on!
    SAY_CROK_FINAL_WP               = 4, // Her reinforcements will arrive shortly, we must bring her down quickly!
    SAY_SVALNA_RESURRECT_CAPTAINS   = 2, // Foolish Crok. You brought my reinforcements with you. Arise, Argent Champions, and serve the Lich King in death!
    SAY_CROK_COMBAT_SVALNA          = 5, // I'll draw her attacks. Return our brothers to their graves, then help me bring her down!
    SAY_SVALNA_AGGRO                = 3, // Come, Scourgebane. I'll show the master which of us is truly worthy of the title of "Champion"!
    SAY_CAPTAIN_DEATH               = 0,
    SAY_CAPTAIN_RESURRECTED         = 1,
    SAY_CAPTAIN_KILL                = 2,
    SAY_CAPTAIN_SECOND_DEATH        = 3,
    SAY_CAPTAIN_SURVIVE_TALK        = 4,
    SAY_CROK_WEAKENING_GAUNTLET     = 6,
    SAY_CROK_WEAKENING_SVALNA       = 7,
    SAY_CROK_DEATH                  = 8,
    // PortEvent
    SAY_PORT_SUMMON                 = 5,
    SAY_PORT_SUMMON_TWO             = 9,
    // SpiderAttack
    SAY_ATTACK_START                = 0,
    // Vengeful Fleshreaper
    SAY_FLESHREAPER_SPAWN           = 0
};

enum Spells
{
    // Rotting Frost Giant
    SPELL_DEATH_PLAGUE              = 72879,
    SPELL_DEATH_PLAGUE_AURA         = 72865,
    SPELL_RECENTLY_INFECTED         = 72884,
    SPELL_DEATH_PLAGUE_KILL         = 72867,
    SPELL_STOMP                     = 64652,
    SPELL_ARCTIC_BREATH             = 72848,

    // Frost Freeze Trap
    SPELL_COLDFLAME_JETS            = 70460,

    // Alchemist Adrianna
    SPELL_HARVEST_BLIGHT_SPECIMEN   = 72155,
    SPELL_HARVEST_BLIGHT_SPECIMEN25 = 72162,

    // Crok Scourgebane
    SPELL_ICEBOUND_ARMOR            = 70714,
    SPELL_SCOURGE_STRIKE            = 71488,
    SPELL_DEATH_STRIKE              = 71489,

    // Sister Svalna
    SPELL_CARESS_OF_DEATH           = 70078,
    SPELL_IMPALING_SPEAR_KILL       = 70196,
    SPELL_REVIVE_CHAMPION           = 70053,
    SPELL_UNDEATH                   = 70089,
    SPELL_IMPALING_SPEAR            = 71443,
    SPELL_AETHER_SHIELD             = 71463,
    SPELL_HURL_SPEAR                = 71466,

    // Captain Arnath
    SPELL_DOMINATE_MIND             = 14515,
    SPELL_FLASH_HEAL_NORMAL         = 71595,
    SPELL_POWER_WORD_SHIELD_NORMAL  = 71548,
    SPELL_SMITE_NORMAL              = 71546,
    SPELL_FLASH_HEAL_UNDEAD         = 71782,
    SPELL_POWER_WORD_SHIELD_UNDEAD  = 71780,
    SPELL_SMITE_UNDEAD              = 71778,

    // Captain Brandon
    SPELL_CRUSADER_STRIKE           = 71549,
    SPELL_DIVINE_SHIELD             = 71550,
    SPELL_JUDGEMENT_OF_COMMAND      = 71551,
    SPELL_HAMMER_OF_BETRAYAL        = 71784,

    // Captain Grondel
    SPELL_CHARGE                    = 71553,
    SPELL_MORTAL_STRIKE             = 71552,
    SPELL_SUNDER_ARMOR              = 71554,
    SPELL_CONFLAGRATION             = 71785,

    // Captain Rupert
    SPELL_FEL_IRON_BOMB_NORMAL      = 71592,
    SPELL_MACHINE_GUN_NORMAL        = 71594,
    SPELL_ROCKET_LAUNCH_NORMAL      = 71590,
    SPELL_FEL_IRON_BOMB_UNDEAD      = 71787,
    SPELL_MACHINE_GUN_UNDEAD        = 71788,
    SPELL_ROCKET_LAUNCH_UNDEAD      = 71786,

    // Invisible Stalker (Float, Uninteractible, LargeAOI)
    SPELL_SOUL_MISSILE              = 72585,

    SPELL_GIANT_INSECT_SWARM        = 70475,
    SPELL_SUMMON_PLAGUED_INSECTS    = 70484,
    SPELL_INVISIBILITY              = 34426,
    SPELL_SIMPLE_TELEPORT           = 64195,

    // Orbs
    SPELL_EMPOWERED_BLOOD           = 70227,
    SPELL_EMPOWERED_BLOOD_TRIGGERD  = 70232,

    // SpiderAttack
    SPELL_RUSH                      = 71801,
    SPELL_CAST_NET_STARTROOM        = 69887,
    SPELL_CAST_NET                  = 69986,
    SPELL_GLACIAL_STRIKE            = 71317,
    SPELL_FROST_NOVA                = 71320,
    SPELL_ICE_TOMB                  = 71331,
    SPELL_FROSTBOLT                 = 71318,
    SPELL_WEB_WRAP                  = 70980,

    //Severed essence spells
    SPELL_CLONE_PLAYER              = 57507,
    SPELL_SEVERED_ESSENCE_10        = 71906,
    SPELL_SEVERED_ESSENCE_25        = 71942,
    //Druid spells
    SPELL_CAT_FORM                  = 57655,
    SPELL_MANGLE                    = 71925,
    SPELL_RIP                       = 71926,
    //Warlock
    SPELL_CORRUPTION                = 71937,
    SPELL_SHADOW_BOLT               = 71936,
    SPELL_RAIN_OF_CHAOS             = 71965,
    //Shaman
    SPELL_REPLENISHING_RAINS        = 71956,
    SPELL_LIGHTNING_BOLT            = 71934,
    //Rouge
    SPELL_DISENGAGE                 = 57635,
    SPELL_FOCUSED_ATTACKS           = 71955,
    SPELL_SINISTER_STRIKE           = 57640,
    SPELL_EVISCERATE                = 71933,
    //Mage
    SPELL_FIREBALL                  = 71928,
    //Warior
    SPELL_BLOODTHIRST               = 71938,
    SPELL_HEROIC_LEAP               = 71961,
    //Dk
    SPELL_DEATH_GRIP                = 57602,
    SPELL_NECROTIC_STRIKE           = 71951,
    SPELL_PLAGUE_STRIKE             = 71924,
    //Priest
    SPELL_GREATER_HEAL              = 71931,
    SPELL_RENEW                     = 71932,
    //Paladin
    SPELL_CLEANSE                   = 57767,
    SPELL_FLASH_OF_LIGHT            = 71930,
    SPELL_RADIANCE_AURA             = 71953,
    //Hunter
    SPELL_SHOOT_10                  = 71927,
    SPELL_SHOOT_25                  = 72258,
};

// Helper defines
// Captain Arnath
#define SPELL_FLASH_HEAL        (IsUndead ? SPELL_FLASH_HEAL_UNDEAD : SPELL_FLASH_HEAL_NORMAL)
#define SPELL_POWER_WORD_SHIELD (IsUndead ? SPELL_POWER_WORD_SHIELD_UNDEAD : SPELL_POWER_WORD_SHIELD_NORMAL)
#define SPELL_SMITE             (IsUndead ? SPELL_SMITE_UNDEAD : SPELL_SMITE_NORMAL)

// Captain Rupert
#define SPELL_FEL_IRON_BOMB     (IsUndead ? SPELL_FEL_IRON_BOMB_UNDEAD : SPELL_FEL_IRON_BOMB_NORMAL)
#define SPELL_MACHINE_GUN       (IsUndead ? SPELL_MACHINE_GUN_UNDEAD : SPELL_MACHINE_GUN_NORMAL)
#define SPELL_ROCKET_LAUNCH     (IsUndead ? SPELL_ROCKET_LAUNCH_UNDEAD : SPELL_ROCKET_LAUNCH_NORMAL)

enum EventTypes
{
    // Highlord Tirion Fordring (at Light's Hammer)
    // The Lich King (at Light's Hammer)
    // Highlord Bolvar Fordragon (at Light's Hammer)
    // High Overlord Saurfang (at Light's Hammer)
    // Muradin Bronzebeard (at Light's Hammer)
    EVENT_TIRION_INTRO_2                = 1,
    EVENT_TIRION_INTRO_3                = 2,
    EVENT_TIRION_INTRO_4                = 3,
    EVENT_TIRION_INTRO_5                = 4,
    EVENT_LK_INTRO_1                    = 5,
    EVENT_TIRION_INTRO_6                = 6,
    EVENT_LK_INTRO_2                    = 7,
    EVENT_LK_INTRO_3                    = 8,
    EVENT_LK_INTRO_4                    = 9,
    EVENT_BOLVAR_INTRO_1                = 10,
    EVENT_LK_INTRO_5                    = 11,
    EVENT_SAURFANG_INTRO_1              = 12,
    EVENT_TIRION_INTRO_H_7              = 13,
    EVENT_SAURFANG_INTRO_2              = 14,
    EVENT_SAURFANG_INTRO_3              = 15,
    EVENT_SAURFANG_INTRO_4              = 16,
    EVENT_SAURFANG_RUN                  = 17,
    EVENT_MURADIN_INTRO_1               = 18,
    EVENT_MURADIN_INTRO_2               = 19,
    EVENT_MURADIN_INTRO_3               = 20,
    EVENT_TIRION_INTRO_A_7              = 21,
    EVENT_MURADIN_INTRO_4               = 22,
    EVENT_MURADIN_INTRO_5               = 23,
    EVENT_MURADIN_RUN                   = 24,

    // Rotting Frost Giant
    EVENT_DEATH_PLAGUE                  = 25,
    EVENT_STOMP                         = 26,
    EVENT_ARCTIC_BREATH                 = 27,

    // Frost Freeze Trap
    EVENT_ACTIVATE_TRAP                 = 28,

    // Crok Scourgebane
    EVENT_SCOURGE_STRIKE                = 29,
    EVENT_DEATH_STRIKE                  = 30,
    EVENT_HEALTH_CHECK                  = 31,
    EVENT_CROK_INTRO_3                  = 32,
    EVENT_START_PATHING                 = 33,

    // Sister Svalna
    EVENT_ARNATH_INTRO_2                = 34,
    EVENT_SVALNA_START                  = 35,
    EVENT_SVALNA_RESURRECT              = 36,
    EVENT_SVALNA_COMBAT                 = 37,
    EVENT_IMPALING_SPEAR                = 38,
    EVENT_AETHER_SHIELD                 = 39,

    // Captain Arnath
    EVENT_ARNATH_FLASH_HEAL             = 40,
    EVENT_ARNATH_PW_SHIELD              = 41,
    EVENT_ARNATH_SMITE                  = 42,
    EVENT_ARNATH_DOMINATE_MIND          = 43,

    // Captain Brandon
    EVENT_BRANDON_CRUSADER_STRIKE       = 44,
    EVENT_BRANDON_DIVINE_SHIELD         = 45,
    EVENT_BRANDON_JUDGEMENT_OF_COMMAND  = 46,
    EVENT_BRANDON_HAMMER_OF_BETRAYAL    = 47,

    // Captain Grondel
    EVENT_GRONDEL_CHARGE_CHECK          = 48,
    EVENT_GRONDEL_MORTAL_STRIKE         = 49,
    EVENT_GRONDEL_SUNDER_ARMOR          = 50,
    EVENT_GRONDEL_CONFLAGRATION         = 51,

    // Captain Rupert
    EVENT_RUPERT_FEL_IRON_BOMB          = 52,
    EVENT_RUPERT_MACHINE_GUN            = 53,
    EVENT_RUPERT_ROCKET_LAUNCH          = 54,

    // Invisible Stalker (Float, Uninteractible, LargeAOI)
    EVENT_SOUL_MISSILE                  = 55,

    // ICC-Porter-Startevent
    EVENT_PORT_SUMMON                   = 56,
    EVENT_PORT_TALK                     = 57,
    EVENT_PORT_DESPAWN                  = 58,
    EVENT_PORT_DESPAWN_TWO              = 59,

    // Spider Attackevent
    EVENT_MOVE_GROUND                   = 60,
    EVENT_RUSH                          = 61,
    EVENT_GLACIAL_STRIKE                = 62,
    EVENT_FROST_NOVA                    = 63,
    EVENT_ICE_TOMB                      = 64,
    EVENT_FROSTBOLT                     = 65,

    //Val'kyr Herald
    EVENT_SEVERED_ESSENCE,
    EVENT_REMOVE_HOVER,

    //Druid spells
    EVENT_CAT_FORM,
    EVENT_MANGLE,
    EVENT_RIP,

    //Warlock
    EVENT_CORRUPTION,
    EVENT_SHADOW_BOLT,
    EVENT_RAIN_OF_CHAOS,

    //Shaman
    EVENT_REPLENISHING_RAINS,
    EVENT_LIGHTNING_BOLT,
    EVENT_CAN_CAST_REPLENISHING_RAINS,

    //Rogue
    EVENT_FOCUSED_ATTACKS,
    EVENT_SINISTER_STRIKE,
    EVENT_EVISCERATE,

    //Mage
    EVENT_FIREBALL,

    //Warrior
    EVENT_BLOODTHIRST,
    EVENT_HEROIC_LEAP,

    //Dead Knight
    EVENT_DEATH_GRIP,
    EVENT_NECROTIC_STRIKE,
    EVENT_PLAGUE_STRIKE,

    //Priest
    EVENT_GREATER_HEAL,
    EVENT_RENEW,

    //Paladin
    EVENT_CLEANSE,
    EVENT_FLASH_OF_LIGHT,
    EVENT_RADIANCE_AURA,
    EVENT_CAN_CAST_RADIANCE_AURA,

    //Hunter
    EVENT_SHOOT,
    EVENT_DISENGAGE,
    EVENT_CAN_CAST_DISENGAGE,
};

enum DataTypesICC
{
    DATA_DAMNED_KILLS       = 1,
    DATA_PLAYER_CLASS       = 2,
};

enum Actions
{
    // Sister Svalna
    ACTION_KILL_CAPTAIN         = 1,
    ACTION_START_GAUNTLET       = 2,
    ACTION_RESURRECT_CAPTAINS   = 3,
    ACTION_CAPTAIN_DIES         = 4,
    ACTION_RESET_EVENT          = 5,
};

enum EventIds
{
    EVENT_AWAKEN_WARD_1 = 22900,
    EVENT_AWAKEN_WARD_2 = 22907,
    EVENT_AWAKEN_WARD_3 = 22908,
    EVENT_AWAKEN_WARD_4 = 22909,
    EVENT_SPAWN_TRASH_1 = 22869,
    EVENT_SPAWN_TRASH_2 = 22870
};

enum MovementPoints
{
    POINT_LAND  = 1,
};

enum WPDatas
{
    WP_THE_DAMNED_RIGHT = 3701100,
    WP_THE_DAMNED_LEFT  = 3701101
};

enum Misc
{
    // Factions
    FACTION_NEUTRAL_RAID = 250
};

enum QuestCredits
{
    KC_ROTTING_FROST_GIANT_A = 38494,
    KC_ROTTING_FROST_GIANT_H = 38490,
};

Position const VengefulFleshreaperLoc[12] =
{
    {4350.479980f, 2981.300049f, 360.593994f, 1.221730f},
    {4354.109863f, 2978.199951f, 360.593994f, 1.431170f},
    {4360.740234f, 2982.070068f, 360.593994f, 1.797690f},
    {4359.919922f, 2977.669922f, 360.592987f, 1.710420f},
    {4367.479980f, 2981.010010f, 360.593994f, 2.094390f},
    {4367.339844f, 2975.840088f, 360.592987f, 1.989680f},
    {4362.379883f, 2973.649902f, 360.592987f, 1.780240f},
    {4360.160156f, 2968.679932f, 360.592987f, 1.675520f},
    {4365.529785f, 2969.550049f, 360.592987f, 1.850050f},
    {4355.229980f, 2972.149902f, 360.592987f, 1.518440f},
    {4351.859863f, 2968.189941f, 360.593994f, 1.413720f},
    {4349.419922f, 2975.739990f, 360.593994f, 1.256640f}
};

Position const FrostWyrmLeftPos        = { -439.829376f, 2038.987915f, 239.192108f, 1.561711f };
Position const FrostWyrmRightPos       = { -436.442902f, 2380.327393f, 236.586838f, 4.806367f };
Position const KorkronLieutenantPoint  = { -41.7986f,    2189.79f,     27.9859f,    1.91986f  };

class DeathPlagueTargetSelector
{
    public:
        explicit DeathPlagueTargetSelector(Unit* caster) : _caster(caster) {}

        bool operator()(WorldObject* object) const
        {
            if (object == _caster)
                return true;

            if (object->GetTypeId() != TYPEID_PLAYER)
                return true;

            if (object->ToUnit()->HasAura(SPELL_RECENTLY_INFECTED) || object->ToUnit()->HasAura(SPELL_DEATH_PLAGUE_AURA))
                return true;

            return false;
        }

    private:
        Unit* _caster;
};

class FrostwingVrykulSearcher
{
    public:
        FrostwingVrykulSearcher(Creature const* source, float range) : _source(source), _range(range) {}

        bool operator()(Unit* unit)
        {
            if (!unit->IsAlive())
                return false;

            switch (unit->GetEntry())
            {
                case NPC_YMIRJAR_BATTLE_MAIDEN:
                case NPC_YMIRJAR_DEATHBRINGER:
                case NPC_YMIRJAR_FROSTBINDER:
                case NPC_YMIRJAR_HUNTRESS:
                case NPC_YMIRJAR_WARLORD:
                    break;
                default:
                    return false;
            }

            if (!unit->IsWithinDist(_source, _range, false))
                return false;

            return true;
        }

    private:
        Creature const* _source;
        float _range;
};

class FrostwingGauntletRespawner
{
    public:
        void operator()(Creature* creature)
        {
            switch (creature->GetOriginalEntry())
            {
                case NPC_YMIRJAR_BATTLE_MAIDEN:
                case NPC_YMIRJAR_DEATHBRINGER:
                case NPC_YMIRJAR_FROSTBINDER:
                case NPC_YMIRJAR_HUNTRESS:
                case NPC_YMIRJAR_WARLORD:
                    break;
                case NPC_CROK_SCOURGEBANE:
                case NPC_CAPTAIN_ARNATH:
                case NPC_CAPTAIN_BRANDON:
                case NPC_CAPTAIN_GRONDEL:
                case NPC_CAPTAIN_RUPERT:
                    creature->AI()->DoAction(ACTION_RESET_EVENT);
                    break;
                case NPC_SISTER_SVALNA:
                    creature->AI()->DoAction(ACTION_RESET_EVENT);
                    // return, this creature is never dead if event is reset
                    return;
                default:
                    return;
            }

            uint32 corpseDelay = creature->GetCorpseDelay();
            uint32 respawnDelay = creature->GetRespawnDelay();
            creature->SetCorpseDelay(1);
            creature->SetRespawnDelay(2);

            if (CreatureData const* data = creature->GetCreatureData())
                creature->SetPosition(data->posX, data->posY, data->posZ, data->orientation);
            creature->DespawnOrUnsummon();

            creature->SetCorpseDelay(corpseDelay);
            creature->SetRespawnDelay(respawnDelay);
        }
};

class CaptainSurviveTalk : public BasicEvent
{
    public:
        explicit CaptainSurviveTalk(Creature const& owner) : _owner(owner) { }

        bool Execute(uint64 /*currTime*/, uint32 /*diff*/)
        {
            _owner.AI()->Talk(SAY_CAPTAIN_SURVIVE_TALK);
            return true;
        }

    private:
        Creature const& _owner;
};

// at Light's Hammer
class npc_highlord_tirion_fordring_lh : public CreatureScript
{
    public:
        npc_highlord_tirion_fordring_lh() : CreatureScript("npc_highlord_tirion_fordring_lh") { }

        struct npc_highlord_tirion_fordringAI : public ScriptedAI
        {
            npc_highlord_tirion_fordringAI(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript())
            {
            }

            void Reset()
            {
                _events.Reset();
                _theLichKing.Clear();
                _bolvarFordragon.Clear();
                _factionNPC.Clear();
                _damnedKills = 0;
            }

            // IMPORTANT NOTE: This is triggered from per-GUID scripts
            // of The Damned SAI
            void SetData(uint32 type, uint32 data)
            {
                if (type == DATA_DAMNED_KILLS && data == 1)
                {
                    if (++_damnedKills == 2)
                    {
                        if (Creature* theLichKing = me->FindNearestCreature(NPC_THE_LICH_KING_LH, 150.0f))
                        {
                            if (Creature* bolvarFordragon = me->FindNearestCreature(NPC_HIGHLORD_BOLVAR_FORDRAGON_LH, 150.0f))
                            {
                                if (Creature* factionNPC = me->FindNearestCreature(_instance->GetData(DATA_TEAM_IN_INSTANCE) == HORDE ? NPC_SE_HIGH_OVERLORD_SAURFANG : NPC_SE_MURADIN_BRONZEBEARD, 50.0f))
                                {
                                    me->setActive(true);
                                    _theLichKing = theLichKing->GetGUID();
                                    theLichKing->setActive(true);
                                    _bolvarFordragon = bolvarFordragon->GetGUID();
                                    bolvarFordragon->setActive(true);
                                    _factionNPC = factionNPC->GetGUID();
                                    factionNPC->setActive(true);
                                }
                            }
                        }

                        if (!_bolvarFordragon || !_theLichKing || !_factionNPC)
                            return;

                        Talk(SAY_TIRION_INTRO_1);
                        _events.ScheduleEvent(EVENT_TIRION_INTRO_2, 4000);
                        _events.ScheduleEvent(EVENT_TIRION_INTRO_3, 14000);
                        _events.ScheduleEvent(EVENT_TIRION_INTRO_4, 18000);
                        _events.ScheduleEvent(EVENT_TIRION_INTRO_5, 31000);
                        _events.ScheduleEvent(EVENT_LK_INTRO_1, 35000);
                        _events.ScheduleEvent(EVENT_TIRION_INTRO_6, 51000);
                        _events.ScheduleEvent(EVENT_LK_INTRO_2, 58000);
                        _events.ScheduleEvent(EVENT_LK_INTRO_3, 74000);
                        _events.ScheduleEvent(EVENT_LK_INTRO_4, 86000);
                        _events.ScheduleEvent(EVENT_BOLVAR_INTRO_1, 100000);
                        _events.ScheduleEvent(EVENT_LK_INTRO_5, 108000);

                        if (_instance->GetData(DATA_TEAM_IN_INSTANCE) == HORDE)
                        {
                            _events.ScheduleEvent(EVENT_SAURFANG_INTRO_1, 120000);
                            _events.ScheduleEvent(EVENT_TIRION_INTRO_H_7, 129000);
                            _events.ScheduleEvent(EVENT_SAURFANG_INTRO_2, 139000);
                            _events.ScheduleEvent(EVENT_SAURFANG_INTRO_3, 150000);
                            _events.ScheduleEvent(EVENT_SAURFANG_INTRO_4, 162000);
                            _events.ScheduleEvent(EVENT_SAURFANG_RUN, 170000);
                        }
                        else
                        {
                            _events.ScheduleEvent(EVENT_MURADIN_INTRO_1, 120000);
                            _events.ScheduleEvent(EVENT_MURADIN_INTRO_2, 124000);
                            _events.ScheduleEvent(EVENT_MURADIN_INTRO_3, 127000);
                            _events.ScheduleEvent(EVENT_TIRION_INTRO_A_7, 136000);
                            _events.ScheduleEvent(EVENT_MURADIN_INTRO_4, 144000);
                            _events.ScheduleEvent(EVENT_MURADIN_INTRO_5, 151000);
                            _events.ScheduleEvent(EVENT_MURADIN_RUN, 157000);
                        }
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (_damnedKills != 2)
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_TIRION_INTRO_2:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                            break;
                        case EVENT_TIRION_INTRO_3:
                            Talk(SAY_TIRION_INTRO_2);
                            break;
                        case EVENT_TIRION_INTRO_4:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                            break;
                        case EVENT_TIRION_INTRO_5:
                            Talk(SAY_TIRION_INTRO_3);
                            break;
                        case EVENT_LK_INTRO_1:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_POINT_NO_SHEATHE);
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                                theLichKing->AI()->Talk(SAY_LK_INTRO_1);
                            break;
                        case EVENT_TIRION_INTRO_6:
                            Talk(SAY_TIRION_INTRO_4);
                            break;
                        case EVENT_LK_INTRO_2:
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                                theLichKing->AI()->Talk(SAY_LK_INTRO_2);
                            break;
                        case EVENT_LK_INTRO_3:
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                                theLichKing->AI()->Talk(SAY_LK_INTRO_3);
                            break;
                        case EVENT_LK_INTRO_4:
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                                theLichKing->AI()->Talk(SAY_LK_INTRO_4);
                            break;
                        case EVENT_BOLVAR_INTRO_1:
                            if (Creature* bolvarFordragon = ObjectAccessor::GetCreature(*me, _bolvarFordragon))
                            {
                                bolvarFordragon->AI()->Talk(SAY_BOLVAR_INTRO_1);
                                bolvarFordragon->setActive(false);
                            }
                            break;
                        case EVENT_LK_INTRO_5:
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                            {
                                theLichKing->AI()->Talk(SAY_LK_INTRO_5);
                                theLichKing->setActive(false);
                            }
                            break;
                        case EVENT_SAURFANG_INTRO_1:
                            if (Creature* saurfang = ObjectAccessor::GetCreature(*me, _factionNPC))
                                saurfang->AI()->Talk(SAY_SAURFANG_INTRO_1);
                            break;
                        case EVENT_TIRION_INTRO_H_7:
                            Talk(SAY_TIRION_INTRO_H_5);
                            break;
                        case EVENT_SAURFANG_INTRO_2:
                            if (Creature* saurfang = ObjectAccessor::GetCreature(*me, _factionNPC))
                                saurfang->AI()->Talk(SAY_SAURFANG_INTRO_2);
                            break;
                        case EVENT_SAURFANG_INTRO_3:
                            if (Creature* saurfang = ObjectAccessor::GetCreature(*me, _factionNPC))
                                saurfang->AI()->Talk(SAY_SAURFANG_INTRO_3);
                            break;
                        case EVENT_SAURFANG_INTRO_4:
                            if (Creature* saurfang = ObjectAccessor::GetCreature(*me, _factionNPC))
                                saurfang->AI()->Talk(SAY_SAURFANG_INTRO_4);
                            break;
                        case EVENT_MURADIN_RUN:
                        case EVENT_SAURFANG_RUN:
                            if (Creature* factionNPC = ObjectAccessor::GetCreature(*me, _factionNPC))
                                factionNPC->GetMotionMaster()->MovePath(factionNPC->GetSpawnId()*10, false);
                            me->setActive(false);
                            _damnedKills = 3;
                            break;
                        case EVENT_MURADIN_INTRO_1:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->AI()->Talk(SAY_MURADIN_INTRO_1);
                            break;
                        case EVENT_MURADIN_INTRO_2:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            break;
                        case EVENT_MURADIN_INTRO_3:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                            break;
                        case EVENT_TIRION_INTRO_A_7:
                            Talk(SAY_TIRION_INTRO_A_5);
                            break;
                        case EVENT_MURADIN_INTRO_4:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->AI()->Talk(SAY_MURADIN_INTRO_2);
                            break;
                        case EVENT_MURADIN_INTRO_5:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->AI()->Talk(SAY_MURADIN_INTRO_3);
                            break;
                        default:
                            break;
                    }
                }
            }

        private:
            EventMap _events;
            InstanceScript* const _instance;
            ObjectGuid _theLichKing;
            ObjectGuid _bolvarFordragon;
            ObjectGuid _factionNPC;
            uint16 _damnedKills;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_highlord_tirion_fordringAI>(creature);
        }
};

class npc_rotting_frost_giant : public CreatureScript
{
    public:
        npc_rotting_frost_giant() : CreatureScript("npc_rotting_frost_giant") { }

        struct npc_rotting_frost_giantAI : public ScriptedAI
        {
            npc_rotting_frost_giantAI(Creature* creature) : ScriptedAI(creature)
            {
               me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
               me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
            }

            void Reset()
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_DEATH_PLAGUE, 15001);
                _events.ScheduleEvent(EVENT_STOMP, urand(5000, 8000));
                _events.ScheduleEvent(EVENT_ARCTIC_BREATH, urand(10000, 15000));
            }

            void JustDied(Unit* /*killer*/)
            {
                _events.Reset();
                for (Player& player : me->GetMap()->GetPlayers())
                {
                    player.KilledMonsterCredit(KC_ROTTING_FROST_GIANT_A);
                    player.KilledMonsterCredit(KC_ROTTING_FROST_GIANT_H);
                }
                me->SummonCreature(NPC_KOR_KRON_LIEUTENANT, KorkronLieutenantPoint, TEMPSUMMON_DEAD_DESPAWN, 30000);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DEATH_PLAGUE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0,
                                                            [pred = DeathPlagueTargetSelector(me->GetVictim())](Unit* unit)
                                                            { return !pred(unit); }))
                            {
                                Talk(EMOTE_DEATH_PLAGUE_WARNING, target);
                                DoCast(target, SPELL_DEATH_PLAGUE);
                            }
                            _events.ScheduleEvent(EVENT_DEATH_PLAGUE, 15001);
                            break;
                        case EVENT_STOMP:
                            DoCastVictim(SPELL_STOMP);
                            _events.ScheduleEvent(EVENT_STOMP, urand(15000, 18000));
                            break;
                        case EVENT_ARCTIC_BREATH:
                            DoCastVictim(SPELL_ARCTIC_BREATH);
                            _events.ScheduleEvent(EVENT_ARCTIC_BREATH, urand(26000, 33000));
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

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_rotting_frost_giantAI>(creature);
        }
};

class npc_frost_freeze_trap : public CreatureScript
{
    public:
        npc_frost_freeze_trap() : CreatureScript("npc_frost_freeze_trap") { }

        struct npc_frost_freeze_trapAI: public ScriptedAI
        {
            npc_frost_freeze_trapAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
                me->SetReactState(REACT_PASSIVE);
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case 1000:
                    case 11000:
                        _events.ScheduleEvent(EVENT_ACTIVATE_TRAP, uint32(action));
                        break;
                    default:
                        break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (me->IsInCombat())
                    me->CombatStop(false);

                _events.Update(diff);

                if (_events.ExecuteEvent() == EVENT_ACTIVATE_TRAP)
                {
                    DoCastSelf(SPELL_COLDFLAME_JETS);
                    _events.ScheduleEvent(EVENT_ACTIVATE_TRAP, 22000);
                }
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_frost_freeze_trapAI>(creature);
        }
};

class npc_alchemist_adrianna : public CreatureScript
{
    public:
        npc_alchemist_adrianna() : CreatureScript("npc_alchemist_adrianna") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            player->PrepareQuestMenu(creature->GetGUID());

            QuestMenu &qm = player->PlayerTalkClass->GetQuestMenu();
            QuestMenu qm2;

            for (uint32 i = 0; i<qm.GetMenuItemCount(); ++i)
            {
                switch (qm.GetItem(i).QuestId)
                {
                    case QUEST_RESIDUE_RENDEZVOUS_10:
                        if ((player->GetDifficulty(true) == RAID_DIFFICULTY_10MAN_NORMAL) || (player->GetDifficulty(true) == RAID_DIFFICULTY_10MAN_HEROIC))
                            qm2.AddMenuItem(qm.GetItem(i).QuestId, qm.GetItem(i).QuestIcon);
                        break;
                    case QUEST_RESIDUE_RENDEZVOUS_25:
                        if ((player->GetDifficulty(true) == RAID_DIFFICULTY_25MAN_NORMAL) || (player->GetDifficulty(true) == RAID_DIFFICULTY_25MAN_HEROIC))
                            qm2.AddMenuItem(qm.GetItem(i).QuestId, qm.GetItem(i).QuestIcon);
                        break;
                    default:
                        break;
                }
            }
            
            qm.ClearMenu();

            for (uint32 i = 0; i<qm2.GetMenuItemCount(); ++i)
                qm.AddMenuItem(qm2.GetItem(i).QuestId, qm2.GetItem(i).QuestIcon);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

            if (!creature->FindCurrentSpellBySpellId(SPELL_HARVEST_BLIGHT_SPECIMEN) && !creature->FindCurrentSpellBySpellId(SPELL_HARVEST_BLIGHT_SPECIMEN25))
                if (player->HasAura(SPELL_ORANGE_BLIGHT_RESIDUE) && player->HasAura(SPELL_GREEN_BLIGHT_RESIDUE))
                    creature->CastSpell(creature, SPELL_HARVEST_BLIGHT_SPECIMEN, false);

            return true;
        }
};

class npc_questgiver_icc : public CreatureScript
{
    public:
        npc_questgiver_icc() : CreatureScript("npc_questgiver_icc") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {    
            player->PrepareQuestMenu(creature->GetGUID());

            QuestMenu &qm = player->PlayerTalkClass->GetQuestMenu();
            QuestMenu qm2;

            for (uint32 i = 0; i<qm.GetMenuItemCount(); ++i)
            {
                switch (qm.GetItem(i).QuestId)
                {
                case QUEST_DEPROGRAMMING_10:
                case QUEST_SECURING_THE_RAMPARTS_10:
                case QUEST_BLOOD_QUICKENING_10:
                case QUEST_RESPITE_FOR_A_TORNMENTED_SOUL_10:
                    if ((player->GetDifficulty(true) == RAID_DIFFICULTY_10MAN_NORMAL) || (player->GetDifficulty(true) == RAID_DIFFICULTY_10MAN_HEROIC))
                        qm2.AddMenuItem(qm.GetItem(i).QuestId, qm.GetItem(i).QuestIcon);
                    break;
                case QUEST_DEPROGRAMMING_25:
                case QUEST_SECURING_THE_RAMPARTS_25:
                case QUEST_BLOOD_QUICKENING_25:
                case QUEST_RESPITE_FOR_A_TORNMENTED_SOUL_25:
                    if ((player->GetDifficulty(true) == RAID_DIFFICULTY_25MAN_NORMAL) || (player->GetDifficulty(true) == RAID_DIFFICULTY_25MAN_HEROIC))
                        qm2.AddMenuItem(qm.GetItem(i).QuestId, qm.GetItem(i).QuestIcon);
                    break;
                default:
                    break;
                }
            }

            qm.ClearMenu();

            for (uint32 i = 0; i<qm2.GetMenuItemCount(); ++i)
                qm.AddMenuItem(qm2.GetItem(i).QuestId, qm2.GetItem(i).QuestIcon);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }
};

class boss_sister_svalna : public CreatureScript
{
    public:
        boss_sister_svalna() : CreatureScript("boss_sister_svalna") { }

        struct boss_sister_svalnaAI : public BossAI
        {
            boss_sister_svalnaAI(Creature* creature) : BossAI(creature, DATA_SISTER_SVALNA),
                _isEventInProgress(false), fallbackTimer(0)
            {
               me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
               me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
            }

            void InitializeAI()
            {
                if (!me->isDead())
                    Reset();

                // me->SetReactState(REACT_PASSIVE);
                me->SetCanFly(true);
                me->SetDisableGravity(true);
                me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_HOVER);
                me->SetHover(true);
                // me->SetImmuneToAll(true);
                _isEventInProgress = false;
            }
            
            void AttackStart(Unit* victim)
            {
                if (me->HasReactState(REACT_PASSIVE) || me->IsImmuneToPC() || me->IsImmuneToNPC())
                    return;
                    
                BossAI::AttackStart(victim);
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);
                Talk(SAY_SVALNA_DEATH);

                for (Player& player : me->GetMap()->GetPlayers())
                    player.CombatStop();

                uint64 delay = 1;
                for (uint32 i = 0; i < 4; ++i)
                {
                    if (Creature* crusader = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_CAPTAIN_ARNATH + i))))
                    {
                        if (crusader->IsAlive() && crusader->GetEntry() == crusader->GetCreatureData()->id)
                        {
                            crusader->m_Events.AddEvent(new CaptainSurviveTalk(*crusader), crusader->m_Events.CalculateTime(delay));
                            delay += 6000;
                        }
                    }
                }
            }

            void EnterCombat(Unit* who)
            {
                if (me->HasReactState(REACT_PASSIVE) || me->IsImmuneToPC() || me->IsImmuneToNPC())
                {
                    me->CombatStop(false);
                    me->SetImmuneToAll(true);
                    me->SetReactState(REACT_PASSIVE);
                    return;
                }
				
                BossAI::EnterCombat(who);
                if (Creature* crok = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_CROK_SCOURGEBANE))))
                {
                    crok->AI()->Talk(SAY_CROK_COMBAT_SVALNA);
                    crok->AI()->AttackStart(me);
                }
                events.ScheduleEvent(EVENT_SVALNA_COMBAT, 9000);
                events.ScheduleEvent(EVENT_IMPALING_SPEAR, urand(40000, 50000));
                events.ScheduleEvent(EVENT_AETHER_SHIELD, urand(100000, 110000));
				me->GetMotionMaster()->MoveFall();
            }

            void KilledUnit(Unit* victim)
            {
                switch (victim->GetTypeId())
                {
                    case TYPEID_PLAYER:
                        Talk(SAY_SVALNA_KILL);
                        break;
                    case TYPEID_UNIT:
                        switch (victim->GetEntry())
                        {
                            case NPC_CAPTAIN_ARNATH:
                            case NPC_CAPTAIN_BRANDON:
                            case NPC_CAPTAIN_GRONDEL:
                            case NPC_CAPTAIN_RUPERT:
                                Talk(SAY_SVALNA_KILL_CAPTAIN);
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }

            // @todo Fix reset behavior
            // void JustReachedHome()
            // {
            //     BossAI::JustReachedHome();
            //     me->SetReactState(REACT_PASSIVE);
            //     me->SetDisableGravity(false);
            //     me->SetHover(false);
            // }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_KILL_CAPTAIN:
                        me->CastCustomSpell(SPELL_CARESS_OF_DEATH, SPELLVALUE_MAX_TARGETS, 1, me, true);
                        break;
                    case ACTION_START_GAUNTLET:
                        me->setActive(true);
                        _isEventInProgress = true;
                        me->SetImmuneToAll(true);
                        events.ScheduleEvent(EVENT_SVALNA_START, 25000);
                        break;
                    case ACTION_RESURRECT_CAPTAINS:
                        events.ScheduleEvent(EVENT_SVALNA_RESURRECT, 7000);
                        break;
                    case ACTION_CAPTAIN_DIES:
                        Talk(SAY_SVALNA_CAPTAIN_DEATH);
                        break;
                    case ACTION_RESET_EVENT:
                        me->setActive(false);
                        Reset();
                        break;
                    default:
                        break;
                }
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_HURL_SPEAR && me->HasAura(SPELL_AETHER_SHIELD))
                {
                    me->RemoveAurasDueToSpell(SPELL_AETHER_SHIELD);
                    Talk(EMOTE_SVALNA_BROKEN_SHIELD, caster);
                }
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != EFFECT_MOTION_TYPE || id != POINT_LAND)
                    return;
                
                StartCombat();
            }

            void StartCombat()
            {
                if (!_isEventInProgress)
                    return;

                _isEventInProgress = false;
                me->setActive(false);
                me->SetImmuneToAll(false);
                me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_HOVER);
                me->SetDisableGravity(false);
                me->SetHover(false);
                me->SetCanFly(false);
                me->SetReactState(REACT_AGGRESSIVE);
                DoZoneInCombat(NULL, 150.0f);
                fallbackTimer = 0;
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                switch (spell->Id)
                {
                    case SPELL_IMPALING_SPEAR_KILL:
                        me->Kill(target);
                        break;
                    case SPELL_IMPALING_SPEAR:
                        if (TempSummon* summon = target->SummonCreature(NPC_IMPALING_SPEAR, *target))
                        {
                            Talk(EMOTE_SVALNA_IMPALE, target);
                            summon->CastCustomSpell(VEHICLE_SPELL_RIDE_HARDCODED, SPELLVALUE_BASE_POINT0, 1, target, false);
                            summon->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_UNK1 | UNIT_FLAG2_ALLOW_ENEMY_INTERACT);
                        }
                        break;
                    default:
                        break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (fallbackTimer)
                {
                    if (fallbackTimer <= diff)
                        StartCombat();
                    else
                        fallbackTimer -= diff;
                }

                if (!UpdateVictim() && !_isEventInProgress)
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SVALNA_START:
                            Talk(SAY_SVALNA_EVENT_START);
                            break;
                        case EVENT_SVALNA_RESURRECT:
                            fallbackTimer = 30 * IN_MILLISECONDS;
                            Talk(SAY_SVALNA_RESURRECT_CAPTAINS);
                            DoCastSelf(SPELL_REVIVE_CHAMPION, false);
                            break;
                        case EVENT_SVALNA_COMBAT:
                            me->SetReactState(REACT_DEFENSIVE);
                            Talk(SAY_SVALNA_AGGRO);
                            if (Creature* crok = me->FindNearestCreature(NPC_CROK_SCOURGEBANE, 100.0f))
                            {
                                crok->setFaction(FACTION_NEUTRAL_RAID); 
                                crok->AI()->AttackStart(me);
                            }
                            break;
                        case EVENT_IMPALING_SPEAR:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, false, -SPELL_IMPALING_SPEAR))
                            {
                                DoCastSelf(SPELL_AETHER_SHIELD);
                                DoCast(target, SPELL_IMPALING_SPEAR);
                            }
                            events.ScheduleEvent(EVENT_IMPALING_SPEAR, urand(20000, 25000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint32 fallbackTimer;
            bool _isEventInProgress;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<boss_sister_svalnaAI>(creature);
        }
};

class npc_crok_scourgebane : public CreatureScript
{
    public:
        npc_crok_scourgebane() : CreatureScript("npc_crok_scourgebane") { }

        struct npc_crok_scourgebaneAI : public npc_escortAI
        {
            npc_crok_scourgebaneAI(Creature* creature) : npc_escortAI(creature),
                _instance(creature->GetInstanceScript()), _respawnTime(creature->GetRespawnDelay()),
                _corpseDelay(creature->GetCorpseDelay())
            {
                SetDespawnAtEnd(false);
                SetDespawnAtFar(false);
                _isEventActive = false;
                _isEventDone = _instance->GetBossState(DATA_SISTER_SVALNA) == DONE;
                _didUnderTenPercentText = false;
            }

            void Reset()
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_SCOURGE_STRIKE, urand(7500, 12500));
                _events.ScheduleEvent(EVENT_DEATH_STRIKE, urand(25000, 30000));
                me->SetReactState(REACT_DEFENSIVE);
                _didUnderTenPercentText = false;
                _wipeCheckTimer = 1000;
            }

            void DoAction(int32 action)
            {
                if (action == ACTION_START_GAUNTLET)
                {
                    if (_isEventDone || !me->IsAlive())
                        return;

                    _isEventActive = true;
                    _isEventDone = true;
                    // Load Grid with Sister Svalna
                    me->GetMap()->LoadGrid(4356.71f, 2484.33f);
                    if (Creature* svalna = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(DATA_SISTER_SVALNA))))
                        svalna->AI()->DoAction(ACTION_START_GAUNTLET);
                    Talk(SAY_CROK_INTRO_1);
                    _events.ScheduleEvent(EVENT_ARNATH_INTRO_2, 7000);
                    _events.ScheduleEvent(EVENT_CROK_INTRO_3, 14000);
                    _events.ScheduleEvent(EVENT_START_PATHING, 37000);
                    me->SetImmuneToNPC(true);
                    me->setActive(true);
                    for (uint32 i = 0; i < 4; ++i)
                        if (Creature* crusader = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(DATA_CAPTAIN_ARNATH + i))))
                            crusader->AI()->DoAction(ACTION_START_GAUNTLET);
                }
                else if (action == ACTION_RESET_EVENT)
                {
                    _isEventActive = false;
                    _isEventDone = _instance->GetBossState(DATA_SISTER_SVALNA) == DONE;
                    me->setActive(false);
                    _aliveTrash.clear();
                    _currentWPid = 0;
                }
            }

            void SetGuidData(ObjectGuid guid, uint32 type/* = 0*/) override
            {
                if (type == ACTION_VRYKUL_DEATH)
                {
                    _aliveTrash.erase(guid);
                    if (_aliveTrash.empty())
                    {
                        SetEscortPaused(false);
                        if (_currentWPid == 4 && _isEventActive)
                        {
                            _isEventActive = false;
                            me->setActive(false);
                            Talk(SAY_CROK_FINAL_WP);
                            if (Creature* svalna = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(DATA_SISTER_SVALNA))))
                                svalna->AI()->DoAction(ACTION_RESURRECT_CAPTAINS);
                        }
                    }
                }
            }

            void WaypointReached(uint32 waypointId)
            {
                switch (waypointId)
                {
                    // pause pathing until trash pack is cleared
                    case 0:
                        Talk(SAY_CROK_COMBAT_WP_0);
                        if (!_aliveTrash.empty())
                            SetEscortPaused(true);
                        break;
                    case 1:
                        Talk(SAY_CROK_COMBAT_WP_1);
                        if (!_aliveTrash.empty())
                            SetEscortPaused(true);
                        break;
                    case 4:
                        if (_aliveTrash.empty() && _isEventActive)
                        {
                            _isEventActive = false;
                            me->setActive(false);
                            Talk(SAY_CROK_FINAL_WP);
                            if (Creature* svalna = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(DATA_SISTER_SVALNA))))
                                svalna->AI()->DoAction(ACTION_RESURRECT_CAPTAINS);
                        }
                        break;
                    default:
                        break;
                }
            }

            void WaypointStart(uint32 waypointId)
            {
                _currentWPid = waypointId;
                switch (waypointId)
                {
                    case 0:
                    case 1:
                    case 4:
                    {
                        // get spawns by home position
                        float minY = 2600.0f;
                        float maxY = 2650.0f;
                        if (waypointId == 1)
                        {
                            minY -= 50.0f;
                            maxY -= 50.0f;
                            // at waypoints 1 and 2 she kills one captain
                            if (Creature* svalna = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(DATA_SISTER_SVALNA))))
                                svalna->AI()->DoAction(ACTION_KILL_CAPTAIN);
                        }
                        else if (waypointId == 4)
                        {
                            minY -= 100.0f;
                            maxY -= 100.0f;
                        }

                        // get all nearby vrykul
                        std::list<Creature*> temp;
                        FrostwingVrykulSearcher check(me, 80.0f);

                        me->VisitAnyNearbyObject<Creature, Trinity::ContainerAction>(80.0f, temp, check);

                        _aliveTrash.clear();
                        for (std::list<Creature*>::iterator itr = temp.begin(); itr != temp.end(); ++itr)
                            if ((*itr)->GetHomePosition().GetPositionY() < maxY && (*itr)->GetHomePosition().GetPositionY() > minY)
                                _aliveTrash.insert((*itr)->GetGUID());
                        break;
                    }
                    // at waypoints 1 and 2 she kills one captain
                    case 2:
                        if (Creature* svalna = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(DATA_SISTER_SVALNA))))
                            svalna->AI()->DoAction(ACTION_KILL_CAPTAIN);
                        break;
                    default:
                        break;
                }
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                // check wipe
                if (!_wipeCheckTimer)
                {
                    _wipeCheckTimer = 1000;
                    Trinity::AnyPlayerInObjectRangeCheck check(me, 60.0f);

                    Player* player = me->VisitSingleNearbyObject<Player, true>(60.0f, check);
                    // wipe
                    if (!player)
                    {
                        damage *= 100;
                        if (damage >= me->GetHealth())
                        {
                            FrostwingGauntletRespawner respawner;
                            me->VisitAnyNearbyObject<Creature, Trinity::FunctionAction>(333.0f, respawner);
                            Talk(SAY_CROK_DEATH);
                        }
                        return;
                    }
                }

                if (HealthBelowPct(10))
                {
                    if (!_didUnderTenPercentText)
                    {
                        _didUnderTenPercentText = true;
                        if (_isEventActive)
                            Talk(SAY_CROK_WEAKENING_GAUNTLET);
                        else
                            Talk(SAY_CROK_WEAKENING_SVALNA);
                    }

                    damage = 0;
                    DoCastSelf(SPELL_ICEBOUND_ARMOR);
                    _events.ScheduleEvent(EVENT_HEALTH_CHECK, 1000);
                }
            }

            void UpdateEscortAI(uint32 const diff)
            {
                npc_escortAI::UpdateEscortAI(diff);

                if (_wipeCheckTimer <= diff)
                    _wipeCheckTimer = 0;
                else
                    _wipeCheckTimer -= diff;

                if (!UpdateVictim() && !_isEventActive)
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ARNATH_INTRO_2:
                            if (Creature* arnath = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(DATA_CAPTAIN_ARNATH))))
                                arnath->AI()->Talk(SAY_ARNATH_INTRO_2);
                            break;
                        case EVENT_CROK_INTRO_3:
                            Talk(SAY_CROK_INTRO_3);
                            break;
                        case EVENT_START_PATHING:
                            Start(true, true);
                            break;
                        case EVENT_SCOURGE_STRIKE:
                            DoCastVictim(SPELL_SCOURGE_STRIKE);
                            _events.ScheduleEvent(EVENT_SCOURGE_STRIKE, urand(10000, 14000));
                            break;
                        case EVENT_DEATH_STRIKE:
                            if (HealthBelowPct(20))
                                DoCastVictim(SPELL_DEATH_STRIKE);
                            _events.ScheduleEvent(EVENT_DEATH_STRIKE, urand(5000, 10000));
                            break;
                        case EVENT_HEALTH_CHECK:
                            if (HealthAbovePct(15))
                            {
                                me->RemoveAurasDueToSpell(SPELL_ICEBOUND_ARMOR);
                                _didUnderTenPercentText = false;
                            }
                            else
                            {
                                me->DealHeal(me, me->CountPctFromMaxHealth(5));
                                _events.ScheduleEvent(EVENT_HEALTH_CHECK, 1000);
                            }
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            bool CanAIAttack(Unit const* target) const
            {
                // do not see targets inside Frostwing Halls when we are not there
                return (me->GetPositionY() > 2660.0f) == (target->GetPositionY() > 2660.0f);
            }

        private:
            EventMap _events;
            GuidSet _aliveTrash;
            InstanceScript* _instance;
            uint32 _currentWPid;
            uint32 _wipeCheckTimer;
            uint32 const _respawnTime;
            uint32 const _corpseDelay;
            bool _isEventActive;
            bool _isEventDone;
            bool _didUnderTenPercentText;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_crok_scourgebaneAI>(creature);
        }
};

struct npc_argent_captainAI : public ScriptedAI
{
    public:
        npc_argent_captainAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()), _firstDeath(true)
        {
            FollowAngle = PET_FOLLOW_ANGLE;
            FollowDist = PET_FOLLOW_DIST;
            IsUndead = false;
        }

        void JustDied(Unit* /*killer*/)
        {
            if (_firstDeath)
            {
                _firstDeath = false;
                Talk(SAY_CAPTAIN_DEATH);
            }
            else
                Talk(SAY_CAPTAIN_SECOND_DEATH);
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_CAPTAIN_KILL);
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_START_GAUNTLET)
            {
                if (Creature* crok = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_CROK_SCOURGEBANE))))
                {
                    me->SetReactState(REACT_DEFENSIVE);
                    FollowAngle = me->GetAngle(crok) + me->GetOrientation();
                    FollowDist = me->GetDistance2d(crok);
                    me->GetMotionMaster()->MoveFollow(crok, FollowDist, FollowAngle, MOTION_SLOT_IDLE);
                }

                me->setActive(true);
            }
            else if (action == ACTION_RESET_EVENT)
            {
                _firstDeath = true;
            }
        }

        void EnterCombat(Unit* /*target*/)
        {
            me->SetHomePosition(*me);
            if (IsUndead)
                DoZoneInCombat();
        }

        bool CanAIAttack(Unit const* target) const
        {
            // do not see targets inside Frostwing Halls when we are not there
            return (me->GetPositionY() > 2660.0f) == (target->GetPositionY() > 2660.0f);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            // not yet following
            if (me->GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_IDLE) != CHASE_MOTION_TYPE || IsUndead)
            {
                ScriptedAI::EnterEvadeMode();
                return;
            }

            if (!_EnterEvadeMode())
                return;

            if (!me->GetVehicle())
            {
                me->GetMotionMaster()->Clear(false);
                if (Creature* crok = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_CROK_SCOURGEBANE))))
                    me->GetMotionMaster()->MoveFollow(crok, FollowDist, FollowAngle, MOTION_SLOT_IDLE);
            }

            Reset();
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_REVIVE_CHAMPION && !IsUndead)
            {
                IsUndead = true;
                me->setDeathState(JUST_RESPAWNED);
                uint32 newEntry = 0;
                switch (me->GetEntry())
                {
                    case NPC_CAPTAIN_ARNATH:
                        newEntry = NPC_CAPTAIN_ARNATH_UNDEAD;
                        break;
                    case NPC_CAPTAIN_BRANDON:
                        newEntry = NPC_CAPTAIN_BRANDON_UNDEAD;
                        break;
                    case NPC_CAPTAIN_GRONDEL:
                        newEntry = NPC_CAPTAIN_GRONDEL_UNDEAD;
                        break;
                    case NPC_CAPTAIN_RUPERT:
                        newEntry = NPC_CAPTAIN_RUPERT_UNDEAD;
                        break;
                    default:
                        return;
                }

                Talk(SAY_CAPTAIN_RESURRECTED);
                me->UpdateEntry(newEntry, me->GetCreatureData());
                DoCastSelf(SPELL_UNDEATH, true);
                me->SetInCombatWithZone();
                me->SetReactState(REACT_AGGRESSIVE);
                AttackStart(me->FindNearestPlayer(500.0f));
            }
        }

    protected:
        EventMap Events;
        InstanceScript* instance;
        float FollowAngle;
        float FollowDist;
        bool IsUndead;

    private:
        bool _firstDeath;
};

class npc_captain_arnath : public CreatureScript
{
    public:
        npc_captain_arnath() : CreatureScript("npc_captain_arnath") { }

        struct npc_captain_arnathAI : public npc_argent_captainAI
        {
            npc_captain_arnathAI(Creature* creature) : npc_argent_captainAI(creature)
            {
            }

            void Reset()
            {
                Events.Reset();
                Events.ScheduleEvent(EVENT_ARNATH_FLASH_HEAL, urand(4000, 7000));
                Events.ScheduleEvent(EVENT_ARNATH_PW_SHIELD, urand(8000, 14000));
                Events.ScheduleEvent(EVENT_ARNATH_SMITE, urand(3000, 6000));
                if (Is25ManRaid() && IsUndead)
                    Events.ScheduleEvent(EVENT_ARNATH_DOMINATE_MIND, urand(22000, 27000));
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ARNATH_FLASH_HEAL:
                            if (Creature* target = FindFriendlyCreature())
                                DoCast(target, SPELL_FLASH_HEAL);
                            Events.ScheduleEvent(EVENT_ARNATH_FLASH_HEAL, urand(6000, 9000));
                            break;
                        case EVENT_ARNATH_PW_SHIELD:
                        {
                            std::list<Creature*> targets = DoFindFriendlyMissingBuff(40.0f, SPELL_POWER_WORD_SHIELD);
                            DoCast(Trinity::Containers::SelectRandomContainerElement(targets), SPELL_POWER_WORD_SHIELD);
                            Events.ScheduleEvent(EVENT_ARNATH_PW_SHIELD, urand(15000, 20000));
                            break;
                        }
                        case EVENT_ARNATH_SMITE:
                            DoCastVictim(SPELL_SMITE);
                            Events.ScheduleEvent(EVENT_ARNATH_SMITE, urand(4000, 7000));
                            break;
                        case EVENT_ARNATH_DOMINATE_MIND:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_DOMINATE_MIND);
                            Events.ScheduleEvent(EVENT_ARNATH_DOMINATE_MIND, urand(28000, 37000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            Creature* FindFriendlyCreature() const
            {
                Trinity::MostHPMissingInRange u_check(me, 60.0f, 0);

                return me->VisitSingleNearbyObject<Creature>(60.0f, u_check);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_captain_arnathAI>(creature);
        }
};

class npc_captain_brandon : public CreatureScript
{
    public:
        npc_captain_brandon() : CreatureScript("npc_captain_brandon") { }

        struct npc_captain_brandonAI : public npc_argent_captainAI
        {
            npc_captain_brandonAI(Creature* creature) : npc_argent_captainAI(creature)
            {
            }

            void Reset()
            {
                Bubbled = false;
                Events.Reset();
                Events.ScheduleEvent(EVENT_BRANDON_CRUSADER_STRIKE, urand(6000, 10000));
                Events.ScheduleEvent(EVENT_BRANDON_DIVINE_SHIELD, 500);
                Events.ScheduleEvent(EVENT_BRANDON_JUDGEMENT_OF_COMMAND, urand(8000, 13000));
                if (IsUndead)
                    Events.ScheduleEvent(EVENT_BRANDON_HAMMER_OF_BETRAYAL, urand(25000, 30000));
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BRANDON_CRUSADER_STRIKE:
                            DoCastVictim(SPELL_CRUSADER_STRIKE);
                            Events.ScheduleEvent(EVENT_BRANDON_CRUSADER_STRIKE, urand(6000, 12000));
                            break;
                        case EVENT_BRANDON_DIVINE_SHIELD:
                            if (!Bubbled)
                            { 
                                if (HealthBelowPct(20))
                                {
                                    Bubbled = true;
                                    DoCastSelf(SPELL_DIVINE_SHIELD);
                                    Events.ScheduleEvent(EVENT_BRANDON_DIVINE_SHIELD, 20000);
                                }
                                else
                                    Events.ScheduleEvent(EVENT_BRANDON_DIVINE_SHIELD, 500);
                            }                                
                            break;
                        case EVENT_BRANDON_JUDGEMENT_OF_COMMAND:
                            DoCastVictim(SPELL_JUDGEMENT_OF_COMMAND);
                            Events.ScheduleEvent(EVENT_BRANDON_JUDGEMENT_OF_COMMAND, urand(8000, 13000));
                            break;
                        case EVENT_BRANDON_HAMMER_OF_BETRAYAL:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_HAMMER_OF_BETRAYAL);
                            Events.ScheduleEvent(EVENT_BRANDON_HAMMER_OF_BETRAYAL, urand(45000, 60000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        private:
            bool Bubbled;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_captain_brandonAI>(creature);
        }
};

class npc_captain_grondel : public CreatureScript
{
    public:
        npc_captain_grondel() : CreatureScript("npc_captain_grondel") { }

        struct npc_captain_grondelAI : public npc_argent_captainAI
        {
            npc_captain_grondelAI(Creature* creature) : npc_argent_captainAI(creature)
            {
            }

            void Reset()
            {
                Events.Reset();
                Events.ScheduleEvent(EVENT_GRONDEL_CHARGE_CHECK, 500);
                Events.ScheduleEvent(EVENT_GRONDEL_MORTAL_STRIKE, urand(8000, 14000));
                Events.ScheduleEvent(EVENT_GRONDEL_SUNDER_ARMOR, urand(3000, 12000));
                if (IsUndead)
                    Events.ScheduleEvent(EVENT_GRONDEL_CONFLAGRATION, urand(12000, 17000));
            }


            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_GRONDEL_CHARGE_CHECK:
                            DoCastVictim(SPELL_CHARGE);
                            Events.ScheduleEvent(EVENT_GRONDEL_CHARGE_CHECK, 500);
                            break;
                        case EVENT_GRONDEL_MORTAL_STRIKE:
                            DoCastVictim(SPELL_MORTAL_STRIKE);
                            Events.ScheduleEvent(EVENT_GRONDEL_MORTAL_STRIKE, urand(10000, 15000));
                            break;
                        case EVENT_GRONDEL_SUNDER_ARMOR:
                            DoCastVictim(SPELL_SUNDER_ARMOR);
                            Events.ScheduleEvent(EVENT_GRONDEL_SUNDER_ARMOR, urand(5000, 17000));
                            break;
                        case EVENT_GRONDEL_CONFLAGRATION:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_CONFLAGRATION);
                            Events.ScheduleEvent(EVENT_GRONDEL_CONFLAGRATION, urand(10000, 15000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_captain_grondelAI>(creature);
        }
};

class npc_captain_rupert : public CreatureScript
{
    public:
        npc_captain_rupert() : CreatureScript("npc_captain_rupert") { }

        struct npc_captain_rupertAI : public npc_argent_captainAI
        {
            npc_captain_rupertAI(Creature* creature) : npc_argent_captainAI(creature)
            {
            }

            void Reset()
            {
                Events.Reset();
                Events.ScheduleEvent(EVENT_RUPERT_FEL_IRON_BOMB, urand(15000, 20000));
                Events.ScheduleEvent(EVENT_RUPERT_MACHINE_GUN, urand(25000, 30000));
                Events.ScheduleEvent(EVENT_RUPERT_ROCKET_LAUNCH, urand(10000, 15000));
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_RUPERT_FEL_IRON_BOMB:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                DoCast(target, SPELL_FEL_IRON_BOMB);
                            Events.ScheduleEvent(EVENT_RUPERT_FEL_IRON_BOMB, urand(15000, 20000));
                            break;
                        case EVENT_RUPERT_MACHINE_GUN:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                                DoCast(target, SPELL_MACHINE_GUN);
                            Events.ScheduleEvent(EVENT_RUPERT_MACHINE_GUN, urand(25000, 30000));
                            break;
                        case EVENT_RUPERT_ROCKET_LAUNCH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                                DoCast(target, SPELL_ROCKET_LAUNCH);
                            Events.ScheduleEvent(EVENT_RUPERT_ROCKET_LAUNCH, urand(10000, 15000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_captain_rupertAI>(creature);
        }
};

class npc_frostwing_vrykul : public CreatureScript
{
    public:
        npc_frostwing_vrykul() : CreatureScript("npc_frostwing_vrykul") { }

        struct npc_frostwing_vrykulAI : public SmartAI
        {
            npc_frostwing_vrykulAI(Creature* creature) : SmartAI(creature) { }

            bool CanAIAttack(Unit const* target) const
            {
                // do not see targets inside Frostwing Halls when we are not there
                return (me->GetPositionY() > 2660.0f) == (target->GetPositionY() > 2660.0f) && SmartAI::CanAIAttack(target);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_frostwing_vrykulAI(creature);
        }
};

class npc_impaling_spear : public CreatureScript
{
    public:
        npc_impaling_spear() : CreatureScript("npc_impaling_spear") { }

        struct npc_impaling_spearAI : public CreatureAI
        {
            npc_impaling_spearAI(Creature* creature) : CreatureAI(creature)
            {
            }

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                _vehicleCheckTimer = 500;
            }

            void UpdateAI(uint32 diff)
            {
                if (_vehicleCheckTimer <= diff)
                {
                    _vehicleCheckTimer = 500;
                    if (!me->GetVehicle())
                        me->DespawnOrUnsummon(100);
                }
                else
                    _vehicleCheckTimer -= diff;
            }

            uint32 _vehicleCheckTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_impaling_spearAI(creature);
        }
};

class npc_arthas_teleport_visual : public CreatureScript
{
    public:
        npc_arthas_teleport_visual() : CreatureScript("npc_arthas_teleport_visual") { }

        struct npc_arthas_teleport_visualAI : public NullCreatureAI
        {
            npc_arthas_teleport_visualAI(Creature* creature) : NullCreatureAI(creature), _instance(creature->GetInstanceScript())
            {
            }

            void Reset()
            {
                _events.Reset();
                if (_instance->GetBossState(DATA_PROFESSOR_PUTRICIDE) == DONE &&
                    _instance->GetBossState(DATA_BLOOD_QUEEN_LANA_THEL) == DONE &&
                    _instance->GetBossState(DATA_SINDRAGOSA) == DONE)
                    _events.ScheduleEvent(EVENT_SOUL_MISSILE, urand(1000, 6000));
            }

            void UpdateAI(uint32 diff)
            {
                if (_events.Empty())
                    return;

                _events.Update(diff);

                if (_events.ExecuteEvent() == EVENT_SOUL_MISSILE)
                {
                    DoCastAOE(SPELL_SOUL_MISSILE);
                    _events.ScheduleEvent(EVENT_SOUL_MISSILE, urand(5000, 7000));
                }
            }

        private:
            InstanceScript* _instance;
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            // Distance from the center of the spire
            if (creature->GetExactDist2d(4357.052f, 2769.421f) < 100.0f && creature->GetHomePosition().GetPositionZ() < 315.0f)
                return GetIcecrownCitadelAI<npc_arthas_teleport_visualAI>(creature);

            // Default to no script
            return NULL;
        }
};

class npc_lights_hammer_champions : public CreatureScript
{
    public:
        npc_lights_hammer_champions() : CreatureScript("npc_lights_hammer_champions") { }

        struct npc_lights_hammer_championsAI : public SmartAI
        {
            npc_lights_hammer_championsAI(Creature* creature) : SmartAI(creature) { }

            void UpdateAI(uint32 diff) override
            {
                // reset if npcs are in room 1 (otherwise the npcs in the startroom help the players to kill them)
                if (Unit* victim = me->GetVictim())
                    if (me->GetPositionX() < -142.08f && victim->GetPositionX() < -142.08f)
                        EnterEvadeMode();

                SmartAI::UpdateAI(diff);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_lights_hammer_championsAI(creature);
        }
};

class npc_icecrown_marrowgar_room : public CreatureScript
{
    public:
        npc_icecrown_marrowgar_room() : CreatureScript("npc_icecrown_marrowgar_room") { }

        struct npc_icecrown_marrowgar_roomAI : public SmartAI
        {
            npc_icecrown_marrowgar_roomAI(Creature* creature) : SmartAI(creature) { }

            void UpdateAI(uint32 diff) override
            {
                // reset if npcs are in room 1 (otherwise the npcs in the startroom help the players to kill them)
                if (Unit* victim = me->GetVictim())
                    if (me->GetPositionX() > -142.08f && victim->GetPositionX() > -142.08f)
                        EnterEvadeMode();

                SmartAI::UpdateAI(diff);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_icecrown_marrowgar_roomAI(creature);
        }
};

enum
{
    NPC_WEB_WRAP = 38028,
};

class npc_nerubar_broodkeeper : public CreatureScript
{
    public:
        npc_nerubar_broodkeeper() : CreatureScript("npc_nerubar_broodkeeper") { }

        struct npc_nerubar_broodkeeperAI : public SmartAI
        {
            npc_nerubar_broodkeeperAI(Creature* creature) : SmartAI(creature) 
            { 
                CastNet = false;
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void SetData(uint32 type, uint32 data) override
            {
                if (type == 1 && data == 1)
                {
                    me->GetMotionMaster()->MoveFall();
                    me->SetHover(false);
                    if (!CastNet)
                    {
                        CastNet = true;
                        me->RemoveAllAuras();
                        DoCastSelf(SPELL_CAST_NET_STARTROOM, true);
                    }
                }
            }

            void UpdateAI(uint32 diff) override
            {
                // reset if npcs are in room 1 (otherwise the npcs in the startroom help the players to kill them)
                if (Unit* victim = me->GetVictim())
                    if (me->GetPositionX() > -142.08f && victim->GetPositionX() > -142.08f)
                        EnterEvadeMode();

                SmartAI::UpdateAI(diff);
            }

            void SpellHitTarget(Unit* target, const SpellInfo* spellInfo) override
            {
                if (spellInfo->Id == SPELL_WEB_WRAP)
                    webTarget = target->GetGUID();
            }

            void JustSummoned(Creature* creature) override
            {
                if (creature->GetEntry() == NPC_WEB_WRAP)
                    if (auto sai = dynamic_cast<SmartAI*>(creature->AI()))
                        if (auto player = sObjectAccessor->GetPlayer(*me, webTarget))
                        {
                            auto targetList = new ObjectList{ player }; // needs to be created with new, gets automatically cleared up
                            sai->GetScript()->StoreTargetList(targetList, 0);
                        }
            }

        private:
            bool CastNet;
            ObjectGuid webTarget;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_nerubar_broodkeeperAI(creature);
        }
};

enum TheDamnedMisc
{
    // Spells
    SHATTERED_BONES       = 70963,

    // GUIDs
    GUID_THE_DAMNED_LEFT  = 200966,
    GUID_THE_DAMNED_RIGHT = 201066
};

class npc_the_damned : public CreatureScript
{
public:
    npc_the_damned() : CreatureScript("npc_the_damned") { }

    struct npc_the_damnedAI : public SmartAI
    {
        npc_the_damnedAI(Creature* creature) : SmartAI(creature) { }

        void DamageTaken(Unit* /*who*/, uint32& Damage) override
        {
            if (Damage > me->GetHealth())
                for (uint8 i = 0; i < 15; ++i) // we will cast this visual several times = better visual effect
                    DoCastSelf(SHATTERED_BONES, true);
        }

        void UpdateAI(uint32 diff) override
        {
            // reset if npcs are in room 1 (otherwise the npcs in the startroom help the players to kill them)
            if (me->GetSpawnId() && me->GetSpawnId() != GUID_THE_DAMNED_LEFT && me->GetSpawnId() != GUID_THE_DAMNED_RIGHT)
                if (Unit* victim = me->GetVictim())
                    if (me->GetPositionX() > -142.08f && victim->GetPositionX() > -142.08f)
                        EnterEvadeMode();

            SmartAI::UpdateAI(diff);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_the_damnedAI(creature);
    }
};

class npc_rampart_of_skulls_trash : public CreatureScript
{
    public:
        npc_rampart_of_skulls_trash() : CreatureScript("npc_rampart_of_skulls_trash") { }

        struct npc_rampart_of_skulls_trashAI : public SmartAI
        {
            npc_rampart_of_skulls_trashAI(Creature* creature) : SmartAI(creature) { }

            void Reset()
            {                
                ResetThread = false;
                me->setActive(true);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetInCombatWithZone();
                AttackStart(me->GetVictim());
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (attacker->GetTypeId() != TYPEID_PLAYER)
                {
                    damage = 0;
                    if (me->GetHealthPct() < 30.0f)
                        if (!ResetThread)
                            me->SetHealth(me->GetMaxHealth());
                }

                if (attacker->GetTypeId() == TYPEID_PLAYER)
                {                    
                    if (!ResetThread)
                    {
                        me->SetHealth(me->GetMaxHealth());
                        AttackStart(attacker);
                        ResetThread = true;
                    }                        
                }                
            }

            bool CanAIAttack(Unit const* target) const
            {
                // sometimes these npcs try to attack marrowgar - so let them reset
                return (me->GetPositionZ() < 143.541122f) == (target->GetPositionZ() < 143.541122f) && SmartAI::CanAIAttack(target);
            }
        private:
            bool ResetThread;
        };
        
        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_rampart_of_skulls_trashAI(creature);
        }
};

class npc_spider_attack : public CreatureScript
{
    public:
        npc_spider_attack() : CreatureScript("npc_spider_attack") { }

        struct npc_spider_attackAI : public CreatureAI
        {
            npc_spider_attackAI(Creature* creature) : CreatureAI(creature) { }

            void InitializeAI()
            {
                Summoned = false;
                
                if (me->GetEntry() == NPC_FROSTWARDEN_WARRIOR || me->GetEntry() == NPC_FROSTWARDEN_SORCERESS)
                {
                    AttackStart(me->FindNearestPlayer(100.0f));
                    me->SetInCombatWithZone();
                }

                if (me->GetEntry() == NPC_NERUBAR_CHAMPION || me->GetEntry() == NPC_NERUBAR_BROODLING)
                {
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    DoCastSelf(SPELL_CAST_NET);
                    me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), 211.03f);
                    _events.ScheduleEvent(EVENT_MOVE_GROUND, 10 * IN_MILLISECONDS);
                }
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                if (!_EnterEvadeMode())
                    return;

                if (InstanceScript* instance = me->GetInstanceScript())
                    instance->SetData(DATA_SPIDER_ATTACK, NOT_STARTED);                  

                std::list<Creature*> SpiderList;
                me->GetCreatureListWithEntryInGrid(SpiderList, NPC_NERUBAR_BROODLING, 500.0f);
                me->GetCreatureListWithEntryInGrid(SpiderList, NPC_NERUBAR_CHAMPION, 500.0f);
                me->GetCreatureListWithEntryInGrid(SpiderList, NPC_FROSTWARDEN_WARRIOR, 500.0f);
                me->GetCreatureListWithEntryInGrid(SpiderList, NPC_FROSTWARDEN_SORCERESS, 500.0f);
                if (!SpiderList.empty())
                    for (std::list<Creature*>::iterator itr = SpiderList.begin(); itr != SpiderList.end(); itr++)
                        (*itr)->DespawnOrUnsummon();
            }

            void EnterCombat(Unit* /*attacker*/)            
            {
                if (me->GetEntry() == NPC_NERUBAR_BROODLING)
                    Summoned = true; 

                if (me->GetEntry() == NPC_NERUBAR_CHAMPION)
                {
                    Summoned = true;
                    _events.ScheduleEvent(EVENT_RUSH, urand(3, 8) * IN_MILLISECONDS);
                }

                if (me->GetEntry() == NPC_FROSTWARDEN_WARRIOR)
                {
                    Summoned = true;
                    _events.ScheduleEvent(EVENT_GLACIAL_STRIKE, urand(3, 6) * IN_MILLISECONDS);
                }

                if (me->GetEntry() == NPC_FROSTWARDEN_SORCERESS)
                {
                    Summoned = true;
                    _events.ScheduleEvent(EVENT_FROST_NOVA, urand(5, 8) * IN_MILLISECONDS);
                    _events.ScheduleEvent(EVENT_ICE_TOMB,   urand(3, 9) * IN_MILLISECONDS);
                    _events.ScheduleEvent(EVENT_FROSTBOLT,  urand(2, 5) * IN_MILLISECONDS);
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() && Summoned)
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOVE_GROUND:
                            Summoned = true;
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            AttackStart(me->FindNearestPlayer(100.0f));
                            me->SetInCombatWithZone();
                            break;
                        case EVENT_RUSH:
                            DoCastSelf(SPELL_RUSH, true);
                            _events.ScheduleEvent(EVENT_RUSH, urand(6, 12) * IN_MILLISECONDS);
                            break;
                        case EVENT_GLACIAL_STRIKE:
                            DoCastVictim(SPELL_GLACIAL_STRIKE, true);
                            _events.ScheduleEvent(EVENT_GLACIAL_STRIKE, urand(6, 8) * IN_MILLISECONDS);
                            break;
                        case EVENT_FROST_NOVA:
                            DoCastVictim(SPELL_FROST_NOVA, true);
                            _events.ScheduleEvent(EVENT_FROST_NOVA, urand(6, 9) * IN_MILLISECONDS);
                            break;
                        case EVENT_ICE_TOMB:
                            DoCastVictim(SPELL_ICE_TOMB, true);
                            _events.ScheduleEvent(EVENT_ICE_TOMB, urand(12, 15) * IN_MILLISECONDS);
                            break;
                        case EVENT_FROSTBOLT:
                            DoCastVictim(SPELL_FROSTBOLT);
                            _events.ScheduleEvent(EVENT_FROSTBOLT, urand(5, 8) * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
    private:
        EventMap _events;
        bool Summoned;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_spider_attackAI(creature);
    }
};

// Deahtbringer Saurfang End-Event Horde
enum Sounds
{
    SOUND_PEON_WORK_COMPLETE = 6199
};

enum Spells_OUTRO
{
    SPELL_TELEPORT_ANIMATION = 12980,
    SPELL_RIDE_VEHICLE = 70640, // Outro
    SPELL_KNEEL = 68442,
};

struct EndEventSpawns
{
    uint32 entry;
    float x;
    float y;
    float z;
    float o;
};

static const EndEventSpawns EndEventGameObjectHorde[7] =
{
    { GO_HORDE_TELEPORTER,       -560.4184f, 2202.75f,  539.2853f, 0.0f },
    { GO_HORDE_TELEPORTER,       -560.2952f, 2220.215f, 539.2854f, 0.0f, },
    { GO_HORDE_BONFIRE,          -525.3403f, 2229.495f, 539.2918f, 0.0f },
    { GO_HORDE_BLACKSMITH_ANVIL, -517.6285f, 2243.767f, 539.2919f, 5.270896f },
    { GO_HORDE_TENT_1,           -532.8646f, 2229.009f, 539.2921f, 2.530723f },
    { GO_HORDE_TENT_2,           -524.5573f, 2238.092f, 539.292f,  0.1396245f },
    { GO_HORDE_ALLIANCE_FORGE,   -514.4479f, 2245.038f, 539.2912f, 0.0f },
};

static const EndEventSpawns EndEventCreatureHorde[4] =
{
    { NPC_WARSONG_PEON,      -560.4236f, 2220.384f, 539.3687f, 0.0f }, //Left Side
    { NPC_WARSONG_PEON,      -560.4948f, 2202.382f, 539.3687f, 0.0f }, //Right Side
    { NPC_CANDITH_THOMAS,    -560.4236f, 2220.384f, 539.3687f, 0.0f },
    { NPC_MORGAN_TAGESGLANZ, -560.4948f, 2202.382f, 539.3687f, 0.0f },

};

const Position EndEventPeonMove[4] =
{
    { -534.622f, 2222.237f, 539.291f }, //worker1 move
    { -529.503f, 2227.310f, 539.292f }, //worker1 move to tent
    { -541.117f, 2200.193f, 539.292f }, //worker2 move
    { -520.556f, 2233.691f, 539.292f }, //worker2 move to tent
};

const Position EndEventVendorMoveHorde[5] =
{
    { -537.254f, 2208.401f, 539.292f }, //Kandith move
    { -526.418f, 2215.328f, 539.292f }, //Kandith move
    { -530.170f, 2226.231f, 539.292f }, //Kandith move
    { -541.960f, 2219.084f, 539.292f }, //Morgan move
    { -520.941f, 2233.108f, 539.292f }, //Morgan move
};

// Deahtbringer Saurfang End-Event Alliance
static const EndEventSpawns EndEventGameObjectAlliance[7] =
{
    { GO_ALLIANCE_TELEPORTER,       -560.1215f, 2220.413f, 539.2851f, 0.0f },
    { GO_ALLIANCE_TELEPORTER,       -560.2795f, 2202.220f, 539.2851f, 0.0f, },
    { GO_ALLIANCE_TENT,             -532.5261f, 2229.651f, 539.2919f, 5.480334f },
    { GO_ALLIANCE_TENT,             -528.7483f, 2233.471f, 539.2919f, 5.462882f },
    { GO_ALLIANCE_BANNER,           -533.1441f, 2233.967f, 539.2922f, 5.462882f },
    { GO_ALLIANCE_BLACKSMITH_ANVIL, -524.6858f, 2235.885f, 539.2919f, 0.7853968f },
    { GO_HORDE_ALLIANCE_FORGE,      -526.5087f, 2237.542f, 539.2921f, 0.0f },
};

static const EndEventSpawns EndEventCreatureAlliance[8] =
{
    { NPC_STORMWIND_PORTAL,          -529.5121f, 2229.892f, 539.3734f, 0.0f },
    { NPC_ALLIANCE_MASON,            -560.4254f, 2202.019f, 539.3684f, 0.0f },
    { NPC_ALLIANCE_MASON,            -560.3924f, 2220.503f, 539.3683f, 0.0f },
    { NPC_SHELY_STEELBOWELS,         -560.4254f, 2202.019f, 539.3684f, 0.0f },
    { NPC_BRAZIE_GETZ,               -560.3924f, 2220.503f, 539.3683f, 0.0f },
    { NPC_SE_HIGH_OVERLORD_SAURFANG, -521.6962f, 2248.811f, 539.3757f, 4.983534f },
    { NPC_SE_KING_VARIAN_WRYNN,      -527.3386f, 2230.573f, 539.3734f, 5.951573f },
    { NPC_SE_JAINA_PROUDMOORE,       -527.9063f, 2228.637f, 539.373f, 6.265732f },

};

const Position AllianceMoveMisc[4] =
{
    { -523.222f, 2225.147f, 539.292f }, //Muradin Move
    { -524.207f, 2228.194f, 539.292f }, //Saurfang Move
    { -527.906f, 2226.637f, 539.373f, 6.265732f }, //Muradin Move
    { -523.804f, 2230.452f, 539.292f, 3.152798f }, //Saurfang Move

};

const Position EndEventMasonMove[6] =
{
    { -545.802f, 2200.810f, 539.291f },             //worker1 move
    { -525.378f, 2231.084f, 539.292f, 2.426875f },  //worker1 move to tent
    { -538.747f, 2222.292f, 539.291f },             //worker2 move
    { -529.427f, 2227.575f, 539.292f, 2.438657f },  //worker2 move to tent
    { -547.406f, 2207.209f, 539.292f },             //worker1 move home
    { -544.787f, 2200.456f, 539.292f },             //worker2 move home
};

const Position EndEventVendorMoveAlliance[8] =
{
    { -541.960f, 2219.084f, 539.292f },            //Brazie move
    { -531.291f, 2225.389f, 539.292f },            //Brazie move
    { -528.227f, 2226.698f, 539.292f },            //Brazie move
    { -531.138f, 2228.238f, 539.292f, 5.605255f }, //Brazie move final
    { -553.529f, 2201.426f, 539.288f },            //Shely move
    { -529.951f, 2222.520f, 539.291f },            //Shely move
    { -524.940f, 2230.640f, 539.292f },            //Shely move
    { -527.273f, 2232.358f, 539.292f, 5.636658f }, //Shely move final
};

enum ScriptTexts
{
    // High Overlord Saurfang
    SAY_INTRO_HORDE_1       = 0,
    SAY_INTRO_HORDE_3       = 1,
    SAY_INTRO_HORDE_5       = 2,
    SAY_INTRO_HORDE_6       = 3,
    SAY_INTRO_HORDE_7       = 4,
    SAY_INTRO_HORDE_8       = 5,
    SAY_OUTRO_ALLIANCE_8    = 6, //End Event Saurfang - Alliance
    SAY_OUTRO_ALLIANCE_12   = 7, 
    SAY_OUTRO_ALLIANCE_13   = 8,
    SAY_OUTRO_ALLIANCE_14   = 9,
    SAY_OUTRO_ALLIANCE_15   = 10, //End Event Saurfang - Alliance
    SAY_OUTRO_HORDE_1       = 11,
    SAY_OUTRO_HORDE_2       = 12,
    SAY_OUTRO_HORDE_3       = 13,
    SAY_OUTRO_HORDE_4       = 14,

    // Muradin Bronzebeard
    SAY_INTRO_ALLIANCE_1    = 0,
    SAY_INTRO_ALLIANCE_4    = 1,
    SAY_INTRO_ALLIANCE_5    = 2,
    SAY_OUTRO_ALLIANCE_1    = 3, 
    SAY_OUTRO_ALLIANCE_2    = 4, //End Event Saurfang - Alliance
    SAY_OUTRO_ALLIANCE_3    = 5,
    SAY_OUTRO_ALLIANCE_4    = 6,
    SAY_OUTRO_ALLIANCE_5    = 7,
    SAY_OUTRO_ALLIANCE_6    = 8,
    SAY_OUTRO_ALLIANCE_7    = 9,
    SAY_OUTRO_ALLIANCE_9    = 10,
    SAY_OUTRO_ALLIANCE_10   = 11,
    SAY_OUTRO_ALLIANCE_21   = 12, //End Event Saurfang - Alliance

    // Lady Jaina Proudmoore
    SAY_OUTRO_ALLIANCE_17   = 0,
    SAY_OUTRO_ALLIANCE_19   = 1,

    // King Varian Wrynn
    SAY_OUTRO_ALLIANCE_11   = 0,
    SAY_OUTRO_ALLIANCE_16   = 1,
    SAY_OUTRO_ALLIANCE_18   = 2,
    SAY_OUTRO_ALLIANCE_20   = 3,
};

enum MovePoints
{
    POINT_CORPSE = 3781304,
};

Position const SaurfangSpawnZeppelin = { -1.332f, -3.258f, -17.8532f, 1.576761f };

class npc_martyr_stalker_igb_saurfang : public CreatureScript
{
public:
    npc_martyr_stalker_igb_saurfang() : CreatureScript("npc_martyr_stalker_igb_saurfang") { }

    struct npc_martyr_stalker_igb_saurfangAI : public ScriptedAI
    {
        npc_martyr_stalker_igb_saurfangAI(Creature* creature) : ScriptedAI(creature){ }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_START_HORDE_EVENT:
                    Step = 0;
                    JumpToNextStep(50*IN_MILLISECONDS);
                    break;
                case ACTION_START_ALLIANCE_EVENT:
                    if (MotionTransport* zeppelin = CreateTransport())
                    {
                        zeppelin->EnableMovement(true);
                        zeppelin->setActive(true);
                    }
                    Step = 11;
                    JumpToNextStep(2000);
                    break;
                default:
                    break;
            }
        }

        void JumpToNextStep(uint32 Timer)
        {
            PhaseTimer = Timer;
            ++Step;
        }

        MotionTransport* CreateTransport()
        {
            MotionTransport* zeppelin = sTransportMgr->CreateTransport(GO_ALLIANCE_ZEPPELIN_TRANSPORT, 0, me->GetMap());
            zeppelin_guid = zeppelin->GetGUID();
            if (Creature* saurfang = zeppelin->SummonPassenger(NPC_SE_HIGH_OVERLORD_SAURFANG, SaurfangSpawnZeppelin, TEMPSUMMON_MANUAL_DESPAWN))
                saurfang->UpdateEntry(NPC_SE_HIGH_OVERLORD_SAURFANG, saurfang->GetCreatureData());

            return zeppelin;
        }

        void UpdateAI(uint32 diff)
        {
            if (PhaseTimer <= diff)
            {
                switch (Step)
                {
                    // Activate Horde Teleporter + Animation
                case 1:
                    for (uint8 i = 0; i < 2; ++i)
                        if (GameObject* teleporter = me->SummonGameObject(EndEventGameObjectHorde[i].entry, EndEventGameObjectHorde[i].x, EndEventGameObjectHorde[i].y, EndEventGameObjectHorde[i].z, EndEventGameObjectHorde[i].o, 0.f, 0.f, 0.f, 0.f, DAY))
                            teleporter->SetGoState(GO_STATE_ACTIVE);
                    JumpToNextStep(7000);
                    break; 
                    // Spawn two peons and move to the first point
                case 2:
                    if (Creature* peon = me->SummonCreature(EndEventCreatureHorde[0].entry, EndEventCreatureHorde[0].x, EndEventCreatureHorde[0].y, EndEventCreatureHorde[0].z, EndEventCreatureHorde[0].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                    {
                        worker1 = peon->GetGUID();
                        peon->SetWalk(false);
                        peon->GetMotionMaster()->MovePoint(0, EndEventPeonMove[0]);
                    }
                    if (Creature* peon = me->SummonCreature(EndEventCreatureHorde[1].entry, EndEventCreatureHorde[1].x, EndEventCreatureHorde[1].y, EndEventCreatureHorde[1].z, EndEventCreatureHorde[1].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                    {
                        worker2 = peon->GetGUID();
                        peon->SetWalk(false);
                        peon->GetMotionMaster()->MovePoint(0, EndEventPeonMove[2]);
                    }
                    JumpToNextStep(3500);
                    break;
                    // Move to the next point
                case 3:
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker1))
                        peon->GetMotionMaster()->MovePoint(0, EndEventPeonMove[1]);
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker2))
                        peon->GetMotionMaster()->MovePoint(0, EndEventPeonMove[3]);
                    JumpToNextStep(3500);
                    break;
                    // Activate mining emote
                case 4:
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker1))
                        peon->HandleEmoteCommand(EMOTE_STATE_WORK_MINING);
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker2))
                        peon->HandleEmoteCommand(EMOTE_STATE_WORK_MINING);
                    JumpToNextStep(5000);
                    break;
                    // Activate funny peon complete sound
                case 5:
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker1))
                        peon->PlayDistanceSound(SOUND_PEON_WORK_COMPLETE);
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker2))
                        peon->PlayDistanceSound(SOUND_PEON_WORK_COMPLETE);
                    JumpToNextStep(2500);
                    break;
                    // Move first home position and spawn the horde tents and fire
                case 6:
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker1))
                        peon->GetMotionMaster()->MovePoint(0, EndEventPeonMove[0]);
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker2))
                        peon->GetMotionMaster()->MovePoint(0, EndEventPeonMove[2]);
                    for (uint8 i = 2; i < 7; ++i)
                       me->SummonGameObject(EndEventGameObjectHorde[i].entry, EndEventGameObjectHorde[i].x, EndEventGameObjectHorde[i].y, EndEventGameObjectHorde[i].z, EndEventGameObjectHorde[i].o, 0.f, 0.f, 0.f, 0.f, DAY);
                    JumpToNextStep(3000);
                    break;
                    // Move target through the horde teleporter
                case 7:
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker1))
                        peon->GetMotionMaster()->MoveTargetedHome();
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker2))
                        peon->GetMotionMaster()->MoveTargetedHome();
                    JumpToNextStep(3000);
                    break;
                    // Despawn peons
                case 8:
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker1))
                        peon->DespawnOrUnsummon();
                    if (Creature* peon = ObjectAccessor::GetCreature(*me, worker2))
                        peon->DespawnOrUnsummon();
                    JumpToNextStep(2500);
                    break;
                case 9:
                    // Spawn vendors
                    if (Creature* candith = me->SummonCreature(EndEventCreatureHorde[2].entry, EndEventCreatureHorde[2].x, EndEventCreatureHorde[2].y, EndEventCreatureHorde[2].z, EndEventCreatureHorde[2].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                    {
                        worker1 = candith->GetGUID();
                        candith->GetMotionMaster()->MovePoint(0, EndEventVendorMoveHorde[0]);
                    }
                    if (Creature* morgan = me->SummonCreature(EndEventCreatureHorde[3].entry, EndEventCreatureHorde[3].x, EndEventCreatureHorde[3].y, EndEventCreatureHorde[3].z, EndEventCreatureHorde[3].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                    {
                        worker2 = morgan->GetGUID();
                        morgan->GetMotionMaster()->MovePoint(0, EndEventVendorMoveHorde[3]);
                    }
                    JumpToNextStep(4000);
                    break;
                    // Move vendors
                case 10:
                    if (Creature* candith = ObjectAccessor::GetCreature(*me, worker1))
                        candith->GetMotionMaster()->MovePoint(0, EndEventVendorMoveHorde[1]);
                    if (Creature* morgan = ObjectAccessor::GetCreature(*me, worker2))
                        morgan->GetMotionMaster()->MovePoint(0, EndEventVendorMoveHorde[4]);
                    JumpToNextStep(3500);
                    break;
                    // Candith need one more move
                case 11:
                    if (Creature* candith = ObjectAccessor::GetCreature(*me, worker1))
                        candith->GetMotionMaster()->MovePoint(0, EndEventVendorMoveHorde[2]);
                    break;
                //Alliance EndEvent Start
                    // Create Creaturelist Skybreaker Marine and start muradin event talk
                case 12:
                {
                    std::list<Creature*> guardList;
                    me->GetCreatureListWithEntryInGrid(guardList, NPC_SE_SKYBREAKER_MARINE, 100.0f);
                    for (std::list<Creature*>::iterator itr = guardList.begin(); itr != guardList.end(); ++itr)
                    {
                        guard_guids.push_back((*itr)->GetGUID());
                        (*itr)->AI()->DoCast(SPELL_KNEEL);
                    }
                    if (Creature* muradin = me->FindNearestCreature(NPC_SE_MURADIN_BRONZEBEARD, 50.0f, true))
                        muradin_mob = muradin->GetGUID();
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                        muradin->AI()->Talk(SAY_OUTRO_ALLIANCE_2);
                    JumpToNextStep(15000);
                    break;
                }
                    // Let muradin move to his first position
                case 13:
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                    {
                        muradin->SetCanFly(false);
                        muradin->AI()->Talk(SAY_OUTRO_ALLIANCE_3);
                        muradin->SetWalk(false);
                        muradin->GetMotionMaster()->MovePoint(0, AllianceMoveMisc[0]);
                        muradin->AI()->Talk(SAY_OUTRO_ALLIANCE_4);
                    }
                    JumpToNextStep(12000);
                    break;
                    // Set emote state to ready with his main hand and add this to the creature list
                case 14:
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                    {
                        muradin->AI()->Talk(SAY_OUTRO_ALLIANCE_5);
                        muradin->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY1H);
                        float i = 0;
                        for (ObjectGuid guid : guard_guids)
                        {
                            i += 0.75f;
                            if (Creature* guard = sObjectAccessor->GetCreature(*me, guid))
                            {
                                guard->RemoveAllAuras();
                                guard->GetMotionMaster()->MoveFollow(muradin, 2, i);
                                guard->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY1H);
                            }
                        }
                    }
                    JumpToNextStep(17000);
                    break;
                    // stop moving transport
                    // Lets spawn saurfang and despawn saurfang on the ship
                case 15:
                    if (MotionTransport* zeppelin = ObjectAccessor::GetMotionTransport(*me, zeppelin_guid))
                        zeppelin->EnableMovement(false);
                    for (ObjectGuid guid : guard_guids)
                        if (Creature* guard = sObjectAccessor->GetCreature(*me, guid))
                            guard->GetMotionMaster()->Clear();
                    if (Creature* saurfang = me->FindNearestCreature(NPC_SE_HIGH_OVERLORD_SAURFANG, 100.0f))
                        saurfang_mob = saurfang->GetGUID();       
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                    {
                        saurfang->SetHomePosition(EndEventCreatureAlliance[5].x, EndEventCreatureAlliance[5].y, EndEventCreatureAlliance[5].z, EndEventCreatureAlliance[5].o);
                        saurfang->DespawnOrUnsummon();
                        if (Creature* saurfang = me->SummonCreature(EndEventCreatureAlliance[5].entry, EndEventCreatureAlliance[5].x, EndEventCreatureAlliance[5].y, EndEventCreatureAlliance[5].z, EndEventCreatureAlliance[5].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                        {      
                            saurfang_mob = saurfang->GetGUID();
                            saurfang->UpdateEntry(NPC_SE_HIGH_OVERLORD_SAURFANG, saurfang->GetCreatureData());
                            saurfang->SetWalk(true);
                        }
                    }
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                        muradin->AI()->Talk(SAY_OUTRO_ALLIANCE_6);
                    JumpToNextStep(7500);
                    break;
                    // Let saurfang move to muradin
                case 16:
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                        saurfang->GetMotionMaster()->MovePoint(0, AllianceMoveMisc[1]);
                    JumpToNextStep(13000);
                    break;
                    // Muradin facing saurfang and talk to him
                case 17:
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                    {
                        muradin->AI()->Talk(SAY_OUTRO_ALLIANCE_7);
                        if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                            muradin->SetFacingToObject(saurfang);
                    }
                    for (ObjectGuid guid : guard_guids)
                        if (Creature* guard = sObjectAccessor->GetCreature(*me, guid))
                            if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                                guard->SetFacingToObject(saurfang);
                    JumpToNextStep(11000);
                    break;
                    // Saurfang target muradin and talk to him. No SetTarget -> saurfang show to the wrong side
                case 18:
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                    {
                        if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                        {
                            saurfang->SetTarget(muradin->GetGUID());
                            saurfang->SetTarget(ObjectGuid::Empty);
                            saurfang->SetFacingToObject(muradin);
                            saurfang->AI()->Talk(SAY_OUTRO_ALLIANCE_8);
                            saurfang->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY2H);
                        }
                    }
                    JumpToNextStep(10000);
                    break;
                    // Muradin talk and SetTarget to nobody by saurfang
                case 19:
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                        saurfang->SetTarget(ObjectGuid::Empty);
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                        muradin->AI()->Talk(SAY_OUTRO_ALLIANCE_9);
                    JumpToNextStep(15000);
                    break;
                    // Spawn stromwind portal
                case 20:
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                        muradin->AI()->Talk(SAY_OUTRO_ALLIANCE_10);
                    if (Creature* portal = me->SummonCreature(EndEventCreatureAlliance[0].entry, EndEventCreatureAlliance[0].x, EndEventCreatureAlliance[0].y, EndEventCreatureAlliance[0].z, EndEventCreatureAlliance[0].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                    {
                        portal->SetFlag(UNIT_NPC_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        portal->CastSpell(portal, SPELL_TELEPORT_ANIMATION, true);
                        portal->DespawnOrUnsummon(8000);
                    }
                    JumpToNextStep(2500);
                    break;
                    // Spawn varian and jaina
                case 21:
                    if (Creature* varian = me->SummonCreature(EndEventCreatureAlliance[6].entry, EndEventCreatureAlliance[6].x, EndEventCreatureAlliance[6].y, EndEventCreatureAlliance[6].z, EndEventCreatureAlliance[6].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                        varian->CastSpell(varian, SPELL_TELEPORT_ANIMATION, true);
                    if (Creature* jaina = me->SummonCreature(EndEventCreatureAlliance[7].entry, EndEventCreatureAlliance[7].x, EndEventCreatureAlliance[7].y, EndEventCreatureAlliance[7].z, EndEventCreatureAlliance[7].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                        jaina->CastSpell(jaina, SPELL_TELEPORT_ANIMATION, true);
                    JumpToNextStep(1000);
                    break;
                    // Varian talk
                case 22:
                    if (Creature* varian = me->FindNearestCreature(NPC_SE_KING_VARIAN_WRYNN, 100.0f))
                        varian->AI()->Talk(SAY_OUTRO_ALLIANCE_11);
                    JumpToNextStep(8000);
                    break;
                    // Muradin and his guards move
                case 23:
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                    {
                        muradin->GetMotionMaster()->MovePoint(0, AllianceMoveMisc[2]);
                        float i = 0;
                        for (ObjectGuid guid : guard_guids)
                        {
                            i += 0.3f;
                            if (Creature* guard = sObjectAccessor->GetCreature(*me, guid))
                            {
                                guard->GetMotionMaster()->MoveFollow(muradin, 2, i);
                                guard->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY1H);
                                guard->SetFacingToObject(muradin);
                            }
                        }
                        muradin->SetFacingTo(6.14f);
                    }
                    JumpToNextStep(1000);
                    break;
                    // Saurfang move to deathbringer
                case 24:
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                    {
                        saurfang->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STAND_STATE_NONE);
                        if (Creature* deathbringer = ObjectAccessor::GetCreature(*me, ObjectGuid(saurfang->GetInstanceScript()->GetGuidData(DATA_DEATHBRINGER_SAURFANG))))
                        {
                            float x, y, z;
                            deathbringer->GetClosePoint(x, y, z, deathbringer->GetObjectSize());

                            saurfang->SetWalk(true);
                            saurfang->GetMotionMaster()->MovePoint(POINT_CORPSE, x, y, z);
                            JumpToNextStep(me->GetExactDist(deathbringer) * 300);
                        }
                    }
                    break;
                    // Add Movement for the ship
                case 25:   
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                    {
                        saurfang->AI()->Talk(SAY_OUTRO_ALLIANCE_12);
                        saurfang->AI()->Talk(SAY_OUTRO_ALLIANCE_13);
                        saurfang->AI()->DoCast(SPELL_KNEEL);

                    }
                    JumpToNextStep(10000);
                    break;
                    // saurfang bring the deathbringer to the graveyard and say something to the alliance
                case 26:
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                    {
                        if (Creature* deathbringer = ObjectAccessor::GetCreature(*me, ObjectGuid(saurfang->GetInstanceScript()->GetGuidData(DATA_DEATHBRINGER_SAURFANG))))
                        {
                            deathbringer->CastSpell(saurfang, SPELL_RIDE_VEHICLE, true);  // for the packet logs.
                            deathbringer->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            deathbringer->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_DROWNED);
                        }
                        if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                        {
                            saurfang->RemoveAura(SPELL_KNEEL);
                            saurfang->AI()->Talk(SAY_OUTRO_ALLIANCE_14);
                            saurfang->SetWalk(true);
                            saurfang->GetMotionMaster()->MovePoint(0, AllianceMoveMisc[3]);
                        }
                    }
                    JumpToNextStep(12000);
                    break;
                    // Varian talk to Saurfang
                case 27:
                    if (MotionTransport* zeppelin = ObjectAccessor::GetMotionTransport(*me, zeppelin_guid))
                        zeppelin->EnableMovement(true);
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                    {
                        if (Creature* varian = me->FindNearestCreature(NPC_SE_KING_VARIAN_WRYNN, 100.0f))
                            saurfang->SetFacingToObject(varian);
                    }
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                        saurfang->AI()->Talk(SAY_OUTRO_ALLIANCE_15);
                    JumpToNextStep(9000);
                    break;
                    // Varian talk to Muradin
                case 28:
                    if (Creature* varian = me->FindNearestCreature(NPC_SE_KING_VARIAN_WRYNN, 100.0f))
                        varian->AI()->Talk(SAY_OUTRO_ALLIANCE_16);
                    JumpToNextStep(17000);
                    break;
                    // Saurfang do nod
                case 29:
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                        saurfang->SetUInt32Value(UNIT_NPC_EMOTESTATE, TEXT_EMOTE_NOD);
                    JumpToNextStep(2500);
                    break;
                    // Saurfang move home
                case 30:
                    if (Creature* saurfang = ObjectAccessor::GetCreature(*me, saurfang_mob))
                    {
                        saurfang->SetWalk(true);
                        saurfang->GetMotionMaster()->MoveTargetedHome();
                        saurfang->DespawnOrUnsummon(3000);

                        if (Creature* deathbringer = ObjectAccessor::GetCreature(*me, ObjectGuid(saurfang->GetInstanceScript()->GetGuidData(DATA_DEATHBRINGER_SAURFANG))))
                            deathbringer->DespawnOrUnsummon(3000);
                    }
                    JumpToNextStep(3000);
                    break;
                    // Let jaina cry and talk by varian
                case 31:
                    // Clearn transport before delete 
                    if (MotionTransport* zeppelin = ObjectAccessor::GetMotionTransport(*me, zeppelin_guid))
                    {
                        zeppelin->EnableMovement(false);
                        zeppelin->setActive(false);
                    }
                    if (Creature* jaina = me->FindNearestCreature(NPC_SE_JAINA_PROUDMOORE, 100.0f))
                    {
                        jaina->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_CRY_JAINA);
                        jaina->AI()->Talk(SAY_OUTRO_ALLIANCE_17);
                        if (Creature* varian = me->FindNearestCreature(NPC_SE_KING_VARIAN_WRYNN, 100.0f))
                        {
                            varian->SetFacingToObject(jaina);
                            varian->AI()->Talk(SAY_OUTRO_ALLIANCE_18);
                        }
                    }
                    JumpToNextStep(11500);
                    break;
                    // talk by jaina
                case 32:
                    if (Creature* jaina = me->FindNearestCreature(NPC_SE_JAINA_PROUDMOORE, 100.0f))
                    {
                        jaina->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STAND_STATE_NONE);
                        jaina->AI()->Talk(SAY_OUTRO_ALLIANCE_19);
                    }
                    JumpToNextStep(10000);
                    break;
                    // varian talk to muradin
                case 33:
                    if (Creature* varian = me->FindNearestCreature(NPC_SE_KING_VARIAN_WRYNN, 100.0f))
                    {
                        if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                            muradin->SetFacingToObject(varian);
                        varian->AI()->Talk(SAY_OUTRO_ALLIANCE_20);
                    }
                    JumpToNextStep(14000);
                    break;
                    // muradin salute and the guards move home
                case 34:
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                    {
                        muradin->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_SALUTE);
                        muradin->AI()->Talk(SAY_OUTRO_ALLIANCE_21);
                    }
                    for (ObjectGuid guid : guard_guids)
                    {
                        if (Creature* guard = sObjectAccessor->GetCreature(*me, guid))
                        {
                            guard->GetMotionMaster()->MoveTargetedHome();
                            guard->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                        }
                    }

                    JumpToNextStep(4500);
                    break;
                    // varian and jaina despawn. muradin move home
                case 35:
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                        muradin->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                    if (Creature* varian = me->FindNearestCreature(NPC_SE_KING_VARIAN_WRYNN, 100.0f))
                    {
                        varian->CastSpell(varian, SPELL_TELEPORT_ANIMATION, true);
                        varian->DespawnOrUnsummon(1000);
                    }
                    if (Creature* jaina = me->FindNearestCreature(NPC_SE_JAINA_PROUDMOORE, 100.0f))
                    {
                        jaina->CastSpell(jaina, SPELL_TELEPORT_ANIMATION, true);
                        jaina->DespawnOrUnsummon(1000);
                    }
                    if (Creature* muradin = ObjectAccessor::GetCreature(*me, muradin_mob))
                    {
                        muradin->GetMotionMaster()->MoveTargetedHome();
                        muradin->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STAND_STATE_NONE);
                    }
                    JumpToNextStep(7000);
                    break;
                    // Spawn alliance teleporter
                case 36:
                    for (uint8 i = 0; i < 2; ++i)
                        if(GameObject* teleporter = me->SummonGameObject(EndEventGameObjectAlliance[i].entry, EndEventGameObjectAlliance[i].x, EndEventGameObjectAlliance[i].y, EndEventGameObjectAlliance[i].z, EndEventGameObjectAlliance[i].o, 0.f, 0.f, 0.f, 0.f, DAY))
                            teleporter->SetGoState(GO_STATE_ACTIVE);
                    JumpToNextStep(8000);
                    break;
                case 37:
                    // spawn masons
                    if (Creature* mason = me->SummonCreature(EndEventCreatureAlliance[1].entry, EndEventCreatureAlliance[1].x, EndEventCreatureAlliance[1].y, EndEventCreatureAlliance[1].z, EndEventCreatureAlliance[1].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                    {
                        worker1 = mason->GetGUID();
                        mason->AI()->DoCast(SPELL_TELEPORT_ANIMATION);
                        mason->SetWalk(false);
                        mason->GetMotionMaster()->MovePoint(0, EndEventMasonMove[0]);
                    }
                    if (Creature* mason = me->SummonCreature(EndEventCreatureAlliance[2].entry, EndEventCreatureAlliance[2].x, EndEventCreatureAlliance[2].y, EndEventCreatureAlliance[2].z, EndEventCreatureAlliance[2].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                    {
                        worker2 = mason->GetGUID();
                        mason->AI()->DoCast(SPELL_TELEPORT_ANIMATION);
                        mason->SetWalk(false);
                        mason->GetMotionMaster()->MovePoint(0, EndEventMasonMove[2]);
                    }
                    JumpToNextStep(1000);
                    break;
                    //masons move
                case 38:
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker1))
                        mason->GetMotionMaster()->MovePoint(0, EndEventMasonMove[1]);
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker2))
                        mason->GetMotionMaster()->MovePoint(0, EndEventMasonMove[3]);
                    JumpToNextStep(3000);
                    break;
                    //add emote for working
                case 39:
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker1))
                        mason->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_WORK_MINING);
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker2))
                        mason->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_WORK_MINING);
                    JumpToNextStep(13000);
                    break;
                    // move home and spawn the tents
                case 40:
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker1))
                    {
                        mason->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                        mason->GetMotionMaster()->MovePoint(0, EndEventMasonMove[4]);
                    }
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker2))
                    {
                        mason->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                        mason->GetMotionMaster()->MovePoint(0, EndEventMasonMove[5]);
                    }
                    for (uint8 i = 2; i < 7; ++i)
                        me->SummonGameObject(EndEventGameObjectAlliance[i].entry, EndEventGameObjectAlliance[i].x, EndEventGameObjectAlliance[i].y, EndEventGameObjectAlliance[i].z, EndEventGameObjectAlliance[i].o, 0.f, 0.f, 0.f, 0.f, DAY);
                    JumpToNextStep(4000);
                    break;
                    //masons move home
                case 41:
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker1))
                        mason->GetMotionMaster()->MoveTargetedHome();
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker2))
                        mason->GetMotionMaster()->MoveTargetedHome();
                    JumpToNextStep(5000);
                    break;
                    //masons despawn and spawn brazie & shely
                case 42:
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker1))
                        mason->DespawnOrUnsummon();
                    if (Creature* mason = ObjectAccessor::GetCreature(*me, worker2))
                        mason->DespawnOrUnsummon();
                    if (Creature* brazie = me->SummonCreature(EndEventCreatureAlliance[4].entry, EndEventCreatureAlliance[4].x, EndEventCreatureAlliance[4].y, EndEventCreatureAlliance[4].z, EndEventCreatureAlliance[4].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                    {
                        brazie->SetWalk(true);
                        brazie->SetHomePosition(EndEventVendorMoveAlliance[3]);
                        brazie->AI()->DoCast(SPELL_TELEPORT_ANIMATION);
                        brazie->GetMotionMaster()->MovePoint(0, EndEventVendorMoveAlliance[0]);
                    }
                    if (Creature* shely = me->SummonCreature(EndEventCreatureAlliance[3].entry, EndEventCreatureAlliance[3].x, EndEventCreatureAlliance[3].y, EndEventCreatureAlliance[3].z, EndEventCreatureAlliance[3].o, TEMPSUMMON_MANUAL_DESPAWN, DAY))
                    {
                        shely->SetWalk(true);
                        shely->SetHomePosition(EndEventVendorMoveAlliance[7]);
                        shely->AI()->DoCast(SPELL_TELEPORT_ANIMATION);
                        shely->GetMotionMaster()->MovePoint(0, EndEventVendorMoveAlliance[4]);
                    }
                    JumpToNextStep(1000);
                    break;
                    //lets them move
                case 43:
                    if (Creature* brazie = me->FindNearestCreature(NPC_BRAZIE_GETZ, 100.0f))
                    {
                        brazie->GetMotionMaster()->MovePoint(0, EndEventVendorMoveAlliance[1]);
                        brazie->SetWalk(true);
                    }
                    if (Creature* shely = me->FindNearestCreature(NPC_SHELY_STEELBOWELS, 100.0f))
                    {
                        shely->GetMotionMaster()->MovePoint(0, EndEventVendorMoveAlliance[5]);
                        shely->SetWalk(true);
                    }
                    JumpToNextStep(1000);
                    break;
                    //need more move
                case 44:
                    if (Creature* brazie = me->FindNearestCreature(NPC_BRAZIE_GETZ, 100.0f))
                    {
                        brazie->GetMotionMaster()->MovePoint(0, EndEventVendorMoveAlliance[2]);
                        brazie->SetWalk(true);
                    }
                    if (Creature* shely = me->FindNearestCreature(NPC_SHELY_STEELBOWELS, 100.0f))
                    {
                        shely->GetMotionMaster()->MovePoint(0, EndEventVendorMoveAlliance[6]);
                        shely->SetWalk(true);
                    }
                    JumpToNextStep(1000);
                    break;
                    //and move to the targetlocation through the tents
                case 45:
                    if (Creature* brazie = me->FindNearestCreature(NPC_BRAZIE_GETZ, 100.0f))
                    {
                        brazie->GetMotionMaster()->MoveTargetedHome();
                        brazie->SetWalk(true);
                    }
                    if (Creature* shely = me->FindNearestCreature(NPC_SHELY_STEELBOWELS, 100.0f))
                    {
                        shely->GetMotionMaster()->MoveTargetedHome();
                        shely->SetWalk(true);
                    }
                    break;
                default:
                    break;
                }
            }
            else
                PhaseTimer -= diff;
        }

    private:
        ObjectGuid zeppelin_guid = ObjectGuid::Empty;
        std::list<ObjectGuid> guard_guids;
        uint32 PhaseTimer;
        uint32 Step;
        ObjectGuid worker1;
        ObjectGuid worker2;
        ObjectGuid muradin_mob;
        ObjectGuid saurfang_mob;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetIcecrownCitadelAI<npc_martyr_stalker_igb_saurfangAI>(creature);
    }
};

class npc_decaying_colossus : public CreatureScript
{
    public:
        npc_decaying_colossus() : CreatureScript("npc_decaying_colossus") { }

        struct npc_decaying_colossusAI : public SmartAI
        {
            npc_decaying_colossusAI(Creature* creature) : SmartAI(creature)
            {
                me->ApplySpellImmune(0, IMMUNITY_ID, 9484, true);  // Shackle Undead
                me->ApplySpellImmune(0, IMMUNITY_ID, 9485, true);  // Shackle Undead
                me->ApplySpellImmune(0, IMMUNITY_ID, 10955, true); // Shackle Undead
                me->ApplySpellImmune(0, IMMUNITY_ID, 10326, true); // Turn Evil
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_decaying_colossusAI(creature);

        }
};

class npc_icecrown_val_kyr_herald : public CreatureScript
{
    public:
        npc_icecrown_val_kyr_herald() : CreatureScript("npc_icecrown_val_kyr_herald") { }

        struct npc_icecrown_val_kyr_heraldAI : public ScriptedAI
        {
            npc_icecrown_val_kyr_heraldAI(Creature* creature) : ScriptedAI(creature), summons(me) { }
            
            void Reset() override
            { 
                events.Reset(); 
                summons.DespawnAll(); 
            }

            void EnterCombat(Unit* /*who*/) override
            {
                events.Reset();
                summons.DespawnAll();
                me->setActive(true);
                events.ScheduleEvent(EVENT_SEVERED_ESSENCE, 8 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_REMOVE_HOVER, 5 * IN_MILLISECONDS);
                me->SetInCombatWithZone();
            }

            void JustReachedHome() override
            {
                me->setActive(false);
            }

            void JustSummoned(Creature* summon) override
            {
                summons.Summon(summon);
            }

            void MoveInLineOfSight(Unit* who) override
            {
                if (me->IsAlive() && !me->IsInCombat() && who->GetTypeId() == TYPEID_PLAYER && who->GetExactDist2d(me) < 35.0f)
                    AttackStart(who);
            }

            void SummonedCreatureDespawn(Creature* s) override
            {
                summons.Despawn(s);
            }

            void JustDied(Unit *killer) override
            {
                summons.DespawnAll();
            }

            bool CanAIAttack(Unit const* target) const override
            {
                return target->GetExactDist(4357.0f, 2769.0f, 356.0f) < 170.0f;
            }

            void SpellHitTarget(Unit* target, const SpellInfo* spell) override
            {
                // Not good target or too many players
                if (target->GetTypeId() != TYPEID_PLAYER || !target->IsAlive())
                    return;

                if (spell->Id == SPELL_SEVERED_ESSENCE_10 || spell->Id == SPELL_SEVERED_ESSENCE_25)
                {
                    if (Creature* creature = me->SummonCreature(NPC_SEVERED_ESSENCE, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000))
                    {
                        creature->AI()->AttackStart(target);
                        DoZoneInCombat(creature);
                    }
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                switch (events.ExecuteEvent())
                {
                    case EVENT_SEVERED_ESSENCE:
                    {
                        uint8 count = me->GetMap()->Is25ManRaid() ? 2 : 1;
                        for (uint8 i = 0; i < count; ++i)
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 37.5f, true))
                                me->CastSpell(target, RAID_MODE<uint32>(SPELL_SEVERED_ESSENCE_10, SPELL_SEVERED_ESSENCE_25, SPELL_SEVERED_ESSENCE_10, SPELL_SEVERED_ESSENCE_25), true); 
                        events.ScheduleEvent(EVENT_SEVERED_ESSENCE, 25 * IN_MILLISECONDS);
                        break;
                    }                    
                    case EVENT_REMOVE_HOVER:
                        me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, 2);
                        break;
                }

                DoMeleeAttackIfReady();
            }

            private:
                EventMap events;
                SummonList summons;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_icecrown_val_kyr_heraldAI(creature);
        }
};

class npc_icecrown_severed_essence : public CreatureScript
{
public:
    npc_icecrown_severed_essence() : CreatureScript("npc_icecrown_severed_essence") { }
    
    struct npc_icecrown_severed_essenceAI : public ScriptedAI
    {
        npc_icecrown_severed_essenceAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
            playerClass = 0;
        }

        void Reset() override
        {
            events.Reset();
            if (Player* player = me->FindNearestPlayer(100.0f))
                AttackStart(player);
        }

        void IsSummonedBy(Unit* owner) override
        {
            if (owner->GetTypeId() != TYPEID_UNIT || owner->GetEntry() != NPC_VALKYR_HERALD)
                return;

            uiOwnerId = owner->GetGUID();
        }

        void KilledUnit(Unit* victim) override
        {
            if (Unit* newVictim = SelectTarget(SELECT_TARGET_RANDOM, 0, -5.0f))
                AttackStart(newVictim);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            if (Unit* player = me->FindNearestPlayer(100.0f))
            {
                player->CastSpell(me, SPELL_CLONE_PLAYER, true);
                SetData(DATA_PLAYER_CLASS, player->getClass());
            }
            events.Reset();
            switch (playerClass)
            {
                case CLASS_DRUID:
                {
                    events.ScheduleEvent(EVENT_CAT_FORM, 0.1 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_MANGLE, 3 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_RIP, 8 * IN_MILLISECONDS);
                    break;
                }
                case CLASS_WARLOCK:
                {
                    events.ScheduleEvent(EVENT_CORRUPTION, 0.1 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_SHADOW_BOLT, 3 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_RAIN_OF_CHAOS, 8 * IN_MILLISECONDS);
                    break;
                }
                case CLASS_SHAMAN:
                {
                    events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 3 * IN_MILLISECONDS);
                    bCanCastReplenishingRains = true;
                    break;
                }
                case CLASS_ROGUE:
                {
                    events.ScheduleEvent(EVENT_FOCUSED_ATTACKS, 10 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_SINISTER_STRIKE, 2 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_EVISCERATE, 8 * IN_MILLISECONDS);
                    break;
                }
                case CLASS_MAGE:
                {
                    events.ScheduleEvent(EVENT_FIREBALL, 0.1 * IN_MILLISECONDS);
                    break;
                }
                case CLASS_WARRIOR:
                {
                    events.ScheduleEvent(EVENT_BLOODTHIRST, 5 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_HEROIC_LEAP, 8 * IN_MILLISECONDS);
                    break;
                }
                case CLASS_DEATH_KNIGHT:
                {
                    events.ScheduleEvent(EVENT_DEATH_GRIP, 0.1 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_NECROTIC_STRIKE, 4 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_PLAGUE_STRIKE, 7 * IN_MILLISECONDS);
                    break;
                }
                case CLASS_HUNTER:
                {
                    events.ScheduleEvent(EVENT_SHOOT, 0.1 * IN_MILLISECONDS);
                    bCanCastDisengage = true;
                    break;
                }
                case CLASS_PALADIN:
                {
                    events.ScheduleEvent(EVENT_FLASH_OF_LIGHT, 0.1 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_CLEANSE, 3 * IN_MILLISECONDS);
                    bCanCastRadianceAura = true;
                    break;
                case CLASS_PRIEST:
                {
                    events.ScheduleEvent(EVENT_GREATER_HEAL, 0.1 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_RENEW, 1 * IN_MILLISECONDS);
                    break;
                }
                }
            }
        }
        
        void DamageTaken(Unit* /* unit*/, uint32& /*damage*/) override
        {
            switch (playerClass)
            {
                case CLASS_SHAMAN:
                {
                    if (HealthBelowPct(30) && bCanCastReplenishingRains)
                    {
                        events.ScheduleEvent(EVENT_REPLENISHING_RAINS, 0.1 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_CAN_CAST_REPLENISHING_RAINS, 15 * IN_MILLISECONDS);
                        bCanCastReplenishingRains = false;
                    }
                }                
                case CLASS_PALADIN:
                    if (HealthBelowPct(30) && bCanCastRadianceAura)
                    {
                        events.ScheduleEvent(EVENT_RADIANCE_AURA, 0.1 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_CAN_CAST_RADIANCE_AURA, 15 * IN_MILLISECONDS);
                        bCanCastRadianceAura = false;
                    }
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_PLAYER_CLASS)
                playerClass = data;
        }

        void HandleDruidEvents()
        {
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_CAT_FORM:
                    DoCastSelf(SPELL_CAT_FORM);
                    break;
                case EVENT_MANGLE:
                    DoCastVictim(SPELL_MANGLE);
                    events.ScheduleEvent(EVENT_MANGLE, 2 * IN_MILLISECONDS);
                    break;
                case EVENT_RIP:
                    DoCastVictim(SPELL_RIP);
                    events.ScheduleEvent(EVENT_RIP, 8 * IN_MILLISECONDS);
                    break;
            }
        }

        void HandleWarlockEvents()
        {
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_CORRUPTION:
                    if (Unit* unit = SelectTarget(SELECT_TARGET_RANDOM, 0, -5.0f, true, false, -SPELL_CORRUPTION))
                        DoCast(unit, SPELL_CORRUPTION);
                    events.ScheduleEvent(EVENT_CORRUPTION, 20 * IN_MILLISECONDS);
                    break;
                case EVENT_SHADOW_BOLT:
                    DoCastVictim(SPELL_SHADOW_BOLT);
                    events.ScheduleEvent(EVENT_SHADOW_BOLT, 5 * IN_MILLISECONDS);
                    break;
                case EVENT_RAIN_OF_CHAOS:
                    DoCastVictim(SPELL_RAIN_OF_CHAOS);
                    events.ScheduleEvent(EVENT_RAIN_OF_CHAOS, 18 * IN_MILLISECONDS);
                    break;
            }
        }

        void HandleShamanEvents()
        {
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_CAN_CAST_REPLENISHING_RAINS:
                    bCanCastReplenishingRains = true;
                    break;
                case EVENT_REPLENISHING_RAINS:
                    DoCastSelf(SPELL_REPLENISHING_RAINS);
                    break;
                case EVENT_LIGHTNING_BOLT:
                    DoCastVictim(SPELL_LIGHTNING_BOLT);
                    events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 4 * IN_MILLISECONDS);
                    break;
            }
        }

        void HandleRogueEvents()
        {
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_FOCUSED_ATTACKS:
                    if (Unit* unit = SelectTarget(SELECT_TARGET_RANDOM, 0, 5.0f, true, false, -SPELL_FOCUSED_ATTACKS))
                        DoCast(unit, SPELL_FOCUSED_ATTACKS);
                    events.ScheduleEvent(EVENT_FOCUSED_ATTACKS, 15 * IN_MILLISECONDS);
                        break;
                case EVENT_SINISTER_STRIKE:
                    DoCastVictim(SPELL_SINISTER_STRIKE);
                    events.ScheduleEvent(EVENT_SINISTER_STRIKE, 1 * IN_MILLISECONDS);
                    break;
                case EVENT_EVISCERATE:
                    DoCastVictim(SPELL_EVISCERATE);
                    events.ScheduleEvent(EVENT_EVISCERATE, 6 * IN_MILLISECONDS);
                    break;
            }
        }

        void HandleMageEvents()
        {
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_FIREBALL:
                    DoCastVictim(SPELL_FIREBALL);
                    events.ScheduleEvent(EVENT_FIREBALL, 3.5 * IN_MILLISECONDS);
                    break;
            }
        }

        void HandleWarriorEvents()
        {
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_BLOODTHIRST:
                {
                    if (Unit* unit = SelectTarget(SELECT_TARGET_RANDOM, 0, 5.0f, true))
                        DoCast(unit, SPELL_BLOODTHIRST);
                    events.ScheduleEvent(EVENT_BLOODTHIRST, 12 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_HEROIC_LEAP:
                {
                    if (Unit* unit = SelectTarget(SELECT_TARGET_RANDOM, 1, 8.0f, true))
                        DoCast(unit, SPELL_HEROIC_LEAP);
                    events.ScheduleEvent(EVENT_HEROIC_LEAP, 45 * IN_MILLISECONDS);
                    break;
                }
            }
        }

        void HandleDeathKnightEvents()
        {
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_DEATH_GRIP:
                {
                    if (Unit* unit = SelectTarget(SELECT_TARGET_RANDOM, 0, -5.0f, true))
                    {
                        DoCast(unit, EVENT_DEATH_GRIP);
                        me->GetThreatManager().clearReferences();
                        AttackStart(unit);
                    }
                    events.ScheduleEvent(EVENT_DEATH_GRIP, 35 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_NECROTIC_STRIKE:
                {
                    DoCastVictim(SPELL_NECROTIC_STRIKE);
                    events.ScheduleEvent(EVENT_NECROTIC_STRIKE, 6 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_PLAGUE_STRIKE:
                {
                    if (Unit* unit = SelectTarget(SELECT_TARGET_RANDOM, 0, 5.0f, true))
                        DoCast(unit, SPELL_PLAGUE_STRIKE);
                    events.ScheduleEvent(EVENT_PLAGUE_STRIKE, 7 * IN_MILLISECONDS);
                    break;
                }
            }
        }

        void HandleHunterEvents()
        {
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_CAN_CAST_DISENGAGE:
                {
                    bCanCastDisengage = true;
                    break;
                }
                case EVENT_SHOOT:
                {
                    DoCastVictim(RAID_MODE<uint32>(SPELL_SHOOT_10, SPELL_SHOOT_25, SPELL_SHOOT_10, SPELL_SHOOT_25));
                    events.ScheduleEvent(EVENT_SHOOT, 2 * IN_MILLISECONDS);
                    break;
                }
            }
        }

        void HandlePaladinEvents()
        {
            Creature *valkyrHerald;
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_CAN_CAST_RADIANCE_AURA:
                {
                    bCanCastRadianceAura = true;
                    break;
                }
                case EVENT_FLASH_OF_LIGHT:
                {
                    if ((valkyrHerald = me->FindNearestCreature(NPC_VALKYR_HERALD, 40.0f)) && urand(0, 1))
                        DoCast(valkyrHerald, SPELL_FLASH_OF_LIGHT);
                    else
                    {
                        std::list<Creature*> others;
                        me->GetCreatureListWithEntryInGrid(others, NPC_SEVERED_ESSENCE, 40.0f);
                        Unit *pMob = 0;
                        for (std::list<Creature*>::const_iterator itr = others.begin(); itr != others.end(); ++itr)
                            if (!pMob || pMob->GetHealthPct() < (*itr)->GetHealthPct())
                                pMob = (*itr);
                        if (!pMob)
                            pMob = valkyrHerald;
                        if (pMob)
                            DoCast(pMob, SPELL_FLASH_OF_LIGHT);
                    }
                    events.ScheduleEvent(EVENT_FLASH_OF_LIGHT, 5 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_CLEANSE:
                {
                    if ((valkyrHerald = me->FindNearestCreature(NPC_VALKYR_HERALD, 30.0f)))
                        DoCast(valkyrHerald, SPELL_CLEANSE);
                    events.ScheduleEvent(EVENT_CLEANSE, 5 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_RADIANCE_AURA:
                {
                    DoCastSelf(SPELL_RADIANCE_AURA);
                    break;
                }
            }
        }

        void HandlePriestEvents()
        {
            Creature *valkyrHerald;
            switch (uint32 eventId = events.ExecuteEvent())
            {
                case EVENT_RENEW:
                {
                    if ((valkyrHerald = me->FindNearestCreature(NPC_VALKYR_HERALD, 40.0f)) && urand(0, 1))
                        DoCast(valkyrHerald, SPELL_RENEW);
                    else
                    {
                        std::list<Creature*> others;
                        me->GetCreatureListWithEntryInGrid(others, NPC_SEVERED_ESSENCE, 40.0f);
                        Unit* creature = 0;
                        for (std::list<Creature*>::const_iterator itr = others.begin(); itr != others.end(); ++itr)
                            if (!((*itr)->HasAura(SPELL_RENEW)) && (!creature || creature->GetHealthPct() < (*itr)->GetHealthPct()))
                                creature = (*itr);
                        if (!creature)
                        {
                            Aura* creatureAura;
                            for (std::list<Creature*>::const_iterator itr = others.begin(); itr != others.end(); ++itr)
                            {
                                if (!creature && (*itr)->HasAura(SPELL_RENEW))
                                {
                                    creature = (*itr);
                                    continue;
                                }
                                if ((*itr)->HasAura(SPELL_RENEW) &&
                                    ((creatureAura = (*itr)->GetAura(SPELL_RENEW)) && creatureAura->GetDuration() < creature->GetAura(SPELL_RENEW)->GetDuration()))
                                    creature = (*itr);
                            }
                        }
                        if (!creature)
                            creature = valkyrHerald;
                        if (creature)
                            DoCast(creature, SPELL_RENEW);
                    }
                    events.ScheduleEvent(EVENT_RENEW, 5 * IN_MILLISECONDS);
                    break;
                }
                case EVENT_GREATER_HEAL:
                {
                    if ((valkyrHerald = me->FindNearestCreature(NPC_VALKYR_HERALD, 40.0f)) && urand(0, 1))
                        DoCast(valkyrHerald, SPELL_GREATER_HEAL);
                    else
                    {
                        std::list<Creature*> others;
                        me->GetCreatureListWithEntryInGrid(others, NPC_SEVERED_ESSENCE, 40.0f);
                        Unit* creature = 0;
                        for (std::list<Creature*>::const_iterator itr = others.begin(); itr != others.end(); ++itr)
                            if (!creature || creature->GetHealthPct() < (*itr)->GetHealthPct())
                                creature = (*itr);
                        if (!creature)
                            creature = valkyrHerald;
                        if (creature)
                            DoCast(creature, SPELL_GREATER_HEAL);
                    }
                    events.ScheduleEvent(EVENT_GREATER_HEAL, 5 * IN_MILLISECONDS);
                    break;
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            switch (playerClass)
            {
                case CLASS_DRUID:
                    HandleDruidEvents();
                    break;
                case CLASS_WARLOCK:
                    HandleWarlockEvents();
                case CLASS_SHAMAN:
                    HandleShamanEvents();
                case CLASS_ROGUE:
                    HandleRogueEvents();
                case CLASS_MAGE:
                    HandleMageEvents();
                case CLASS_WARRIOR:
                    HandleWarriorEvents();
                case CLASS_DEATH_KNIGHT:
                    HandleDeathKnightEvents();
                case CLASS_HUNTER:
                    HandleHunterEvents();
                case CLASS_PALADIN:
                    HandlePaladinEvents();
                case CLASS_PRIEST:
                    HandlePriestEvents();
                default:
                    break;
            }
            DoMeleeAttackIfReady();
        }
    private:
        uint8 playerClass;
        InstanceScript *instance;
        EventMap events;
        ObjectGuid uiOwnerId;
        bool bCanCastReplenishingRains;
        bool bCanCastDisengage;
        bool bCanCastRadianceAura;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_icecrown_severed_essenceAI(creature);
    }
};

class npc_icc_buff_switcher : public CreatureScript
{
    public:
        npc_icc_buff_switcher() : CreatureScript("npc_icc_buff_switcher") { }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
        {
            if ((creature->GetEntry() == NPC_GARROSH_HELLSCREAM && player->PlayerTalkClass->GetGossipMenu().GetMenuId() == 11206)
                || (creature->GetEntry() == NPC_KING_VARIAN_WRYNN && player->PlayerTalkClass->GetGossipMenu().GetMenuId() == 11204))
            {
                if (!player->GetGroup() || !player->GetGroup()->isRaidGroup() || !player->GetGroup()->IsLeader(player->GetGUID()))
                {
                    player->CLOSE_GOSSIP_MENU();
                    ChatHandler(player->GetSession()).PSendSysMessage("Nur der Raidleiter kann den Buff deaktivieren.");
                    return true;
                }
                if (InstanceScript* inst = creature->GetInstanceScript())
                    if (inst->GetData(DATA_BUFF_AVAILABLE))
                        inst->SetData(DATA_BUFF_AVAILABLE, 0);
                if (creature->GetEntry() == NPC_GARROSH_HELLSCREAM)
                {
                    player->CLOSE_GOSSIP_MENU();
                    return true;
                }
            }
            return false;
        }
};

class spell_icc_stoneform : public SpellScriptLoader
{
    public:
        spell_icc_stoneform() : SpellScriptLoader("spell_icc_stoneform") { }

        class spell_icc_stoneform_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_icc_stoneform_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Creature* target = GetTarget()->ToCreature())
                {
                    target->SetReactState(REACT_PASSIVE);
                    target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    target->SetImmuneToPC(true);
                    target->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_CUSTOM_SPELL_02);
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Creature* target = GetTarget()->ToCreature())
                {
                    target->SetReactState(REACT_AGGRESSIVE);
                    target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    target->SetImmuneToPC(false);
                    target->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_icc_stoneform_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_icc_stoneform_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_icc_stoneform_AuraScript();
        }
};

class spell_icc_sprit_alarm : public SpellScriptLoader
{
    public:
        spell_icc_sprit_alarm() : SpellScriptLoader("spell_icc_sprit_alarm") { }

        class spell_icc_sprit_alarm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icc_sprit_alarm_SpellScript);

            void HandleEvent(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                uint32 trapId = 0;
                switch (GetSpellInfo()->Effects[effIndex].MiscValue)
                {
                    case EVENT_AWAKEN_WARD_1:
                        trapId = GO_SPIRIT_ALARM_1;
                        break;
                    case EVENT_AWAKEN_WARD_2:
                        trapId = GO_SPIRIT_ALARM_2;
                        break;
                    case EVENT_AWAKEN_WARD_3:
                        trapId = GO_SPIRIT_ALARM_3;
                        break;
                    case EVENT_AWAKEN_WARD_4:
                        trapId = GO_SPIRIT_ALARM_4;
                        break;
                    default:
                        return;
                }

                if (GameObject* trap = GetCaster()->FindNearestGameObject(trapId, 5.0f))
                    trap->SetRespawnTime(trap->GetGOInfo()->GetAutoCloseTime());

                std::list<Creature*> wards;
                GetCaster()->GetCreatureListWithEntryInGrid(wards, NPC_DEATHBOUND_WARD, 150.0f);
                wards.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
                for (std::list<Creature*>::iterator itr = wards.begin(); itr != wards.end(); ++itr)
                {
                    if ((*itr)->IsAlive() && (*itr)->HasAura(SPELL_STONEFORM))
                    {
                        (*itr)->AI()->Talk(SAY_TRAP_ACTIVATE);
                        (*itr)->RemoveAurasDueToSpell(SPELL_STONEFORM);
                        if (Unit* target = (*itr)->SelectNearestTarget(30.0f))
                            (*itr)->AI()->AttackStart(target);
                        break;
                    }
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_icc_sprit_alarm_SpellScript::HandleEvent, EFFECT_2, SPELL_EFFECT_SEND_EVENT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_icc_sprit_alarm_SpellScript();
        }
};

class spell_icc_sprit_alarm_putricide : public SpellScriptLoader
{
    public:
        spell_icc_sprit_alarm_putricide() : SpellScriptLoader("spell_icc_sprit_alarm_putricide") { }
        
        class spell_icc_sprit_alarm_putricide_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icc_sprit_alarm_putricide_SpellScript);
            
            void HandleEvent(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                uint32 trapId = 0;
                switch (GetSpellInfo()->Effects[effIndex].MiscValue)
                {
                    case EVENT_SPAWN_TRASH_1:
                        trapId = GO_SPIRIT_ALARM_5;
                        break;
                    case EVENT_SPAWN_TRASH_2:
                        trapId = GO_SPIRIT_ALARM_6;
                        break;
                    default:
                        return;
                }
               
                if (GameObject* trap = GetCaster()->FindNearestGameObject(trapId, 5.0f))
                {
                    for (int i = 0; i < 7; ++i)
                        if (Creature* trash = trap->SummonCreature(NPC_VENGEFUL_FLESHREAPER, VengefulFleshreaperLoc[i], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000))
                        {
                            trash->SetWalk(false);
                            trash->SetSpeedRate(MOVE_RUN, 2.0f);
                            trash->SetSpeedRate(MOVE_WALK, 2.0f);
                            trash->SetInCombatWithZone();
                            trash->AI()->AttackStart(trash->FindNearestPlayer(500.0f));
                            if (i == 1) //only one mob need to talk not all
                                trash->AI()->Talk(SAY_FLESHREAPER_SPAWN);                            
                        }                                            
                    trap->SetRespawnTime(trap->GetGOInfo()->GetAutoCloseTime());
                }               
            }
            
            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_icc_sprit_alarm_putricide_SpellScript::HandleEvent, EFFECT_2, SPELL_EFFECT_SEND_EVENT);
            }
        };
        
        SpellScript* GetSpellScript() const
        {
            return new spell_icc_sprit_alarm_putricide_SpellScript();
        }
};

class spell_frost_giant_death_plague : public SpellScriptLoader
{
    public:
        spell_frost_giant_death_plague() : SpellScriptLoader("spell_frost_giant_death_plague") { }

        class spell_frost_giant_death_plague_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_frost_giant_death_plague_SpellScript);

            bool Load()
            {
                _failed = false;
                return true;
            }

            // First effect
            void CountTargets(std::list<WorldObject*>& targets)
            {
                targets.remove(GetCaster());
                _failed = targets.empty();
            }

            // Second effect
            void FilterTargets(std::list<WorldObject*>& targets)
            {
                // Select valid targets for jump
                targets.remove_if(DeathPlagueTargetSelector(GetCaster()));
                if (!targets.empty())
                {
                    WorldObject* target = Trinity::Containers::SelectRandomContainerElement(targets);
                    targets.clear();
                    targets.push_back(target);
                }

                targets.push_back(GetCaster());
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (GetHitUnit() != GetCaster()) 
                {
                    GetCaster()->CastSpell(GetHitUnit(), SPELL_DEATH_PLAGUE_AURA, true);
                    GetCaster()->CastSpell(GetCaster(), SPELL_RECENTLY_INFECTED, true);
				}
                else if (_failed)
                    GetCaster()->CastSpell(GetCaster(), SPELL_DEATH_PLAGUE_KILL, true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_frost_giant_death_plague_SpellScript::CountTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_frost_giant_death_plague_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ALLY);
                OnEffectHitTarget += SpellEffectFn(spell_frost_giant_death_plague_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }

            bool _failed;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_frost_giant_death_plague_SpellScript();
        }
};

class spell_icc_harvest_blight_specimen : public SpellScriptLoader
{
    public:
        spell_icc_harvest_blight_specimen() : SpellScriptLoader("spell_icc_harvest_blight_specimen") { }

        class spell_icc_harvest_blight_specimen_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icc_harvest_blight_specimen_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetHitUnit()->RemoveAurasDueToSpell(uint32(GetEffectValue()));
            }

            void HandleQuestComplete(SpellEffIndex /*effIndex*/)
            {
                GetHitUnit()->RemoveAurasDueToSpell(uint32(GetEffectValue()));
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_icc_harvest_blight_specimen_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnEffectHitTarget += SpellEffectFn(spell_icc_harvest_blight_specimen_SpellScript::HandleQuestComplete, EFFECT_1, SPELL_EFFECT_QUEST_COMPLETE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_icc_harvest_blight_specimen_SpellScript();
        }
};

class AliveCheck
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (Unit* unit = object->ToUnit())
                return unit->IsAlive();
            return true;
        }
};

class spell_svalna_revive_champion : public SpellScriptLoader
{
    public:
        spell_svalna_revive_champion() : SpellScriptLoader("spell_svalna_revive_champion") { }

        class spell_svalna_revive_champion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_svalna_revive_champion_SpellScript);

            void RemoveAliveTarget(std::list<WorldObject*>& targets)
            {
                targets.remove_if(AliveCheck());
                Trinity::Containers::RandomResize(targets, 2);
            }

            void Land(SpellEffIndex /*effIndex*/)
            {
                Creature* caster = GetCaster()->ToCreature();
                if (!caster)
                    return;

                Position pos;
                caster->GetPosition(&pos);
                caster->GetNearPosition(pos, 5.0f, 0.0f);
                //pos.m_positionZ = caster->GetBaseMap()->GetHeight(caster->GetPhaseMask(), pos.GetPositionX(), pos.GetPositionY(), caster->GetPositionZ(), true, 50.0f);
                //pos.m_positionZ += 0.05f;
                caster->SetHomePosition(pos);
                caster->GetMotionMaster()->MoveLand(POINT_LAND, pos);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_svalna_revive_champion_SpellScript::RemoveAliveTarget, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
                OnEffectHit += SpellEffectFn(spell_svalna_revive_champion_SpellScript::Land, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_svalna_revive_champion_SpellScript();
        }
};

class spell_svalna_remove_spear : public SpellScriptLoader
{
    public:
        spell_svalna_remove_spear() : SpellScriptLoader("spell_svalna_remove_spear") { }

        class spell_svalna_remove_spear_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_svalna_remove_spear_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Creature* target = GetHitCreature())
                {
                    if (Unit* vehicle = target->GetVehicleBase())
                        vehicle->RemoveAurasDueToSpell(SPELL_IMPALING_SPEAR);
                    target->DespawnOrUnsummon(1);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_svalna_remove_spear_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_svalna_remove_spear_SpellScript();
        }
};

class spell_icc_soul_missile : public SpellScriptLoader
{
    public:
        spell_icc_soul_missile() : SpellScriptLoader("spell_icc_soul_missile") { }

        class spell_icc_soul_missile_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icc_soul_missile_SpellScript);

            void RelocateDest()
            {
                static Position const offset = {0.0f, 0.0f, 200.0f, 0.0f};
                const_cast<WorldLocation*>(GetExplTargetDest())->RelocateOffset(offset);
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_icc_soul_missile_SpellScript::RelocateDest);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_icc_soul_missile_SpellScript();
        }
};

class spell_icc_giant_insect_swarm : public SpellScriptLoader
{
    public:
        spell_icc_giant_insect_swarm() : SpellScriptLoader("spell_icc_giant_insect_swarm") { }

        class spell_icc_giant_insect_swarm_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_icc_giant_insect_swarm_AuraScript)

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                Unit* caster = GetCaster();
                if (caster->ToCreature()->GetPositionZ() < 382.0f)
                    PreventDefaultAction();

                if (roll_chance_i(90))
                    return;

                float x, y, z;
                caster->GetPosition(x, y, z);
                y += irand(-10, 30);
                x += irand(-10, 10);
                z += irand(10, 20);
                caster->CastSpell(x, y, z, SPELL_SUMMON_PLAGUED_INSECTS, true);
            }

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (InstanceScript* instance = caster->GetInstanceScript())
                    {
                        // open all the doors
                        instance->HandleGameObject(ObjectGuid(instance->GetGuidData(DATA_PUTRICIDE_DOOR)), true);
                        instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_SCIENTIST_AIRLOCK_DOOR_COLLISION)), true);
                        if (GameObject* go = instance->instance->GetGameObject(ObjectGuid(instance->GetGuidData(GO_SCIENTIST_AIRLOCK_DOOR_ORANGE))))
                            go->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                        if (GameObject* go = instance->instance->GetGameObject(ObjectGuid(instance->GetGuidData(GO_SCIENTIST_AIRLOCK_DOOR_GREEN))))
                            go->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                    }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_icc_giant_insect_swarm_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                OnEffectRemove += AuraEffectRemoveFn(spell_icc_giant_insect_swarm_AuraScript::HandleOnEffectRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_icc_giant_insect_swarm_AuraScript();
        }
};

class at_icc_saurfang_portal : public AreaTriggerScript
{
    public:
        at_icc_saurfang_portal() : AreaTriggerScript("at_icc_saurfang_portal") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            InstanceScript* instance = player->GetInstanceScript();
            if (!instance || instance->GetData(DATA_SAURFANG_DONE) == 0)
                return true;

            player->TeleportTo(631, 4126.35f, 2769.23f, 350.963f, 0.0f);

            if (instance->GetData(DATA_COLDFLAME_JETS) == NOT_STARTED)
            {
                // Process relocation now, to preload the grid and initialize traps
                player->GetMap()->PlayerRelocation(player, 4126.35f, 2769.23f, 350.963f, 0.0f);

                instance->SetData(DATA_COLDFLAME_JETS, IN_PROGRESS);
                std::list<Creature*> traps;
                player->GetCreatureListWithEntryInGrid(traps, NPC_FROST_FREEZE_TRAP, 120.0f);
                traps.sort(Trinity::ObjectDistanceOrderPred(player));
                bool instant = false;
                for (std::list<Creature*>::iterator itr = traps.begin(); itr != traps.end(); ++itr)
                {
                    (*itr)->AI()->DoAction(instant ? 1000 : 11000);
                    instant = !instant;
                }
            }

            return true;
        }
};

class at_icc_shutdown_traps : public AreaTriggerScript
{
    public:
        at_icc_shutdown_traps() : AreaTriggerScript("at_icc_shutdown_traps") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster())
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
                instance->SetData(DATA_COLDFLAME_JETS, DONE);
            return true;
        }
};

class at_icc_start_blood_quickening : public AreaTriggerScript
{
    public:
        at_icc_start_blood_quickening() : AreaTriggerScript("at_icc_start_blood_quickening") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster())
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
                if (instance->GetData(DATA_BLOOD_QUICKENING_STATE) == NOT_STARTED)
                    instance->SetData(DATA_BLOOD_QUICKENING_STATE, IN_PROGRESS);
            return true;
        }
};

class at_icc_start_frostwing_gauntlet : public AreaTriggerScript
{
    public:
        at_icc_start_frostwing_gauntlet() : AreaTriggerScript("at_icc_start_frostwing_gauntlet") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster())
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
                    if (Creature* crok = ObjectAccessor::GetCreature(*player, ObjectGuid(instance->GetGuidData(DATA_CROK_SCOURGEBANE))))
                        crok->AI()->DoAction(ACTION_START_GAUNTLET);
            return true;
        }
};

class at_icc_putricide_trap : public AreaTriggerScript
{
    public:
        at_icc_putricide_trap() : AreaTriggerScript("at_icc_putricide_trap") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster())
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
                if (instance->GetData(DATA_PUTRICIDE_TRAP) == 0)
                {
                    std::list<Creature*> creatures;
                    player->GetCreatureListWithEntryInGrid(creatures, NPC_PUTRICIDE_TRAP, 100.0f);
                    for (std::list<Creature*>::iterator itr = creatures.begin(); itr != creatures.end(); ++itr)
                        (*itr)->CastSpell((*itr), SPELL_GIANT_INSECT_SWARM, false);

                    instance->SetData(DATA_PUTRICIDE_TRAP, 1);
                    // close all the doors
                    instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_SCIENTIST_AIRLOCK_DOOR_COLLISION)), false);
                    instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_SCIENTIST_AIRLOCK_DOOR_ORANGE)), false);
                    instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_SCIENTIST_AIRLOCK_DOOR_GREEN)), false);
                }
            return true;
        }
};

class at_icc_the_damned : public AreaTriggerScript
{
    public:
        at_icc_the_damned() : AreaTriggerScript("at_icc_the_damned") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster())
                return false;

            std::list<Creature*> SpiderList;
            player->GetCreatureListWithEntryInGrid(SpiderList, NPC_NERUBER_BROODKEEPER, 135.0f);
            if (!SpiderList.empty())
                for (std::list<Creature*>::iterator itr = SpiderList.begin(); itr != SpiderList.end(); itr++)
                    (*itr)->AI()->SetData(1, 1);

            if (InstanceScript* instance = player->GetInstanceScript())
                if (instance->GetData(DATA_THE_DAMNED) == 0)
                    instance->SetData(DATA_THE_DAMNED, 1);
            return true;
        }
};

class at_icc_neruber_broodkeeper : public AreaTriggerScript
{
    public:
        at_icc_neruber_broodkeeper() : AreaTriggerScript("at_icc_neruber_broodkeeper") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster())
                return false;

            std::list<Creature*> SpiderList;
            player->GetCreatureListWithEntryInGrid(SpiderList, NPC_NERUBER_BROODKEEPER, 135.0f);
            if (!SpiderList.empty())
                for (std::list<Creature*>::iterator itr = SpiderList.begin(); itr != SpiderList.end(); itr++)
                    (*itr)->AI()->SetData(1, 1);
            return true;
        }
};

class go_empowering_blood_orb : public GameObjectScript
{
public:
    go_empowering_blood_orb() : GameObjectScript("go_empowering_blood_orb") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        player->CastSpell(player, SPELL_EMPOWERED_BLOOD, true);
        player->CastSpell(player, SPELL_EMPOWERED_BLOOD_TRIGGERD, true);
        if (Creature* dummy = go->FindNearestCreature(NPC_ORB_STALKER, 10.0f))
            dummy->RemoveFromWorld(); // remove the visual
        go->Delete(); // no respawn
        return true;
    }
};

class at_icc_spider_attack : public AreaTriggerScript
{
    public:
        at_icc_spider_attack() : AreaTriggerScript("at_icc_spider_attack") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster() || player->isDead())
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
                if (instance->GetData(DATA_SPIDER_ATTACK) == NOT_STARTED)
                {
                    if (Creature* ward = player->SummonCreature(37503, 4180.002441f, 2410.848145f, 211.032471f, 1.00f, TEMPSUMMON_DEAD_DESPAWN, 800))
                        ward->AI()->Talk(SAY_ATTACK_START);
                    instance->SetData(DATA_SPIDER_ATTACK, IN_PROGRESS);
                }                   

            return true;
        }
};

class at_icc_frostwywrm_right : public AreaTriggerScript
{
    public:
        at_icc_frostwywrm_right() : AreaTriggerScript("at_icc_frostwywrm_right") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster())
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
                if (instance->GetData(DATA_FROSTWYRM_RIGHT) == 0)
                {
                    if (Creature* frostwyrm = player->SummonCreature(NPC_SPIRE_FROSTWYRM, FrostWyrmRightPos, TEMPSUMMON_DEAD_DESPAWN, 30000))
                        frostwyrm->AI()->SetData(DATA_FROSTWYRM_RIGHT, DATA_FROSTWYRM_RIGHT);
                    instance->SetData(DATA_FROSTWYRM_RIGHT, 1);
                }
            return true;
        }
};

class at_icc_frostwywrm_left : public AreaTriggerScript
{
    public:
        at_icc_frostwywrm_left() : AreaTriggerScript("at_icc_frostwywrm_left") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster())
                return false;  

            if (InstanceScript* instance = player->GetInstanceScript())
                if (instance->GetData(DATA_FROSTWYRM_LEFT) == 0)
                {
                    if (Creature* frostwyrm = player->SummonCreature(NPC_SPIRE_FROSTWYRM, FrostWyrmLeftPos, TEMPSUMMON_DEAD_DESPAWN, 30000))
                        frostwyrm->AI()->SetData(DATA_FROSTWYRM_LEFT, DATA_FROSTWYRM_LEFT);
                    instance->SetData(DATA_FROSTWYRM_LEFT, 1);
                }
            return true;
        }
};

class at_icc_arthas_teleporter_unlock : public AreaTriggerScript
{
    public:
        at_icc_arthas_teleporter_unlock() : AreaTriggerScript("at_icc_arthas_teleporter_unlock") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (player->IsGameMaster())
                return false;

            if (InstanceScript* instance = player->GetInstanceScript())
                instance->SetData(DATA_ARTHAS_TELEPORTER_UNLOCK, 0);
            return true;
        }
};

void AddSC_icecrown_citadel()
{
    new npc_highlord_tirion_fordring_lh();
    new npc_rotting_frost_giant();
    new npc_frost_freeze_trap();
    new npc_alchemist_adrianna();
    new boss_sister_svalna();
    new npc_crok_scourgebane();
    new npc_captain_arnath();
    new npc_captain_brandon();
    new npc_captain_grondel();
    new npc_captain_rupert();
    new npc_frostwing_vrykul();
    new npc_impaling_spear();
    new npc_arthas_teleport_visual();
    new npc_lights_hammer_champions();
    new npc_nerubar_broodkeeper();
    new npc_the_damned();
    new npc_rampart_of_skulls_trash();
    new npc_spider_attack();
    new npc_martyr_stalker_igb_saurfang();
    new npc_decaying_colossus();
    new npc_icecrown_val_kyr_herald();
    new npc_icecrown_severed_essence();
    new npc_icecrown_marrowgar_room();
    new npc_questgiver_icc();
    new npc_icc_buff_switcher();
    new spell_icc_stoneform();
    new spell_icc_sprit_alarm();
    new spell_icc_sprit_alarm_putricide();
    new spell_frost_giant_death_plague();
    new spell_icc_harvest_blight_specimen();
    new spell_trigger_spell_from_caster("spell_svalna_caress_of_death", SPELL_IMPALING_SPEAR_KILL);
    new spell_svalna_revive_champion();
    new spell_svalna_remove_spear();
    new spell_icc_soul_missile();
    new spell_icc_giant_insect_swarm();
    new at_icc_saurfang_portal();
    new at_icc_shutdown_traps();
    new at_icc_start_blood_quickening();
    new at_icc_start_frostwing_gauntlet();
    new at_icc_putricide_trap();
    new at_icc_the_damned();
    new at_icc_neruber_broodkeeper();
    new at_icc_spider_attack();
    new at_icc_frostwywrm_right();
    new at_icc_frostwywrm_left();
    new at_icc_arthas_teleporter_unlock();
    new go_empowering_blood_orb();        
}
