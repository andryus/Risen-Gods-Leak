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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "event.h"
#include "SpellHistory.h"
#include "DBCStores.h"

enum IronDwarfEvent
{
    SPELL_CREATE_MUG    =   42518,
    NPC_BREWFEST_REVELER    = 24484,
    NPC_GUZZLER             = 23709,
    NPC_GOB_TRIGGER         = 1000144,
    NPC_KEG_TRIGGER         = 1000149,
    NPC_HERALD              = 24536,
    NPC_WIRT                = 23685,
    GOB_MINION_MOLE_MACHINE = 194316,
    GOB_MAIN_MOLE_MACHINE   = 186880,
    GOB_MACHINE_STOMPEAKS   = 191549,
    GOB_HORDE_QUEST         = 189990,
    GOB_ALLIANZ_QUEST       = 189989,
    SPELL_KEG_MARKER        = 42761,
    SPELL_KNOCK_BACK        = 42299,
    SPELL_KEG_DRINK         = 42436,
    SPELL_ATTACK_KEG        = 42393,
    SPELL_SLEEP             = 50650,
    SPELL_THROW_MUG         = 42300,
    SPELL_TRINK             = 42436,
    SPELL_DRINK_WINE        = 11009,
    ITEM_KEG                = 33096,
    SPELL_BREWFEST_ATTACK   = 42393,
    TEXT_ATTACK             = 0
};

enum BrewfestQuestChugAndChuck
{
    QUEST_CHUG_AND_CHUCK_A  = 12022,
    QUEST_CHUG_AND_CHUCK_H  = 12191,
    NPC_BREWFEST_STOUT      = 24108,
};

class item_brewfest_ChugAndChuck : public ItemScript
{
public:
    item_brewfest_ChugAndChuck() : ItemScript("item_brewfest_ChugAndChuck") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/)
    {
        if (player->GetSpellHistory()->HasCooldown(SPELL_TRINK))
        {
            player->SendEquipError(EQUIP_ERR_CANT_DO_RIGHT_NOW, item, NULL);
            return true;
        }

        if (Unit * target = player->GetSelectedUnit())
        {
            if (target->GetEntry() == NPC_GUZZLER && player->IsInRange(target, 0.0f, 15.0f, true) && target->IsAlive())
            {
                player->GetSpellHistory()->AddCooldown(SPELL_TRINK, ITEM_KEG, std::chrono::seconds(3));
                player->CastSpell(player, SPELL_DRINK_WINE, true);
                uint8 success = urand(0,100);
                if (success > 75)
                {
                    player->CastSpell(target, SPELL_THROW_MUG, true);
                    target->SendSpellMiss(player, SPELL_THROW_MUG, SPELL_MISS_MISS);
                }
                else
                {
                    player->Kill(target);
                    player->CastSpell(target, SPELL_THROW_MUG, true);
                    target->ToCreature()->DespawnOrUnsummon(5000);
                    /*uint16 drunkvalue = (player->GetDrunkValue()+2560) > 0xFFFF ? 0xFFFF : player->GetDrunkValue()+2560;
                    player->SetDrunkValue(drunkvalue, ITEM_KEG);*/
                }
                if (Creature* wirt = player->FindNearestCreature(NPC_WIRT, 500.0f, true))
                {
                     wirt->AI()->DoCast(player, SPELL_CREATE_MUG);
                     wirt->AI()->DoCast(player, SPELL_THROW_MUG);
                }
                return true;
            }
        }


        if (player->GetQuestStatus(QUEST_CHUG_AND_CHUCK_A) == QUEST_STATUS_INCOMPLETE
            || player->GetQuestStatus(QUEST_CHUG_AND_CHUCK_H) == QUEST_STATUS_INCOMPLETE)
        {
            if (Creature* pStout = player->FindNearestCreature(NPC_BREWFEST_STOUT, 10.0f)) // spell range
            {
                return false;
            } else
                player->SendEquipError(EQUIP_ERR_OUT_OF_RANGE, item, NULL);
        } else
            player->SendEquipError(EQUIP_ERR_CANT_DO_RIGHT_NOW ,item, NULL);

        return true;
    }
};

/*####
## npc_brewfest_trigger
####*/
enum eBrewfestBarkQuests
{
    BARK_FOR_THE_THUNDERBREWS = 11294,
    BARK_FOR_TCHALIS_VOODOO_BREWERY = 11408,
    BARK_FOR_THE_BARLEYBREWS = 11293,
    BARK_FOR_DROHNS_DISTILLERY = 11407,
    SPELL_RAMSTEIN_SWIFT_WORK_RAM   = 43880,
    SPELL_BREWFEST_RAM = 43883,
    SPELL_RAM_FATIGUE = 43052,
    SPELL_SPEED_RAM_GALLOP = 42994,
    SPELL_SPEED_RAM_CANTER = 42993,
    SPELL_SPEED_RAM_TROT = 42992,
    SPELL_SPEED_RAM_NORMAL = 43310,
    SPELL_SPEED_RAM_EXHAUSED = 43332,
    NPC_SPEED_BUNNY_GREEN = 24263,
    NPC_SPEED_BUNNY_YELLOW = 24264,
    NPC_SPEED_BUNNY_RED = 24265,
    NPC_BARKER_BUNNY_1 = 24202,
    NPC_BARKER_BUNNY_2 = 24203,
    NPC_BARKER_BUNNY_3 = 24204,
    NPC_BARKER_BUNNY_4 = 24205,
};

