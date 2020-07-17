#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include "TestHelpers/DataSets.h"
#include "Shared.h"

using namespace combat;

namespace bdata = test::bdata;
namespace tt = boost::test_tools;

/************************************
* Test Setup
*************************************/

std::pair<AttackerParams, VictimParams> createBaseParams(bool attackerIsPlayer = false, bool victimIsPlayer = false)
{
    return { createAttacker(attackerIsPlayer, 1), createVictim(victimIsPlayer, 1) };
}

auto createBaseCheckTable(std::pair<AttackerParams, VictimParams> params)
{
    return Formulas::CreateBaseTable(params.first, params.second).checks;
}

/************************************
* Tests
*************************************/

BOOST_AUTO_TEST_SUITE(Combat);

BOOST_AUTO_TEST_SUITE(Formula);

// 
// Build
//

BOOST_AUTO_TEST_SUITE(Build);

// test generate table size

BOOST_AUTO_TEST_CASE(TableSizeTest)
{
    const auto table = Formulas::CreateBaseTable({player},{boss});

    BOOST_TEST(table.mods.size() == table.checks.size());
    BOOST_TEST(table.mods.size() == (uint8)AttackTableEntry::SIZE);
}

BOOST_AUTO_TEST_SUITE_END();

// 
// Basic input values
//

BOOST_AUTO_TEST_SUITE(Input);

// Check if the base-table value-output matches our expected results
auto createInputValueTable()
{
    auto [attacker, victim] = createBaseParams();

    attacker.crit = 0.0f;

    attacker.hit = 1.0f;
    victim.miss = 2.0f;

    attacker.expertise = 3.0f;
    victim.parry = 7.5f;
    victim.dodge = 4.75f;

    victim.block = 4.0f;
    victim.resist = 5.0f;
    victim.deflect = 6.0f;

    return Formulas::CreateBaseTable(attacker, victim).mods;
}

BOOST_DATA_TEST_CASE(
    TableValueTest,
    test::createContainerDataset(createInputValueTable()) ^ bdata::make(
        { 
            1.0f,   // 0 - miss
            5.0f,   // 1 - resist
            6.0f,   // 2 - deflect
            1.75f,  // 3 - dodge
            4.5f,   // 4 - parry
            9.6f,   // 5 - glancing
            4.0f,   // 6 - block
            0.0f,   // 7- crit
           -15.0f   // 8- crushing
        }),
    tableEntry, expected)
{
    BOOST_TEST(tableEntry == expected, tt::tolerance(0.0001f));
}

BOOST_AUTO_TEST_SUITE_END();

// 
// Variation of different attacker-victimsetups => todo: somehow parametrize into one test?
//

BOOST_AUTO_TEST_SUITE(Combatants);

// Check attack from mob to mob

// todo: find out whats wrong about this
/*constexpr bool allowedAttacksVersus[] =
{
//  miss  resi  defl   doge  pry   glance block crit crush
    true, true, false, true, true, false, true, true, true,     // mob -> mob
    true, true, false, true, true, false, true, true, false,    // player -> player
    true, true, false, true, true, false, true, true, true,     // mob -> player
    true, true, false, true, true, true,  true, true, false     // player -> mob
};*()

BOOST_DATA_TEST_CASE(
    Mob_MobTest,
    (   
        test::createContainerDataset(createBaseCheckTable(createBaseParams(false, false))) +
        test::createContainerDataset(createBaseCheckTable(createBaseParams(true, true))) +
        test::createContainerDataset(createBaseCheckTable(createBaseParams(false, true))) +
        test::createContainerDataset(createBaseCheckTable(createBaseParams(true, false))) 
    ) ^ bdata::make(allowedAttacksVersus),
    allowedEntry, expected)
{
    BOOST_TEST(allowedEntry == expected);
}*/

// Check attack from player to player

BOOST_DATA_TEST_CASE(
    Player_PlayerTest,
    test::createContainerDataset(createBaseCheckTable(createBaseParams(true, true))) ^ bdata::make(
        {
            true,    // 0 - miss
            true,    // 1 - resist
            false,   // 2 - deflect
            true,    // 3 - dodge
            true,    // 4 - parry
            false,   // 5 - glancing
            true,    // 6 - block
            true,    // 7 - crit
            false    // 8- crushing
        }),
    allowedEntry, expected)
{
    BOOST_TEST(allowedEntry == expected);
}

// Check attack from mob to player

