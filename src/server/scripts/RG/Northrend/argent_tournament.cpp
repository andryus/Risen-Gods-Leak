/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2012-2015 Rising Gods <http://www.rising-gods.de/>
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
#include "ScriptedGossip.h"
#include "SpellAuras.h"
#include "Vehicle.h"
#include "SpellScript.h"
#include "GameObjectAI.h"
#include "CombatAI.h"
#include "Group.h" 
#include "Unit.h"
#include "ScriptedEscortAI.h"
#include "ScriptedFollowerAI.h"
#include "Player.h"
#include "WorldSession.h"

enum eSpell
{
    //kill credits
    SPELL_DUMMY_CHARGE_CREDIT       = 62658,
    SPELL_DUMMY_MELEE_CREDIT        = 62672,
    SPELL_DUMMY_RANGED_CREDIT       = 62673,
    SPELL_ARGENT_VALIANT_CREDIT     = 63049,
    SPELL_ARGENT_CHAMPION_CREDIT    = 63516,

    //most needed spells
    SPELL_THRUST                    = 62544, 
    SPELL_CHARGE                    = 63010, 
    SPELL_SHIELD_BREAKER            = 65147,
    SPELL_DEFEND                    = 62719,
    
    //spells for trainingsdummy
    SPELL_CHARGE_DEFEND             = 64100,
    SPELL_COUNTERATTACK             = 62709,
    SPELL_PLAYER_BREAK_SHIELD       = 62626,
    SPELL_PLAYER_CHARGE             = 62874,
    
    //get kraken!
    SPELL_FROST_BREATH              = 66514,
    SPELL_KRAKE_KC                  = 66717,
    SPELL_FLAMING_SPEAR             = 66588,
    SPELL_CREAT_TOOTH               = 66994,
    SPELL_PLAYER_ON_MOUNT           = 63034,
    SPELL_TOOTH_EXPLOSION           = 66598,

    //Breakfast for Champion
    SPELL_STORMFORGED_MOLE_MACHINE  = 66492,
    SPELL_SUMMON_DEEP_JORMUNGAR     = 66510,
    SPELL_STORMFORGED_MARAUDER      = 66491,
    
    //near ICC
    SPELL_THUNDERING_THRUST         = 62710,

    //achievements
    SPELL_LANCE_A_LOT_1             = 64805,    //Darnassus
    SPELL_LANCE_A_LOT_2             = 64808,    //The Exodar
    SPELL_LANCE_A_LOT_3             = 64809,    //Gnomeregan
    SPELL_LANCE_A_LOT_4             = 64810,    //Ironforge
    SPELL_LANCE_A_LOT_5             = 64811,    //Orgrimmar
    SPELL_LANCE_A_LOT_6             = 64812,    //Sen'Jin
    SPELL_LANCE_A_LOT_7             = 64813,    //Silvermoon City
    SPELL_LANCE_A_LOT_8             = 64814,    //Stormwind
    SPELL_LANCE_A_LOT_9             = 64815,    //Thunder Bluff
    SPELL_LANCE_A_LOT_10            = 64816,    //The Undercity

    // Black Knight's Curse
    SPELL_TELEPORT_COSMETIC         = 52096,
    SPELL_CURSE_CREDIT              = 66785,

    SPELL_CHARGE_BLACK_KNIGHT       = 63003, 
    SPELL_DARK_SHIELD               = 64505,

    //Pennants
    SPELL_DARNASSUS_PENNANT     = 63443,
    SPELL_EXODAR_PENNANT        = 63439,
    SPELL_GNOMEREGAN_PENNANT    = 63442,
    SPELL_IRONFORGE_PENNANT     = 63440,
    SPELL_ORGRIMMAR_PENNANT     = 63444,
    SPELL_SENJIN_PENNANT        = 63446,
    SPELL_SILVERMOON_PENNANT    = 63438,
    SPELL_STORMWIND_PENNANT     = 62727,
    SPELL_THUNDERBLUFF_PENNANT  = 63445,
    SPELL_UNDERCITY_PENNANT     = 63441,

    //argetn Suire/Gruntling
    SPELL_CHECK_MOUNT           = 67039,
    SPELL_CHECK_TIRED           = 67334,
    SPELL_SQUIRE_BANK_ERRAND    = 67368,
    SPELL_SQUIRE_POSTMAN        = 67376,
    SPELL_SQUIRE_SHOP           = 67377,
    SPELL_SQUIRE_TIRED          = 67401,

    //A Blade fit for a Champion
    SPELL_WARTSBGONE_LIP_BALM   = 62574,
    SPELL_FROG_LOVE             = 62537,
    SPELL_WARTS                 = 62581,
    SPELL_SUMMON_ASHWOOD_BRAND  = 62554,

    //Get Kraken!
    SPELL_SUBMERGE              = 37550,
    SPELL_EMERGE                = 20568,

    //Chum the Water
    SUMMON_ANGRY_KVALDIR            = 66737,
    SUMMON_NORTH_SEA_MAKO           = 66738,
    SUMMON_NORTH_SEA_THRESHER       = 66739,
    SUMMON_NORTH_SEA_BLUE_SHARK     = 66740,

    //
    SPELL_BLESSING_OF_PEACE     = 66719,
    SPELL_AURA_GRIP_OF_SCOURGE  = 60231,
    SPELL_GRIP_OF_SCOURGE       = 60212,

    //Threat from ABove
    SPELL_WING_BUFFET           = 65260,
    SPELL_CHILLMAW_FROST_BREATH = 65248,
    SPELL_THROW_DYNAMITE        = 65128,
    SPELL_TIME_BOMB             = 65130,
};

enum eQuest
{
    QUEST_THE_ASPIRANT_S_CHALLENGE_H    = 13680,
    QUEST_THE_ASPIRANT_S_CHALLENGE_A    = 13679,
        
    QUEST_AMONG_THE_CHAMPIONS_1     = 13790,
    QUEST_AMONG_THE_CHAMPIONS_2     = 13814,
    QUEST_AMONG_THE_CHAMPIONS_3     = 13811,
    QUEST_AMONG_THE_CHAMPIONS_4     = 13793,
    
    QUEST_THE_BLACK_KNIGHTS_FALLING = 13664,
    QUEST_THE_BLACK_KNIGHTS_CURSE   = 14016,

    QUEST_CHAMP_ORGRIMMAR       = 13726,
    QUEST_CHAMP_UNDERCITY       = 13729,
    QUEST_CHAMP_SENJIN          = 13727,
    QUEST_CHAMP_SILVERMOON      = 13731,
    QUEST_CHAMP_THUNDERBLUFF    = 13728,
    QUEST_CHAMP_STORMWIND       = 13699,
    QUEST_CHAMP_IRONFORGE       = 13713,
    QUEST_CHAMP_GNOMEREGAN      = 13723,
    QUEST_CHAMP_DARNASSUS       = 13725,
    QUEST_CHAMP_EXODAR          = 13724,
};

enum eNPCs
{
    NPC_ARGENT_VALIANT          = 33448,
    NPC_ARGENT_CHAMPION         = 33707,
    NPC_SUNREAVER_HAWKSTRIDER   = 33844, //mount A
    NPC_QUEL_DOREI_STEED        = 33845, //mount H
    NPC_ARGENT_WARHORSE         = 33782, //mount neutral

    NPC_CHARGE_TARGET           = 33272,
    NPC_MELEE_TARGET            = 33229,
    NPC_RANGED_TARGET           = 33243,
    
    NPC_BLACK_KNIGHT            = 33785,
    NPC_CULT_ASSASSIN           = 35127,
    NPC_CULT_SABOTEUR           = 35116,
    NPC_BLACK_KNIGHT_S_GRAVE    = 34735,
    
    NPC_MAIDEN_OF_ASHWOOD_LAKE  = 33220,

    NPC_FALLEN_HERO_S_SPIRIT    = 32149,
    NPC_FALLEN_HERO_S_SPIRIT_PROXY = 35055,
    
    //Chum the Water
    NPC_ANGRY_KVALDIR           = 35072,
    NPC_NORTH_SEA_MAKO          = 35071,
    NPC_NORTH_SEA_THRESHER      = 35060,
    NPC_NORTH_SEA_BLUE_SHARK    = 35061,

    NPC_CHILLMAW                = 33687,

    NPC_NORTH_SEA_KRAKEN        = 34925,
    NPC_KVALDIR_DEEPCALLER      = 35092
};

enum eGOs
{
    GO_MYSTERIOUS_SNOW_MOUND        = 195308,
    GO_DRAK_MAR_LILY_PAD            = 194239,
    GO_BLADE_OF_DRAK_MAR            = 194238,
};

enum eGossip
{
    //squire danny and david talk the same
    GOSSIP_SQUIRE                   = 14407,
    GOSSIP_SQUIRE_2                 = 14476,
    GOSSIP_OPTIONID_SQUIRE_RDY      = 60023, //"I am ready to fight!"
    GOSSIP_OPTIONID_SQUIRE_HOW_TO   = 60022, //"How do the Argent Crusader raiders fight?"

    //Argent Valiant
    GOSSIP_VALIANT_RDY              = 10473,
    GOSSIP_VALIANT_NOT_RDY          = 10472,
    GOSSIP_OPTION_CAMP_VAL_RDY      = 10466, //"I am ready to fight!"

    GOSSIP_TEXTID_MAIDEN_DEFAULT    = 14319,
    GOSSIP_TEXTID_MAIDEN_REWARD     = 14320,
    GOSSIP_OPTIONID_MAIDEN          = 33220,
};

enum eText
{
    SAY_FIGHT_START         = 0,
    SAY_FIGHT_WIN           = 1,
    SAY_FIGHT_LOOSE         = 2,
};

enum eMore
{
    TITLE_CRUSADER                  = 123,

    ITEM_MARK_OF_THE_CHAMPION       = 45500,
    ITEM_MARK_OF_THE_VALIANT        = 45127,
    ITEM_ASHWOOD_BRAND              = 44981,
    ITEM_KRAKEN_TOOTH               = 46955,

    MODEL_BLACK_KNIGHT_GRYPHON      = 28652,
};

enum eEvents
{
    EVENT_NULL,
    //most needed events
    EVENT_DEFEND,
    EVENT_SHIELD_BREAKER,
    EVENT_MOVE,
    
    //Trainingsdummy
    EVENT_DUMMY_RECAST_DEFEND,
    EVENT_DUMMY_RESET,

    //Get Kraken!
    EVENT_SUBMERGE,
    EVENT_EMERGE,
    EVENT_DESPAWN,
    EVENT_FROST_BREATH,
    EVENT_EXIT_PASSENGER,

    //The Black Knight's Fall
    EVENT_TALK,
    EVENT_BLACK_KNIGHT_MOUNTED_FIGHT,
    EVENT_BLACK_KNIGHT_NON_MOUNTED_FIGHT,

    //Threat from Above
    EVENT_THROW_DYNAMITE,
    EVENT_TIME_BOMB,
};

enum eCitys
{
    SW, //Stormwind
    IF, //Ironforge
    GR, //Gnomeregan
    ED, //Exodar
    DN, //Darnassus
    OG, //Orgrimmar
    SJ, //Sen'Jin
    TB, //Thunderbluff
    UC, //Untercity
    SM, //Silvermoon
    MAX
};

//QNS Array (QNS = Quest or NPC or Spell)
enum eQuestNpcSpell
{
    QUEST,
    NPC_CHAMPION,
    NPC_VALIANT,
    SPELL,
};
uint32 QNS[10][4] =
{ // Quest  Champ  Vali   Spell
    {13665, 33747, 33561, 64814}, //Stormwind
    {13745, 33743, 33564, 64810}, //Ironforge
    {13750, 33740, 33558, 64809}, //Gnomeregan
    {13756, 33739, 33562, 64808}, //Exodar
    {13761, 33738, 33559, 64805}, //Darnassus
    {13767, 33744, 33306, 64811}, //Orgrimmar
    {13772, 33745, 33285, 64812}, //Sen'Jin
    {13777, 33748, 33383, 64815}, //Thunderbluff
    {13782, 33749, 33384, 64816}, //Untercity
    {13787, 33746, 33382, 64813}, //Silvermoon
};

