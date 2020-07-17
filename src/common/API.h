#pragma once

#ifdef ENABLE_SCRIPT_RELOAD
#  if WIN32
#    define API_EXPORT __declspec(dllexport)
#    define API_IMPORT __declspec(dllimport)
#  elif
#    define API_EXPORT __attribute__((visibility("default")))
#    define API_IMPORT
#  else
#    error compiler not supported!
#  endif
#else
#  define API_EXPORT
#  define API_IMPORT
#endif

#ifdef EXPORT_GAME
#  define GAME_API API_EXPORT
#else
#  define GAME_API API_IMPORT
#endif

#ifdef EXPORT_DB
#  define DB_API API_EXPORT
#else
#  define DB_API API_IMPORT
#endif

#ifdef EXPORT_COMMON
#  define COMMON_API API_EXPORT
#else
#  define COMMON_API API_IMPORT
#endif

#ifdef EXPORT_SCRIPT
#  define SCRIPT_API API_EXPORT
#else
#  define SCRIPT_API API_IMPORT
#endif
