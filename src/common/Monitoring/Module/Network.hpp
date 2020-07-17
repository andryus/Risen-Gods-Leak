#pragma once

#ifdef CONFIG_WITH_PROMETHEUS_MONITORING
#include "Monitoring/Module.hpp"

namespace RG { namespace Monitoring {

class Network : public Module
{
protected:
    Network();

public:
    static Network& Instance();
    static void ReportPacketSend(std::size_t bytes);
    static void ReportPacketReceived(std::size_t bytes);
    static void ReportPacketDropped(std::size_t bytes);

private:
    prometheus::Family<prometheus::Counter>& family_packet_send_count_total_;
    prometheus::Counter& packet_send_count_total_;
    prometheus::Family<prometheus::Counter>& family_packet_send_bytes_total_;
    prometheus::Counter& packet_send_bytes_total_;

    prometheus::Family<prometheus::Counter>& family_packet_received_count_total_;
    prometheus::Counter& packet_received_count_total_;
    prometheus::Family<prometheus::Counter>& family_packet_received_bytes_total_;
    prometheus::Counter& packet_received_bytes_total_;

    prometheus::Family<prometheus::Counter>& family_packet_dropped_count_total_;
    prometheus::Counter& packet_dropped_count_total_;
    prometheus::Family<prometheus::Counter>& family_packet_dropped_bytes_total_;
    prometheus::Counter& packet_dropped_bytes_total_;
};

}}

#define MONITOR_NETWORK(WHAT) ::RG::Monitoring::Network::WHAT
#else
#define MONITOR_NETWORK(WHAT)
#endif