class npc_brewfest_trigger : public CreatureScript
{
public:
    npc_brewfest_trigger() : CreatureScript("npc_brewfest_trigger") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_brewfest_triggerAI(creature);
    }

    struct npc_brewfest_triggerAI : public ScriptedAI
    {
        npc_brewfest_triggerAI(Creature* c) : ScriptedAI(c) {}

        void MoveInLineOfSight(Unit *who)
        {
            if (!who)
                return;

            if (who->GetTypeId() == TYPEID_PLAYER)
            {
                Player* player = who->ToPlayer();
                if (!(player->GetAura(SPELL_BREWFEST_RAM)))
                    return;

                if (player->GetQuestStatus(BARK_FOR_THE_THUNDERBREWS) == QUEST_STATUS_INCOMPLETE||
                    player->GetQuestStatus(BARK_FOR_TCHALIS_VOODOO_BREWERY) == QUEST_STATUS_INCOMPLETE||
                    player->GetQuestStatus(BARK_FOR_THE_BARLEYBREWS) == QUEST_STATUS_INCOMPLETE||
                    player->GetQuestStatus(BARK_FOR_DROHNS_DISTILLERY) == QUEST_STATUS_INCOMPLETE)
                {
                    uint32 creditMarkerId = me->GetEntry();
                    if ((creditMarkerId >= 24202) && (creditMarkerId <= 24205))
                    {
                        // 24202: Brewfest Barker Bunny 1, 24203: Brewfest Barker Bunny 2, 24204: Brewfest Barker Bunny 3, 24205: Brewfest Barker Bunny 4
                        if (!player->GetReqKillOrCastCurrentCount(BARK_FOR_THE_BARLEYBREWS, creditMarkerId)||
                            !player->GetReqKillOrCastCurrentCount(BARK_FOR_THE_THUNDERBREWS, creditMarkerId)||
                            !player->GetReqKillOrCastCurrentCount(BARK_FOR_DROHNS_DISTILLERY, creditMarkerId)||
                            !player->GetReqKillOrCastCurrentCount(BARK_FOR_TCHALIS_VOODOO_BREWERY, creditMarkerId))
                            player->KilledMonsterCredit(creditMarkerId, me->GetGUID());
                        // Caso para quest 11293 que no se completa teniendo todas las marcas
                        if (player->GetReqKillOrCastCurrentCount(BARK_FOR_THE_BARLEYBREWS, NPC_BARKER_BUNNY_1)&&
                            player->GetReqKillOrCastCurrentCount(BARK_FOR_THE_BARLEYBREWS, NPC_BARKER_BUNNY_2)&&
                            player->GetReqKillOrCastCurrentCount(BARK_FOR_THE_BARLEYBREWS, NPC_BARKER_BUNNY_3)&&
                            player->GetReqKillOrCastCurrentCount(BARK_FOR_THE_BARLEYBREWS, NPC_BARKER_BUNNY_4))
                            player->CompleteQuest(BARK_FOR_THE_BARLEYBREWS);
                    }
                }
            }
        }
    };
};

class spell_brewfest_speed : public SpellScriptLoader
{
public:
    spell_brewfest_speed() : SpellScriptLoader("spell_brewfest_speed") {}

    class spell_brewfest_speed_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_brewfest_speed_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_RAM_FATIGUE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_RAMSTEIN_SWIFT_WORK_RAM))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_BREWFEST_RAM))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SPEED_RAM_GALLOP))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SPEED_RAM_CANTER))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SPEED_RAM_TROT))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SPEED_RAM_NORMAL))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SPEED_RAM_GALLOP))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SPEED_RAM_EXHAUSED))
                return false;
            return true;
        }

        void HandlePeriodicTick(AuraEffect const* aurEff)
        {
            if (GetId() == SPELL_SPEED_RAM_EXHAUSED)
                return;

            Player* pCaster = GetCaster()->ToPlayer();
            if (!pCaster)
                return;
            int i;
            switch (GetId())
            {
                case SPELL_SPEED_RAM_GALLOP:
                    for (i = 0; i < 5; i++)
                        pCaster->AddAura(SPELL_RAM_FATIGUE,pCaster);
                    break;
                case SPELL_SPEED_RAM_CANTER:
                    pCaster->AddAura(SPELL_RAM_FATIGUE,pCaster);
                    break;
                case SPELL_SPEED_RAM_TROT:
                    if (pCaster->HasAura(SPELL_RAM_FATIGUE))
                    {
                        if (pCaster->GetAura(SPELL_RAM_FATIGUE)->GetStackAmount() <= 2)
                            pCaster->RemoveAura(SPELL_RAM_FATIGUE);
                        else
                            pCaster->GetAura(SPELL_RAM_FATIGUE)->ModStackAmount(-2);
                    }
                    break;
                case SPELL_SPEED_RAM_NORMAL:
                    if (pCaster->HasAura(SPELL_RAM_FATIGUE))
                    {
                        if (pCaster->GetAura(SPELL_RAM_FATIGUE)->GetStackAmount() <= 4)
                            pCaster->RemoveAura(SPELL_RAM_FATIGUE);
                        else
                            pCaster->GetAura(SPELL_RAM_FATIGUE)->ModStackAmount(-4);
                    }
                    break;
            }

            switch (aurEff->GetId())
            {
                case SPELL_SPEED_RAM_TROT:
                    if (aurEff->GetTickNumber() == 4)
                        pCaster->KilledMonsterCredit(NPC_SPEED_BUNNY_GREEN);
                    break;
                case SPELL_SPEED_RAM_CANTER:
                    if (aurEff->GetTickNumber() == 8)
                        pCaster->KilledMonsterCredit(NPC_SPEED_BUNNY_YELLOW);
                    break;
                case SPELL_SPEED_RAM_GALLOP:
                    if (aurEff->GetTickNumber() == 8)
                        pCaster->KilledMonsterCredit(NPC_SPEED_BUNNY_RED);
                    break;
            }
            if (pCaster->HasAura(SPELL_RAM_FATIGUE))
                if (pCaster->GetAura(SPELL_RAM_FATIGUE)->GetStackAmount() >= 100)
                    pCaster->CastSpell(pCaster,SPELL_SPEED_RAM_EXHAUSED, false);
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Player* pCaster = GetCaster()->ToPlayer();
            if (!pCaster)
                return;

            if (pCaster->HasAura(SPELL_BREWFEST_RAM) || pCaster->HasAura(SPELL_RAMSTEIN_SWIFT_WORK_RAM))
            {
                if (GetId() == SPELL_SPEED_RAM_EXHAUSED)
                {
                    if (pCaster->HasAura(SPELL_RAM_FATIGUE))
                        pCaster->GetAura(SPELL_RAM_FATIGUE)->ModStackAmount(-15);
                }
                else if (!pCaster->HasAura(SPELL_RAM_FATIGUE) || pCaster->GetAura(SPELL_RAM_FATIGUE)->GetStackAmount() < 100)
                {
                    switch (GetId())
                    {
                        case SPELL_SPEED_RAM_GALLOP:
                            if (!pCaster->HasAura(SPELL_SPEED_RAM_EXHAUSED))
                                pCaster->CastSpell(pCaster, SPELL_SPEED_RAM_CANTER, false);
                            break;
                        case SPELL_SPEED_RAM_CANTER:
                            if (!pCaster->HasAura(SPELL_SPEED_RAM_GALLOP))
                                pCaster->CastSpell(pCaster, SPELL_SPEED_RAM_TROT, false);
                            break;
                        case SPELL_SPEED_RAM_TROT:
                            if (!pCaster->HasAura(SPELL_SPEED_RAM_CANTER))
                                pCaster->CastSpell(pCaster, SPELL_SPEED_RAM_NORMAL, false);
                            break;
                    }
                }
            }
        }

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Player* pCaster = GetCaster()->ToPlayer();

            if (!pCaster)
                return;

            switch (GetId())
            {
                case SPELL_SPEED_RAM_GALLOP:
                    pCaster->GetAura(SPELL_SPEED_RAM_GALLOP)->SetDuration(4000);
                    break;
                case SPELL_SPEED_RAM_CANTER:
                    pCaster->GetAura(SPELL_SPEED_RAM_CANTER)->SetDuration(4000);
                    break;
                case SPELL_SPEED_RAM_TROT:
                    pCaster->GetAura(SPELL_SPEED_RAM_TROT)->SetDuration(4000);
                    break;
             }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_brewfest_speed_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED, AURA_EFFECT_HANDLE_REAL);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_brewfest_speed_AuraScript::HandlePeriodicTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            OnEffectRemove += AuraEffectRemoveFn(spell_brewfest_speed_AuraScript::OnRemove, EFFECT_2, SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_brewfest_speed_AuraScript();
    }
};

