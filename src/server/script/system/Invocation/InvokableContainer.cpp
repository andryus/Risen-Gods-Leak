#include "InvokableContainer.h"
#include "ScriptInvokable.h"

namespace script
{

    InvokableContainer::InvokableContainer() {};
    InvokableContainer::~InvokableContainer() {};

    BaseInvokable* InvokableContainer::GetTop() const
    {
        if (invokables.empty())
            return nullptr;
        else
            return invokables.back().invokable.get();
    }

    void InvokableContainer::Remove(uint64 id)
    {
        // iterate reverse for performance:
        // natural undoable order tends to pop from back
        for (size_t i = invokables.size(); i > 0; i--)
        {
            const size_t pos = i - 1;
            auto& entry = invokables.at(pos);
            if (entry.id == id)
            {
                if (entry.state == InvokableState::Default)
                    invokables.erase(invokables.begin() + pos);
                else
                    entry.state = InvokableState::Delete; // deferred delete in case of self-deletion
                return;
            }
        }

        ASSERT(false); // removal must never happen with an invalid id
    }

}
