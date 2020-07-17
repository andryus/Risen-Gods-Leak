#include "RG/Premium/PremiumCodeMgr.hpp"
#include <cpr/api.h>

// create mock object to get acces to protected functions
class PremiumCodeMock : public PremiumCodeMgr
{
public:
    // override constructor to prevent calling super constructor
    PremiumCodeMock()
    {
    }

    PremiumCode ParseCodeTest(std::string code, cpr::Response& response)
    {
        return ParseCode(code, response);
    }

    bool CheckInvalidationTest(cpr::Response& response)
    {
        return CheckInvalidation(response);
    }
};