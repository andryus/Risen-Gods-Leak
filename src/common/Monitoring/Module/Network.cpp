#include "Monitoring/Module/Network.hpp"

using namespace RG::Monitoring;

Network::Network() :
    Module("network"),
    family_packet_send_count_total_(AddCounter("send_count_total", "Number of send packets")),
    packet_send_count_total_(family_packet_send_count_total_.Add({})),
    family_packet_send_bytes_total_(AddCounter("send_bytes_total", "Number of send bytes")),
    packet_send_bytes_total_(family_packet_send_bytes_total_.Add({})),
    family_packet_received_count_total_(AddCounter("received_count_total", "Number of received packets")),
    packet_received_count_total_(family_packet_received_count_total_.Add({})),
    family_packet_received_bytes_total_(AddCounter("received_bytes_total", "Number of received bytes")),
    packet_received_bytes_total_(family_packet_received_bytes_total_.Add({})),
    family_packet_dropped_count_total_(AddCounter("dropped_count_total", "Number of dropped packets")),
    packet_dropped_count_total_(family_packet_dropped_count_total_.Add({})),
    family_packet_dropped_bytes_total_(AddCounter("dropped_bytes_total", "Number of dropped bytes")),
    packet_dropped_bytes_total_(family_packet_dropped_bytes_total_.Add({}))
{}

Network& Network::Instance()
{
    static Network instance;
    return instance;
}

void Network::ReportPacketSend(std::size_t bytes)
{
    Instance().packet_send_count_total_.Increment();
    Instance().packet_send_bytes_total_.Increment(bytes);
}

void Network::ReportPacketReceived(std::size_t bytes)
{
    Instance().packet_received_count_total_.Increment();
    Instance().packet_received_bytes_total_.Increment(bytes);
}

void Network::ReportPacketDropped(std::size_t bytes)
{
    Instance().packet_dropped_count_total_.Increment();
    Instance().packet_dropped_bytes_total_.Increment(bytes);
}