class item_brewfest_ram_reins : public ItemScript
{
public:
    item_brewfest_ram_reins() : ItemScript("item_brewfest_ram_reins") { }

    bool OnUse(Player* player, Item* item, const SpellCastTargets & /*targets*/)
    {
        if ((player->HasAura(SPELL_BREWFEST_RAM) || player->HasAura(SPELL_RAMSTEIN_SWIFT_WORK_RAM)) && !player->HasAura(SPELL_SPEED_RAM_EXHAUSED))
        {
            if (player->HasAura(SPELL_SPEED_RAM_NORMAL))
                player->CastSpell(player,SPELL_SPEED_RAM_TROT,false);
            else if (player->HasAura(SPELL_SPEED_RAM_TROT))
            {
                if (player->GetAura(SPELL_SPEED_RAM_TROT)->GetDuration() < 3000)
                    player->GetAura(SPELL_SPEED_RAM_TROT)->SetDuration(4000);
                else
                  player->CastSpell(player,SPELL_SPEED_RAM_CANTER,false);
            } else if (player->HasAura(SPELL_SPEED_RAM_CANTER))
            {
                if (player->GetAura(SPELL_SPEED_RAM_CANTER)->GetDuration() < 3000)
                    player->GetAura(SPELL_SPEED_RAM_CANTER)->SetDuration(4000);
                else
                  player->CastSpell(player,SPELL_SPEED_RAM_GALLOP,false);
            } else if (player->HasAura(SPELL_SPEED_RAM_GALLOP))
                player->GetAura(SPELL_SPEED_RAM_GALLOP)->SetDuration(4000);
        }
        else
            player->SendEquipError(EQUIP_ERR_CANT_DO_RIGHT_NOW ,item, NULL);

        return true;
    }
};

/*####
## npc_brewfest_apple_trigger
####*/

#define SPELL_RAM_FATIGUE 43052

class npc_brewfest_apple_trigger : public CreatureScript
{
public:
    npc_brewfest_apple_trigger() : CreatureScript("npc_brewfest_apple_trigger") { }

    struct npc_brewfest_apple_triggerAI : public ScriptedAI
    {
        npc_brewfest_apple_triggerAI(Creature* c) : ScriptedAI(c) {}

        void MoveInLineOfSight(Unit *who)
        {
            Player *player = who->ToPlayer();
            if (!player)
                return;
            if (player->HasAura(SPELL_RAM_FATIGUE) && me->GetDistance(player->GetPositionX(),player->GetPositionY(),player->GetPositionZ()) <= 7.5f)
                player->RemoveAura(SPELL_RAM_FATIGUE);
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_brewfest_apple_triggerAI(creature);
    }
};

/*####
## npc_brewfest_keg_thrower
####*/

enum BrewfestKegThrower
{
    SPELL_THROW_KEG = 43660,
    ITEM_BREWFEST_KEG = 33797
};

class npc_brewfest_keg_thrower : public CreatureScript
{
public:
    npc_brewfest_keg_thrower() : CreatureScript("npc_brewfest_keg_thrower") { }

    struct npc_brewfest_keg_throwerAI : public ScriptedAI
    {
        npc_brewfest_keg_throwerAI(Creature* c) : ScriptedAI(c) {}

        void MoveInLineOfSight(Unit *who)
        {
            Player *player = who->ToPlayer();
            if (!player)
                return;
                                                 /* BREWFEST EXPLOIT FIX BELOW */
        if ( !player->IsActiveQuest(11407)       /* Bark for Drohn's Distillery! */
                && !player->IsActiveQuest(11408) /* Bark for T'chali's Voodoo Brewery! */
                && !player->IsActiveQuest(11294) /* Bark for the Thunderbrews! */
                && !player->IsActiveQuest(11293) /* Bark for the Barleybrews! */
                && (player->HasAura(SPELL_BREWFEST_RAM) || player->HasAura(SPELL_RAMSTEIN_SWIFT_WORK_RAM))
                && me->GetDistance(player->GetPositionX(),player->GetPositionY(),player->GetPositionZ()) <= 25.0f
                && !player->HasItemCount(ITEM_BREWFEST_KEG,1))
            {
                me->CastSpell(player,SPELL_THROW_KEG,false);
                me->CastSpell(player,42414,false);
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_brewfest_keg_throwerAI(creature);
    }
};

/*####
## npc_brewfest_keg_receiver
####*/

enum BrewfestKegReceiver
{
    SPELL_CREATE_TICKETS = 44501,
    QUEST_THERE_AND_BACK_AGAIN_A = 11122,
    QUEST_THERE_AND_BACK_AGAIN_H = 11412,
    NPC_BREWFEST_DELIVERY_BUNNY = 24337
};

class npc_brewfest_keg_receiver : public CreatureScript
{
public:
    npc_brewfest_keg_receiver() : CreatureScript("npc_brewfest_keg_receiver") { }

