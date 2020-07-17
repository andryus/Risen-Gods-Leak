#pragma once
#include "CombatDefines.h"
#include "Utilities/Random.h"
#include "Utilities/Span.h"

namespace combat
{
    template<typename ResultType>
    using AttackTableEntryData = std::tuple<AttackTableEntry, float, ResultType>;
    template<typename ResultType, uint8 Size>
    using AttackTable = std::array<AttackTableEntryData<ResultType>, Size>;

    template<typename ResultType>
    using TableFormatEntry = std::pair<AttackTableEntry, ResultType>;
    template<typename ResultType, size_t Size>
    using TableFormatRawArray = const TableFormatEntry<ResultType>(&)[Size];
    template<typename ResultType, size_t Size>
    using TableFormatArray = std::array<TableFormatEntry<ResultType>, Size>;
    template<typename ResultType>
    using TableFormatSpan = span<TableFormatEntry<ResultType>>;

    class AttackTableHandler
    {
    public:

        template<typename ResultType, size_t Size>
        static constexpr auto BuildAttackTable(const BaseTable& table, TableFormatRawArray<ResultType, Size> format) { return AttackTableHandler::BuildAttackTableImpl<Size>(table, make_span(format)); }
        template<typename ResultType, size_t Size>
        static constexpr auto BuildAttackTable(const BaseTable& table, TableFormatArray<ResultType, Size> format) { return AttackTableHandler::BuildAttackTableImpl<Size>(table, make_span(format)); }

        template<typename ResultType, size_t Size>
        static ResultType RollAttackTable(AttackTable<ResultType, Size> attackTable, ResultType defaultResult);

    private:

        template<uint8 Size, typename ResultType>
        static constexpr auto BuildAttackTableImpl(const BaseTable& table, TableFormatSpan<ResultType> format)
        {
            const auto attackTable = table.mods;
            const auto canEntryHappen = table.checks;

            // build full attack table
            AttackTable<ResultType, Size> outTable;

            uint8 index = 0;
            for (const auto[entry, result] : format)
            {
                auto&[entryType, entryValue, resultType] = outTable[index];
                entryType = entry;
                resultType = result;

                if (!canEntryHappen[entry])
                    entryValue = 0.0f;
                else
                    entryValue = std::max(attackTable[entry], 0.0f);

                index++;
            }

            return outTable;
        }
    };

    template<typename ResultType, size_t Size>
    ResultType AttackTableHandler::RollAttackTable(AttackTable<ResultType, Size> attackTable, ResultType defaultResult)
    {
        const auto roll = frand(0.0001f, 100.0f); // should never roll zero, otherwise attacks might miss

        float sum = 0;
        for (const auto[entry, percent, result] : attackTable)
        {
            (void)entry;

            // table-generation code should fixup negative values
            ASSERT(percent >= 0.0f);

            sum += percent;

            if (roll <= sum)
            {
                ASSERT(percent);

                return result;
            }
        }

        return defaultResult;
    }


}
