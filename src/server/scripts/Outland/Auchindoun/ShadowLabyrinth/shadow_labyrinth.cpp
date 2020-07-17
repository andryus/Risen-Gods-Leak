#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"

enum MarkOfMaliceSpells
{
    SPELL_MARK_OF_MALICE_DAMAGE = 33494,
};

class spell_mark_of_maliceAuraScript : public AuraScript
{
    PrepareAuraScript(spell_mark_of_maliceAuraScript);

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& procInfo)
    {
        if (!GetCaster() || !aurEff->GetBase() || !aurEff->GetBase()->GetUnitOwner())
            return;

        if (aurEff->GetBase()->GetCharges() <= 1)
            aurEff->GetBase()->GetUnitOwner()->CastSpell(GetCaster(), SPELL_MARK_OF_MALICE_DAMAGE, true);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_mark_of_maliceAuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

void AddSC_shadow_labyrinth()
{
    new SpellScriptLoaderEx<spell_mark_of_maliceAuraScript>("spell_mark_of_malice");
}