    struct npc_brewfest_keg_receiverAI : public ScriptedAI
    {
        npc_brewfest_keg_receiverAI(Creature* c) : ScriptedAI(c) {}

        void MoveInLineOfSight(Unit *who)
        {
            Player *player = who->ToPlayer();
            if (!player)
                return;

            if ((player->HasAura(SPELL_BREWFEST_RAM) || player->HasAura(SPELL_RAMSTEIN_SWIFT_WORK_RAM))
                && me->GetDistance(player->GetPositionX(),player->GetPositionY(),player->GetPositionZ()) <= 5.0f
                && player->HasItemCount(ITEM_BREWFEST_KEG,1))
            {
                player->CastSpell(me,SPELL_THROW_KEG,true);
                player->DestroyItemCount(ITEM_BREWFEST_KEG,1,true);
                if (player->HasAura(SPELL_BREWFEST_RAM))
                    player->GetAura(SPELL_BREWFEST_RAM)->SetDuration(player->GetAura(SPELL_BREWFEST_RAM)->GetDuration() + 30000);
                if (player->HasAura(SPELL_RAMSTEIN_SWIFT_WORK_RAM))
                    player->GetAura(SPELL_RAMSTEIN_SWIFT_WORK_RAM)->SetDuration(player->GetAura(SPELL_RAMSTEIN_SWIFT_WORK_RAM)->GetDuration() + 30000);
                if (player->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_A)
                    || player->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_H))
                {
                    player->CastSpell(player,SPELL_CREATE_TICKETS,true);
                }
                else
                {
                    player->KilledMonsterCredit(NPC_BREWFEST_DELIVERY_BUNNY);
                    if (player->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_A) == QUEST_STATUS_INCOMPLETE)
                        player->AreaExploredOrEventHappens(QUEST_THERE_AND_BACK_AGAIN_A);
                    if (player->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_H) == QUEST_STATUS_INCOMPLETE)
                        player->AreaExploredOrEventHappens(QUEST_THERE_AND_BACK_AGAIN_H);
                    if (player->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_A) == QUEST_STATUS_COMPLETE
                        || player->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_H) == QUEST_STATUS_COMPLETE)
                        player->RemoveAura(SPELL_BREWFEST_RAM);
                }
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_brewfest_keg_receiverAI(creature);
    }
};

/*####
## npc_brewfest_ram_master
####*/
#define GOSSIP_ITEM_RAM "Do you have additional work?"
#define GOSSIP_ITEM_RAM_REINS "Give me another Ram Racing Reins"
#define SPELL_BREWFEST_SUMMON_RAM 43720

class npc_brewfest_ram_master : public CreatureScript
{
public:
    npc_brewfest_ram_master() : CreatureScript("npc_brewfest_ram_master") { }

    bool OnGossipHello(Player *player, Creature *creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

            if (player->GetSpellHistory()->HasCooldown(SPELL_BREWFEST_SUMMON_RAM)
                && !player->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_A)
                && !player->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_H)
                && (player->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_A) == QUEST_STATUS_INCOMPLETE
                || player->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_H) == QUEST_STATUS_INCOMPLETE))
                player->GetSpellHistory()->ResetCooldown(SPELL_BREWFEST_SUMMON_RAM);

            if (!player->HasAura(SPELL_BREWFEST_RAM) && ((player->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_A) == QUEST_STATUS_INCOMPLETE
            || player->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_H) == QUEST_STATUS_INCOMPLETE
            || (!player->GetSpellHistory()->HasCooldown(SPELL_BREWFEST_SUMMON_RAM) &&
                (player->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_A)
                || player->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_H))))))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_RAM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

            if ((player->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_A)
                || player->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_H))
                && !player->HasItemCount(33306,1,true))
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_RAM_REINS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

        player->SEND_GOSSIP_MENU(384, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*uiSender*/, uint32 uiAction)
    {
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            if (player->HasItemCount(ITEM_BREWFEST_KEG,1))
                player->DestroyItemCount(ITEM_BREWFEST_KEG,1,true);
            player->CastSpell(player,SPELL_BREWFEST_SUMMON_RAM,true);
            player->GetSpellHistory()->AddCooldown(SPELL_BREWFEST_SUMMON_RAM,0, std::chrono::seconds(18*60*60));
        }
        if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
        {
            player->CastSpell(player,44371,false);
        }
        return true;
    }
};


/*######
# Brewfest Reveler
######*/

enum BrewfestReveler
{
    SPELL_TOAST         =   41586,
    ACTION_RUN_AWAY     =   1
};

enum BOTM
{
    ACHIEV_BREW_OF_THE_MONTH    = 2796,
};

enum Brew_PlayerCustomFlags
{
    BREW_JANUARY    =   0x00000001,
    BREW_FEBRUARY   =   0x00000002,
    BREW_MARCH      =   0x00000004,
    BREW_APRIL      =   0x00000008,
    BREW_MAY        =   0x00000010,
    BREW_JUNE       =   0x00000020,
    BREW_JULY       =   0x00000040,
    BREW_AUGUST     =   0x00000080,
    BREW_SEPTEMBER  =   0x00000100,
    BREW_OCTOBER    =   0x00000200,
    BREW_NOVEMBER   =   0x00000400,
    BREW_DECEMBER   =   0x00000800
};


class BrewOfTheYearMailer : public PlayerScript
{
    public:
    BrewOfTheYearMailer() : PlayerScript("BrewOfTheYearMailer") { }

