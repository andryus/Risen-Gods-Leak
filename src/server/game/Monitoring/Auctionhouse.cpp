#include "Auctionhouse.hpp"

using namespace RG::Monitoring;

Auctionhouse& Auctionhouse::Instance()
{
    static Auctionhouse auctionhouse;
    return auctionhouse;
}

Auctionhouse::Auctionhouse() :
        Module("auctionhouse"),
        family_queries_total_(AddCounter("queries_total", "Total number of executed auction house searches")),
        queries_filtered_total_(family_queries_total_.Add({{"type", "filtered"}})),
        queries_unfiltered_total_(family_queries_total_.Add({{"type", "unfiltered"}})),
        queries_fullscan_total_(family_queries_total_.Add({{"type", "fullscan"}})),
        family_auctions_created_total_(AddCounter("auctions_created_total", "Total number of created auctions")),
        auctions_created_(family_auctions_created_total_.Add({})),
        family_auctions_removed_total_(AddCounter("auctions_removed_total", "Total number of removed auctions")),
        auctions_expired_(family_auctions_removed_total_.Add({{"action", "expired"}})),
        auctions_canceled_(family_auctions_removed_total_.Add({{"action", "cancelled"}})),
        auctions_buyout_(family_auctions_removed_total_.Add({{"action", "buyout"}})),
        auctions_won_(family_auctions_removed_total_.Add({{"action", "won"}})),
        family_bids_total_(AddCounter("bids_total", "Total number of auction bids")),
        bids_total_(family_bids_total_.Add({}))
{}

void Auctionhouse::ReportQuery(bool filtered)
{
    if (filtered)
        Instance().queries_filtered_total_.Increment();
    else
        Instance().queries_unfiltered_total_.Increment();
}

void Auctionhouse::ReportFullscan()
{
    Instance().queries_fullscan_total_.Increment();
}

void Auctionhouse::ReportAuctionCreated()
{
    Instance().auctions_created_.Increment();
}

void Auctionhouse::ReportAuctionExpired()
{
    Instance().auctions_expired_.Increment();
}

void Auctionhouse::ReportAuctionCancelled()
{
    Instance().auctions_canceled_.Increment();
}

void Auctionhouse::ReportAuctionWon(bool buyout)
{
    if (buyout)
        Instance().auctions_buyout_.Increment();
    else
        Instance().auctions_won_.Increment();
}

void Auctionhouse::ReportAuctionBid()
{
    Instance().bids_total_.Increment();
}
