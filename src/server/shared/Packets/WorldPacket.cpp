#include "WorldPacket.h"
#include "MessageBuffer.h"

WorldPacket::WorldPacket() : ByteBuffer(WorldPacket::ResourceSize) { }

WorldPacket::WorldPacket(uint16 opcode, size_t res) : ByteBuffer(res), m_opcode(opcode) { }

WorldPacket::WorldPacket(WorldPacket&& packet) : ByteBuffer(std::move(packet)), m_opcode(packet.m_opcode)
{
    packet.m_opcode = 0;
}

WorldPacket::WorldPacket(uint16 opcode, MessageBuffer&& buffer) : ByteBuffer(buffer.Move()), m_opcode(opcode) { }

WorldPacket::WorldPacket(uint16 opcode, ByteBuffer && buffer) : ByteBuffer(std::move(buffer)), m_opcode(opcode) { }

void WorldPacket::Initialize(uint16 opcode, size_t newres)
{
    clear();
    reserve(newres);
    m_opcode = opcode;
}
