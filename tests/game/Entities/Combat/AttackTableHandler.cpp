#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include "TestHelpers/DataSets.h"
#include "Entities/Combat/AttackTableHandler.h"

// 
// Final attack table, arbitrary input
//

using namespace combat;

namespace bdata = boost::unit_test::data;
namespace tt = boost::test_tools;

/************************************
* Test Setup - Helper
*************************************/

// helper functions

template<uint8 Size = 9>
constexpr auto createDefaultFormat()
{
    TableFormatArray<uint8, Size> format = {};

    for (uint8 i = 0; i < Size; i++)
    {
        format[i].first = (AttackTableEntry)i;
        format[i].second = i;
    }

    return format;
};

template<uint8 Size>
using Array = std::array<std::pair<float, bool>, 9>;

template<size_t Size>
constexpr auto createInputAttackTable(std::array<std::pair<float, bool>, Size> values, TableFormatArray<uint8, Size> format)
{
    BaseTable table = {};

    for (const auto fmt : format)
    {
        const auto entry = fmt.first;

        table.mods[entry] = values[(uint8)entry].first;
        table.checks[entry] = values[(uint8)entry].second;
    }

    return table;
}

template<uint32 Size>
constexpr auto createAttackTable(BaseTable table, TableFormatArray<uint8, Size> format)
{
    return AttackTableHandler::BuildAttackTable(table, format);
}

/************************************
* Tests
*************************************/

BOOST_AUTO_TEST_SUITE(Combat);

BOOST_AUTO_TEST_SUITE(AttackTable);
// 
// Attack-Table, arbitrary input
//

// check if build-process generally gives us expected output

constexpr auto valueRemapper = [](AttackTableEntryData<uint8> entry) {return std::get<1>(entry); };

BOOST_AUTO_TEST_SUITE(Construction);

namespace 
{
    constexpr float defaultEntries[] =
    {
        1.0f,
        6.87f,
        3.23f,
        2.1f,
        42.7f,
        -10.18f,
        -5.64f,
        0.0f,
        5.9f,
    };

    constexpr auto defaultTableFormat = createDefaultFormat();

    constexpr auto defaultProvidedTable = createInputAttackTable<9>(
        { {
            { defaultEntries[0], true },
            { defaultEntries[1], true },
            { defaultEntries[2], true },
            { defaultEntries[3], true },
            { defaultEntries[4], true },
            { defaultEntries[5], true },
            { defaultEntries[6], true },
            { defaultEntries[7], true },
            { defaultEntries[8], true }
            } }, defaultTableFormat
        );

    // test size of full formatted table
    BOOST_AUTO_TEST_CASE(Full_SizeTest)
    {
        const auto tableSize = createAttackTable<9>(defaultProvidedTable, defaultTableFormat).size();

        BOOST_TEST(tableSize == (uint8)AttackTableEntry::SIZE);
        BOOST_TEST(tableSize == defaultProvidedTable.mods.size());
        BOOST_TEST(tableSize == defaultTableFormat.size());
    }

    // test size of only partially used table

    constexpr auto partialTableFormat = createDefaultFormat<4>();

    constexpr auto partialProvidedTable = createInputAttackTable<4>(
        { {
            { defaultEntries[0], true },
            { defaultEntries[1], true },
            { defaultEntries[2], true },
            { defaultEntries[3], true },
            }}, partialTableFormat
        );

    BOOST_AUTO_TEST_CASE(Partial_SizeTest)
    {
        const auto tableSize = createAttackTable<4>(partialProvidedTable, partialTableFormat).size();

        BOOST_TEST(tableSize == 4);
        BOOST_TEST(tableSize == partialTableFormat.size());
    }

    // test values

    BOOST_DATA_TEST_CASE(
        ResultTest,
        test::createContainerDataset(createAttackTable<9>(defaultProvidedTable, defaultTableFormat), valueRemapper) ^ bdata::make(
            defaultEntries),
        tableEntry, expected)
    {
        BOOST_TEST(tableEntry == std::max(0.0f, expected), tt::tolerance(0.0001f));
    }
}

// check if building correctly handles different value-combinations

namespace
{
    constexpr auto singleFormat = createDefaultFormat<1>();

    constexpr auto buildSingleTable(bool isNegative, bool isActive)
    {
        return createAttackTable<1>(createInputAttackTable<1>({ { { 1.0f * (isNegative ? -1.0f : 1.0f), isActive } } }, singleFormat), singleFormat).front();
    };

