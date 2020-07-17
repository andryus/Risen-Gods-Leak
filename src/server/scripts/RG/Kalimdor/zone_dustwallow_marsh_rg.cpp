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
#include "SpellInfo.h"
#include "SpellScript.h"
#include "GameObjectAI.h"

enum CaptureRaptor
{
    SPELL_CAPTURE_RAPTOR_CREDIT = 42337,
    SPELL_CAPTURE_RAPTOR_ITEM   = 42325,

    SAY_REACHED_HEALTH          = 0,
};

class npc_capture_raptor : public CreatureScript
{
public:
    npc_capture_raptor() : CreatureScript("npc_capture_raptor") { }

    struct npc_capture_raptorAI : public ScriptedAI
    {
        npc_capture_raptorAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override 
        {
            _reachedHealth = false;
        }

        void SpellHit(Unit* caster, const SpellInfo* spellInfo) override
        {
            if (caster && spellInfo)
            {
                if (spellInfo->Id == SPELL_CAPTURE_RAPTOR_ITEM)
                {
                    me->CastSpell(caster, SPELL_CAPTURE_RAPTOR_CREDIT, true);
                    me->DespawnOrUnsummon(2000);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (!_reachedHealth && me->GetHealthPct() < 20)
            {
                Talk(SAY_REACHED_HEALTH);
                _reachedHealth = true;
            }

            DoMeleeAttackIfReady();
        }

        private:
            bool _reachedHealth;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_capture_raptorAI(creature);
    }
};

/*###############
## Recover The Cargo - Quest: 11140
###############*/

enum RecoverTheCargo 
{
    SPELL_SALVAGE_WRECKAGE          = 42287,
    SPELL_SUMMON_LOCKBOX            = 42288,
    SPELL_SUMMON_MIREFIN_BURROWER   = 42289
};

struct go_wreckage_and_debris : public GameObjectAI
{
    go_wreckage_and_debris(GameObject* go) : GameObjectAI(go) {}

    void SpellHit(Unit* /*unit*/, const SpellInfo* spellInfo) override 
    {
        if (spellInfo->Id == SPELL_SALVAGE_WRECKAGE) 
        {
            go->DespawnOrUnsummon(1000);
            go->SetRespawnTime(180);
        }
    }
};

class spell_q11140_salvage_wreckage : public SpellScript 
{
    PrepareSpellScript(spell_q11140_salvage_wreckage);

    void OnDummyEffect(SpellEffIndex effIndex) 
    {
        PreventHitDefaultEffect(effIndex);

        if (Player* player = GetCaster()->ToPlayer()) 
        {
            // Give Quest Item Or Spawn Murloc
            if (urand(0, 100) < 30)
            {
                player->CastSpell(player, SPELL_SUMMON_MIREFIN_BURROWER, true);
                return;
            }
            player->CastSpell(player, SPELL_SUMMON_LOCKBOX, true);
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_q11140_salvage_wreckage::OnDummyEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_dustwallow_marsh_rg()
{
    new npc_capture_raptor();
    new GameObjectScriptLoaderEx<go_wreckage_and_debris>("go_wreckage_and_debris");
    new SpellScriptLoaderEx<spell_q11140_salvage_wreckage>("spell_q11140_salvage_wreckage");
}