    void OnLogin(Player* player, bool /*firstLogin*/)
    {
        if (!player->HasAchieved(ACHIEV_BREW_OF_THE_MONTH))
            return;

        time_t rawtime;
        struct tm * timeinfo;
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        switch( timeinfo->tm_mon )
        {
        case 0: //january
            if(!player->HasCustomFlag(BREW_JANUARY))
            {
                MailDraft(212,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_JANUARY);
            }
            break;
        case 1:
            if(!player->HasCustomFlag(BREW_FEBRUARY))
            {
                MailDraft(213,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_FEBRUARY);
            }
            break;
        case 2:
            if(!player->HasCustomFlag(BREW_MARCH))
            {
                MailDraft(214,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_MARCH);
            }
            break;
        case 3:
            if(!player->HasCustomFlag(BREW_APRIL))
            {
                MailDraft(215,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_APRIL);
            }
            break;
        case 4:
            if(!player->HasCustomFlag(BREW_MAY))
            {
                MailDraft(216,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_MAY);
            }
            break;
        case 5:
            if(!player->HasCustomFlag(BREW_JUNE))
            {
                MailDraft(217,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_JUNE);
            }
            break;
        case 6:
            if(!player->HasCustomFlag(BREW_JULY))
            {
                MailDraft(218,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_JULY);
            }
            break;
        case 7:
            if(!player->HasCustomFlag(BREW_AUGUST))
            {
                MailDraft(219,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_AUGUST);
            }
            break;
        case 8:
            if(!player->HasCustomFlag(BREW_SEPTEMBER))
            {
                MailDraft(220,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_SEPTEMBER);
            }
            break;
        case 9:
            if(!player->HasCustomFlag(BREW_OCTOBER))
            {
                MailDraft(221,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_OCTOBER);
            }
            break;
        case 10:
            if(!player->HasCustomFlag(BREW_NOVEMBER))
            {
                MailDraft(222,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_NOVEMBER);
            }
            break;
        case 11:
            if(!player->HasCustomFlag(BREW_DECEMBER))
            {
                MailDraft(223,true).SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 27817));
                player->SetCustomFlag(BREW_DECEMBER);
            }
            break;
        }
        CharacterDatabase.CommitTransaction(trans);
    }
};

/* ********************** */
/*    Das Zwergenevent    */
/* ********************** */

/*
11:59 Uhr - ZwergenEvent in der DB startet (Eventid: 65)
11:59 Uhr - spawn Maintrigger
11:59 Uhr - spawn Fass Trigger mit Lila Pfeil
12:00 Uhr - Start Event
12:00 Uhr - Feiernde Braufest NPCs m�ssen weglaufen
12:00 Uhr - Event Start (spawn)
12:05 Uhr - Event Stop
12:05 Uhr - Entscheidung ob Fail oder nicht
12:05 Uhr - Despawn Fass Trigger
12:05 Uhr - Falls NICHT Fail - diese Zahnr�der f�r Quest spawnen (f�r 15 Minuten)
12:06 Uhr - Feiernde NPCs wieder auf den Platz holen
12:06 Uhr - Despawn Gruzzler
12:06 Uhr - Despawn Maintrigger
*/



const Position heraldLoc[]=
{
    { -5155.980f, -619.082f,  397.942f, 0}, //Eisenschmiede
    { 1199.600f,  -4293.889f, 21.2320f, 0}, //Orgrimmar
};

const Position kegLoc[]=
{
    {-5147.460449f, -577.559692f, 397.177063f, 0.814758f}, //IF 
    {-5184.864746f, -599.649048f, 397.177032f, 3.204334f}, //IF 
    {-5159.609375f, -630.465149f, 397.199982f, 4.525764f}, //IF 
    {1221.366211f, -4297.119629f, 21.191860f, 0.222226f},  //OG 
    {1185.572998f, -4313.315430f, 21.294044f, 3.943054f},  //OG 
    {1183.690674f, -4274.621582f, 21.191896f, 2.122893f},  //OG 
};

enum creatureText
{
    SAY_START,
    SAY_SONG1,
    SAY_SONG2,
    SAY_SONG3,
    SAY_SONG4,
    SAY_SONG5,
    SAY_WIN,
    SAY_ALLY_WIN,
    SAY_HORDE_WIN,
};

enum doAction
{
    ActionIronWIn   = 0,
    ActionHordeWin  = 1,
    ActionAllyWin   = 2,
    ActionYellStart = 3,
};

#define GOB_FUNNY_KEG       186478
#define SPELL_FUNNY_KEG     42696

static uint32 npcList[3] = { 1000145, 1000146, 1000147 }; // CHANGEME!!

class brewfest_npc_funny_keg : public CreatureScript
{
public:
    brewfest_npc_funny_keg() : CreatureScript("brewfest_npc_funny_keg") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new brewfest_npc_funny_kegAI(creature);
    }

    struct brewfest_npc_funny_kegAI : public ScriptedAI
    {
        uint32 timer;
        brewfest_npc_funny_kegAI(Creature* c) : ScriptedAI(c)
        {
            me->SummonGameObject(GOB_FUNNY_KEG, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, 0);
            timer = 1500;
        }



        void UpdateAI(uint32 diff)
        {
            if (timer <= diff)
            {
                if (Player* player = me->FindNearestPlayer(2.5f))
                {
                    if (!player->IsGameMaster())
                    {
                        if (GameObject* go = me->FindNearestGameObject(GOB_FUNNY_KEG, 2.0f))
                        {
                            player->CastSpell(player, SPELL_FUNNY_KEG, true);
                            go->Delete();
                            me->DespawnOrUnsummon();
                        }
                    }

                }
            } else timer -= diff;
        }

    };
};

class brewfest_npc_herald : public CreatureScript
{
public:
    brewfest_npc_herald() : CreatureScript("brewfest_npc_herald") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new brewfest_npc_heraldAI(creature);
    }

    struct brewfest_npc_heraldAI : public ScriptedAI
    {
        brewfest_npc_heraldAI(Creature* c) : ScriptedAI(c) { }

        uint32 sayTimer;
        uint8 sayCount;
        uint8 stopCount;

        void Reset()
        {
            sayTimer = 15000;
            sayCount = 0;
            stopCount = 0;
        }

        void DoAction(int32 action)
        {
            if (action == ActionIronWIn)
            {
                Talk(SAY_WIN);
                me->DespawnOrUnsummon(5000);
            }
            if (action == ActionHordeWin)
                Talk(SAY_HORDE_WIN);
            if (action == ActionAllyWin)
                Talk(SAY_ALLY_WIN);
            if (action == ActionYellStart)
                Talk(SAY_START);
        }

        void UpdateAI(uint32 diff)
        {
            if (stopCount <= 4)
            {
                if (sayTimer < diff)
                {
                    switch(sayCount)
                    {
                        case 0:
                            Talk(SAY_SONG1);
                            sayTimer = 8000;
                            ++sayCount;
                            break;
                        case 1:
                            Talk(SAY_SONG2);
                            sayTimer = 8000;
                            ++sayCount;
                            break;
                        case 2:
                            Talk(SAY_SONG3);
                            sayTimer = 8000;
                            ++sayCount;
                            break;
                        case 3:
                            Talk(SAY_SONG4);
                            sayTimer = 8000;
                            ++sayCount;
                            break;
                        case 4:
                            Talk(SAY_SONG5);
                            sayTimer = 28000;
                            sayCount = 0;
                            ++stopCount;
                            break;
                        default:
                            break;
                    }
                }else sayTimer -= diff;
            }
        }
    };
};

