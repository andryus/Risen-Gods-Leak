// This file was created automatically from your script configuration!
// Use CMake to reconfigure this file, never change it on your own!

#cmakedefine SCRIPT_DYNAMIC_RELOAD

#include "ScriptLoadRef.h"

@SCRIPTS_FORWARD_DECL@

#ifdef SCRIPT_DYNAMIC_RELOAD
#include "revision_data.h"

#if WIN32
#  define CONTENT_API_EXPORT __declspec(dllexport)
#  define CONTENT_API_IMPORT __declspec(dllimport)
#else
#  define CONTENT_API_EXPORT __attribute__((visibility("default")))
#  define CONTENT_API_IMPORT
#endif

extern "C" {
#else
#  define CONTENT_API_EXPORT
#  define CONTENT_API_IMPORT
#endif

void CONTENT_API_EXPORT registerScripts()
{
@SCRIPTS_INVOKE@}

#ifdef SCRIPT_DYNAMIC_RELOAD
}
#endif
