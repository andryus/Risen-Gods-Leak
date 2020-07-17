void RegisterGeneralHandlers();

void RegisterAccountHandlers();
void RegisterCharacterHandlers();
void RegisterEventHandlers();
void RegisterGuildHandlers();
void RegisterTicketHandlers();

void AddSC_api_handlers()
{
    RegisterGeneralHandlers();

    RegisterAccountHandlers();
    RegisterCharacterHandlers();
    RegisterEventHandlers();
    RegisterGuildHandlers();
    RegisterTicketHandlers();
}