class brewfest_dark_iron_gruzzler : public CreatureScript
{
public:
    brewfest_dark_iron_gruzzler() : CreatureScript("brewfest_dark_iron_gruzzler") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new brewfest_dark_iron_gruzzlerAI(creature);
    }

    struct brewfest_dark_iron_gruzzlerAI : public ScriptedAI
    {
        brewfest_dark_iron_gruzzlerAI(Creature* c) : ScriptedAI(c) { }

        void Reset()
        {
            castspell = false;
            talktext = false;
            talkTimer = urand (2000, 4000);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetSpeedRate(MOVE_RUN, 0.3f);
        }

        void DamageTaken(Unit* /*killer*/, uint32 &damage)
        {
            damage = 0;
        }

        void DoAction(int32 /*action*/)
        {
            me->SetStandState(UNIT_STAND_STATE_SIT);
            drinkTimer=1000;
            sleepTimer=5000;
        }

        void UpdateAI(uint32 diff)
        {
            if (talkTimer <= diff && !talktext)
            {
                Talk(TEXT_ATTACK); 
                talktext = true;
            } else talkTimer -= diff;

            if(!castspell)
            {
                if ((me->FindNearestCreature(npcList[0], 0.4f)) || (me->FindNearestCreature(npcList[1], 0.4f)) || (me->FindNearestCreature(npcList[2], 0.4f)))
                {
                    DoCastSelf(SPELL_BREWFEST_ATTACK, true);
                    me->SetStandState(UNIT_STAND_STATE_SIT);
                    drinkTimer =  1000;
                    sleepTimer = 15000;
                    castspell = true;                    
                }
            }
            if (me->GetStandState() == UNIT_STAND_STATE_SIT)
            {
                if (drinkTimer <= diff)
                {
                    me->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
                    drinkTimer = 1000;
                } else drinkTimer -= diff;

                if (sleepTimer <= diff)
                    me->SetStandState(UNIT_STAND_STATE_SLEEP);
                else sleepTimer -= diff;
            }
        }

    private:
        uint32 drinkTimer;
        uint32 sleepTimer;
        uint32 talkTimer;
        bool castspell;
        bool talktext;
    };
};

enum GobTriggerSpells
{
    SPELL_SUMMON_MOLE_MACHINE   = 62899,
};

class brewfest_gob_trigger : public CreatureScript
{
public:
    brewfest_gob_trigger() : CreatureScript("brewfest_gob_trigger") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new brewfest_gob_triggerAI(creature);
    }

    struct brewfest_gob_triggerAI : public ScriptedAI
    {
        brewfest_gob_triggerAI(Creature* c) : ScriptedAI(c) {}

        uint8 stepCount;
        uint32 actionTimer;
        bool castSpell;

        void Reset()
        {
            me->setFaction(14);
            me->SetReactState(REACT_PASSIVE);
            stepCount = 0;
            actionTimer = 1000;
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_KNOCK_BACK, true);
            castSpell = false;
        }

        void UpdateAI(uint32 diff)
        {
            if (!castSpell)
            {
                if (Map* map = me->GetMap())
                {
                    Map::PlayerList const &players = map->GetPlayers();
                    for (Player& player : map->GetPlayers())
                    {
                        if (player.IsInRange(me,0.0f, 3.0f, true) && !player.IsGameMaster())
                            DoCast(&player, SPELL_KNOCK_BACK, true);
                    }
                    castSpell = true;
                }
            }

            if (actionTimer < diff)
            {
                switch(stepCount)
                {
                    case 0:
                        if (GameObject* go = me->SummonGameObject(GOB_MACHINE_STOMPEAKS, me->GetPositionX(),  me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, 0))
                        {
                            go->SetLootState(GO_READY);
                            go->UseDoorOrButton(11500);
                        }

                        actionTimer = 2500;
                        break;
                    case 1:
                            for (uint8 i = 0; i < 3; i++)
                                if (Creature* guzzler = me->SummonCreature(NPC_GUZZLER, me->GetPositionX() + 1.5f,  me->GetPositionY() + 1.5f, me->GetPositionZ() + 2.0f, TEMPSUMMON_MANUAL_DESPAWN))
                                {
                                    if (Creature* keg = guzzler->FindNearestCreature(npcList[i], 5000.0f, true))
                                        guzzler->GetMotionMaster()->MovePoint(0, keg->GetPositionX(), keg->GetPositionY(), keg->GetPositionZ());
                                    else if (Creature* keg = guzzler->FindNearestCreature(npcList[0], 5000.0f, true))
                                        guzzler->GetMotionMaster()->MovePoint(0, keg->GetPositionX(), keg->GetPositionY(), keg->GetPositionZ());
                                    else if (Creature* keg = guzzler->FindNearestCreature(npcList[1], 5000.0f, true))
                                        guzzler->GetMotionMaster()->MovePoint(0, keg->GetPositionX(), keg->GetPositionY(), keg->GetPositionZ());
                                    else if (Creature* keg = guzzler->FindNearestCreature(npcList[2], 5000.0f, true))
                                        guzzler->GetMotionMaster()->MovePoint(0, keg->GetPositionX(), keg->GetPositionY(), keg->GetPositionZ());
                                }

                        actionTimer = 2000;
                        break;
                    case 2:
                        if (GameObject* go = me->FindNearestGameObject(GOB_MAIN_MOLE_MACHINE, 1.0f))
                            go->Delete();
                        me->DisappearAndDie();
                    default:
                        break;
                }
                ++stepCount;
            }else actionTimer -= diff;
        }
    };
};

