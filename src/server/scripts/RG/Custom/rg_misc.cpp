#include "SharedDefines.h"
#include "WorldSession.h"
#include "ScriptMgr.h"
#include "Battleground.h"
#include "Group.h"
#include "GuildMgr.h"
#include "GameObjectAI.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "SpellAuras.h"
#include "Chat.h"
#include "ScriptedGossip.h"
#include "InstanceScript.h"
#include "GameTime.h"
#include "DBCStores.h"

class GeneralPlayerScript : public PlayerScript
{
public:
    GeneralPlayerScript() : PlayerScript("rg_aura_remover") {}

    void OnMapChanged(Player* player)
    {
        player->RemoveAurasDueToSpell(42013); // QAE Charm

        player->RemoveAurasDueToSpell(59414); // Pulsierende Schockwellenaura

        player->RemoveAurasDueToSpell(64169); // Brunnen der geistigen Gesundheit

        player->RemoveAurasDueToSpell(64125); // Quetschen

        player->RemoveAurasDueToSpell(57055); // Mini

        player->RemoveAurasDueToSpell(46432); // Macht des Sturms

        player->RemoveAurasDueToSpell(48278); // Paralysieren

        player->RemoveAurasDueToSpell(29660); // Negative Aufladung
        player->RemoveAurasDueToSpell(29659); // Positive Aufladung

        player->RemoveAurasDueToSpell(46392); // fukusierter angriff (bg-ws)
        player->RemoveAurasDueToSpell(46393); // brutaler angriff (bg-ws)
        player->RemoveAurasDueToSpell(70871); // Essenz der Blutk�nigin
        player->RemoveAurasDueToSpell(71822); // Schattenresonanz

        player->RemoveAura(73077); // Goblin Rocket Pack
        player->DestroyItemCount(49278, 1, true); // Goblin Rocket Pack

        /// These buffs are to strong to be allowed during active progress
        player->RemoveAurasDueToSpell(57524); // Metanoia (by NPC 29569)
        player->RemoveAurasDueToSpell(54483); // Vigor of Sseratus
        player->RemoveAurasDueToSpell(59617); // Bolthorn's Rune of Flame
        player->RemoveAurasDueToSpell(59407); // Shadows in the Dark
        player->RemoveAurasDueToSpell(51605); // Zeal
        player->RemoveAurasDueToSpell(51777); // Arcane Focus
        player->RemoveAurasDueToSpell(51800); // Might of Malygos
        player->RemoveAurasDueToSpell(60964); // Warchief Blessing
        player->RemoveAurasDueToSpell(61011); // Warchief Blessing
        player->RemoveAurasDueToSpell(64670); // Thralls Blessing
        player->RemoveAurasDueToSpell(59756); // Sylvanas Blessing
        if (player->getClass() != CLASS_WARLOCK)
            player->RemoveAurasDueToSpell(44977); // Fel Armor
        player->RemoveAurasDueToSpell(22817); // Fengus' Ferocity
        player->RemoveAurasDueToSpell(22818); // Mol'dar's Moxie
        player->RemoveAurasDueToSpell(22888); // Rallying Cry of the Dragonslayer
        player->RemoveAurasDueToSpell(22820); // Slip'kik's Savvy
        player->RemoveAurasDueToSpell(24425); // Spirit of Zandalar
        player->RemoveAurasDueToSpell(65636); // Vehicle Scaling
        player->RemoveAurasDueToSpell(60683); // Vehicle Scaling
    }

    void OnQuestStatusChange(Player* player, uint32 questId, QuestStatus status) 
    { 
        if (status != QUEST_STATUS_INCOMPLETE)
            return;

        switch (questId)
        {
            case 11527:
                if (!player->HasItemCount(34387))
                    player->AddItem(34387, 5);
                break;
            case 10284:
                player->KilledMonsterCredit(20156);
                break;
            case 12498:
                if (!player->HasItemCount(38305))
                    player->AddItem(38305, 1);
                for (uint32 i = 0; i < 30; i++)
                    player->KilledMonsterCredit(28005);
                break;
            case 110211:
                player->AreaExploredOrEventHappens(110211);
                break;
            default: break;
        }
    }
};