const Position ValiantPos[6] =
{
    {8577.195f, 952.895f, 547.555f}, //valiant wp 1
    {8587.488f, 955.929f, 547.559f}, //valiant wp 2
    {8596.207f, 962.627f, 547.554f}, //valiant wp 3
    {8574.142f, 919.429f, 547.554f, 1.471f}, //spawn valiant
    {8533.593f, 1068.559f, 551.514f, 1.146f}, //spawn champion
    {8542.550f, 1087.500f, 556.987f} //champion wp
};

/*######
## npc_dame_evniki_kapsalis
######*/

class npc_dame_evniki_kapsalis : public CreatureScript
{
public:
    npc_dame_evniki_kapsalis() : CreatureScript("npc_dame_evniki_kapsalis") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->HasTitle(TITLE_CRUSADER))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, player->GetOptionTextWithEntry(GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_TEXT_BROWSE_GOODS_ID), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*Sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());
        return true;
    }
};

/*######
## npc_squire_david
######*/

class npc_squire_david : public CreatureScript
{
public:
    npc_squire_david() : CreatureScript("npc_squire_david") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->GetQuestStatus(QUEST_THE_ASPIRANT_S_CHALLENGE_H) != QUEST_STATUS_NONE ||
            player->GetQuestStatus(QUEST_THE_ASPIRANT_S_CHALLENGE_A) != QUEST_STATUS_NONE)
        {
            if (player->GetVehicleBase())
            {
                if (player->GetVehicleBase()->GetEntry() == NPC_SUNREAVER_HAWKSTRIDER ||
                    player->GetVehicleBase()->GetEntry() == NPC_QUEL_DOREI_STEED)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_OPTIONID_SQUIRE_RDY), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            }
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_OPTIONID_SQUIRE_HOW_TO), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
        }

        player->SEND_GOSSIP_MENU(GOSSIP_SQUIRE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*Sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            if (Creature* aValiant = creature->SummonCreature(NPC_ARGENT_VALIANT, ValiantPos[3]))
                aValiant->AI()->SetGuidData(player->GetGUID());
        }
        else
        {
            player->SEND_GOSSIP_MENU(GOSSIP_SQUIRE_2, creature->GetGUID());
        }
        return true;
    }
};


/*######
## npc_argent_valiant
######*/

class npc_argent_valiant : public CreatureScript
{
public:
    npc_argent_valiant() : CreatureScript("npc_argent_valiant") { }

    struct npc_argent_valiantAI : public ScriptedAI
    {
        npc_argent_valiantAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 shieldBreakerTimer;
        uint32 defendTimer;
        uint32 moveTimer;
        uint32 talkTimer;
        ObjectGuid playerGUID;
        uint8 wp;
        bool bTalk;

        void Reset()
        {
            shieldBreakerTimer = 7000;
            defendTimer = 8000;
            moveTimer = 1000;
            talkTimer = 3000;
            playerGUID.Clear();
            wp = 0;
            bTalk = false;
        }

        void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
        {
            playerGUID =guid;
        }

        void DamageTaken(Unit* pDoneBy, uint32& damage)
        {
            if (damage > me->GetHealth())
            {
                damage = 0;
                wp++;
                me->SetHealth(me->GetMaxHealth());
                me->setFaction(35);
                me->CombatStop(true);
                me->DespawnOrUnsummon(7000);
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                {
                    if (PlayerHasQuest(player))
                        me->AI()->Talk(SAY_FIGHT_WIN, player);
                    pDoneBy->CastSpell(player, SPELL_ARGENT_VALIANT_CREDIT, true);
                }
            }
        }

        void KilledUnit(Unit* /*victim*/)
        {
            wp++;
            me->setFaction(35);
            me->DespawnOrUnsummon(3000);
            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
            {
                if (PlayerHasQuest(player))
                    me->AI()->Talk(SAY_FIGHT_LOOSE, player);
            }
            me->CombatStop(true);
            me->SetHealth(me->GetMaxHealth());
        }

        bool PlayerHasQuest(Player* player)
        {
            if (player->GetQuestStatus(QUEST_THE_ASPIRANT_S_CHALLENGE_H) == QUEST_STATUS_INCOMPLETE ||
                player->GetQuestStatus(QUEST_THE_ASPIRANT_S_CHALLENGE_A) == QUEST_STATUS_INCOMPLETE)
                return true;
            else return false;
        }

        void UpdateAI(uint32 diff)
        { 
            switch(wp)
            {
                case 0:
                case 1:
                case 2:
                {
                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                    {
                        me->GetMotionMaster()->MovePoint(0, ValiantPos[wp]);
                        wp++;
                    }
                    break;
                }
                case 3:
                {
                    if (!bTalk)
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                        {
                            if (Unit* target = player->GetVehicleBase())
                            {
                                me->SetTarget(target->GetGUID());
                                DoStartMovement(target);
                            }
                        }
                        else
                        {
                            EnterEvadeMode();
                            return;
                        }

                        wp++;
                        bTalk = true;
                    }
                    break;
                }
                case 4:
                {
                    if (talkTimer <= diff)
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, playerGUID);
                        if (!player)
                        {
                            EnterEvadeMode();
                            return;
                        }
                        if (bTalk)
                        {
                            if (PlayerHasQuest(player))
                                me->AI()->Talk(SAY_FIGHT_START, player);
                            bTalk = false;
                            talkTimer = 3500;
                        }
                        else
                        {
                            me->setFaction(14);
                            DoCastSelf(SPELL_DEFEND, true);
                            DoCastSelf(SPELL_DEFEND, true);
                            me->Attack(player->GetVehicleBase(), false);
                            wp++;
                        }
                    }
                    else talkTimer -= diff;
                    break;
                }
                case 5:
                {
                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                        if (me->GetVictim())
                        {
                            DoStartMovement(me->GetVictim());
                            DoCastVictim(SPELL_CHARGE);
                        }

                    if (moveTimer <= diff)
                    {
                        Position pos;
                        me->GetRandomNearPosition(pos,(urand(15,40)));
                        me->GetMotionMaster()->MovePoint(0, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
                        moveTimer = 8000;
                    } else moveTimer -= diff;

                    if (shieldBreakerTimer <= diff)
                    {
                        DoCastVictim(SPELL_SHIELD_BREAKER);
                        shieldBreakerTimer = 7000;
                    } else shieldBreakerTimer -= diff;

                    if (defendTimer <= diff)
                    {
                        DoCastSelf(SPELL_DEFEND, true);
                        defendTimer = 10000;
                    } else defendTimer -= diff;

                    DoMeleeAttackIfReady();
                    break;
                }
                default: 
                    break;
            }
        }
    };

    CreatureAI *GetAI(Creature* creature) const
    {
        return new npc_argent_valiantAI(creature);
    }
};

/*######
* npc_tournament_training_dummy
######*/

class npc_tournament_training_dummy : public CreatureScript
{
    public:
        npc_tournament_training_dummy(): CreatureScript("npc_tournament_training_dummy"){}

        struct npc_tournament_training_dummyAI : ScriptedAI
        {
            npc_tournament_training_dummyAI(Creature* creature) : ScriptedAI(creature) 
            {
                SetCombatMovement(false);
            }

            EventMap events;
            bool isVulnerable;

            void Reset()
            {
                me->SetControlled(true, UNIT_STATE_STUNNED);
                isVulnerable = false;

                // Cast Defend spells to max stack size
                switch (me->GetEntry())
                {
                    case NPC_CHARGE_TARGET:
                        DoCast(SPELL_CHARGE_DEFEND);
                        break;
                    case NPC_RANGED_TARGET:
                        me->CastCustomSpell(SPELL_DEFEND, SPELLVALUE_AURA_STACK, 3, me);
                        break;
                }

                events.Reset();
                events.ScheduleEvent(EVENT_DUMMY_RECAST_DEFEND, 5000);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                if (!_EnterEvadeMode())
                    return;

                Reset();
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                damage = 0;
                events.RescheduleEvent(EVENT_DUMMY_RESET, 10000);
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                switch (me->GetEntry())
                {
                    case NPC_CHARGE_TARGET:
                        if (spell->Id == SPELL_PLAYER_CHARGE)
                            if (isVulnerable)
                                DoCast(caster, SPELL_DUMMY_CHARGE_CREDIT, true);
                        break;
                    case NPC_MELEE_TARGET:
                        if (spell->Id == SPELL_THRUST)
                        {
                            DoCast(caster, SPELL_DUMMY_MELEE_CREDIT, true);

                            if (Unit* target = caster->GetVehicleBase())
                                DoCast(target, SPELL_COUNTERATTACK, true);
                        }
                        break;
                    case NPC_RANGED_TARGET:
                        if (spell->Id == SPELL_PLAYER_BREAK_SHIELD)
                        {
                            if (isVulnerable)
                                DoCast(caster, SPELL_DUMMY_RANGED_CREDIT, true);
                            else if (auto plr = caster->ToPlayer())
                            {
                                if (plr->GetVehicle() && plr->GetVehicle()->GetCreatureEntry() == 1010765)
                                    DoCast(caster, SPELL_DUMMY_RANGED_CREDIT, true);
                            }
                        }
                        break;
                }

                if (spell->Id == SPELL_PLAYER_BREAK_SHIELD)
                    if (!me->HasAura(SPELL_CHARGE_DEFEND) && !me->HasAura(SPELL_DEFEND))
                        isVulnerable = true;
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case EVENT_DUMMY_RECAST_DEFEND:
                        switch (me->GetEntry())
                        {
                            case NPC_CHARGE_TARGET:
                            {
                                if (!me->HasAura(SPELL_CHARGE_DEFEND))
                                    DoCast(SPELL_CHARGE_DEFEND);
                                break;
                            }
                            case NPC_RANGED_TARGET:
                            {
                                Aura* defend = me->GetAura(SPELL_DEFEND);
                                if (!defend || defend->GetStackAmount() < 3 || defend->GetDuration() <= 8000)
                                    DoCast(SPELL_DEFEND);
                                break;
                            }
                        }
                        isVulnerable = false;
                        events.ScheduleEvent(EVENT_DUMMY_RECAST_DEFEND, 5000);
                        break;
                    case EVENT_DUMMY_RESET:
                        if (UpdateVictim())
                        {
                            EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
                            events.ScheduleEvent(EVENT_DUMMY_RESET, 10000);
                        }
                        break;
                }

                if (!UpdateVictim())
                    return;

                if (!me->HasUnitState(UNIT_STATE_STUNNED))
                    me->SetControlled(true, UNIT_STATE_STUNNED);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_tournament_training_dummyAI(creature);
        }

};

/*######
## npc_squire_danny
######*/

class npc_squire_danny : public CreatureScript
{
public:
    npc_squire_danny(): CreatureScript("npc_squire_danny"){}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->GetQuestStatus(QUEST_CHAMP_ORGRIMMAR) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_CHAMP_UNDERCITY) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_CHAMP_SENJIN) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_CHAMP_SILVERMOON) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_CHAMP_THUNDERBLUFF) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_CHAMP_STORMWIND) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_CHAMP_IRONFORGE) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_CHAMP_GNOMEREGAN) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_CHAMP_DARNASSUS) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_CHAMP_EXODAR) == QUEST_STATUS_INCOMPLETE)
        {
            if (player->GetVehicleBase())
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_OPTIONID_SQUIRE_RDY), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        }

        player->SEND_GOSSIP_MENU(GOSSIP_SQUIRE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            if (Creature* aChampion = creature->SummonCreature(NPC_ARGENT_CHAMPION, ValiantPos[4]))
                aChampion->AI()->SetGuidData(player->GetGUID());
        }
        return true;
    }
};

/*######
## npc_argent_champion
######*/

