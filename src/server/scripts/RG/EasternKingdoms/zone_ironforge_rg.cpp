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
#include "SpellScript.h"

class spell_mortar_animate : public SpellScriptLoader
{
public:
    spell_mortar_animate() : SpellScriptLoader("spell_mortar_animate") { }

    class spell_mortar_animate_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_mortar_animate_SpellScript);

        void Activate(SpellEffIndex index)
        {
            PreventHitDefaultEffect(index);
            GetHitGObj()->SendCustomAnim(0);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_mortar_animate_SpellScript::Activate, EFFECT_0, SPELL_EFFECT_ACTIVATE_OBJECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_mortar_animate_SpellScript();
    }
};

void AddSC_ironforge_rg()
{
    new spell_mortar_animate();
}