class brewfest_keg_trigger : public CreatureScript
{
public:
    brewfest_keg_trigger() : CreatureScript("brewfest_keg_trigger") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new brewfest_keg_triggerAI(creature);
    }

    struct brewfest_keg_triggerAI : public ScriptedAI
    {
        brewfest_keg_triggerAI(Creature* c) : ScriptedAI(c)
        {
            me->SetFloatValue(OBJECT_FIELD_SCALE_X, 1.5f);
            DoCastSelf(SPELL_KEG_MARKER);
            me->SetHealth(4000);
        }

        uint8 hitCounter;
        uint8 failCounter;
        GuidVector guzzlerList;

        void Reset()
        {
            hitCounter = urand(19,20);
            failCounter = 0;
            guzzlerList.clear();
        }

        void UpdateAI(uint32 /*diff*/)
        {
            if (failCounter == hitCounter)
                me->DisappearAndDie();

            if (Creature* guzzler = me->FindNearestCreature(NPC_GUZZLER, 6.0f, true))
            {
                GuidVector::iterator itr = std::find(guzzlerList.begin(), guzzlerList.end(),guzzler->GetGUID());
                if (itr == guzzlerList.end() || guzzlerList.empty())
                {
                    guzzlerList.push_back(guzzler->GetGUID());
                    guzzler->GetMotionMaster()->MoveIdle();
                    guzzler->ToCreature()->AI()->DoAction(1);
                    ++failCounter;
                }
            }

        }

        void JustDied(Unit* /*killer*/)
        {
            me->DespawnOrUnsummon();
        }
    };
};

enum BrefestSound
{
    SOUND_ALLIANCE  = 11810,
    SOUND_HORDE     = 11814
};

class brewfest_main_trigger : public CreatureScript
{
public:
    brewfest_main_trigger() : CreatureScript("brewfest_main_trigger") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new brewfest_main_triggerAI(creature);
    }

    struct brewfest_main_triggerAI : public ScriptedAI
    {
        brewfest_main_triggerAI(Creature* c) : ScriptedAI(c)
        {
            whichMap = me->GetMapId();
            DoSpawnKegTrigger();
        }

        std::list<Creature*> revelerList;
        std::list<Creature*> guzzlerList;

        uint8 whichMap;
        uint8 eventCount;
        uint8 waveCount;
        uint32 waveTimer;
        uint32 eventTimer;
        uint32 failTimer;
        uint32 superKegTimer;
        uint32 brewfestSoundTimer;
        bool failEvent;
        bool spawnGuzzler;
        bool superKeg;

        void Reset()
        {
            revelerList.clear();
            guzzlerList.clear();

            eventTimer = 50000;
            eventCount = 0;
            waveTimer = 3000;
            waveCount = 0;
            failEvent = false;
            failTimer = 2000;
            spawnGuzzler = false;
            superKegTimer = 30000;
            brewfestSoundTimer = 20000;
        }

        void DoSpawnKegTrigger()
        {
            if (whichMap == 0)
            {
                me->SummonCreature(npcList[0], kegLoc[0], TEMPSUMMON_MANUAL_DESPAWN);
                me->SummonCreature(npcList[1], kegLoc[1], TEMPSUMMON_MANUAL_DESPAWN);
                me->SummonCreature(npcList[2], kegLoc[2], TEMPSUMMON_MANUAL_DESPAWN);
            }
            if (whichMap == 1)
            {
                me->SummonCreature(npcList[0], kegLoc[3], TEMPSUMMON_MANUAL_DESPAWN);
                me->SummonCreature(npcList[1], kegLoc[4], TEMPSUMMON_MANUAL_DESPAWN);
                me->SummonCreature(npcList[2], kegLoc[5], TEMPSUMMON_MANUAL_DESPAWN);
            }
        }

        void DoSpawnHerald()
        {
            if (whichMap == 0)
                if (Creature* herald = me->SummonCreature(NPC_HERALD, heraldLoc[0], TEMPSUMMON_MANUAL_DESPAWN))
                    herald->AI()->DoAction(ActionYellStart);
            if (whichMap == 1)
                if (Creature* herald = me->SummonCreature(NPC_HERALD, heraldLoc[1], TEMPSUMMON_MANUAL_DESPAWN))
                    herald->AI()->DoAction(ActionYellStart);
        }

        void MoveRevelers()
        {
            me->GetCreatureListWithEntryInGrid(revelerList, NPC_BREWFEST_REVELER, 80.0f);

            if (revelerList.empty())
                return;

            for( std::list<Creature*>::iterator itr = revelerList.begin(); itr != revelerList.end(); ++itr )
                if (Creature* reveler = *itr)
                    reveler->AI()->DoAction(ACTION_RUN_AWAY);
        }

        void RestoreRevelers()
        {
            if (revelerList.empty())
                return;

            for (std::list<Creature*>::iterator itr = revelerList.begin(); itr != revelerList.end(); ++itr )
                if (Creature* reveler = *itr)
                {
                    reveler->SetVisible(true);
                    reveler->GetMotionMaster()->MoveTargetedHome();
                }
        }

        void DoSpawnGuzzler()
        {
            Position pos;
            me->GetPosition(&pos);

            if (whichMap == 0)
            {
                me->GetRandomNearPosition(pos, float(urand(2, 30)));
                me->SummonCreature(NPC_GOB_TRIGGER, pos, TEMPSUMMON_MANUAL_DESPAWN);
            }
            else if (whichMap == 1)
            {
                me->GetRandomNearPosition(pos, float(urand(2, 27)));
                me->SummonCreature(NPC_GOB_TRIGGER, pos, TEMPSUMMON_MANUAL_DESPAWN);
            }
        }

        void DoSpawnSuperKeg()
        {
            Position pos;
            me->GetPosition(&pos);
            me->GetRandomNearPosition(pos, float(urand(5, 30)));
            me->SummonCreature(NPC_KEG_TRIGGER, pos, TEMPSUMMON_MANUAL_DESPAWN);
        }

        void SetDataTrigger() // See further action in the SAI-Script of the Creatures which are listed below
        {
            std::list<Creature*> SummonList;
            me->GetCreatureListWithEntryInGrid(SummonList, 24484, 100.0f);  // Brewfest Reveler SAI
            me->GetCreatureListWithEntryInGrid(SummonList, 23685, 100.0f);  // Gordok Brew Barker SAI
            me->GetCreatureListWithEntryInGrid(SummonList, 23683, 100.0f);  // Maeve Barleybrew SAI
            me->GetCreatureListWithEntryInGrid(SummonList, 23684, 100.0f);  // Ita Thunderbrew SAI
            me->GetCreatureListWithEntryInGrid(SummonList, 24492, 100.0f);  // Drohn's Distillery Barker SAI
            me->GetCreatureListWithEntryInGrid(SummonList, 24493, 100.0f);  // T'chali's Voodoo Brewery Barker SAI
            if (!SummonList.empty())
                for (std::list<Creature*>::iterator itr = SummonList.begin(); itr != SummonList.end(); itr++)
                    (*itr)->AI()->SetData(1, 1);
        }

        void EventResult()
        {
            if ((me->FindNearestCreature(npcList[0], 500.0f)) || (me->FindNearestCreature(npcList[1], 500.0f)) || (me->FindNearestCreature(npcList[2], 500.0f)))
                DoSpawnQuestObjects();
        }

        void DoSpawnQuestObjects() //Win Funktion
        {
            if (whichMap == 0)
            {
                me->SummonGameObject(GOB_ALLIANZ_QUEST, me->GetPositionX(),  me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, 900 * IN_MILLISECONDS);
                if (Creature* herald = me->FindNearestCreature(NPC_HERALD, 500.f))
                    herald->AI()->DoAction(ActionAllyWin);
            }

            if (whichMap == 1)
            {
                me->SummonGameObject(GOB_HORDE_QUEST, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, 900 * IN_MILLISECONDS);
                if (Creature* herald = me->FindNearestCreature(NPC_HERALD, 500.f))
                    herald->AI()->DoAction(ActionHordeWin);
            }
        }

        void CleanEventPlace(bool herald = false) //Putzkolonne
        {
            while (Creature* guzzler = me->FindNearestCreature(NPC_GUZZLER, 500.0f, true))
                guzzler->DespawnOrUnsummon();
            if (!herald)
                while (Creature* herald = me->FindNearestCreature(NPC_HERALD, 500.0f, true))
                    herald->DespawnOrUnsummon();
            while (Creature* gob_trigger = me->FindNearestCreature(NPC_GOB_TRIGGER, 500.0f, true))
                gob_trigger->DespawnOrUnsummon();
        }

        void DespawnQuestGob()
        {
            if (GameObject* go = me->FindNearestGameObject(GOB_HORDE_QUEST, 5.0f))
                go->Delete();
            if (GameObject* go = me->FindNearestGameObject(GOB_ALLIANZ_QUEST, 5.0f))
                go->Delete();
        }

        void UpdateAI(uint32 diff)
        {
            if (brewfestSoundTimer < diff)
            {
                me->PlayDirectSound(me->GetMapId() == 1 ? SOUND_HORDE : SOUND_ALLIANCE, NULL);
                brewfestSoundTimer = 15000;
            } else brewfestSoundTimer -= diff;

            if (!failEvent && (failTimer < diff))
            {
                if ((!me->FindNearestCreature(npcList[0], 500.0f, true)) &&
                    (!me->FindNearestCreature(npcList[1], 500.0f, true)) &&
                    (!me->FindNearestCreature(npcList[2], 500.0f, true)))
                {
                    if (Creature* herald = me->FindNearestCreature(NPC_HERALD, 500.0f, true))
                        herald->AI()->DoAction(ActionIronWIn);
                    else
                        me->MonsterSay("Devs have failed - cannot find Herald. Please write a TrollThread at the RG Forum. Facepalm Oo", 0, 0);
                    RestoreRevelers();
                    CleanEventPlace(true);
                    failEvent = true;
                }
                failTimer = 2000;
            } else failTimer -= diff;

            if (!failEvent && spawnGuzzler && waveCount <= 100)
            {
                if (waveTimer < diff)
                {
                    DoSpawnGuzzler();
                    ++waveCount;
                    waveTimer = 3000;
                } else waveTimer -= diff;

                if (superKegTimer < diff)
                {
                    DoSpawnSuperKeg();
                    superKegTimer = 30000;
                } else superKegTimer -= diff;
            }
            else
                if (waveCount == 100)
                    spawnGuzzler = false;

            if (!failEvent)
            {
                if (eventTimer < diff)
                {
                    switch(eventCount)
                    {
                        case 0:
                            DoSpawnHerald();
                            SetDataTrigger();
                            MoveRevelers();
                            eventTimer = 10000;
                            break;
                        case 1:
                            spawnGuzzler = true;
                            eventTimer = 300000;
                            break;
                        case 2:
                            RestoreRevelers();
                            eventTimer = 10000;
                            break;
                        case 3:
                            EventResult();
                            eventTimer = 50000;
                            break;
                        case 4:
                            CleanEventPlace();
                            eventTimer = 900000;
                            break;
                        case 5:
                            DespawnQuestGob();
                            break;
                        default:
                            break;
                    }
                    ++eventCount;
                }else eventTimer -= diff;
            }
        }
    };
};