class npc_argent_champion : public CreatureScript
{
public:
    npc_argent_champion(): CreatureScript("npc_argent_champion"){}

    struct npc_argent_championAI : public ScriptedAI
    {
        npc_argent_championAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 shieldBreakerTimer;
        uint32 defendTimer;
        uint32 moveTimer;
        uint32 talkTimer;
        ObjectGuid playerGUID;
        uint8 wp;
        bool bTalk;

        void Reset()
        {
            shieldBreakerTimer = 5000;
            defendTimer = 7000;
            moveTimer = 7000;
            talkTimer = 10000;
            playerGUID.Clear();
            wp = 0;
            bTalk = false;
        }

        void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
        {
            playerGUID = guid;
        }

        void DamageTaken(Unit* pDoneBy, uint32& damage)
        {
            if (damage > me->GetHealth())
            {
                damage = 0;
                wp++;
                me->SetHealth(me->GetMaxHealth());
                me->setFaction(35);
                me->CombatStop(true);
                me->DespawnOrUnsummon(7000);
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                {
                    me->AI()->Talk(SAY_FIGHT_WIN, player);
                    pDoneBy->CastSpell(player, SPELL_ARGENT_CHAMPION_CREDIT, true);
                }
            }
        }

        void KilledUnit(Unit* /*victim*/)
        {
            wp++;
            me->setFaction(35);
            me->DespawnOrUnsummon(3000);
            me->AI()->Talk(SAY_FIGHT_LOOSE, ObjectAccessor::GetPlayer(*me, playerGUID));
            me->CombatStop(true);
            me->SetHealth(me->GetMaxHealth());
            playerGUID.Clear();
        }

