#include "API.h"

void AddSC_api_addonendpoint();
void AddSC_api_handlers();

GAME_API void AddSC_api()
{
    AddSC_api_addonendpoint();
    AddSC_api_handlers();
}