BOOST_DATA_TEST_CASE(
    Mob_PlayerTest,
    test::createContainerDataset(createBaseCheckTable(createBaseParams(false, true))) ^ bdata::make(
        {
            true,    // 0 - miss
            true,    // 1 - resist
            false,   // 2 - deflect
            true,    // 3 - dodge
            true,    // 4 - parry
            false,   // 5 - glancing
            true,    // 6 - block
            true,    // 7 - crit
            true     // 8- crushing
        }),
    allowedEntry, expected)
{
    BOOST_TEST(allowedEntry == expected);
}

// Check attack from player to mob

BOOST_DATA_TEST_CASE(
    Player_MobTest,
    test::createContainerDataset(createBaseCheckTable(createBaseParams(true, false))) ^ bdata::make(
        {
            true,    // 0 - miss
            true,    // 1 - resist
            false,   // 2 - deflect
            true,    // 3 - dodge
            true,    // 4 - parry
            true,    // 5 - glancing
            true,    // 6 - block
            true,    // 7 - crit
            false    // 8- crushing
        }),
    allowedEntry, expected)
{
    BOOST_TEST(allowedEntry == expected);
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(Variations);

// Check attack from behind

auto createBehindValueTable(bool isPlayer)
{
    auto [attacker, victim] = createBaseParams(false, isPlayer);

    victim.isAttackedFromBehind = true;

    return createBaseCheckTable({attacker, victim});
}

BOOST_DATA_TEST_CASE( // MOB
    Mob_BehindTest,
    test::createContainerDataset(createBehindValueTable(false)) ^ bdata::make(
        {
            true,    // 0 - miss
            true,    // 1 - resist
            false,   // 2 - deflect
            true,    // 3 - dodge
            false,   // 4 - parry
            false,   // 5 - glancing
            false,    // 6 - block
            true,    // 7 - crit
            true    // 8- crushing
        }),
    allowedEntry, expected)
{
    BOOST_TEST(allowedEntry == expected);
}

BOOST_DATA_TEST_CASE( // PLAYER
    Player_BehindTest,
    test::createContainerDataset(createBehindValueTable(true)) ^ bdata::make(
        {
            true,    // 0 - miss
            true,    // 1 - resist
            false,   // 2 - deflect
            false,    // 3 - dodge
            false,   // 4 - parry
            false,   // 5 - glancing
            false,    // 6 - block
            true,    // 7 - crit
            true    // 8- crushing
        }),
    allowedEntry, expected)
{
    BOOST_TEST(allowedEntry == expected);
}

// Check attack against defenseless target

auto createDefenslessValueTable(bool isPlayer)
{
    auto[attacker, victim] = createBaseParams(false, isPlayer);

    victim.cantDefend = true;

    return createBaseCheckTable({ attacker, victim });
}

BOOST_DATA_TEST_CASE( // MOB
    Mob_DefenselessTest,
    test::createContainerDataset(createDefenslessValueTable(false)) ^ bdata::make(
        {
            true,    // 0 - miss
            true,    // 1 - resist
            false,   // 2 - deflect
            false,    // 3 - dodge
            false,   // 4 - parry
            false,   // 5 - glancing
            false,    // 6 - block
            true,    // 7 - crit
            true    // 8- crushing
        }),
    allowedEntry, expected)
{
    BOOST_TEST(allowedEntry == expected);
}

BOOST_DATA_TEST_CASE( // PLAYER
    Player_DefenselessTest,
    test::createContainerDataset(createDefenslessValueTable(true)) ^ bdata::make(
        {
            true,    // 0 - miss
            true,    // 1 - resist
            false,   // 2 - deflect
            false,    // 3 - dodge
            false,   // 4 - parry
            false,   // 5 - glancing
            false,    // 6 - block
            true,    // 7 - crit
            true    // 8- crushing
        }),
    allowedEntry, expected)
{
    BOOST_TEST(allowedEntry == expected);
}

// Check ranged attacks

auto createRangedValueTable()
{
    auto[attacker, victim] = createBaseParams();

    attacker.isRanged = true;

    return createBaseCheckTable({ attacker, victim });
}

BOOST_DATA_TEST_CASE( // RANGED
    RangedTest,
    test::createContainerDataset(createRangedValueTable()) ^ bdata::make(
        {
            true,    // 0 - miss
            true,    // 1 - resist
            true,    // 2 - deflect
            true,    // 3 - dodge
            true,    // 4 - parry
            false,    // 5 - glancing
            true,    // 6 - block
            true,    // 7 - crit
            true    // 8- crushing
        }),
    allowedEntry, expected)
{
    BOOST_TEST(allowedEntry == expected);
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();