        void DoMeleeAttackIfReady()
        {
            if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

            if (me->isAttackReady())
            {
                if (me->IsWithinMeleeRange(me->GetVictim()))
                {
                    DoCastVictim(SPELL_THRUST);
                    me->resetAttackTimer();
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (playerGUID && me->GetVictim() && me->GetVictim()->GetTypeId() == TYPEID_PLAYER)
                if (!me->GetVictim()->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
                {
                    KilledUnit(me);
                    return;
                }

            switch(wp)
            {
                case 0:
                {
                    me->GetMotionMaster()->MovePoint(0, ValiantPos[5]);
                    wp++;
                }
                case 1:
                {
                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                        if (!bTalk)
                        {
                            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                            {
                                if (Unit* target = player->GetVehicleBase())
                                {
                                    me->SetTarget(target->GetGUID());
                                    DoStartMovement(target);
                                }
                            }
                            else
                                KilledUnit(me);

                            wp++;
                            bTalk = true;
                        }
                    break;
                }
                case 2:
                {
                    if (talkTimer <= diff)
                    {
                        if (bTalk)
                        {
                            me->AI()->Talk(SAY_FIGHT_START, ObjectAccessor::GetPlayer(*me, playerGUID));
                            bTalk = false;
                            talkTimer = 3500;
                        }
                        else
                        {
                            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                            {
                                me->setFaction(14);
                                me->SetLootRecipient(player); // resets settings if NULL
                                me->SetAuraStack(SPELL_DEFEND, me, 3);
                                me->Attack(player->GetVehicleBase(), false);
                                wp++;
                            }
                            else
                                KilledUnit(me);
                        }
                    }
                    else talkTimer -= diff;
                    break;
                }
                case 3:
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, playerGUID);
                    if (!player)
                    {
                        KilledUnit(me);
                        return;
                    }

                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                        if (me->GetVictim())
                        {
                            DoStartMovement(player->GetVehicleBase());
                            DoCastVictim(SPELL_CHARGE);
                        }

                    if (moveTimer <= diff)
                    {
                        Position pos;
                        me->GetRandomNearPosition(pos,(urand(15,40)));
                        me->GetMotionMaster()->MovePoint(0, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
                        moveTimer = 7000;
                    } else moveTimer -= diff;

                    if (shieldBreakerTimer <= diff)
                    {
                        me->CastSpell(player->GetVehicleBase(), SPELL_SHIELD_BREAKER, true);
                        shieldBreakerTimer = 5000;
                    } else shieldBreakerTimer -= diff;

                    if (defendTimer <= diff)
                    {
                        DoCastSelf(SPELL_DEFEND, true);
                        defendTimer = 7000;
                    } else defendTimer -= diff;

                    DoMeleeAttackIfReady();
                    break;
                }
                default: 
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_argent_championAI (creature);
    }

};

/*######
## npc_champion_valiant
##
## all champions and valiants, for "Among the Champion" and "Grand Melee"
######*/

class npc_champion_valiant : public CreatureScript
{
public:
    npc_champion_valiant(): CreatureScript("npc_champion_valiant"){}

    struct npc_champion_valiantAI : public ScriptedAI
    {
        npc_champion_valiantAI(Creature* creature) : ScriptedAI(creature) { }

        ObjectGuid playerGUID;
        EventMap events;

        void Reset()
        {
            me->RestoreFaction();
            me->LoadCreaturesAddon();
            playerGUID.Clear();

            events.Reset();
            if (IsChampion(NPC_CHAMPION))
            {
                events.ScheduleEvent(EVENT_DEFEND, 7000);
                events.ScheduleEvent(EVENT_SHIELD_BREAKER, 5000);
                events.ScheduleEvent(EVENT_MOVE, 7000);
            }
            else
            {
                events.ScheduleEvent(EVENT_DEFEND, 10000);
                events.ScheduleEvent(EVENT_SHIELD_BREAKER, 7000);
                events.ScheduleEvent(EVENT_MOVE, 10000);
            }
        }

        void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
        {
            playerGUID = guid;
            Player* player = ObjectAccessor::GetPlayer(*me, playerGUID); // at this point player is valid

            if (Unit* vehicle = player->GetVehicleBase())
            {
                me->setFaction(16);
                me->Attack(vehicle, false);

                me->SetLootRecipient(player);

                me->AI()->Talk(SAY_FIGHT_START, player);

                if (IsChampion(NPC_CHAMPION))
                    me->SetAuraStack(SPELL_DEFEND, me, 3);
                else
                    me->SetAuraStack(SPELL_DEFEND, me, 2);
            }
            else
                Reset();
        }
        
        void DamageTaken(Unit* /*pDoneBy*/, uint32& damage)
        {
            if (damage > me->GetHealth())
            {
                damage = 0;
                me->SetHealth(me->GetMaxHealth());
                me->RestoreFaction();
                me->CombatStop(true);

                Player* player = ObjectAccessor::GetPlayer(*me, playerGUID);
                if (!player)
                {
                    EnterEvadeMode();
                    return;
                }

                player->CombatStop(true);
                if (Unit* vehicle = player->GetVehicleBase())
                    vehicle->CombatStop(true);
                me->DespawnOrUnsummon(8000);
                me->SetCorpseDelay(1);
                me->SetRespawnDelay(4);
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                
                me->AI()->Talk(SAY_FIGHT_WIN, player);

                //Rewards
                if (IsChampion(NPC_CHAMPION))
                    player->AddItem(ITEM_MARK_OF_THE_CHAMPION, 1);
                else
                    player->AddItem(ITEM_MARK_OF_THE_VALIANT, 1);

                for (int i = 0; i < MAX; i++)
                {
                    if (IsChampion(NPC_CHAMPION))
                    {
                        if (me->GetEntry() == QNS[i][NPC_CHAMPION])
                            player->CastSpell(player, QNS[i][SPELL], true);
                    }
                    else
                    {
                        if (me->GetEntry() == QNS[i][NPC_VALIANT])
                            player->CastSpell(player, QNS[i][SPELL], true);
                    }
                }
            }
        }

        void KilledUnit(Unit* /*victim*/)
        {
            me->RestoreFaction();
            me->DespawnOrUnsummon(3000);
            me->SetCorpseDelay(1);
            me->SetRespawnDelay(4);
            me->AI()->Talk(SAY_FIGHT_LOOSE, ObjectAccessor::GetPlayer(*me, playerGUID));
            me->CombatStop(true);
        }
        
        void DoMeleeAttackIfReady()
        {
            if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

            if (me->isAttackReady())
            {
                if (me->IsWithinMeleeRange(me->GetVictim()))
                {
                    DoCastVictim(SPELL_THRUST);
                    me->resetAttackTimer();
                }
            }
        }

        bool IsChampion(int npcType)
        {
            for (int i = 0; i < MAX; i++)
            {
                if (npcType == NPC_CHAMPION)
                {
                    if (me->GetEntry() == QNS[i][NPC_CHAMPION])
                        return true;
                }
                else if (npcType == NPC_VALIANT)
                {
                    if (me->GetEntry() == QNS[i][NPC_VALIANT])
                        return false;
                }
            }

            //only to prevent possible server crashes:
            return false;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
            {
                DoStartMovement(me->GetVictim());
                DoCastVictim(SPELL_CHARGE);
            }

            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_DEFEND:
                {
                    if (me->GetAuraCount(SPELL_DEFEND) < 3)
                        DoCastSelf(SPELL_DEFEND, true);

                    if (IsChampion(NPC_CHAMPION))
                        events.ScheduleEvent(EVENT_DEFEND, 7000);
                    else events.ScheduleEvent(EVENT_DEFEND, 10000);
                    break;
                }
                case EVENT_SHIELD_BREAKER:
                {
                    DoCastVictim(SPELL_SHIELD_BREAKER);
                    if (IsChampion(NPC_CHAMPION))
                        events.ScheduleEvent(EVENT_SHIELD_BREAKER, 5000);
                    else events.ScheduleEvent(EVENT_SHIELD_BREAKER, 7000);
                    break;
                }
                case EVENT_MOVE:
                {
                    Position pos;
                    me->GetRandomNearPosition(pos,(urand(15, 40)));
                    me->GetMotionMaster()->MovePoint(0, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
                    if (IsChampion(NPC_CHAMPION))
                        events.ScheduleEvent(EVENT_MOVE, 7000);
                    else events.ScheduleEvent(EVENT_MOVE, 10000);
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
    
    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        //if player has required aura of my faction, then I tell him that and don't want to fight with him
        for (int i = 0; i < MAX; i++)
        {
            if (creature->GetEntry() == QNS[i][NPC_VALIANT] 
                || creature->GetEntry() == QNS[i][NPC_CHAMPION])
                if(player->HasAura(QNS[i][SPELL]))
                {
                    player->SEND_GOSSIP_MENU(player->GetGossipTextId(GOSSIP_VALIANT_NOT_RDY, creature), creature->GetGUID());
                    return true;
                }
        }

        //if player has quest and is on Vehicle he can start the fight
        for (int i = 0; i < MAX; i++)
        {
            if (creature->GetEntry() == QNS[i][NPC_CHAMPION])
            {
                if (player->GetQuestStatus(QUEST_AMONG_THE_CHAMPIONS_1) == QUEST_STATUS_INCOMPLETE 
                    || player->GetQuestStatus(QUEST_AMONG_THE_CHAMPIONS_2) == QUEST_STATUS_INCOMPLETE
                    || player->GetQuestStatus(QUEST_AMONG_THE_CHAMPIONS_3) == QUEST_STATUS_INCOMPLETE
                    || player->GetQuestStatus(QUEST_AMONG_THE_CHAMPIONS_4) == QUEST_STATUS_INCOMPLETE)
                    if (player->GetVehicleBase() && !player->GetVehicleBase()->IsInCombat())
                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_OPTION_CAMP_VAL_RDY, 1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            }
            else if (creature->GetEntry() == QNS[i][NPC_VALIANT])
                for (int j = 0; j < MAX; j++)
                {
                    if (player->GetQuestStatus(QNS[j][QUEST]) == QUEST_STATUS_INCOMPLETE)
                        if (player->GetVehicleBase() && !player->GetVehicleBase()->IsInCombat())
                            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_OPTION_CAMP_VAL_RDY, 1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                }
        }
        
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(GOSSIP_VALIANT_RDY, creature), creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            creature->AI()->SetGuidData(player->GetGUID());
        }
        return true;
    }
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_champion_valiantAI (creature);
    }

};

/*######
## npc_black_knight
######*/

#define MAX_HP 12500

class npc_black_knight : public CreatureScript
{
public:
    npc_black_knight() : CreatureScript("npc_black_knight") { }

    struct npc_black_knightAI : public ScriptedAI
    {
        npc_black_knightAI(Creature* creature) : ScriptedAI(creature)
        {
            creature->SetReactState(REACT_PASSIVE);
            creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            creature->Mount(MODEL_BLACK_KNIGHT_GRYPHON);
        }
        
        EventMap events;
        ObjectGuid playerGUID;
        uint32  action;
        uint32 outOfCombatActionTimer;
        uint8 phase;

        void Reset()
        {
            playerGUID.Clear();
            action = 0;
            outOfCombatActionTimer = 0;
            phase = EVENT_TALK;
            events.Reset();
            events.ScheduleEvent(EVENT_SHIELD_BREAKER, 6 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_MOVE, 4 * IN_MILLISECONDS);
        }

        void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
        {
            playerGUID = guid;
      
            if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                me->SetLootRecipient(player);
        }
        
        void EnterEvadeMode(EvadeReason /*why*/) override{return;}

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                if (victim->GetGUID() == playerGUID)
                    me->DespawnOrUnsummon(0);
        }

        void DamageTaken(Unit* /*pDoneBy*/, uint32& damage)
        {
            //if me have 12500 hp or less and this is before the mountless fight
            if (damage >= (me->GetHealth() - MAX_HP) && phase != EVENT_BLACK_KNIGHT_NON_MOUNTED_FIGHT)
            {
                me->SetMaxHealth(MAX_HP);
                damage = 0;
                me->SetHealth(me->GetMaxHealth());
                me->Dismount();
                phase = EVENT_TALK;
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }
        }

        void DoMeleeAttackIfReady()
        {
            if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

            if (me->isAttackReady())
            {
                if (me->IsWithinMeleeRange(me->GetVictim()))
                {
                    if (phase == EVENT_BLACK_KNIGHT_MOUNTED_FIGHT)
                        DoCastVictim(SPELL_THRUST);
                    else me->AttackerStateUpdate(me->GetVictim());
                    me->resetAttackTimer();
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            switch(phase)
            {
                case EVENT_TALK:
                    if (outOfCombatActionTimer <= diff)
                    {
                            switch(action)
                            {
                                case 0:
                                    outOfCombatActionTimer = 1 * IN_MILLISECONDS;
                                    action++;
                                    break;
                                case 1:
                                    me->GetMotionMaster()->MovePoint(0, 8420.71f, 962.71f, 544.68f);

                                    action++;
                                    break;
                                case 2:
                                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                                    {
                                        outOfCombatActionTimer = 6 * IN_MILLISECONDS;
                                        action++;
                                    }
                                    break;
                                case 3:
                                {
                                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                    me->SetReactState(REACT_AGGRESSIVE);

                                    Player* player = ObjectAccessor::GetPlayer(*me, playerGUID);
                                    if (!player || player->isDead())
                                    {
                                        me->DespawnOrUnsummon(0);
                                        return;
                                    }

                                    Unit* target = player->GetVehicleBase();
                                    if (!target)
                                        target = player;

                                    me->SetTarget(target->GetGUID());
                                    if (me->GetAggroRange(target) <= MAX_AGGRO_RADIUS)
                                        me->Attack(target, false);
                                    else
                                        DoStartMovement(me->GetVictim());

                                    phase = EVENT_BLACK_KNIGHT_MOUNTED_FIGHT;
                                    
                                    outOfCombatActionTimer = 2 * IN_MILLISECONDS;
                                    action++;
                                    break;
                                }
                                case 4:
                                    DoCastSelf(SPELL_DARK_SHIELD, true);
                                    if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                                    {
                                        if (Creature* vehicle = player->GetVehicleCreatureBase())
                                            vehicle->DespawnOrUnsummon(0);
                                        me->AI()->Talk(0, player);
                                    }
                                    else
                                    {
                                        me->DespawnOrUnsummon(0);
                                        return;
                                    }

                                    outOfCombatActionTimer = 4 * IN_MILLISECONDS;
                                    action++;
                                    break;
                                case 5:
                                    me->AI()->Talk(1, ObjectAccessor::GetPlayer(*me, playerGUID));

                                    outOfCombatActionTimer = 4 * IN_MILLISECONDS;
                                    action++;
                                    break;
                                case 6: 
                                    if (Player* player = ObjectAccessor::GetPlayer(*me, playerGUID))
                                    {
                                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                        me->SetReactState(REACT_AGGRESSIVE);
                                        me->Attack(player, false);
                                        phase = EVENT_BLACK_KNIGHT_NON_MOUNTED_FIGHT;

                                        action++;
                                    }
                                    else
                                    {
                                        me->DespawnOrUnsummon();
                                        return;
                                    }
                                    break;
                            }
                    } 
                    else 
                        outOfCombatActionTimer -= diff;
                    break; //EVENT_TALK
                case EVENT_BLACK_KNIGHT_MOUNTED_FIGHT:
                    if (!UpdateVictim())
                        return;
            
                    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                    {
                        DoStartMovement(me->GetVictim());
                        DoCastVictim(SPELL_CHARGE_BLACK_KNIGHT);
                    }

                    events.Update(diff);

                    switch (events.ExecuteEvent())
                    {
                        case EVENT_SHIELD_BREAKER:
                        {
                            DoCastVictim(SPELL_SHIELD_BREAKER);
                            events.ScheduleEvent(EVENT_SHIELD_BREAKER, 6000);
                            break;
                        }
                        case EVENT_MOVE:
                        {
                            Position pos;
                            me->GetRandomNearPosition(pos,(urand(15, 40)));
                            me->GetMotionMaster()->MovePoint(0, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
                            events.ScheduleEvent(EVENT_MOVE, 4000);
                            break;
                        }
                    }

                    DoMeleeAttackIfReady();
                    break; //EVENT_BLACK_KNIGHT_MOUNTED_FIGHT
                case EVENT_BLACK_KNIGHT_NON_MOUNTED_FIGHT:
                    if (!UpdateVictim())
                        return;
                    DoMeleeAttackIfReady();
                    break;
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_black_knightAI(creature);
    }
};

/*######
## npc_squire_cavin
######*/

class npc_squire_cavin : public CreatureScript
{
public:
    npc_squire_cavin() : CreatureScript("npc_squire_cavin") { }
    
    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(QUEST_THE_BLACK_KNIGHTS_FALLING) == QUEST_STATUS_INCOMPLETE)
        {
            if (player->GetVehicleBase())
            {
                if (player->GetVehicleBase()->GetEntry() == NPC_SUNREAVER_HAWKSTRIDER ||
                    player->GetVehicleBase()->GetEntry() == NPC_QUEL_DOREI_STEED      ||
                    player->GetVehicleBase()->GetEntry() == NPC_ARGENT_WARHORSE)
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(creature->GetEntry()), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            }
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }

        return false;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if(action == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            //summon Black Knight
            if (Creature*blackKnight = creature->SummonCreature(NPC_BLACK_KNIGHT, 8480.089f, 962.908f, 547.4f, 3.1f))
                blackKnight->AI()->SetGuidData(player->GetGUID());
            //me start to talk
            creature->AI()->SetGuidData(player->GetGUID(), EVENT_TALK);

        }
        return true;
    }

    struct npc_squire_cavinAI : public ScriptedAI
    {
        npc_squire_cavinAI(Creature* creature) : ScriptedAI(creature) {}
        
        uint32 talkTimer;
        uint8 talkCount;
        bool canTalk;
        ObjectGuid playerGUID;

        void Reset()
        {
            talkTimer = 0;
            talkCount = 0;
            canTalk = false;
            playerGUID.Clear();
        }

        void SetGuidData(ObjectGuid guid, uint32 id) override
        {
            playerGUID = guid;
            
            if (id == EVENT_TALK && !canTalk)
                canTalk = true;
        }

        void UpdateAI(uint32 diff)
        {
            if (canTalk)
            {
                if (talkTimer <= diff)
                {
                    switch (talkCount)
                    {
                        case 0:
                            me->AI()->Talk(talkCount, ObjectAccessor::GetPlayer(*me, playerGUID));
                            talkTimer = 5 * IN_MILLISECONDS;
                            talkCount++;
                            break;
                        case 1:
                            me->AI()->Talk(talkCount, ObjectAccessor::GetPlayer(*me, playerGUID));
                            talkTimer = 5 * IN_MILLISECONDS;
                            talkCount++;
                            break;
                        case 2:
                        default:
                            Reset();
                            break;
                    }
                }
                else
                    talkTimer -= diff;
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_squire_cavinAI(creature);
    }
};

/*#######
## npc_boneguard_commander
#######*/

class npc_boneguard_commander : public CreatureScript
{
public:
    npc_boneguard_commander() : CreatureScript("npc_boneguard_commander") { }

    struct npc_boneguard_commanderAI : public ScriptedAI
    {
        npc_boneguard_commanderAI(Creature* creature) : ScriptedAI(creature) { }
        
        EventMap events;

        void Reset()
        {
            events.Reset();
            events.ScheduleEvent(EVENT_DEFEND, 8000);
            events.ScheduleEvent(EVENT_SHIELD_BREAKER, 7000);
            events.ScheduleEvent(EVENT_MOVE, 10000);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            //Hack, to Prevent evade bug
            me->DespawnOrUnsummon();
            me->SetRespawnTime(1);
            me->SetCorpseDelay(1);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
            {
                if (Aura* defend = me->GetAura(SPELL_DEFEND))
                {
                    if (defend->GetDuration() < 8000 || defend->GetStackAmount() < 3)
                        DoCastSelf(SPELL_DEFEND, true);
                }
                else me->SetAuraStack(SPELL_DEFEND, me, 3);
                return;
            }
                
            if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
            {
                DoCastVictim(SPELL_CHARGE);
                DoStartMovement(me->GetVictim());
            }

            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_DEFEND:
                {
                    if (me->GetAuraCount(SPELL_DEFEND) < 3)
                        DoCastSelf(SPELL_DEFEND, true);
                    events.ScheduleEvent(EVENT_DEFEND, 8000);
                    break;
                }
                case EVENT_SHIELD_BREAKER:
                {
                    DoCastVictim(SPELL_SHIELD_BREAKER);
                    events.ScheduleEvent(EVENT_SHIELD_BREAKER, 7000);
                    break;
                }
                case EVENT_MOVE:
                {
                    Position pos;
                    me->GetRandomNearPosition(pos,(urand(15, 40)));
                    me->GetMotionMaster()->MovePoint(0, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
                    events.ScheduleEvent(EVENT_MOVE, 10000);
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_boneguard_commanderAI(creature);
    }
};

/*#######
## npc_boneguard_lieutenant
#######*/

class npc_boneguard_lieutenant : public CreatureScript
{
public:
    npc_boneguard_lieutenant() : CreatureScript("npc_boneguard_lieutenant") { }

    struct npc_boneguard_lieutenantAI : public ScriptedAI
    {
        npc_boneguard_lieutenantAI(Creature* creature) : ScriptedAI(creature) {}
        
        EventMap events;

        void Reset()
        {
            me->AddAura(SPELL_THUNDERING_THRUST, me);
            events.Reset();
            events.ScheduleEvent(EVENT_MOVE, 10000);
        }
        
        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            //Hack, to Prevent evade bug
            me->DespawnOrUnsummon();
            me->SetRespawnTime(1);
            me->SetCorpseDelay(1);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
            {
                if (Aura* defend = me->GetAura(SPELL_DEFEND))
                {
                    if (defend->GetDuration() < 10000 || defend->GetStackAmount() != 1)
                        me->SetAuraStack(SPELL_DEFEND, me, 1);
                }
                else me->SetAuraStack(SPELL_DEFEND, me, 1);
                return;
            }
                
            if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
            {
                DoCastVictim(SPELL_CHARGE);
                DoStartMovement(me->GetVictim());
            }

            events.Update(diff);

            if (events.ExecuteEvent() == EVENT_MOVE)
            {
                Position pos;
                me->GetRandomNearPosition(pos,(urand(15, 40)));
                me->GetMotionMaster()->MovePoint(0, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
                events.ScheduleEvent(EVENT_MOVE, 10000);
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_boneguard_lieutenantAI(creature);
    }
};

/*##########
## npc_north_sea_krake
##########*/

class npc_north_sea_krake : public CreatureScript
{
public:
    npc_north_sea_krake() : CreatureScript("npc_north_sea_krake") { }

    struct npc_north_sea_krakeAI : public ScriptedAI
    {
        npc_north_sea_krakeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->setActive(true);
            Reset();
        }

        EventMap events;

        void Reset()
        {
            me->SetControlled(true, UNIT_STATE_ROOT);
            
            //Force to despawn after 25 seconds, if normal despawntimer bugged
            me->DespawnOrUnsummon(25000); 

            //set Repsawn after 1 second (minimum)
            me->SetCorpseDelay(1);
            me->SetRespawnDelay(0);

            events.Reset();
            events.ScheduleEvent(EVENT_EMERGE, 500);
            events.ScheduleEvent(EVENT_FROST_BREATH, urand(6000,9000));
        }

        void JustDied(Unit* DoneBy)
        {
            //Tooth
            DoCastSelf(SPELL_TOOTH_EXPLOSION, true);
            me->CastSpell(DoneBy, SPELL_CREAT_TOOTH, true);
        }

        void SpellHit(Unit* caster, const SpellInfo* pSpell)
        {
            if (pSpell->Id == SPELL_FLAMING_SPEAR)
                caster->ToPlayer()->KilledMonsterCredit(35009);
                //me->CastSpell(caster, SPELL_KRAKE_KC, true); //dosen't work
        }

        void SubEmerge(bool emerge = true)
        {
            me->RemoveAllAuras();
            me->InterruptNonMeleeSpells(false);
            events.CancelEvent(EVENT_FROST_BREATH); //otherwise event break animation

            if (emerge)
            {
                me->RemoveFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                DoCastSelf(SPELL_EMERGE, false);
            }
            else
            {
                DoCastSelf(SPELL_SUBMERGE, false);
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_SUBMERGE:
                    SubEmerge(false);
                    events.RescheduleEvent(EVENT_DESPAWN, 4000);
                    break;
                case EVENT_DESPAWN:
                    me->DespawnOrUnsummon();
                    break;
                case EVENT_EMERGE:
                    SubEmerge(true);
                    events.RescheduleEvent(EVENT_SUBMERGE, 20000);
                    break;
                case EVENT_FROST_BREATH:
                    DoCast(SPELL_FROST_BREATH);
                    events.RescheduleEvent(EVENT_FROST_BREATH, urand(7000,9000));
                    break;
                }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_north_sea_krakeAI (creature);
    }
};

/* ####
## quest: What Do You Feed a Yeti, Anyway?
#### */

class spell_q14112_14145_chum_the_water: public SpellScriptLoader
{
    public:
        spell_q14112_14145_chum_the_water() : SpellScriptLoader("spell_q14112_14145_chum_the_water") { }

        class spell_q14112_14145_chum_the_water_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q14112_14145_chum_the_water_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(SUMMON_ANGRY_KVALDIR) || !sSpellMgr->GetSpellInfo(SUMMON_NORTH_SEA_MAKO) || !sSpellMgr->GetSpellInfo(SUMMON_NORTH_SEA_THRESHER) || !sSpellMgr->GetSpellInfo(SUMMON_NORTH_SEA_BLUE_SHARK))
                    return false;
                return true;
            }

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                //caster->CastSpell(caster, RAND(SUMMON_ANGRY_KVALDIR, SUMMON_NORTH_SEA_MAKO, SUMMON_NORTH_SEA_THRESHER, SUMMON_NORTH_SEA_BLUE_SHARK));
                float x;
                float y;
                x = caster->GetPositionX() + frand(-15.0, 15.0);
                y = caster->GetPositionY() + frand(-15.0, 15.0);
                caster->SummonCreature(RAND(NPC_ANGRY_KVALDIR, NPC_NORTH_SEA_MAKO, NPC_NORTH_SEA_THRESHER, NPC_NORTH_SEA_BLUE_SHARK), x, y, caster->GetPositionZ(), 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
            }

            SpellCastResult CheckCast()
            {
                if (!GetCaster()->IsInWater())
                    return SPELL_FAILED_ONLY_UNDERWATER;
                if (GetCaster()->GetZoneId() != 210)
                    return SPELL_FAILED_INCORRECT_AREA;
                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_q14112_14145_chum_the_water_SpellScript::CheckCast);
                OnEffectHitTarget += SpellEffectFn(spell_q14112_14145_chum_the_water_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_q14112_14145_chum_the_water_SpellScript();
        }
};

/* ####
## quest: Breakfast for Champions
### */

class spell_q14076_14092_pund_drum : public SpellScriptLoader
{
    public:
        spell_q14076_14092_pund_drum() : SpellScriptLoader("spell_q14076_14092_pund_drum") { }

        class spell_q14076_14092_pund_drum_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q14076_14092_pund_drum_SpellScript);
            
            void Function(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (GameObject* snowMound = caster->FindNearestGameObject(GO_MYSTERIOUS_SNOW_MOUND, 15.0f))
                {
                    //despawn mysterious snow mound
                    snowMound->DespawnOrUnsummon();

                    if (urand(0,1))
                        //Summon Stormforged Mole Machine
                        snowMound->CastSpell(caster, SPELL_STORMFORGED_MOLE_MACHINE);
                    else
                        //Summon Deep Jormungar
                        snowMound->CastSpell(caster, SPELL_SUMMON_DEEP_JORMUNGAR); 
                }
            }

            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_q14076_14092_pund_drum_SpellScript::Function, EFFECT_ALL, SPELL_EFFECT_ANY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_q14076_14092_pund_drum_SpellScript();
        }
};

class go_stormforged_mole_machine : public GameObjectScript
{
    public:
        go_stormforged_mole_machine() : GameObjectScript("go_stormforged_mole_machine") { }

        struct go_stormforged_mole_machineAI : public GameObjectAI
        {
            go_stormforged_mole_machineAI(GameObject* gameObject) : GameObjectAI(gameObject) 
            {
                _active = false;
                _hasSpawnDwarf = false;
                _spawnTimer = 3000;
            }
            
            void UpdateAI(uint32 diff)
            {
                if (!_active)
                {
                    // mole machine drill upward
                    go->SetGoState(GO_STATE_ACTIVE);
                    _active = true;
                }

                if (!_hasSpawnDwarf)
                {
                    if (_spawnTimer <= diff)
                    {
                        go->CastSpell(go->FindNearestPlayer(20.0), SPELL_STORMFORGED_MARAUDER);
                        _hasSpawnDwarf = true;
                    }
                    else _spawnTimer -= diff;
                }
            }

          private:
              bool _active;
              bool _hasSpawnDwarf;
              uint32 _spawnTimer;
        };

        GameObjectAI* GetAI(GameObject* go) const
        {
            return new go_stormforged_mole_machineAI(go);
        }
};

/*####
### Quest: The Black Knight's Curse
####*/

class npc_black_knights_grave : public CreatureScript
{
public:
    npc_black_knights_grave() : CreatureScript("npc_black_knights_grave") { }

    struct npc_black_knights_graveAI : public ScriptedAI
    {
        npc_black_knights_graveAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            actionTimer = 0;
            action = 0;
            bactive = false;
            saboteurGUID.Clear();
            assassinGUID.Clear();
            playerGUID.Clear();
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!who || bactive)
                return;

            if (me->GetDistance2d(who) <= 8.0f)
                if (who->GetTypeId() == TYPEID_PLAYER)
                    if (who->ToPlayer()->GetQuestStatus(QUEST_THE_BLACK_KNIGHTS_CURSE) == QUEST_STATUS_INCOMPLETE)
                    {
                        playerGUID = who->GetGUID();
                        bactive = true;
                    }
        }

        void UpdateAI(uint32 diff)
        {
            if (bactive)
            {
                if (actionTimer <= diff)
                {
                    switch (action)
                    {
                        case 0:

                            if (Creature* saboteur = me->SummonCreature(NPC_CULT_SABOTEUR, 8452.655273f, 459.639465f, 596.072449f, 6.282160f))
                            {
                                saboteurGUID = saboteur->GetGUID();
                                saboteur->AI()->Talk(0);
                                saboteur->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                saboteur->SetImmuneToAll(true);
                                saboteur->SetFlag(UNIT_FIELD_FLAGS, CREATURE_FLAG_EXTRA_CIVILIAN);
                                saboteur->SetReactState(REACT_PASSIVE);
                            }
                            else Reset();

                            if (Creature* assassin = me->SummonCreature(NPC_CULT_ASSASSIN, 8455.83f, 459.622f, 596.072f, 1.62868f))
                            {
                                assassinGUID = assassin->GetGUID();
                                assassin->SetStandState(UNIT_STAND_STATE_KNEEL);
                                assassin->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                assassin->SetImmuneToAll(true);
                                assassin->SetFlag(UNIT_FIELD_FLAGS, CREATURE_FLAG_EXTRA_CIVILIAN);
                                assassin->SetReactState(REACT_PASSIVE);
                            }
                            else Reset();

                            actionTimer = 6000;
                            action++;
                            break;
                        case 1:
                        case 2:
                            if (Creature* saboteur = ObjectAccessor::GetCreature(*me, saboteurGUID))
                            {
                                saboteur->AI()->Talk(action);
                                actionTimer = 6000;
                                action++;
                            }
                            else
                                EnterEvadeMode();
                            break;
                        case 3:
                        {
                            Creature* saboteur = ObjectAccessor::GetCreature(*me, saboteurGUID);
                            Creature* assassin = ObjectAccessor::GetCreature(*me, assassinGUID);
                            if (!saboteur || ! assassin)
                            {
                                EnterEvadeMode();
                                return;
                            }
                            saboteur->CastSpell(saboteur, SPELL_TELEPORT_COSMETIC, true);
                            saboteur->DespawnOrUnsummon(1000);
                            assassin->AI()->Talk(0);
                            actionTimer = 3000;
                            action++;
                            break;
                        }
                        case 4:
                            if (Creature* assassin = ObjectAccessor::GetCreature(*me, assassinGUID))
                            {
                                assassin->setFaction(16);
                                assassin->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                assassin->SetImmuneToAll(false);
                                assassin->RemoveFlag(UNIT_FIELD_FLAGS, CREATURE_FLAG_EXTRA_CIVILIAN);
                                assassin->SetReactState(REACT_AGGRESSIVE);
                                assassin->SetStandState(UNIT_STAND_STATE_STAND);
                                assassin->Attack(ObjectAccessor::GetPlayer(*me, playerGUID), false);

                                actionTimer = 60000;
                                action++;
                            }
                            else
                                EnterEvadeMode();
                            break;
                        case 5:
                            Reset();
                            break;
                    }
                }
                else actionTimer -= diff;
            }
        }

        private:
            bool bactive;
            uint32 actionTimer;
            uint32 action;
            ObjectGuid saboteurGUID;
            ObjectGuid assassinGUID;
            ObjectGuid playerGUID;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_black_knights_graveAI (creature);
    }
};

class npc_cult_assassin : public CreatureScript
{
public:
    npc_cult_assassin() : CreatureScript("npc_cult_assassin") { }

    struct npc_cult_assassinAI : public ScriptedAI
    {
        npc_cult_assassinAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            EnterEvadeMode();
        }

        void JustDied(Unit* DoneBy)
        {
            if (DoneBy->GetTypeId() == TYPEID_PLAYER)
                if (DoneBy->ToPlayer()->GetQuestStatus(QUEST_THE_BLACK_KNIGHTS_CURSE) == QUEST_STATUS_INCOMPLETE)
                    DoneBy->CastSpell(DoneBy, SPELL_CURSE_CREDIT, true);
            me->FindNearestCreature(NPC_BLACK_KNIGHT_S_GRAVE, 100.0f)->AI()->Reset();
        }

        void UpdateAI(uint32 /*diff*/)
        {
            if (!UpdateVictim())
              return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_cult_assassinAI (creature);
    }
};

/*######
## npc_argent_squire/gruntling
######*/

enum Misc
{
    STATE_BANK                  = 0x1,
    STATE_SHOP                  = 0x2,
    STATE_MAIL                  = 0x4,

    GOSSIP_ACTION_MAIL          = 3,
    ACHI_PONY_UP                = 3736,
    GOSSIP_DEFAULT              = 50000
};

class npc_argent_squire : public CreatureScript
{
public:
    npc_argent_squire() : CreatureScript("npc_argent_squire") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pPlayer->HasAchieved(ACHI_PONY_UP))
            if (!pCreature->HasAura(SPELL_SQUIRE_TIRED))
            {
                uint8 buff = (STATE_BANK | STATE_SHOP | STATE_MAIL);
                if (pCreature->HasAura(SPELL_SQUIRE_BANK_ERRAND))
                    buff = STATE_BANK;
                if (pCreature->HasAura(SPELL_SQUIRE_SHOP))
                    buff = STATE_SHOP;
                if (pCreature->HasAura(SPELL_SQUIRE_POSTMAN))
                    buff = STATE_MAIL;

                if (buff & STATE_BANK)
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_MONEY_BAG, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_BANK);
                if (buff & STATE_SHOP)
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 1), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
                if (buff & STATE_MAIL)
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 2), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_MAIL);
            }

        // Horde
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_SENJIN))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 3), GOSSIP_SENDER_MAIN, SPELL_SENJIN_PENNANT);
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_UNDERCITY))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 4), GOSSIP_SENDER_MAIN, SPELL_UNDERCITY_PENNANT);
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_ORGRIMMAR))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 5), GOSSIP_SENDER_MAIN, SPELL_ORGRIMMAR_PENNANT);
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_SILVERMOON))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 6), GOSSIP_SENDER_MAIN, SPELL_SILVERMOON_PENNANT);
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_THUNDERBLUFF))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 7), GOSSIP_SENDER_MAIN, SPELL_THUNDERBLUFF_PENNANT);

        //Alliance
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_DARNASSUS))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 8), GOSSIP_SENDER_MAIN, SPELL_DARNASSUS_PENNANT);
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_EXODAR))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 9), GOSSIP_SENDER_MAIN, SPELL_EXODAR_PENNANT);
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_GNOMEREGAN))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 10), GOSSIP_SENDER_MAIN, SPELL_GNOMEREGAN_PENNANT);
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_IRONFORGE))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 11), GOSSIP_SENDER_MAIN, SPELL_IRONFORGE_PENNANT);
        if (pPlayer->GetQuestRewardStatus(QUEST_CHAMP_STORMWIND))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, pPlayer->GetOptionTextWithEntry(GOSSIP_DEFAULT, 12), GOSSIP_SENDER_MAIN, SPELL_STORMWIND_PENNANT);

        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*sender*/, uint32 action)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (action >= 1000) // remove old pennant aura
            pCreature->AI()->SetData(0, 0);
        switch (action)
        {
            case GOSSIP_ACTION_BANK:
                pCreature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_BANKER);
                pPlayer->GetSession()->SendShowBank(pCreature->GetGUID());
                if (!pCreature->HasAura(SPELL_SQUIRE_BANK_ERRAND))
                    pCreature->AddAura(SPELL_SQUIRE_BANK_ERRAND, pCreature);
                if (!pPlayer->HasAura(SPELL_CHECK_TIRED))
                    pPlayer->AddAura(SPELL_CHECK_TIRED, pPlayer);
                break;
            case GOSSIP_ACTION_TRADE:
                pCreature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_VENDOR);
                pPlayer->GetSession()->SendListInventory(pCreature->GetGUID());
                if (!pCreature->HasAura(SPELL_SQUIRE_SHOP))
                    pCreature->AddAura(SPELL_SQUIRE_SHOP, pCreature);
                if (!pPlayer->HasAura(SPELL_CHECK_TIRED))
                    pPlayer->AddAura(SPELL_CHECK_TIRED, pPlayer);
                break;
            case GOSSIP_ACTION_MAIL:
                pCreature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_MAILBOX);
                pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_VENDOR);
                if (!pCreature->HasAura(SPELL_SQUIRE_POSTMAN))
                    pCreature->AddAura(SPELL_SQUIRE_POSTMAN, pCreature);
                if (!pPlayer->HasAura(SPELL_CHECK_TIRED))
                    pPlayer->AddAura(SPELL_CHECK_TIRED, pPlayer);
                break;

            case SPELL_SENJIN_PENNANT:
            case SPELL_UNDERCITY_PENNANT:
            case SPELL_ORGRIMMAR_PENNANT:
            case SPELL_SILVERMOON_PENNANT:
            case SPELL_THUNDERBLUFF_PENNANT:
            case SPELL_DARNASSUS_PENNANT:
            case SPELL_EXODAR_PENNANT:
            case SPELL_GNOMEREGAN_PENNANT:
            case SPELL_IRONFORGE_PENNANT:
            case SPELL_STORMWIND_PENNANT:
                pCreature->AI()->SetData(1, action);
                break;
        }
        pPlayer->PlayerTalkClass->SendCloseGossip();
        return true;
    }

    struct npc_argent_squireAI : public ScriptedAI
    {
        npc_argent_squireAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_current_pennant = 0;
            check_timer = 500;
        }

        uint32 m_current_pennant;
        uint32 check_timer;

        void UpdateAI(uint32 diff)
        {
            // have to delay the check otherwise it wont work
            if (check_timer && (check_timer <= diff))
            {
                if (me->GetOwner() && me->GetOwner()->ToPlayer()->HasAchieved(ACHI_PONY_UP))
                    me->AddAura(SPELL_CHECK_MOUNT, me);

                if (Aura* tired = me->GetOwner()->GetAura(SPELL_CHECK_TIRED))
                {
                    int32 duration = tired->GetDuration();
                    tired = me->AddAura(SPELL_SQUIRE_TIRED, me);
                    tired->SetDuration(duration);
                }
                check_timer = 0;
            }
            else check_timer -= diff;

            if (me->HasAura(SPELL_SQUIRE_TIRED) && me->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_BANKER | UNIT_NPC_FLAG_MAILBOX | UNIT_NPC_FLAG_VENDOR))
            {
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_BANKER | UNIT_NPC_FLAG_MAILBOX | UNIT_NPC_FLAG_VENDOR);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }
        }

        void SetData(uint32 add, uint32 spell)
        {
            if (add)
            {
                me->AddAura(spell, me);
                m_current_pennant = spell;
            }
            else if (m_current_pennant)
            {
                me->RemoveAura(m_current_pennant);
                m_current_pennant = 0;
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_argent_squireAI(creature);
    }
};

