#pragma once

#include <vector>
#include <cstddef>

namespace dote {

/// \brief  A class that parses a TCP DNS packet that can add/remove EDNS padding
class DnsPacket
{
  public:
    /// \brief  Wrap a TCP DNS packet for modification
    ///
    /// \param packet  The TCP DNS packet to wrap
    explicit DnsPacket(std::vector<char> packet);

    /// \brief  Remove the EDNS padding from this packet
    ///
    /// \return  True if padding found and removed
    bool removeEdnsPadding();

    /// \brief  Get the wrapped packet
    ///
    /// \return  The wrapped packet
    const std::vector<char>& packet() const;

    /// \brief  Check if this packet is valid
    ///
    /// \return  True if the packet is valid
    bool valid() const;

    /// \brief  Get the UDP DNS packet
    ///
    /// \return  The UDP DNS packet
    char* data();

    /// \brief  Get the length of the UDP DNS packet
    ///
    /// \return  The length of the UDP DNS packet
    size_t length() const;

    /// \brief  Move the wrapped packet out, don't use
    ///         this instance after calling
    ///
    /// \return  The wrapped packet
    std::vector<char> move();

  private:
    /// \brief  Skip past a set of queries
    ///
    /// \param count  The number of queries
    /// \param it  The iterator at the start of the queries section
    ///
    /// \return  The iterator after the queries
    std::vector<char>::const_iterator skipQueries(
        unsigned short count,
        std::vector<char>::const_iterator it) const;

    /// \brief  Skip past a set of responses
    ///
    /// \param count  The number of responses
    /// \param it  The iterator at the start of the response section
    ///
    /// \return  The iterator after the responses
    std::vector<char>::const_iterator skipResponses(
        unsigned short count,
        std::vector<char>::const_iterator it) const;

    /// \brief  Remove EDNS padding from options section
    ///
    /// \param it  The start of the options section
    /// \param length  The length of the options section
    ///
    /// \return  The length of the options section after removing the padding
    unsigned short removePaddingOption(
        std::vector<char>::iterator it,
        unsigned short length);

    /// The wrapped packet
    std::vector<char> m_packet;
};
  
}  // namespace dote