enum PlayerLoginAuraMisc
{
    SPELL_LFG_DEBUFF   = 71328,
    SPELL_GYMERS_BUDDY = 55430
};

class PlayerLoginAuraCheck : public PlayerScript
{
    public:
        PlayerLoginAuraCheck() : PlayerScript("PlayerLoginAuraCheck") { }
    
        void OnLogin(Player* player, bool /*firstLogin*/)
        {
            if (game::time::GetUptime() <= 900)
                player->RemoveAurasDueToSpell(SPELL_LFG_DEBUFF);

            player->RemoveAurasDueToSpell(SPELL_GYMERS_BUDDY);
        }
};

enum BuffstackingSpells
{
    SPELL_LEADER_OF_THE_PACK = 24932,
    SPELL_TOBEN              = 29801,
    SPELL_OVERKILL           = 58427,
    SPELL_MASTER_OF_SUBTLETY = 31665,
    SPELL_TOTEM_OF_WRATH     = 63283,
    SPELL_ARCANE_POTENCY_1   = 57529,
    SPELL_ARCANE_POTENCY_2   = 57531,
    SPELL_BARSKIN_EFFECT     = 66530,
    SPELL_IMPROVED_BARSKIN_1 = 63410,
    SPELL_IMPROVED_BARSKIN_2 = 63411,

    // Seals
    SPELL_SEAL_OF_VENGEANCE       = 31801,
    SPELL_SEAL_OF_CORRUPTION      = 53736,
    SPELL_SEAL_OF_WISDOM          = 54940,
    SPELL_SEAL_OF_LIGHT           = 20165,
    SPELL_INNER_FIRE              = 588
};

static const uint32 blessings[] = { 19740, 20042, 25782, 19742, 20244, 25894}; // SMD/Verbesserter SDM/GSDM//SDW Verbesserter SDW/ Gro�er SDW
static const uint32 paladinAuras[] = { 465, 20138, 19746, 20254}; // Aura der Hingabe Rang 1, Verbesserte Aura der Hingabe / Konzentration , verbesserte Konzentration
static const uint32 seals[] = { 20187, 31801, 31803, 31804, 53733, 53736, 53742 }; //Richturteil der Rechtschaffenheit, Siegel der Vergeltung, Heilige Vergeltung, Richturteil der Vergeltung, Richturteil der Verderbnis, Siegel der Verderbnis, Blutverderbnis
static const uint32 warriorShouts[] = { 469, 1160, 6673 }; // Befehlsruf, Demoralisierender, Schlachtruf
static const uint32 iDeffStance = 29594; //Verbesserte Verteidigungshaltung
static const uint32 iBerserkStance = 7381; //Verbesserte Berserkerhaltung

static const uint32 hunter[] = {13165,61846,13163,5118,13159,34074,34666,1130, 1494, 19878, 19879, 19880,19882, 19884};//Verbesserter Aspekt des Falken, Aspekt des Drachenfalken, Verbesserter Aspekt des Affen, Orientierung, Meister der Aspekte, Tiere abrichten, Verbessertes Mal des Jaegers, Verbessertes Faehrtenlesen
static const uint32 iVilePoisons = 32645;

static const uint32 iImprovedPowerwordFortitude = 1243; //Verbessertes Machtwort: Seelenstaerke, Singlebuff
static const uint32 iImprovedPowerwordFortutudeRaidbuff = 21562; //Verbessertes Machtwort: Seelenstaerke Raidbuff
static const uint32 iImprovedInnerFire = 588; //Verbessertes inneres feuer

