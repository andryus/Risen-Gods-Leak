/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2014-2016 Rising Gods <http://www.rising-gods.de/>
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
 * Scriptnames of files in this file should be prefixed with "npc_pet_hun_".
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"

enum HunterSpells
{
    SPELL_HUNTER_CRIPPLING_POISON       = 30981,   // Viper
    SPELL_HUNTER_DEADLY_POISON          = 34655,   // Venomous Snake
    SPELL_HUNTER_MIND_NUMBING_POISON    = 25810    // Viper
};

enum HunterCreatures
{
    NPC_HUNTER_VIPER                    = 19921
};

class npc_pet_hunter_snake_trap : public CreatureScript
{
    public:
        npc_pet_hunter_snake_trap() : CreatureScript("npc_pet_hunter_snake_trap") { }

        struct npc_pet_hunter_snake_trapAI : public ScriptedAI
        {
            npc_pet_hunter_snake_trapAI(Creature* creature) : ScriptedAI(creature) { }

            void EnterCombat(Unit* /*who*/) override { }

            void Reset() override
            {
                _spellTimer = 0;

                CreatureTemplate const* Info = me->GetCreatureTemplate();

                _isViper = Info->Entry == NPC_HUNTER_VIPER ? true : false;

                me->SetMaxHealth(uint32(107 * (me->getLevel() - 40) * 0.025f));
                // Add delta to make them not all hit the same time
                uint32 delta = (rand32() % 7) * 100;
                me->SetStatFloatValue(UNIT_FIELD_BASEATTACKTIME, float(Info->baseattacktime + delta));
                me->SetStatFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER, float(Info->attackpower));

                // Start attacking attacker of owner on first ai update after spawn - move in line of sight may choose better target
                if (!me->GetVictim() && me->IsSummon())
                    if (Unit* Owner = me->ToTempSummon()->GetSummoner())
                        if (Owner->getAttackerForHelper())
                            AttackStart(Owner->getAttackerForHelper());
            }

            // Redefined for random target selection:
            void MoveInLineOfSight(Unit* who) override
            {
                if (!me->GetVictim() && me->CanCreatureAttack(who))
                {
                    if (me->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
                        return;

                    float attackRadius = me->GetAttackDistance(who);
                    if (me->IsWithinDistInMap(who, attackRadius) && me->IsWithinLOSInMap(who))
                    {
                        if (!(rand32() % 5))
                        {
                            me->setAttackTimer(BASE_ATTACK, (rand32() % 10) * 100);
                            _spellTimer = (rand32() % 10) * 100;
                            AttackStart(who);
                        }
                    }
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                if (me->GetVictim()->HasBreakableByDamageCrowdControlAura(me))
                {
                    me->InterruptNonMeleeSpells(false);
                    return;
                }

                if (_spellTimer <= diff)
                {
                    if (_isViper) // Viper
                    {
                        if (urand(0, 2) == 0) //33% chance to cast
                        {
                            uint32 spell;
                            if (urand(0, 1) == 0)
                                spell = SPELL_HUNTER_MIND_NUMBING_POISON;
                            else
                                spell = SPELL_HUNTER_CRIPPLING_POISON;

                            DoCastVictim(spell);
                        }

                        _spellTimer = 3 * IN_MILLISECONDS;
                    }
                    else // Venomous Snake
                    {
                        if (urand(0, 2) == 0) // 33% chance to cast
                            DoCastVictim(SPELL_HUNTER_DEADLY_POISON);
                        _spellTimer = 1500 + (rand32() % 5) * 100;
                    }
                }
                else
                    _spellTimer -= diff;

                DoMeleeAttackIfReady();
            }

        private:
            bool _isViper;
            uint32 _spellTimer;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_hunter_snake_trapAI(creature);
        }
};

class spell_hun_wild_hunt : public SpellScriptLoader
{
    public:
        spell_hun_wild_hunt() : SpellScriptLoader("spell_hun_wild_hunt") { }

        class spell_hun_wild_hunt_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_wild_hunt_AuraScript);

            void HandleAfterApplyRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                float healthPct = GetTarget()->GetHealthPct();
                GetTarget()->UpdateStats(STAT_STAMINA);
                GetTarget()->SetHealth(GetTarget()->CountPctFromMaxHealth(healthPct));
                GetTarget()->UpdateAttackPowerAndDamage();
            }

            void Register() override
            {
                AfterEffectApply += AuraEffectApplyFn(spell_hun_wild_hunt_AuraScript::HandleAfterApplyRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_hun_wild_hunt_AuraScript::HandleAfterApplyRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_hun_wild_hunt_AuraScript();
        }
};

void AddSC_hunter_pet_scripts()
{
    new npc_pet_hunter_snake_trap();
    new spell_hun_wild_hunt();
}
