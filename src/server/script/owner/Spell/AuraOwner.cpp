#include "AuraOwner.h"
#include "SpellOwner.h"
#include "SpellAuras.h"

SCRIPT_OWNER_IMPL(Aura, script::Scriptable)
{
    return dynamic_cast<Aura*>(&base);
}

AuraHandle::AuraHandle(const Aura& aura) :
    AuraHandle(&aura) { }

AuraHandle::AuraHandle(const Aura* aura) :
    object(aura ? aura->GetOwner() : nullptr)
{
    auraId = aura ? aura->GetSpellInfo()->Id : 0;
}

Aura* AuraHandle::Load() const
{
    if (auraId == 0)
        return nullptr;

    if (auto* owner = object.Load(); owner && owner->ToUnit())
        return owner->ToUnit()->GetAura(auraId);
    else
        return nullptr;
}

std::string AuraHandle::Print() const
{
    return "Aura: " + std::to_string(auraId);
}

Aura* AuraHandle::FromString(std::string_view string, script::Scriptable& base)
{
    const SpellId spell = script::fromString<SpellId>(string, base);

    if (Unit* object = script::castSourceTo<Unit>(base))
        return object->GetAura(spell.id);

    return nullptr;
}

SCRIPT_COMPONENT_IMPL(Aura, Map)
{
    return script::castSourceTo<Map>(owner.GetOwner());
}


SCRIPT_COMPONENT_IMPL(Aura, MapArea)
{
    return script::castSourceTo<MapArea>(owner.GetOwner());
}

SCRIPT_COMPONENT_IMPL(Aura, Encounter)
{
    return script::castSourceTo<Encounter>(owner.GetOwner());
}
