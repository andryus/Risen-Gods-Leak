#pragma once

#include "ObjectGuid.h"
#include "Define.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <unordered_set>

class Unit;
class Spell;
class SpellCastTargets;

namespace RG { namespace Logging {

class SpellTimingPlayerScript;
class SpellTimingWorldScript;

class SpellCastLogEntry
{
public:
    void Start(const Unit* caster, const Spell* spell,  const SpellCastTargets* targets);

    void Cancel();
    void Success();

private:
    void WriteResult(bool canceled);

private:
#pragma pack(push, 1)
    struct
    {
        uint64 time_start;

        uint32 caster_guid;
        float  caster_position_x;
        float  caster_position_y;
        float  caster_position_z;
        uint16 caster_position_map;

        uint32 spell_id;

        uint32 target_mask;
        uint64 target_object_guid;
        uint32 target_item_id;
        uint64 target_item_guid;
        float  target_source_x;
        float  target_source_y;
        float  target_source_z;
        uint16 target_source_map;
        float  target_dest_x;
        float  target_dest_y;
        float  target_dest_z;
        uint16 target_dest_map;

        uint64 time_end;
        bool   canceled;
    } data;
#pragma pack(pop)
};

class SpellTiming
{
    friend class SpellTimingWorldScript;

private:
    struct FileHandle
    {
        std::chrono::system_clock::time_point last_rotation_;
        std::chrono::system_clock::time_point last_flush_;
        std::ofstream output_;

        template<typename T>
        void write(const T& value)
        {
            static_assert(std::is_pod<T>::value, "Can only write POD types");
            output_.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }
    };

    class EntryWriter
    {
        friend class SpellTiming;
    private:
        explicit EntryWriter(FileHandle& file_handle) :
                file_handle_(file_handle)
        {}

    public:
        template<typename T>
        void write(const T& data, const std::chrono::system_clock::time_point& now)
        {
            RotateIfNeeded(file_handle_, now);
            file_handle_.write(data);
            FlushIfNeeded(file_handle_, now);
        }

    private:
        FileHandle& file_handle_;
    };

public:
    static std::unique_ptr<SpellCastLogEntry> StartLog(const Unit* caster, const Spell* spell);
    static EntryWriter NewWriter();

private:
    static void Reload();
    static void Shutdown();

    static void RotateIfNeeded(FileHandle& handle, const std::chrono::system_clock::time_point& now);
    static void FlushIfNeeded(FileHandle& handle, const std::chrono::system_clock::time_point& now);

private:
    static bool enabled_;
    static std::unordered_set<uint32> enabled_maps_;
    static std::unordered_set<ObjectGuid> forced_players_;
    static bool ignore_triggered_;

    static std::string output_directory_;
    static std::chrono::seconds rotation_period_;
    static std::chrono::milliseconds flush_period_;

    thread_local static FileHandle file_handles_;
};

}}
