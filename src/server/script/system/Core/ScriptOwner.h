#pragma once
#include "ScriptTarget.h"
#include "ScriptHandle.h"

namespace script
{
   
    class InvokationCtx;

    template<class Type>
    struct OwnerTypeId;

#define SCRIPT_OWNER_BASE(Me, Base)                             \
    template<>                                                  \
    struct script::TargetBase<Me> { using type = Base; };       \
                                                                \
    template<>                                                  \
    struct script::ScriptType<Me> : std::true_type {};          \
                                                                \
    template<>                                                  \
    struct script::TargetCast<Me, Base>                         \
    {                                                           \
        Me* operator()(Base& base) const;                       \
    };                                                          \
                                                                \
    template<>                                                  \
    struct script::OwnerTypeId<Me>                              \
    {                                                           \
        uint64 operator()() const;                              \
    };                                                          \
                                                                \
    template<>                                                  \
    script::Scriptable*                                         \
        script::impl::upCast<script::Scriptable, Me>(Me& me);   \

#define SCRIPT_OWNER_HANDLE(Me, Base, HANDLE)       \
    SCRIPT_OWNER_BASE(Me, Base)                     \
    SCRIPT_STORED_TARGET(Me, HANDLE)                \

#define SCRIPT_OWNER_SHARED(Me, Base)                        \
    SCRIPT_SHARED_HANDLE(Me)                                 \
    SCRIPT_OWNER_HANDLE(Me, Base, script::SharedHandle<Me>)  \

#define SCRIPT_OWNER(Me, Base)                                          \
    SCRIPT_OWNER_HANDLE(Me, Base, typename StoredTarget<Base>::Handle)  \

#define SCRIPT_OWNER_IMPL(Me, Base)                                     \
    uint64 script::OwnerTypeId<Me>::operator()() const                  \
    {                                                                   \
        return typeid(Me).hash_code();                                  \
    }                                                                   \
                                                                        \
    template<>                                                          \
    script::Scriptable*                                                 \
        script::impl::upCast<script::Scriptable, Me>(Me& me)            \
    {                                                                   \
        return &me;                                                     \
    }                                                                   \
                                                                        \
    Me* script::FromString<Me>::operator()                              \
        (std::string_view string, script::Scriptable& ctx) const        \
    {                                                                   \
        auto* me =                                                      \
            script::StoredTarget<Me>::Handle::FromString(string, ctx);  \
                                                                        \
        return script::castSourceTo<Me>(me);                            \
    }                                                                   \
                                                                        \
                                                                        \
    Me* script::TargetCast<Me, Base>::operator()(Base& base) const      \

#define SCRIPT_OWNER_SHARED_IMPL(Me, Base) \
    SCRIPT_SHARED_HANDLE_IMPL(Me)          \
    SCRIPT_OWNER_IMPL(Me, Base)            \

}
