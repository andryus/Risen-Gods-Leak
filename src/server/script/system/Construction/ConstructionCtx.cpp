#include "ConstructionCtx.h"
#include "InvokationCtx.h"

namespace script
{

    void _createFile(InvokationCtx& ctx, bool verbose)
    {
        ctx.ChangeLogging(verbose ? LogType::VERBOSE : LogType::NORMAL);
    }

}
