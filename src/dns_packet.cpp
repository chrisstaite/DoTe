#include "dns_packet.h"

#include <arpa/inet.h>

namespace dote {

namespace {

/// \brief  A TCP DNS header format
struct __attribute__((packed)) DnsHeader
{
    unsigned short length;
    unsigned short id;
    unsigned short flags;
    unsigned short queries;
    unsigned short answers;
    unsigned short authorities;
    unsigned short additional;
};
static_assert(sizeof(DnsHeader) == 14, "DNS Header isn't packed");

/// The key for an EDNS additional section
constexpr unsigned short OPT = 41;

/// The EDNS option for EDNS padding
constexpr unsigned short PADDING = 12;

/// \brief  Get the DNS header from a packet
///
/// \param packet  The packet to get the header from
///
/// \return  The header or nullptr if not valid
const DnsHeader* getHeader(const std::vector<char>& packet)
{
    if (packet.size() < sizeof(DnsHeader))
    {
        return nullptr;
    }
    auto header = reinterpret_cast<const DnsHeader*>(packet.data());
    if (ntohs(header->length) != packet.size() - sizeof(header->length))
    {
        return nullptr;
    }
    return header;
}

/// \brief  Skip a DNS name
///
/// \param current  The start of the DNS name
/// \param end  The end iterator
///
/// \return  The iterator at the end of the name
std::vector<char>::const_iterator skipName(
        std::vector<char>::const_iterator current,
        std::vector<char>::const_iterator end)
{
    while (current != end)
    {
        if (*current == 0u)
        {
            return current + 1u;
        }
        else if ((*current & 0xc0) == 0xc0)
        {
            return current + 2u;
        }
        else if (end - current > *current + 1u)
        {
            current += *current + 1u;
        }
        else
        {
            current = end;
        }
    }
    return current;
}

/// \brief  Skip a fixed length
///
/// \param current  The start iterator
/// \param end  The end iterator
/// \param length  The number of bytes to skip
///
/// \return  The iterator at the end
std::vector<char>::const_iterator skipFixed(
        std::vector<char>::const_iterator current,
        std::vector<char>::const_iterator end,
        size_t length)
{
    if (end - current > length)
    {
        current += length;
    }
    else
    {
        current = end;
    }
    return current;
}

/// \brief  Get an unsigned short from a given iterator
///
/// \tparam It  The type of the iterator
///
/// \param it  The iterator to read the unsigned short from
///
/// \return  The value read
template<typename It>
unsigned short getShort(It it)
{
    return ntohs(*reinterpret_cast<const unsigned short*>(&*it));
}

/// \brief  Set an unsigned short to a given iterator
///
/// \param it  The iterator to write the unsigned short to
/// \param value  The value to set
void setShort(
        std::vector<char>::iterator it,
        unsigned short value)
{
    *reinterpret_cast<unsigned short*>(&*it) = htons(value);
}

}  // anon namespace

DnsPacket::DnsPacket(std::vector<char> packet) :
    m_packet(std::move(packet))
{ }

bool DnsPacket::removeEdnsPadding()
{
    auto header = getHeader(m_packet);
    if (header == nullptr)
    {
        return false;
    }
    auto it = m_packet.cbegin();
    it += sizeof(DnsHeader);
    it = skipQueries(ntohs(header->queries), it);
    it = skipResponses(ntohs(header->answers), it);
    it = skipResponses(ntohs(header->authorities), it);
    auto count = ntohs(header->additional);
    bool success = false;
    while (count-- && it != m_packet.end() && !success)
    {
        if (*it != 0u || m_packet.end() - it < 11u ||
                getShort(it + 1) != OPT)
        {
            it = skipResponses(1, it);
        }
        else
        {
            auto cDataLength = it + 9u;
            auto length = getShort(cDataLength);
            if (m_packet.end() - it >= length + 11u)
            {
                auto dataLength = m_packet.begin() +
                    (cDataLength - m_packet.cbegin());
                auto options = dataLength + sizeof(unsigned short);
                unsigned short optionsLength =
                    removePaddingOption(options, length);
                setShort(dataLength, optionsLength);
                unsigned short diff = length - optionsLength;
                reinterpret_cast<DnsHeader*>(m_packet.data())->length =
                    htons(ntohs(header->length) - diff);
                std::copy(options + diff, m_packet.end(), options);
                m_packet.resize(m_packet.size() - diff);
                success = true;
            }
        }
    }
    return success;
}

unsigned short DnsPacket::removePaddingOption(
    std::vector<char>::iterator it,
    unsigned short length)
{
    auto begin = it;
    auto end = it + length;
    while (it != end)
    {
        auto optLength = getShort(it + sizeof(unsigned short));
        auto fullLength = (sizeof(unsigned short) * 2) + optLength;
        if (getShort(it) == PADDING)
        {
            // Remove this one
            std::copy(it + fullLength, end, it);
            end -= fullLength;
        }
        else
        {
            // Skip other options
            it += fullLength;
        }
    }
    return it - begin;
}

std::vector<char>::const_iterator DnsPacket::skipQueries(
    unsigned short count,
    std::vector<char>::const_iterator it) const
{
    while (count-- && it != m_packet.end())
    {
        it = skipName(it, m_packet.end());
        it = skipFixed(it, m_packet.end(), 4u);
    }
    return it;
}

std::vector<char>::const_iterator DnsPacket::skipResponses(
    unsigned short count,
    std::vector<char>::const_iterator it) const
{
    while (count-- && it != m_packet.end())
    {
        it = skipName(it, m_packet.end());
        it = skipFixed(it, m_packet.end(), 8u);
        if (m_packet.end() - it >= sizeof(unsigned short))
        {
            auto length = getShort(it);
            it += sizeof(unsigned short);
            if (m_packet.end() - it >= length)
            {
                it += length;
            }
            else
            {
                it = m_packet.end();
            }
        }
        else
        {
            it = m_packet.end();
        }
    }
    return it;
}

const std::vector<char>& DnsPacket::packet() const
{
    return m_packet;
}

bool DnsPacket::valid() const
{
    return getHeader(m_packet) != nullptr;
}

char* DnsPacket::data()
{
    return m_packet.data() + 2;
}

size_t DnsPacket::length() const
{
    auto header = getHeader(m_packet);
    return header == nullptr ? 0u : ntohs(header->length);
}

std::vector<char> DnsPacket::move()
{
    return std::move(m_packet);
}

}  // namespace dote