static const uint32 shaman[] = { 324, 52127, 51940, 8026, 974}; //Blitzschild, Wasserschild, Waffe der Lebensgeister, Waffe der Flammenzunge
static const uint32 warlock[] = { 18693, 30149, 706, 28176, 28189, 47892, 47893, 30147, 55151, 32795, 55195, 30147 }; //Verbesserter Gesundheitsstein, Gezaehmtes Tier, Daemonenr�stung, Teufelsr�stung, Feuerstein, Zauberstein, Nochmal gezaehmtes tier.. Warum auch immer

class ChangeTalentSpecPlayerScript : public PlayerScript
{
public:
    ChangeTalentSpecPlayerScript() : PlayerScript("rg_buffstacking") { }

    void OnSwitchTalentSpec(Player* player, uint8 /*spec*/)
    {
        // remove several auras when player changes specc, to avoid buguse
        // Buffstacking purposes for passive talents and active stacking spells
        switch (player->getClass())
        {

        case CLASS_WARRIOR:
            if (player->HasAura(SPELL_TOBEN))
            {
                player->RemoveAppliedAuras(SPELL_TOBEN, [&](AuraApplication const* app) { return app->GetBase()->GetCasterGUID() != player->GetGUID(); });
                player->RemoveAurasDueToSpell(SPELL_LEADER_OF_THE_PACK);
            }
            if (Aura* a = player->GetAuraOfRankedSpell(iBerserkStance, player->GetGUID()))
                a->RecalculateAmountOfEffects();
            else if (Aura* a = player->GetAuraOfRankedSpell(iDeffStance, player->GetGUID()))
                a->RecalculateAmountOfEffects();

            if (Group* group = player->GetGroup())
            {
                for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                {
                    if (Player* member = itr->GetSource())
                    {
                        for (uint32 i = 0; i < sizeof(warriorShouts) / sizeof(uint32); i++)
                        {
                            if (Aura* a = member->GetAuraOfRankedSpell(warriorShouts[i], player->GetGUID()))
                                a->RecalculateAmountOfEffects();
                        }
                    }
                }
            }
            break;
        case CLASS_PALADIN:
            player->RemoveAurasDueToSpell(SPELL_SEAL_OF_CORRUPTION);
            player->RemoveAurasDueToSpell(SPELL_SEAL_OF_VENGEANCE);
            player->RemoveAurasDueToSpell(SPELL_SEAL_OF_WISDOM);
            player->RemoveAurasDueToSpell(SPELL_SEAL_OF_LIGHT);

            if (Group* group = player->GetGroup())
            {
                if (Aura* a = player->GetAuraOfRankedSpell(25780, player->GetGUID())) //Zorn
                    a->RecalculateAmountOfEffects();

                for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                {
                    if (Player* member = itr->GetSource())
                    {
                        for (uint32 i = 0; i < sizeof(seals) / sizeof(uint32); i++) //SDV, Dot und Richturteil
                        {
                            if (Aura* a = member->GetAuraOfRankedSpell(seals[i], player->GetGUID()))
                                a->RecalculateAmountOfEffects();
                        }

                        for (uint32 i = 0; i < sizeof(paladinAuras) / sizeof(uint32); i++)
                        {
                            if (Aura* a = member->GetAuraOfRankedSpell(paladinAuras[i], player->GetGUID()))
                                a->RecalculateAmountOfEffects();
                        }

                        for (uint32 i = 0; i < sizeof(blessings) / sizeof(uint32); i++)
                        {
                            if (Aura* a = member->GetAuraOfRankedSpell(blessings[i], player->GetGUID()))
                                a->RecalculateAmountOfEffects();
                        }
                    }
                }
            }
            break;
        case CLASS_HUNTER:
            for (uint32 i = 0; i < sizeof(hunter) / sizeof(uint32); i++)
            {
                if (Aura* a = player->GetAuraOfRankedSpell(hunter[i], player->GetGUID()))
                    a->RecalculateAmountOfEffects();
            }
            break;
        case CLASS_ROGUE:
            if (player->HasAura(SPELL_OVERKILL))
                player->RemoveAurasDueToSpell(SPELL_OVERKILL);
            if (player->HasAura(SPELL_MASTER_OF_SUBTLETY))
                player->RemoveAurasDueToSpell(SPELL_MASTER_OF_SUBTLETY);
            if (Aura* a = player->GetAuraOfRankedSpell(iVilePoisons, player->GetGUID()))
                a->RecalculateAmountOfEffects();
            break;            
        case CLASS_PRIEST:
            if (Aura* a = player->GetAuraOfRankedSpell(iImprovedInnerFire, player->GetGUID()))
                a->RecalculateAmountOfEffects();

            if (Aura* a = player->GetAuraOfRankedSpell(iImprovedPowerwordFortitude, player->GetGUID()))
                a->RecalculateAmountOfEffects();

            if (Aura* a = player->GetAuraOfRankedSpell(iImprovedPowerwordFortutudeRaidbuff, player->GetGUID()))
                a->RecalculateAmountOfEffects();

            if (Aura* a = player->GetAuraOfRankedSpell(SPELL_INNER_FIRE, player->GetGUID()))
                a->Remove();
            break;
        case CLASS_DEATH_KNIGHT:
            if (Aura* a = player->GetAuraOfRankedSpell(48266, player->GetGUID()))
                a->RecalculateAmountOfEffects();
            if (Aura* a = player->GetAuraOfRankedSpell(48263, player->GetGUID()))
                a->RecalculateAmountOfEffects();
            break;
        case CLASS_SHAMAN:
            player->RemoveAurasDueToSpell(SPELL_TOTEM_OF_WRATH);
            for (uint32 i = 0; i < sizeof(shaman) / sizeof(uint32); i++)
            {
                if (Aura* a = player->GetAuraOfRankedSpell(shaman[i], player->GetGUID()))
                    a->RecalculateAmountOfEffects();
            }
            break;
        case CLASS_MAGE:
            player->RemoveAurasDueToSpell(SPELL_ARCANE_POTENCY_1);
            player->RemoveAurasDueToSpell(SPELL_ARCANE_POTENCY_2);
            break;
        case CLASS_WARLOCK:
            for (uint32 i = 0; i < sizeof(warlock) / sizeof(uint32); i++)
            {
                if (Aura* a = player->GetAuraOfRankedSpell(warlock[i], player->GetGUID()))
                    a->RecalculateAmountOfEffects();
            }
            break;
        case CLASS_DRUID:
            if (Group* group = player->GetGroup())
            {
                if (Aura* a = player->GetAuraOfRankedSpell(48412, player->GetGUID()))
                    a->RecalculateAmountOfEffects();

                for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                {
                    if (Player* member = itr->GetSource())
                    {
                        if (Aura* a = member->GetAuraOfRankedSpell(1126, player->GetGUID()))
                            a->RecalculateAmountOfEffects();
                        if (Aura* a = member->GetAuraOfRankedSpell(21849, player->GetGUID()))
                            a->RecalculateAmountOfEffects();

                    }
                }
            }
            player->RemoveAurasDueToSpell(SPELL_BARSKIN_EFFECT);
            if (player->HasAura(SPELL_IMPROVED_BARSKIN_1) || player->HasAura(SPELL_IMPROVED_BARSKIN_2))
                player->CastSpell(player, SPELL_BARSKIN_EFFECT, true);
            break;
        default:
            break;
        }
    }

};

