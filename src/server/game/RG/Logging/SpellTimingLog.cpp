#include "SpellTimingLog.hpp"

#include "Spell.h"
#include "ScriptMgr.h"
#include "Config.h"
#include <boost/algorithm/string/trim_all.hpp>

using namespace RG::Logging;

#define DEFINE(what__) decltype(what__) what__

DEFINE(SpellTiming::enabled_)(false);
DEFINE(SpellTiming::enabled_maps_);
DEFINE(SpellTiming::forced_players_);
DEFINE(SpellTiming::ignore_triggered_)(true);
DEFINE(SpellTiming::output_directory_)("./spell_timing/");
DEFINE(SpellTiming::rotation_period_)(std::chrono::seconds(1 * DAY));
DEFINE(SpellTiming::flush_period_)(std::chrono::seconds(1 * MINUTE));

thread_local SpellTiming::FileHandle SpellTiming::file_handles_;

#undef DEFINE

void SpellTiming::RotateIfNeeded(FileHandle& handle, const std::chrono::system_clock::time_point& now)
{
    if (now - handle.last_rotation_ >= rotation_period_)
    {
        if (handle.output_.is_open())
        {
            handle.output_.flush();
            handle.output_.close();
        }

        std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&time, &tm);

        std::ostringstream tid;
        tid << std::this_thread::get_id();

        std::string filename = Trinity::StringFormat(
                "%04u-%02u-%02u_%02u-%02u-%02u_spelltiming.%s",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                tid.str().c_str());
        handle.output_.open(output_directory_ + "/" + filename + ".bin", std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
        handle.last_rotation_ = now;
    }
}

void SpellTiming::FlushIfNeeded(FileHandle& handle, const std::chrono::system_clock::time_point& now)
{
    if (now - handle.last_flush_ >= flush_period_)
    {
        if (handle.output_.is_open())
            handle.output_.flush();

        handle.last_flush_ = now;
    }
}

void SpellTiming::Reload()
{
    enabled_            = sConfigMgr->GetBoolDefault("RG.Logging.SpellTiming.Enabled", false);
    ignore_triggered_   = sConfigMgr->GetBoolDefault("RG.Logging.SpellTiming.IgnoreTriggered", true);
    output_directory_   = sConfigMgr->GetStringDefault("RG.Logging.SpellTiming.OutputDir", "./spell_timing/");
    rotation_period_    = std::chrono::seconds(sConfigMgr->GetIntDefault("RG.Logging.SpellTiming.RotationPeriod", 1 * DAY));
    flush_period_       = std::chrono::milliseconds(sConfigMgr->GetIntDefault("RG.Logging.SpellTiming.FlushPeriod", 1 * MINUTE * IN_MILLISECONDS));

    std::istringstream mis{ sConfigMgr->GetStringDefault("RG.Logging.SpellTiming.EnabledMaps", "559,562,572,617,618") };
    enabled_maps_.clear();
    for (std::string line; std::getline(mis, line, ',');)
    {
        boost::trim_all(line);
        enabled_maps_.emplace(std::stoul(line));
    }

    std::istringstream pis{ sConfigMgr->GetStringDefault("RG.Logging.SpellTiming.ForcedPlayers", "") };
    forced_players_.clear();
    for (std::string line; std::getline(pis, line, ',');)
    {
        boost::trim_all(line);
        forced_players_.emplace(ObjectGuid(static_cast<uint64>(std::stoull(line))));
    }

    if (enabled_maps_.empty() && forced_players_.empty())
        enabled_ = false;
}

void SpellTiming::Shutdown()
{
    enabled_ = false;
}

std::unique_ptr<SpellCastLogEntry> SpellTiming::StartLog(const Unit* caster, const Spell* spell)
{
    if (!enabled_)
        return {};

    if (!(caster->GetTypeId() == TYPEID_PLAYER || caster->IsPet()))
        return {};

    if (ignore_triggered_ && spell->IsTriggered())
        return {};

    if (forced_players_.count(caster->GetGUID()) != 0)
        return std::make_unique<SpellCastLogEntry>();

    if (enabled_maps_.count(caster->GetMapId()) == 0)
        return {};

    return std::make_unique<SpellCastLogEntry>();
}

SpellTiming::EntryWriter SpellTiming::NewWriter()
{
    return SpellTiming::EntryWriter(file_handles_);
}

void SpellCastLogEntry::Start(const Unit* caster, const Spell* spell, const SpellCastTargets* targets)
{
    const auto now = std::chrono::system_clock::now();

    data.time_start = static_cast<uint64>(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());

    data.caster_guid = caster->GetGUID().GetCounter();
    caster->GetPosition(data.caster_position_x, data.caster_position_y, data.caster_position_z);
    data.caster_position_map = static_cast<uint16>(caster->GetMapId());

    data.spell_id = spell->GetSpellInfo()->Id;

    data.target_mask = targets->GetTargetMask();
    data.target_object_guid = static_cast<uint64>(targets->GetObjectTargetGUID());
    data.target_item_guid = static_cast<uint64>(targets->GetItemTargetGUID());
    data.target_item_id = targets->GetItemTargetEntry();

    const auto& src_loc = targets->GetSrc()->_position;
    src_loc.GetPosition(data.target_source_x, data.target_source_y, data.target_source_z);
    data.target_source_map = static_cast<uint16>(src_loc.GetMapId());

    const auto& dst_loc = targets->GetSrc()->_position;
    dst_loc.GetPosition(data.target_dest_x, data.target_dest_y, data.target_dest_z);
    data.target_dest_map = static_cast<uint16>(dst_loc.GetMapId());
}

void SpellCastLogEntry::Cancel()
{
    WriteResult(true);
}

void SpellCastLogEntry::Success()
{
    WriteResult(false);
}

void SpellCastLogEntry::WriteResult(bool canceled)
{
    const auto now = std::chrono::system_clock::now();

    data.time_end = static_cast<uint64>(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
    data.canceled = canceled;

    auto writer = SpellTiming::NewWriter();
    writer.write(data, now);
}

namespace RG { namespace Logging {

class SpellTimingWorldScript : public WorldScript
{
public:
    SpellTimingWorldScript() : WorldScript("rg_logging_spell_timing_world") {}

    void OnConfigLoad(bool reload) override
    {
        SpellTiming::Reload();
    }

    void OnShutdown() override
    {
        SpellTiming::Shutdown();
    }
};

void AddSC_rg_logging_spell_timing()
{
    new SpellTimingWorldScript();
}

}}
