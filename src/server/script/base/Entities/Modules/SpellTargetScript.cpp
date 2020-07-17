#include "SpellTargetScript.h"
#include "AuraScript.h"
#include "AuraHooks.h"
#include "UnitHooks.h"

#include "SpellAuras.h"

SCRIPT_MODULE_STATE_IMPL(SpellTarget)
{
    std::unordered_map<const Aura*, script::StoredTarget<Aura>> receivedAuras;

    SCRIPT_MODULE_PRINT(SpellTarget) 
    { 
        return std::to_string(receivedAuras.size()) + " Stored Aura(s)";
    }
};

SCRIPT_ACTION1_INLINE(StoreCombatAura, Scripts::SpellTarget, Aura&, aura)
{
    const Unit* unit = script::castSourceTo<Unit>(me);
    ASSERT_DEV(unit, "");

    if (aura.GetCasterGUID() != unit->GetGUID() && aura.GetMaxDuration() > 0)
    {
        ASSERT_DEV(!me.receivedAuras.count(&aura), "");

        me.receivedAuras.emplace(&aura, aura);
    }
}

SCRIPT_ACTION1_INLINE(RemoveCombatAura, Scripts::SpellTarget, Aura&, aura)
{
    const Unit* unit = script::castSourceTo<Unit>(me);
    ASSERT_DEV(unit, "");

    if (aura.GetCasterGUID() != unit->GetGUID() && aura.GetMaxDuration() > 0)
        me.receivedAuras.erase(&aura);
}

SCRIPT_ACTION_INLINE(RemoveAllAuras, Scripts::SpellTarget)
{
    // copy, to aliviate issues
    std::vector<script::StoredTarget<Aura>> auras;
    auras.reserve(me.receivedAuras.size());
    for (const auto& [aura, stored] : me.receivedAuras)
    {
        util::meta::Unreferenced(aura);

        auras.emplace_back(stored);
    }

    me.receivedAuras.clear();

    for (const script::StoredTarget<Aura> stored : auras)
        if (auto aura = stored.Load())
            me <<= Do::RemoveAura.Bind(*aura);
}

SCRIPT_MODULE_IMPL(SpellTarget)
{
    me <<= On::EnterCombat |= BeginBlock
        += On::ReceivedAura::Any()
            |= Do::StoreCombatAura
        += On::LostAura::Any()
            |= Do::RemoveCombatAura
    EndBlock;

    me <<= On::LeaveCombat
        |= Do::RemoveAllAuras;
}
