#include <boost/test/unit_test.hpp>

#include "PremiumCodeMock.hpp"

BOOST_AUTO_TEST_CASE(PremiumCodeTest)
{
    PremiumCodeMock mgr;

    static const std::pair<CodeType, std::string> dbCodeNames[] =
    {
        { CodeType::Unknown, "UNKNOWN" },
        { CodeType::CharMoveOnRealm, "CHARMOVEONREALM" },
        { CodeType::CharChangeName, "RENAME" },
        { CodeType::CharCustomize, "CUSTOMIZE" },
        { CodeType::CharChangeRace, "RACECHANGE" },
        { CodeType::CharChangeFaction, "FACTIONCHANGE" },
        { CodeType::Pet, "PET" },
        { CodeType::TradingCard, "TRADINGCARDGOODS" }
    };

    for (auto codeMap : dbCodeNames)
    {
        auto response = cpr::Response();
        response.status_code = 200;
        response.text = "{\"type\":\"" + codeMap.second + "\",\"used\":0}";
        response.error = cpr::Error();

        auto premium = mgr.ParseCodeTest("afas54", response);

        BOOST_CHECK(premium.IsValid());
        BOOST_CHECK(premium.GetType() == codeMap.first);
        BOOST_CHECK(premium.GetAmount() == 1);
    }
}