/*######
## Quest 13666 & 13673:  A Blade fit for a Champion
######*/

class npc_lake_frog : public CreatureScript
{
public:
    npc_lake_frog(): CreatureScript("npc_lake_frog"){}

    struct npc_lake_frogAI : public FollowerAI
    {
        npc_lake_frogAI(Creature *c) : FollowerAI(c) {}

        uint32 followTimer;
        bool following;

        void Reset ()
        {
            following = false;
            followTimer = 15000;
        }

        void UpdateAI(uint32 diff)
        {
            if(following)
            {
                if(followTimer <= diff)
                {
                    SetFollowComplete();
                    //despawn and respawn after 15 sekonds
                    me->SetCorpseDelay(1);
                    me->SetRespawnDelay(14);
                    me->DespawnOrUnsummon();
                    Reset();
                }
                else followTimer -= diff;
            }
        }

        void ReceiveEmote(Player* player, uint32 emote)
        {
            if (following)
                return;

            if (emote == TEXT_EMOTE_KISS)
            {
                if(!player->HasAura(SPELL_WARTSBGONE_LIP_BALM))
                    player->AddAura(SPELL_WARTS, player);
                else if(roll_chance_i(10) && !player->HasAura(SPELL_WARTS))
                {
                    player->SummonCreature(NPC_MAIDEN_OF_ASHWOOD_LAKE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30000);
                    me->DisappearAndDie();
                    me->SetCorpseDelay(1);
                    me->SetRespawnDelay(14);
                }
                else
                {
                    player->RemoveAura(SPELL_WARTSBGONE_LIP_BALM);
                    me->AddAura(SPELL_FROG_LOVE, me);
                    if (!HasFollowState(STATE_FOLLOW_INPROGRESS))
                        StartFollow(player, 35, NULL);
                    following = true;
                }
            }
        }

    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lake_frogAI(creature);
    }
};

