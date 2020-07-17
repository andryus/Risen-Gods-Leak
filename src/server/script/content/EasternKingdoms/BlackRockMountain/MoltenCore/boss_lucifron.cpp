#include "ScriptInclude.h"

constexpr SpellAbility ABILITY_IMPENDING_DOOM = { 19702, 15s };
constexpr SpellAbility ABILITY_LUCIFRON_CURSE = { 19703, 20s };
constexpr SpellAbility ABILITY_SHADOW_SHOCK = { 20603, 6s };

/**********************************
* Lucifron
* BOSS #12118
**********************************/

BOSS_SCRIPT(Lucifron)
{
    me <<= On::Init
        |= Do::AddAbility::Multi( 
                ABILITY_IMPENDING_DOOM,
                ABILITY_LUCIFRON_CURSE, 
                ABILITY_SHADOW_SHOCK);
}

SCRIPT_FILE(boss_lucifron)