//RG Custom "Verstecktes Anmelden"
#define SQ_GOSSIP_MENU 60091
enum SecretQueuingNpcTexts
{
    SQ_NPC_TEXT1 = 2002061,
    SQ_NPC_TEXT2,
    SQ_NPC_TEXT3
};

bool CheckGroup(Player* player, bool map)
{
    if (player->IsGameMaster() || sWorld->getBoolConfig(CONFIG_ARENA_SOLOQUEUE_ENABLED))
        return true;

    bool res = false;
    if (Group* g = player->GetGroup())
    {
        uint32 c = g->GetMembersCount();
        if (c == 2 || c == 3 || c == 5)
        {
            uint8 slot = (c == 5 ? 2 : (c == 3));
            uint32 id = player->GetArenaTeamId(slot);
            if (ArenaTeam* at = sArenaTeamMgr->GetArenaTeamById(id))
            {
                res = true;
                for (Group::MemberSlotList::const_iterator itr = g->GetMemberSlots().begin(); itr != g->GetMemberSlots().end(); ++itr)
                {
                    Player* p = ObjectAccessor::FindPlayer(itr->guid);
                    if (!at->IsMember(itr->guid) || (map && (!p || !p->IsSecretQueuingActive())))
                    {
                        res = false;
                        break;
                    }
                }
            }
        }
    }
    return res;
}