class npc_maiden_of_ashwood_lake : public CreatureScript
{
public:
    npc_maiden_of_ashwood_lake(): CreatureScript("npc_maiden_of_ashwood_lake"){}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if(!player->HasItemCount(ITEM_ASHWOOD_BRAND, 1, true))
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_OPTIONID_MAIDEN), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            player->SEND_GOSSIP_MENU(GOSSIP_TEXTID_MAIDEN_DEFAULT, creature->GetGUID());
            return true;
        }

        player->SEND_GOSSIP_MENU(GOSSIP_TEXTID_MAIDEN_DEFAULT, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        switch(action)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                player->CastSpell(player, SPELL_SUMMON_ASHWOOD_BRAND, true);
                player->SEND_GOSSIP_MENU(GOSSIP_TEXTID_MAIDEN_REWARD, creature->GetGUID());
                creature->DespawnOrUnsummon(3000);
                break;
        }
        return true;
    }
};

/*##########
## npc_maiden_of_drak_mar
###########*/

class npc_maiden_of_drak_mar : public CreatureScript
{
public:
    npc_maiden_of_drak_mar(): CreatureScript("npc_maiden_of_drak_mar"){}

    struct npc_maiden_of_drak_marAI : public ScriptedAI
    {
        uint32 phase;
        uint32 phaseTimer;
        ObjectGuid firstGobGuid;
        ObjectGuid secondGobGuid;

        npc_maiden_of_drak_marAI(Creature *c) : ScriptedAI(c)
        {
            phase = 0;
            phaseTimer = 2000;
            if (GameObject* go = me->SummonGameObject(GO_DRAK_MAR_LILY_PAD, 4602.977f, -1600.141f, 156.7834f, 0.7504916f, 0, 0, 0, 0, 0))
                firstGobGuid = go->GetGUID();
            me->SetUnitMovementFlags(MOVEMENTFLAG_WATERWALKING);
            me->SetStandState(UNIT_STAND_STATE_SIT);
        }

        void UpdateAI(uint32 diff)
        {
            if (phaseTimer <= diff)
            {
                phase++;
                    switch(phase)
                    {
                        case 1:
                            me->AI()->Talk(0);
                            phaseTimer = 5000;
                            break;
                        case 2:
                            me->AI()->Talk(1);
                            phaseTimer = 6000;
                            break;
                        case 3:
                            me->AI()->Talk(2);
                            phaseTimer = 7000;
                            break;
                        case 4:
                            me->AI()->Talk(3);
                            if (GameObject* go = me->SummonGameObject(GO_BLADE_OF_DRAK_MAR, 4603.351f, -1599.288f, 156.8822f, 3.042516f, 0, 0, 0, 0, 0))
                                secondGobGuid = go->GetGUID();
                            phaseTimer = 30000;
                            break;
                        case 5:
                        default:
                            if (GameObject* go = GameObject::GetGameObject(*me, firstGobGuid))
                                go->RemoveFromWorld();
                            if (GameObject* go = GameObject::GetGameObject(*me, secondGobGuid))
                                go->RemoveFromWorld();
                            me->DespawnOrUnsummon();
                            break;
                    }
            }
            else
            {
                phaseTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_maiden_of_drak_marAI(creature);
    }
};

/*###
## npc_black_knight_s_gryphon
###*/

class npc_black_knight_s_gryphon : public CreatureScript
{
public:
    npc_black_knight_s_gryphon() : CreatureScript("npc_black_knight_s_gryphon") { }

