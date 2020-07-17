#pragma once

#define SCRIPT_DECL(NAME)   \
    void _refTo##NAME();    \

#define SCRIPT_FILE(NAME)     \
    void _refTo##NAME()       \
    {                         \
        static int x = 100;   \
    }                         \

#define SCRIPT_CALL(NAME)   \
    _refTo##NAME();         \