class npc_secret_queuing : public CreatureScript
{
public:

    npc_secret_queuing() : CreatureScript("npc_secret_queuing")
    {
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->GetMapId() != 35)
        {
            player->AddGossipItem(SQ_GOSSIP_MENU, 1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->SEND_GOSSIP_MENU(SQ_NPC_TEXT1, creature->GetGUID());
        }
        else
        {
            player->AddGossipItem(SQ_GOSSIP_MENU, 2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(SQ_NPC_TEXT2, creature->GetGUID());
        }
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
        case GOSSIP_ACTION_INFO_DEF+1:
        {
            if (CheckGroup(player, false))
            {
                player->PlayerTalkClass->SendCloseGossip();
                player->TeleportTo(35, -96.682671f, 149.799850f, -40.380783f, 3.130642f);
            }
            else
            {
                player->SEND_GOSSIP_MENU(SQ_NPC_TEXT3, creature->GetGUID());
            }
            break;
        }
        case GOSSIP_ACTION_INFO_DEF+2:
            player->PlayerTalkClass->SendCloseGossip();
            player->TeleportTo(571, 5807.75f, 588.346985f, 660.939026f, 1.663f); //Dalaran
            break;
        }

        return true;
    }
};

enum SecretQueuingEvents
{
    EVENT_CHECK_PLAYERS = 1,
    EVENT_SCHEDULE_CHECK
};

enum SecretQueuingTimers
{
    TIMER_CHECK_PLAYERS = 300 * IN_MILLISECONDS,
    TIMER_SCHEDULE_CHECK = 10 * IN_MILLISECONDS
};

inline bool IsInvitedForArena(Player* player)
{
    return player->IsInvitedForBattlegroundQueueType(BATTLEGROUND_QUEUE_2v2)
        || player->IsInvitedForBattlegroundQueueType(BATTLEGROUND_QUEUE_3v3)
        || player->IsInvitedForBattlegroundQueueType(BATTLEGROUND_QUEUE_5v5);
}

class instance_secret_queuing : public InstanceMapScript
{
public:
    instance_secret_queuing() : InstanceMapScript("instance_secret_queuing", 35) { }

    struct instance_secret_queuing_InstanceMapScript : public InstanceScript
    {
        instance_secret_queuing_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            removingPlayers = false;
        }

        void OnPlayerEnter(Player* player) override
        {
            //if (!events.Empty() || (player && player->IsGameMaster()))
            //    return;
            //events.ScheduleEvent(EVENT_SCHEDULE_CHECK, TIMER_SCHEDULE_CHECK);
        }

        void OnPlayerRemove(Player* player) override
        {
            //if (!events.Empty() || removingPlayers || (player && (player->IsGameMaster() || IsInvitedForArena(player))))
            //    return;
            //events.ScheduleEvent(EVENT_SCHEDULE_CHECK, TIMER_SCHEDULE_CHECK);
        }

