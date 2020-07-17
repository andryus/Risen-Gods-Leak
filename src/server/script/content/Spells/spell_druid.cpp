#include "ScriptInclude.h"

constexpr SpellId SpellMangleBear = 33878;

SPELL_SCRIPT(Druid_Berserker)
{
    me <<= On::CastSuccess
        |= Select::SpellTarget |= Do::RemoveSpellCooldown(SpellMangleBear);
}

SCRIPT_FILE(spell_druid)
