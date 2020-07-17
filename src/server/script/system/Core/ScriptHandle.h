#pragma once
#include <string_view>

namespace script
{
    class Scriptable;

    template<class StoredT>
    struct SharedHandle {};

#define SCRIPT_SHARED_HANDLE(TYPE)                        \
    template<>                                            \
    struct script::SharedHandle<TYPE>                     \
    {                                                     \
        using Stored = TYPE;                              \
                                                          \
        SharedHandle();                                   \
        ~SharedHandle();                                  \
        SharedHandle(Stored* stored);                     \
        SharedHandle(Stored& stored);                     \
                                                          \
        TYPE* Load() const;                               \
                                                          \
        std::string Print() const;                        \
                                                          \
        static TYPE* FromString                           \
            (std::string_view string,                     \
                script::Scriptable&);                     \
                                                          \
    private:                                              \
                                                          \
        std::weak_ptr<TYPE> stored;                       \
    };                                                    \
                                                          \
    extern template struct script::SharedHandle<TYPE>;    \

#define SCRIPT_SHARED_HANDLE_IMPL(TYPE)                         \
namespace script {                                              \
    SharedHandle<TYPE>::SharedHandle() = default;               \
    SharedHandle<TYPE>::~SharedHandle() = default;              \
                                                                \
    SharedHandle<TYPE>::SharedHandle(TYPE* stored) :            \
        stored(stored ?                                         \
            stored->weak_from_this() : std::weak_ptr<TYPE>{})   \
        {}                                                      \
                                                                \
    SharedHandle<TYPE>::SharedHandle(TYPE& stored) :            \
        SharedHandle<TYPE>(&stored) {}                          \
                                                                \
    TYPE* SharedHandle<TYPE>::Load() const                      \
    {                                                           \
        return stored.lock().get();                             \
    }                                                           \
                                                                \
    TYPE* SharedHandle<TYPE>::FromString(                       \
        std::string_view string, script::Scriptable &)          \
    {                                                           \
        return nullptr;                                         \
    }                                                           \
                                                                \
    std::string SharedHandle<TYPE>::Print() const               \
    {                                                           \
        return #TYPE "???";                                     \
    }                                                           \
                                                                \
    template struct SharedHandle<TYPE>;                         \
}                                                               \


}