    // deactivation test
    const std::array<AttackTableEntryData<uint8>, 2> activationTables =
    {
        buildSingleTable(false, true),     // Active
        buildSingleTable(false, false),    // Inactive
    };

    BOOST_DATA_TEST_CASE(AttackTableValueDeactivationTest,
        test::createContainerDataset(activationTables, valueRemapper) ^ bdata::make(
            {
                true,
                false,
            }),
        chance, expected)
    {
        if (expected)
            BOOST_TEST(chance > 0.0f);
        else
            BOOST_TEST(chance == 0.0f);
    }

    // cap test
    const std::array<AttackTableEntryData<uint8>, 1> signTable =
    {
        buildSingleTable(true, true),
    };

    BOOST_AUTO_TEST_CASE(AttackTableValueCapNegativeTest)
    {
        BOOST_TEST(std::get<1>(signTable[0]) == 0.0f);
    }
}

BOOST_AUTO_TEST_SUITE_END();


// check if roll-function gives statistically correct results

namespace 
{
    BOOST_AUTO_TEST_SUITE(Roll);

    constexpr std::array<float, 4> values =
    {
        10.0f,
        20.0f,
        5.0f,
        60.0f,
    };

    const auto tolerance = tt::tolerance(0.02f);

    template<size_t Size>
    auto makeExpected(std::array<float, Size> expected, float default)
    {
        std::array<float, Size + 1> out;

        for (const uint8 i : iterateTo(Size))
            out[i] = expected[i];

        out.back() = default;

        return out;
    }

    template<uint8 Size>
    auto rollForValues(std::array<float, Size> values)
    {
        std::array<std::pair<float, bool>, Size> inputTable;
        for (auto [value, tableInput] : iterateMulti(values, inputTable))
        {
            tableInput = { value, true};
        }

        constexpr auto format = createDefaultFormat<Size>();
        const auto table = createAttackTable<Size>(createInputAttackTable<Size>(inputTable, format), format);

        constexpr uint32 numIterations = 1000000;
        uint32 numResults[Size + 1] = {};

        for (const uint8 i : iterateTo(numIterations))
        {
            const auto result = AttackTableHandler::RollAttackTable<uint8, Size>(table, Size);

            numResults[result]++;
        }

        std::array<float, Size + 1> percentValues;
        for (const auto [num, percent] : iterateMulti(numResults, percentValues))
        {
            percent = (num / (float)numIterations) * 100.0f;
        }

        return percentValues;
    }

    // statistic distribution

    BOOST_DATA_TEST_CASE(AttackTableRollStatisticalTest,
        test::createContainerDataset(rollForValues(values)) ^ bdata::make(makeExpected(values, 5.0f)),
        rolled, expected)
    {
        BOOST_TEST(rolled == expected, tolerance); // todo: calculate stastical error-tolerance?
    }

    // empty table
    // table should be all 0%

    constexpr std::array<float, 3> emptyTestValues =
    {
        0.0f,
        0.0f,
        0.0f,
    };

    BOOST_DATA_TEST_CASE(AttackTableRollEmptyTest,
        test::createContainerDataset(rollForValues(emptyTestValues)) ^ bdata::make(makeExpected(emptyTestValues, 100.0f)),
        rolled, expected)
    {
        BOOST_TEST(rolled == expected);
    }

    // default-value
    // table should be 33.3% our value, 66.6% default

    constexpr std::array<float, 1> defaultTestvalues =
    {
        33.333333333f
    };

    BOOST_DATA_TEST_CASE(AttackTableRollDefaultTest,
        test::createContainerDataset(rollForValues(defaultTestvalues)) ^ bdata::make(makeExpected(defaultTestvalues, 66.66666666f)),
        rolled, expected)
    {
        BOOST_TEST(rolled == expected, tolerance);
    }

    // push-off
    // table should cut off top 5% or #2, and totally leave out #3 + defaults

    constexpr std::array<float, 3> pushOffTestvalues =
    {
        25.0f,
        80.0f,
        30.0f,
    };

    constexpr std::array<float, 3> pushOffExpectedValues =
    {
        25.0f,
        75.0f,
        0.0f
    };

    BOOST_DATA_TEST_CASE(AttackTableRollPushOffTest,
        test::createContainerDataset(rollForValues(pushOffTestvalues)) ^ bdata::make(makeExpected(pushOffExpectedValues, 0.0f)),
        rolled, expected)
    {
        BOOST_TEST(rolled == expected, tolerance);
    }

    BOOST_AUTO_TEST_SUITE_END();
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();