    struct npc_black_knight_s_gryphonAI : public npc_escortAI
    {
        npc_black_knight_s_gryphonAI(Creature* creature) : npc_escortAI(creature) {}

        ObjectGuid riderGUID;

        void AttackStart(Unit* /*who*/) {}
        void EnterCombat(Unit* /*who*/) {}
        void EnterEvadeMode(EvadeReason /*why*/) override {}
        void JustDied(Unit* /*killer*/){}
        void OnCharmed(bool /*apply*/) {}

        void Reset()
        {
            me->SetCanFly(false);
            me->SetSpeedRate(MOVE_RUN, 2.0f);
            riderGUID.Clear();
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            if (who->ToPlayer() && apply)
            {
                riderGUID = who->GetGUID();
                Start(false, true, who->GetGUID());
            }
        }

        void WaypointReached(uint32 wp)
        {
            switch (wp)
            {
                case 23:
                    me->SetCanFly(true);
                    me->SetSpeedRate(MOVE_FLIGHT, 3.8f);
                    break;
                case 45:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, riderGUID))
                    {
                        player->KilledMonsterCredit(me->GetEntry());
                        player->ExitVehicle();
                    }
                    else
                        EnterEvadeMode(EVADE_REASON_OTHER);
                    break;
             }
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_black_knight_s_gryphonAI (creature);
    }
};

/* ####
## 
#### */
class npc_scourge_converter : public CreatureScript
{
public:
    npc_scourge_converter() : CreatureScript("npc_scourge_converter") { }

    struct npc_scourge_converterAI : public ScriptedAI
    {
        npc_scourge_converterAI(Creature* creature) : ScriptedAI(creature) { }
        
        uint32 checkTimer;
        void Reset()
        {
            checkTimer = 10 * IN_MILLISECONDS;
            EnterEvadeMode();
        }
        
        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
            {
                if (checkTimer <= diff)
                {
                    if (Creature* creature = me->FindNearestCreature(NPC_FALLEN_HERO_S_SPIRIT, 20.0f))
                        if (!creature->HasAura(SPELL_AURA_GRIP_OF_SCOURGE) && !creature->HasAura(SPELL_BLESSING_OF_PEACE))
                            me->CastSpell(creature, SPELL_GRIP_OF_SCOURGE, true);
                    checkTimer = 20 * IN_MILLISECONDS;
                }
                else checkTimer -= diff;
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_scourge_converterAI (creature);
    }
};

/* ####
## 
#### */
class npc_fallen_hero_s_spirit : public CreatureScript
{
public:
    npc_fallen_hero_s_spirit() : CreatureScript("npc_fallen_hero_s_spirit") { }

    struct npc_fallen_hero_s_spiritAI : public ScriptedAI
    {
        npc_fallen_hero_s_spiritAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 questTimer;
        bool canQuest;

        void Reset()
        {
            EnterEvadeMode();
            questTimer = 25 * IN_MILLISECONDS;
            canQuest = true;
        }

        void SpellHit(Unit* caster, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_AURA_GRIP_OF_SCOURGE)
            {
                me->SetWalk(false);
                me->GetMotionMaster()->MovePoint(0, 7413.701f, 2487.439f, 388.507f);
                me->AddAura(SPELL_AURA_GRIP_OF_SCOURGE, me); //prevent for remove aura
            }

            if (spell->Id == SPELL_BLESSING_OF_PEACE && canQuest)
            {
                caster->ToPlayer()->KilledMonsterCredit(NPC_FALLEN_HERO_S_SPIRIT_PROXY);
                me->AI()->Talk(0);
                canQuest = false;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (me->GetDistance(7413.701f, 2487.439f, 388.507f) == 0.0f)
                me->DespawnOrUnsummon();

            if (!canQuest)
            {
                if (questTimer <= diff)
                {
                    canQuest = true;
                    questTimer = 25 * IN_MILLISECONDS;
                }
                else questTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_fallen_hero_s_spiritAI (creature);
    }
};

/* ####
## Chillmaw 33687
#### */

class npc_chillmaw : public CreatureScript
{
public:
    npc_chillmaw() : CreatureScript("npc_chillmaw") { }

    struct npc_chillmawAI : public ScriptedAI
    {
        npc_chillmawAI(Creature* creature) : ScriptedAI(creature) {}
        
        EventMap events;
        bool lowhp75;
        bool lowhp50;
        bool lowhp35;
        bool lowhp25;

        void Reset()
        {
            lowhp75 = false;
            lowhp50 = false;
            lowhp35 = false;
            lowhp25 = false;
            
            me->SetCorpseDelay(60);
            me->SetRespawnDelay(0);

            events.Reset();
            events.ScheduleEvent(EVENT_FROST_BREATH, 10*IN_MILLISECONDS);
        }
        
        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            //Hack, to Prevent evade bug
            me->DespawnOrUnsummon();
            me->SetRespawnDelay(1);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;
            
            //Exit Vehicle and attack player
            if (me->GetHealthPct() <= 75.0f && !lowhp75)
            {
                lowhp75 = true;
                events.ScheduleEvent(EVENT_EXIT_PASSENGER, Seconds(1));
            }
            if (me->GetHealthPct() <= 50.0f && !lowhp50)
            {
                lowhp50 = true;
                events.ScheduleEvent(EVENT_EXIT_PASSENGER, Seconds(1));
            }
            if (me->GetHealthPct() <= 25.0f && !lowhp25)
            {
                lowhp25 = true;
                events.ScheduleEvent(EVENT_EXIT_PASSENGER, Seconds(1));
            }

            //cast knockback
            if (me->GetHealthPct() <= 35.0f && !lowhp35)
            {
                lowhp35 = true;
                DoCastVictim(SPELL_WING_BUFFET);
            }

            //events
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_FROST_BREATH:
                    DoCastVictim(SPELL_CHILLMAW_FROST_BREATH);
                    events.RescheduleEvent(EVENT_FROST_BREATH, urand(10,11)*IN_MILLISECONDS);
                    break;
                case EVENT_EXIT_PASSENGER:
                    if (Unit* passenger = me->GetVehicleKit()->GetPassenger(0))
                    {
                        passenger->ExitVehicle();
                        if (Player* target = passenger->FindNearestPlayer(50.0f))
                            passenger->SetInCombatWith(target);
                        passenger->ToCreature()->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    }
                    break;
                default:
                    break;
            }
            
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_chillmawAI (creature);
    }
};

/* ####
## npc_cultist_bombardir
#### */

class npc_cultist_bombardir : public CreatureScript
{
public:
    npc_cultist_bombardir() : CreatureScript("npc_cultist_bombardir") { }

    struct npc_cultist_bombardirAI : public ScriptedAI
    {
        npc_cultist_bombardirAI(Creature* creature) : ScriptedAI(creature) { }
        
        EventMap events;
        uint32 checkTimer;

        void Reset()
        {
            events.Reset();
            events.ScheduleEvent(EVENT_THROW_DYNAMITE, urand(5,8)*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_TIME_BOMB, urand(18,20)*IN_MILLISECONDS);

            checkTimer = 1*IN_MILLISECONDS;
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            me->DespawnOrUnsummon(5000);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
            {
                if (checkTimer <= diff)
                {
                    // if we are not on chillmaw, something went wrong -> despawn
                    if (Creature* chillmaw = me->FindNearestCreature(NPC_CHILLMAW, 10.0f))
                    {
                        if (!me->IsOnVehicle(chillmaw) && !me->IsInCombat())
                            me->DespawnOrUnsummon();
                    }
                    else 
                        me->DespawnOrUnsummon();

                    checkTimer = 5*IN_MILLISECONDS;
                }
                else
                    checkTimer -= diff;
                return;
            }

            //events
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_THROW_DYNAMITE:
                    DoCastVictim(SPELL_THROW_DYNAMITE);
                    events.RescheduleEvent(EVENT_THROW_DYNAMITE, urand(5,8)*IN_MILLISECONDS);
                    break;
                case EVENT_TIME_BOMB:
                    DoCastVictim(SPELL_TIME_BOMB);
                    events.RescheduleEvent(EVENT_TIME_BOMB, urand(6,7)*IN_MILLISECONDS);
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_cultist_bombardirAI (creature);
    }
};

/*###
## npc_thrall_at_ground
###*/

class npc_thrall_at_ground : public CreatureScript
{
public:
    npc_thrall_at_ground() : CreatureScript("npc_thrall_at_ground") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_thrall_at_groundAI (creature);
    }

    struct npc_thrall_at_groundAI : public npc_escortAI
    {
        npc_thrall_at_groundAI(Creature* creature) : npc_escortAI(creature), summons(me) 
        {
            me->setActive(true);
            me->SetReactState(REACT_PASSIVE);
        }

        uint32 action;
        uint32 actionTimer;
        bool canTalk;
        SummonList summons;

        void AttackStart(Unit* /*who*/) {}
        void EnterCombat(Unit* /*who*/) {}

        void Reset()
        {
            me->setActive(true);
            action = 0;
            actionTimer = 0;
            canTalk = false;
            Start(false);
            summons.DespawnAll();
            Summon();
        }

        void CorpseRemoved(uint32& /*respawnDelay*/) 
        {
            summons.DespawnAll();
        }

        void JustSummoned(Creature* summon)
        {
            summon->setActive(true);
        }