        void ScheduleCheckEvent()
        {
            if (events.Empty() && !removingPlayers)
            {
                events.ScheduleEvent(EVENT_CHECK_PLAYERS, TIMER_CHECK_PLAYERS);
                for (Player& player : instance->GetPlayers())
                {
                    if (player.GetSession())
                        ChatHandler(player.GetSession()).PSendSysMessage(120017, (uint32)(TIMER_CHECK_PLAYERS/60000));
                }
            }
        }

        void RemovePlayers()
        {
            removingPlayers = true;
            for (Player& player : instance->GetPlayers())
                player.TeleportTo(571, 5807.75f, 588.346985f, 660.939026f, 1.663f); //Dalaran
            removingPlayers = false;
        }

        bool CheckPlayers()
        {
            bool ok = true;
            for (Player& player : instance->GetPlayers())
            {
                if (IsInvitedForArena(&player))
                {
                    ok = true;
                    break;
                }
                else if (!CheckGroup(&player, true))
                    ok = false;
            }
            return ok;
        }

        void Update(uint32 diff) override
        {
            if (events.Empty())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CHECK_PLAYERS:
                {
                    if (!CheckPlayers())
                        RemovePlayers();
                    break;
                }
                case EVENT_SCHEDULE_CHECK:
                {
                    if (events.Empty() && !CheckPlayers())
                        ScheduleCheckEvent();
                    break;
                }
                default:
                    break;
                }
            }
        }

    protected:
        EventMap events;
        bool removingPlayers;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_secret_queuing_InstanceMapScript(map);
    }
};

class PlayerLoginPvPRank : public PlayerScript
{
    public:
        PlayerLoginPvPRank() : PlayerScript("PlayerLoginPvPRank") { }
    
        void OnLogin(Player* player, bool /*firstLogin*/)
        {
            player->GetSession()->SendNameQueryOpcode(player->GetGUID());
            uint32 rank = player->pvpData.GetRank();

            if (!rank)
                return;

            static const uint32 achievementsA[14] = { 442, 470, 471, 441, 440, 439, 472, 438, 437, 436, 435, 473, 434, 433 };
            static const uint32 achievementsH[14] = { 454, 468, 453, 450, 452, 451, 449, 469, 448, 447, 444, 446, 445, 443 };
            const uint32 *achievements = (player->GetOTeam() == ALLIANCE) ? achievementsA : achievementsH;
            const uint32 titleOffset = (player->GetOTeam() == ALLIANCE) ? 0 : 14;

            // if reward is disabled remove all titles (tmp fix)
            if (!sWorld->getBoolConfig(CONFIG_PVP_REWARD_TITLE))
            {
                for (uint32 i = 1; i <= 14; ++i)
                    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleOffset + i))
                        player->SetTitle(titleEntry, true);

                for (uint32 i = 0; i < 14; ++i)
                    player->RemoveAchievement(achievements[i]);

                return;
            }

            for (uint32 i = 1; i <= 14; i++)
            {
                if (rank != i)
                    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleOffset + i))
                        player->SetTitle(titleEntry, true);
            }
            if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleOffset + rank))
                if (!player->HasTitle(titleEntry))
                {
                    if (player->GetUInt32Value(PLAYER_CHOSEN_TITLE) == 0)
                        player->SetUInt32Value(PLAYER_CHOSEN_TITLE, titleEntry->bit_index);
                    player->SetTitle(titleEntry, false);
                }

            uint32 highestRank = 0;

            for (int32 i = 13; i >= 0; i--)
            {
                if (player->HasAchieved(achievements[i]))
                {
                    if (!highestRank)
                        highestRank = i + 1;
                    else
                        player->RemoveAchievement(achievements[i]);
                }
            }
            if (rank && rank > highestRank)
            {
                if (highestRank)
                    player->RemoveAchievement(achievements[highestRank - 1]);
                if (AchievementEntry const *TitleAchiev = sAchievementStore.LookupEntry(achievements[rank - 1]))
                    player->CompletedAchievement(TitleAchiev);
            }
        }
};

class PlayerLoginRubyEvent : public PlayerScript
{
    public:
        PlayerLoginRubyEvent() : PlayerScript("PlayerLoginRubyEvent") { }
    
