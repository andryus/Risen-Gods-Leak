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
#include "SpellScript.h"

enum KingoftheFoulwealdMisc
{
    GO_BANNER = 178205
};

class spell_destroy_karangs_banner : public SpellScriptLoader
{
    public:
        spell_destroy_karangs_banner() : SpellScriptLoader("spell_destroy_karangs_banner") { }

        class spell_destroy_karangs_banner_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_destroy_karangs_banner_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                    if (GameObject* banner = caster->FindNearestGameObject(GO_BANNER, 20.0f))
                        banner->Delete();
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_destroy_karangs_banner_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_destroy_karangs_banner_SpellScript();
        }
};

void AddSC_ashenvale_rg()
{
    new spell_destroy_karangs_banner();
}
