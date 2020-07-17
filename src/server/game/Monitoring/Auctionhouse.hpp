#pragma once

#ifdef CONFIG_WITH_PROMETHEUS_MONITORING
#include <Monitoring/Module.hpp>

namespace RG { namespace Monitoring {

class Auctionhouse : public Module
{
private:
    Auctionhouse();

public:
    static Auctionhouse& Instance();
    static void ReportQuery(bool filtered);
    static void ReportFullscan();
    static void ReportAuctionCreated();
    static void ReportAuctionExpired();
    static void ReportAuctionCancelled();
    static void ReportAuctionWon(bool buyout);
    static void ReportAuctionBid();

private:
    prometheus::Family<prometheus::Counter>& family_queries_total_;
    prometheus::Counter& queries_filtered_total_;
    prometheus::Counter& queries_unfiltered_total_;
    prometheus::Counter& queries_fullscan_total_;

    prometheus::Family<prometheus::Counter>& family_auctions_created_total_;
    prometheus::Counter& auctions_created_;
    prometheus::Family<prometheus::Counter>& family_auctions_removed_total_;
    prometheus::Counter& auctions_expired_;
    prometheus::Counter& auctions_canceled_;
    prometheus::Counter& auctions_buyout_;
    prometheus::Counter& auctions_won_;

    prometheus::Family<prometheus::Counter>& family_bids_total_;
    prometheus::Counter& bids_total_;
};

}}
#define MONITOR_AUCTIONHOUSE(WHAT) ::RG::Monitoring::Auctionhouse::WHAT
#else
#define MONITOR_AUCTIONHOUSE(WHAT)
#endif