        void Summon()
        {
            //summon garrosh and following wrynn
            if (Creature* garrosh = me->SummonCreature(35372, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
            {
                garrosh->GetMotionMaster()->MoveFollow(me, 1.0f, M_PI_F*0.75f);
                summons.Summon(garrosh); //first position in list for following actions

                //summon leader guard with following garrosh
                if (Creature* leaderGuard = me->SummonCreature(35460, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
                {
                    leaderGuard->GetMotionMaster()->MoveFollow(garrosh, 6.0f, M_PI_F*0.75f);
                    summons.Summon(leaderGuard);

                    //summon three guards with diffrent angles and following leader guard
                    if (Creature* guard = me->SummonCreature(35460, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
                    {
                        guard->GetMotionMaster()->MoveFollow(leaderGuard, 1.0f, M_PI_F*0.50f);
                        summons.Summon(guard);
                    }

                    if (Creature* guard = me->SummonCreature(35460, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
                    {
                        guard->GetMotionMaster()->MoveFollow(leaderGuard, 1.0f, M_PI_F*0.75f);
                        summons.Summon(guard);
                    }

                    if (Creature* guard = me->SummonCreature(35460, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
                    {
                        guard->GetMotionMaster()->MoveFollow(leaderGuard, 1.0f, M_PI_F*1.0f);
                        summons.Summon(guard);
                    }
                }
            }

            //summon Tirion
            if (Creature* tirion = me->SummonCreature(35361, 8514.146f, 998.494f, 547.470f, 1.571f))
                summons.Summon(tirion);
        }
        
        void WaypointReached(uint32 wp)
        {
            switch (wp)
            {
                case 7:
                    if (Creature* beluus = Find(35501, 200.0f))
                        beluus->AI()->Talk(1);
                    break;
                case 11:
                    SetEscortPaused(true);
                    canTalk = true;
                    break;
                case 16:
                    SetEscortPaused(true);
                    if (Creature* garrosh = GetGarrosh())
                    {
                        garrosh->GetMotionMaster()->MovePoint(0, 8512.238f, 1009.026f, 547.470f);
                        if (Creature* tirion = GetTirion())
                            garrosh->SetTarget(tirion->GetGUID());
                    }
                    canTalk = true;
                    break;
             }
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (!canTalk)
                return;

            if (actionTimer <= diff)
            {
                switch (action)
                {
                    case 0:
                        SetActionTime(3);
                        action++;
                        break;
                    case 1:
                        if (GetGarrosh())
                            GetGarrosh()->SetOrientation(3.0f);
                        me->SetOrientation(3.0f);
                        SetActionTime(2);
                        action++;
                        break;
                    case 2:
                        me->AI()->Talk(0);
                        SetActionTime(13);
                        action++;
                        break;
                    case 3:
                        if (GetGarrosh())
                            GetGarrosh()->AI()->Talk(0);
                        SetActionTime(13);
                        action++;
                        break;
                    case 4:
                        me->AI()->Talk(1);
                        SetActionTime(9);
                        action++;
                        break;
                    case 5:
                        if (GetGarrosh())
                            GetGarrosh()->AI()->Talk(1);
                        SetActionTime(9);
                        action++;
                        break;
                    case 6:
                        SetEscortPaused(false);
                        canTalk = false;
                        SetActionTime(2);
                        action++;
                        break;
                    case 7:
                        if (GetTirion())
                            GetTirion()->AI()->Talk(3);
                        SetActionTime(8);
                        action++;
                        break;
                    case 8:
                        me->AI()->Talk(2);
                        SetActionTime(10);
                        action++;
                        break;
                    case 9:
                        if (GetGarrosh())
                            GetGarrosh()->AI()->Talk(2);
                        SetActionTime(4);
                        action++;
                        break;
                    case 10:
                        if (GetTirion())
                            GetTirion()->AI()->Talk(4);
                        SetActionTime(10);
                        action++;
                        break;
                    case 11:
                        me->AI()->Talk(3);
                        SetActionTime(4);
                        action++;
                        break;
                    case 12:
                        if (GetGarrosh())
                            GetGarrosh()->AI()->Talk(3);
                        SetActionTime(13);
                        action++;
                        break;
                    case 13:
                        me->AI()->Talk(4);
                        SetActionTime(3);
                        action++;
                        break;
                    case 14:
                        if (GetTirion())
                            GetTirion()->AI()->Talk(5);
                        SetActionTime(6);
                        action++;
                        break;
                    case 15:
                        me->AI()->Talk(5);
                        SetActionTime(9);
                        action++;
                        break;
                    case 16:
                        if (GetGarrosh())
                            GetGarrosh()->AI()->Talk(4);
                        SetActionTime(2);
                        action++;
                        break;
                    case 17:
                        if (GetTirion())
                            GetTirion()->AI()->Talk(6);
                        SetActionTime(4);
                        action++;
                        break;
                    case 18:
                        SetEscortPaused(false);
                        if (Creature* tirion = GetTirion())
                        {
                            tirion->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);
                            tirion->GetMotionMaster()->MovePoint(0, 8515.24f, 853.895f, 558.142f);
                        }
                        if (GetGarrosh())
                            GetGarrosh()->GetMotionMaster()->MoveFollow(me, 1.0f, M_PI_F*0.75f);
                        SetActionTime(0);
                        canTalk = false;
                        break;
                }
            }
            else actionTimer -= diff;
        }
        
        Creature* Find(uint32 entry, float range = 10.0f)
        {
            if (Creature* c = me->FindNearestCreature(entry, range))
                return c;
            return NULL;
        }

        Creature* GetGarrosh()
        {
            if (Creature* c = ObjectAccessor::GetCreature(*me, *summons.begin()))
                return c;
            return NULL;
        }

        Creature* GetTirion()
        {
            if (Creature* c = Find(35361, 20.0f))
                return c;
            return NULL;
        }

        void SetActionTime(uint32 time)
        {
            actionTimer = time * IN_MILLISECONDS;
        }
    };
};

/*###
## npc_wrynn_at_ground
###*/

class npc_wrynn_at_ground : public CreatureScript
{
public:
    npc_wrynn_at_ground() : CreatureScript("npc_wrynn_at_ground") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wrynn_at_groundAI (creature);
    }

    struct npc_wrynn_at_groundAI : public npc_escortAI
    {
        npc_wrynn_at_groundAI(Creature* creature) : npc_escortAI(creature), summons(me) 
        {
            me->setActive(true);
            me->SetReactState(REACT_PASSIVE);
        }

        uint32 action;
        uint32 actionTimer;
        bool canTalk;
        SummonList summons;

        void AttackStart(Unit* /*who*/) {}
        void EnterCombat(Unit* /*who*/) {}

        void Reset()
        {
            me->setActive(true);
            action = 0;
            actionTimer = 0;
            canTalk = false;
            Start(false);
            summons.DespawnAll();
            Summon();
        }

        void CorpseRemoved(uint32& /*respawnDelay*/) 
        {
            summons.DespawnAll();
        }
                
        void JustSummoned(Creature* summon)
        {
            summon->setActive(true);
        }

        void Summon()
        {
            //summon jaina and following wrynn
            if (Creature* jaina = me->SummonCreature(35320, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
            {
                jaina->GetMotionMaster()->MoveFollow(me, 1.0f, M_PI_F*0.75f);
                summons.Summon(jaina); //first position in list for following actions

                //summon leader guard with following jaina
                if (Creature* leaderGuard = me->SummonCreature(35322, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
                {
                    leaderGuard->GetMotionMaster()->MoveFollow(jaina, 6.0f, M_PI_F*0.75f);
                    summons.Summon(leaderGuard);

                    //summon three guards with diffrent angles and following leader guard
                    if (Creature* guard = me->SummonCreature(35322, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
                    {
                        guard->GetMotionMaster()->MoveFollow(leaderGuard, 1.0f, M_PI_F*0.50f);
                        summons.Summon(guard);
                    }

                    if (Creature* guard = me->SummonCreature(35322, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
                    {
                        guard->GetMotionMaster()->MoveFollow(leaderGuard, 1.0f, M_PI_F*0.75f);
                        summons.Summon(guard);
                    }

                    if (Creature* guard = me->SummonCreature(35322, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()))
                    {
                        guard->GetMotionMaster()->MoveFollow(leaderGuard, 1.0f, M_PI_F*1.00f);
                        summons.Summon(guard);
                    }
                }
            }

            //summon Tirion
            if (Creature* tirion = me->SummonCreature(35361, 8514.146f, 998.494f, 547.470f, 1.571f))
                summons.Summon(tirion);
        }

        void WaypointReached(uint32 wp)
        {
            switch (wp)
            {
                case 5:
                    if (Creature* beluus = Find(35501, 200.0f))
                        beluus->AI()->Talk(0);
                    break;
                case 14:
                    SetEscortPaused(true);
                    if (Creature* jaina = GetJaina())
                    {
                        jaina->GetMotionMaster()->MovePoint(0, 8512.238f, 1009.026f, 547.470f);
                        if (Creature* tirion = GetTirion())
                            jaina->SetTarget(tirion->GetGUID());
                    }
                    canTalk = true;
                    break;
             }
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (!canTalk)
                return;

            if (actionTimer <= diff)
            {
                switch (action)
                {
                    case 0:
                        SetActionTime(2);
                        action++;
                        break;
                    case 1:
                        me->AI()->Talk(0);
                        SetActionTime(3);
                        action++;
                        break;
                    case 2:
                        if (GetTirion())
                            GetTirion()->AI()->Talk(0);
                        SetActionTime(8);
                        action++;
                        break;
                    case 3:
                        if (GetJaina())
                            GetJaina()->AI()->Talk(0);
                        SetActionTime(8);
                        action++;
                        break;
                    case 4:
                        me->AI()->Talk(1);
                        SetActionTime(11);
                        action++;
                        break;
                    case 5:
                        if (GetTirion())
                            GetTirion()->AI()->Talk(1);
                        SetActionTime(12);
                        action++;
                        break;
                    case 6:
                        if (GetJaina())
                            GetJaina()->AI()->Talk(1);
                        SetActionTime(5);
                        action++;
                        break;
                    case 7:
                        me->AI()->Talk(2);
                        SetActionTime(8);
                        action++;
                        break;
                    case 8:
                        me->AI()->Talk(3);
                        SetActionTime(12);
                        action++;
                        break;
                    case 9:
                        if (GetTirion())
                            GetTirion()->AI()->Talk(2);
                        SetActionTime(6);
                        action++;
                        break;
                    case 10:
                        if (Creature* tirion = GetTirion())
                        {
                            tirion->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);
                            tirion->GetMotionMaster()->MovePoint(0, 8515.24f, 853.895f, 558.142f);
                        }
                        SetActionTime(2);
                        action++;
                        break;
                    case 11:
                        SetEscortPaused(false);
                        if (GetJaina())
                            GetJaina()->GetMotionMaster()->MoveFollow(me, 1.0f, M_PI_F*0.75f);
                        canTalk = false;
                        SetActionTime(2);
                        action++;
                        break;
                }
            }
            else actionTimer -= diff;
        }
        
        Creature* Find(uint32 entry, float range = 10.0f)
        {
            if (Creature* c = me->FindNearestCreature(entry, range))
                return c;
            return NULL;
        }

        Creature* GetJaina()
        {
            if (Creature* c = ObjectAccessor::GetCreature(*me, *summons.begin()))
                return c;
            return NULL;
        }

        Creature* GetTirion()
        {
            if (Creature* c = Find(35361, 20.0f))
                return c;
            return NULL;
        }

        void SetActionTime(uint32 time)
        {
            actionTimer = time * IN_MILLISECONDS;
        }
    };
};

class FlamingSpearTargets
{
    public:
        bool operator()(WorldObject* object) const
        {
            return object->GetEntry() != NPC_NORTH_SEA_KRAKEN || object->GetEntry() != NPC_KVALDIR_DEEPCALLER;
        }
};

class spell_flaming_spear : public SpellScriptLoader
{
    public:
        spell_flaming_spear() : SpellScriptLoader("spell_flaming_spear") { }

        class spell_flaming_spear_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_flaming_spear_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(FlamingSpearTargets());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_flaming_spear_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_TARGET_ANY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_flaming_spear_SpellScript();
        }
};

class spell_campaign_warhorse_abilities : public SpellScriptLoader
{
    public:
        spell_campaign_warhorse_abilities() : SpellScriptLoader("spell_campaign_warhorse_abilities") { }

        class spell_campaign_warhorse_abilities_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_campaign_warhorse_abilities_SpellScript);

            SpellCastResult CheckCast()
            {
                if (Unit* target = GetExplTargetUnit())
                    if (target->GetTypeId() == TYPEID_PLAYER)
                        return SPELL_FAILED_BAD_TARGETS;
                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_campaign_warhorse_abilities_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_campaign_warhorse_abilities_SpellScript();
        }
};

void AddSC_argent_tournament()
{
    new npc_dame_evniki_kapsalis();
    new npc_squire_david();
    new npc_argent_valiant();
    new npc_tournament_training_dummy();
    new npc_squire_danny;
    new npc_argent_champion;
    new npc_champion_valiant;
    new npc_black_knight;
    new npc_squire_cavin;
    new npc_boneguard_commander;
    new npc_boneguard_lieutenant;
    new npc_north_sea_krake;
    new spell_q14112_14145_chum_the_water();
    new spell_q14076_14092_pund_drum();
    new go_stormforged_mole_machine();
    new npc_black_knights_grave();
    new npc_cult_assassin();
    new npc_argent_squire();
    new npc_lake_frog();
    new npc_maiden_of_ashwood_lake();
    new npc_maiden_of_drak_mar();
    new npc_black_knight_s_gryphon();
    new npc_scourge_converter();
    new npc_fallen_hero_s_spirit();
    new npc_chillmaw();
    new npc_cultist_bombardir();
    new npc_thrall_at_ground();
    new npc_wrynn_at_ground();
    new spell_flaming_spear();
    new spell_campaign_warhorse_abilities();
}
