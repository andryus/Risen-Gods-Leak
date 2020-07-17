#pragma once

#include "Define.h"

// from blizzard lua
enum GMTicketSystemStatus
{
    GMTICKET_QUEUE_STATUS_DISABLED = 0,
    GMTICKET_QUEUE_STATUS_ENABLED = 1
};

enum GMTicketStatus
{
    GMTICKET_STATUS_HASTEXT = 0x06,
    GMTICKET_STATUS_DEFAULT = 0x0A
};

enum GMTicketResponse
{
    GMTICKET_RESPONSE_ALREADY_EXIST = 1,
    GMTICKET_RESPONSE_CREATE_SUCCESS = 2,
    GMTICKET_RESPONSE_CREATE_ERROR = 3,
    GMTICKET_RESPONSE_UPDATE_SUCCESS = 4,
    GMTICKET_RESPONSE_UPDATE_ERROR = 5,
    GMTICKET_RESPONSE_TICKET_DELETED = 9
};

// from Blizzard LUA:
// GMTICKET_ASSIGNEDTOGM_STATUS_NOT_ASSIGNED = 0;    -- ticket is not currently assigned to a gm
// GMTICKET_ASSIGNEDTOGM_STATUS_ASSIGNED = 1;        -- ticket is assigned to a normal gm
// GMTICKET_ASSIGNEDTOGM_STATUS_ESCALATED = 2;        -- ticket is in the escalation queue
enum GMTicketEscalationStatus
{
    TICKET_UNASSIGNED = 0,
    TICKET_ASSIGNED = 1,
    TICKET_IN_ESCALATION_QUEUE = 2
};

// from blizzard lua
enum GMTicketOpenedByGMStatus
{
    GMTICKET_OPENEDBYGM_STATUS_NOT_OPENED = 0,      // ticket has never been opened by a gm
    GMTICKET_OPENEDBYGM_STATUS_OPENED = 1       // ticket has been opened by a gm
};

enum LagReportType
{
    LAG_REPORT_TYPE_LOOT = 1,
    LAG_REPORT_TYPE_AUCTION_HOUSE = 2,
    LAG_REPORT_TYPE_MAIL = 3,
    LAG_REPORT_TYPE_CHAT = 4,
    LAG_REPORT_TYPE_MOVEMENT = 5,
    LAG_REPORT_TYPE_SPELL = 6
};