        void OnLogin(Player* player, bool /*firstLogin*/)
        {
            if (player->IsActiveQuest(110211))
                player->AreaExploredOrEventHappens(110211);
        }
};


class frostmourne_item_vendor : public CreatureScript
{
public:
    frostmourne_item_vendor() : CreatureScript("rg_frostmourne_item_vendor")
    {
        fm_start_guid_ = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Fusion.Frostmourne.CharacterGuid.Start", 0));
        fm_end_guid_ = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Fusion.Frostmourne.CharacterGuid.End", 0));
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if ((player->GetGUID().GetCounter() >= fm_start_guid_ && player->GetGUID().GetCounter() <= fm_end_guid_)
            || player->IsGameMaster())
        {
            if (creature->IsVendor())
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR,
                        player->GetOptionTextWithEntry(GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_TEXT_BROWSE_GOODS_ID),
                        GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        player->PlayerTalkClass->SendGossipMenu(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->GetSession()->SendListInventory(creature->GetGUID());
                break;
            default:
                player->CLOSE_GOSSIP_MENU();
                break;
        }
        player->CLOSE_GOSSIP_MENU();
        return true;
    }
private:
    uint32 fm_start_guid_;
    uint32 fm_end_guid_;
};


class frostmourne_player_cleanup : public PlayerScript
{
    enum
    {
        ITEM_TITANGUARD_RECEIPE = 44945,
        ITEM_TITANGUARD_SCROLL = 44946,
        ENCHANTMENT_TITANGUARD = 3851,

        ITEM_TABARD_OF_THE_DEFENDER = 38314,
        ITEM_TABARD_OF_FROST = 23709,
        ITEM_CARVED_OGRE_IDOL = 49704,
        ITEM_CARVED_OGRE_IDOL_CREATE = 23716,

        SPELL_SWIFT_WHITE_MECHANOSTRIDER = 23223,
        ITEM_SWIFT_WHITE_MECHANOSTRIDER = 18773,
        SPELL_GREAT_GRAY_KODO = 23248,
        ITEM_GREAT_GRAY_KODO = 18795,

        SPELL_WINTERSPRING_FROSTSABER = 17229,
        ITEM_WINTERSPRING_FROSTSABER = 13086,
        SPELL_VENOMHIDE_RAVASAUR = 64659,
        ITEM_VENOMHIDE_RAVASAUR = 46102,

        FACTION_THE_DEFILERS = 510,
        ITEM_BATTLE_TABARD_OF_THE_DEFILERS = 20131,

        FACTION_THE_LEAGUE_OF_ARATHOR = 509,
        ITEM_ARATHOR_BATTLE_TABARD = 20132,
    };
public:
    frostmourne_player_cleanup() : PlayerScript("rg_frostmourne_player_cleanup")
    {
        fm_start_guid_ = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Fusion.Frostmourne.CharacterGuid.Start", 0));
        fm_end_guid_ = static_cast<uint32>(sConfigMgr->GetIntDefault("RG.Fusion.Frostmourne.CharacterGuid.End", 0));
    }

    void ExchangeItem(Player* player, uint32 from, uint32 to)
    {
        if (uint32 count = player->GetItemCount(from, true))
        {
            player->DestroyItemCount(from, count, true);
            player->AddItem(to, count);
        }
    }

    void ExchangeSpell(Player* player, uint32 from, uint32 to)
    {
        if (player->HasSpell(from))
        {
            player->RemoveSpell(from);
            player->LearnSpell(to, false);
        }
    }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        static constexpr auto all = std::numeric_limits<uint32>::max();

        player->DestroyItemCount(ITEM_TITANGUARD_RECEIPE, all, true);
        player->DestroyItemCount(ITEM_TITANGUARD_SCROLL, all, true);
        // todo: titanguard enchantment