enum QuestDarkIronEvent
{
    QUEST_WHEN_I_WAS_DRUNK_ALLIANCE = 12020,
    QUEST_WHEN_I_WAS_DRUNK_HORDE = 12192,
    ACHIEVEMENT_DOWN_WITH_DARK_IRON = 1186,
    NPC_BIZZLE_QUICKLIFT = 27216,
    NPC_BOXEY_BOLTSPINNER = 27215,
};

class npc_dark_iron_event_quest : public CreatureScript
{
public:
    npc_dark_iron_event_quest() : CreatureScript("npc_dark_iron_event_quest") { }

    bool OnQuestComplete(Player* player, Creature* /*creature*/, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_WHEN_I_WAS_DRUNK_ALLIANCE || quest->GetQuestId() == QUEST_WHEN_I_WAS_DRUNK_HORDE )
            player->CompletedAchievement(sAchievementStore.LookupEntry(ACHIEVEMENT_DOWN_WITH_DARK_IRON));

        return true;
    }
};

void AddSC_event_brewfest()
{
    new npc_brewfest_trigger();
    new item_brewfest_ChugAndChuck();
    new spell_brewfest_speed();
    new item_brewfest_ram_reins();
    new npc_brewfest_apple_trigger();
    new npc_brewfest_keg_thrower();
    new npc_brewfest_keg_receiver();
    new npc_brewfest_ram_master();
    new BrewOfTheYearMailer();
    new brewfest_main_trigger();
    new brewfest_npc_herald();
    new brewfest_dark_iron_gruzzler();
    new brewfest_keg_trigger();
    new brewfest_gob_trigger();
    new brewfest_npc_funny_keg();
    new npc_dark_iron_event_quest();
}