        if (player->GetGUID().GetCounter() >= fm_start_guid_ && player->GetGUID().GetCounter() <= fm_end_guid_)
        {
            player->DestroyItemCount(ITEM_TABARD_OF_THE_DEFENDER, all, true);
            player->DestroyItemCount(ITEM_CARVED_OGRE_IDOL, all, true);
            player->DestroyItemCount(ITEM_CARVED_OGRE_IDOL_CREATE, all, true);
            player->DestroyItemCount(ITEM_TABARD_OF_FROST, all, true);

            if (player->GetOTeamId() == TEAM_HORDE)
            {
                ExchangeSpell(player, SPELL_SWIFT_WHITE_MECHANOSTRIDER, SPELL_GREAT_GRAY_KODO);
                ExchangeItem(player, ITEM_SWIFT_WHITE_MECHANOSTRIDER, ITEM_GREAT_GRAY_KODO);

                ExchangeSpell(player, SPELL_WINTERSPRING_FROSTSABER, SPELL_VENOMHIDE_RAVASAUR);
                ExchangeItem(player, ITEM_WINTERSPRING_FROSTSABER, ITEM_VENOMHIDE_RAVASAUR);
            }

            if (player->GetReputationRank(FACTION_THE_DEFILERS) < REP_EXALTED)
                player->DestroyItemCount(ITEM_BATTLE_TABARD_OF_THE_DEFILERS, all, true);

            if (player->GetReputationRank(FACTION_THE_LEAGUE_OF_ARATHOR) < REP_EXALTED)
                player->DestroyItemCount(ITEM_ARATHOR_BATTLE_TABARD, all, true);
        }
    }
private:
    uint32 fm_start_guid_;
    uint32 fm_end_guid_;
};

class GmIslandTradeEnableScript : public PlayerScript
{
public:
    GmIslandTradeEnableScript() : PlayerScript("GmIslandTradeEnableScript") { }

    void OnUpdateZone(Player* player, uint32 newZone, uint32 newArea, uint32 oldZone) override
    {
        constexpr uint32 ZONE_GM_ISLAND = 876;

        if (player->GetSession()->GetSecurity() > GMLEVEL_PREMIUM)
            return;

        if (newZone == ZONE_GM_ISLAND)
            player->setFaction(35);
        else if (oldZone == ZONE_GM_ISLAND)
        {
            player->setFactionForRace(player->getRace());
        }
    }
};

class npc_rg_arena_queue : public CreatureScript
{
public:

    npc_rg_arena_queue() : CreatureScript("npc_rg_arena_queue") { }

    bool OnGossipHello(Player* player, Creature*) override
    {
        uint32 rating = player->pvpData.GetArenaRating();
        uint32 groupRating = rating;
        Group* grp = player->GetGroup();
        if (grp)
        {
            uint32 memberCount = 0;
            groupRating = 0;
            for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member)
                    continue;

                groupRating += member->pvpData.GetArenaRating();
                memberCount++;
            }

            if (memberCount > 0)
                groupRating = uint32(std::round(float(groupRating) / float(memberCount)));
        }

        player->SendUpdateWorldState(10005, rating);
        player->SendUpdateWorldState(10006, groupRating);

        return false;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*uiSender*/, uint32 action)
    {
        switch (action)
        {
            case GOSSIP_OPTION_GOSSIP:
                player->PlayerTalkClass->ClearMenus();
                player->JoinArenaSoloQueue(true);
                player->CLOSE_GOSSIP_MENU();
                return true;
            default:
                return false;
        }
    }
};

void AddSC_custom_rg()
{
    new GeneralPlayerScript();
    new ChangeTalentSpecPlayerScript();
    new npc_secret_queuing();
    new instance_secret_queuing();
    new PlayerLoginAuraCheck();
    new PlayerLoginPvPRank();
    new PlayerLoginRubyEvent();
    new frostmourne_item_vendor();
    new frostmourne_player_cleanup();
    new GmIslandTradeEnableScript();
    new npc_rg_arena_queue();
}
