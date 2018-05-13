
#pragma once

#include <sys/socket.h>

#include <vector>
#include <string>

namespace dote {

/// \brief  Parse command line configuration
class ConfigParser
{
  public:
    /// \brief  All the details required to connect to a forwarder
    struct Forwarder
    {
        /// The IP and port to connect to and forward to
        sockaddr_storage remote;
        /// The host to verify the certificate common name against
        std::string host;
        /// The base64 encoded SHA-256 hash of the certificate
        std::vector<unsigned char> pin;
    };

    /// \brief  The server configuration to listen on
    struct Server
    {
        /// The IP and port to bind to
        sockaddr_storage address;
    };

    /// \brief  Parse the configuration
    ///
    /// \param argc  The number of arguments in argv
    /// \param argv  The parameters passed in
    ConfigParser(int argc, char* const argv[]);

    ConfigParser(const ConfigParser&) = delete;
    ConfigParser& operator=(const ConfigParser&) = delete;

    /// \brief  Whether the parameters were valid or not
    ///
    /// \return  True if the parameters were parsed correctly
    bool valid() const;

    /// \brief  Get the forwarders that were passed in
    ///
    /// \return  The forwarders that were configured
    const std::vector<Forwarder>& forwarders() const;

    /// \brief  Get the servers that were passed in
    ///
    /// \return  The servers that were configured
    const std::vector<Server>& servers() const;

    /// \brief  Get the ciphers to use
    ///
    /// \return  The ciphers to use
    const std::string& ciphers() const;

    /// \brief  Get the IP address to connect to and get the
    ///         hostname and pin for, if requested
    ///
    /// \return  The IP address to get the hostname and pin for
    const sockaddr_storage& ipLookup() const;

    /// \brief  Get the maximum number of outgoing forwarder
    ///         connections to have open at a single time
    ///
    /// \return  The maximum number of connections
    std::size_t maxConnections() const;

    /// \brief  Whether the process should fork and daemonise
    ///
    /// \return  True if it should fork
    bool daemonise() const;

    /// \brief  Get the path for the PID file that should be
    ///         written with the PID of the process
    ///
    /// \return  The path to write or an empty string not to
    const std::string& pidFile() const;

  private:
    /// \brief  Parse the configuration
    ///
    /// \param argc  The number of arguments in argv
    /// \param argv  The parameters passed in
    void parseConfig(int argc, char* const argv[]);

    /// \brief  Set the default forwarders
    void defaultForwarders();

    /// \brief  Set the default servers
    void defaultServers();

    /// \brief  Parse an IP and optional port into a server
    ///
    /// The server can be of the following forms:
    ///  - 127.0.0.1
    ///  - [::1]
    ///  - [::1]:53
    ///  - 127.0.0.1:53
    ///
    /// \param server       The server to parse
    /// \param defaultPort  The port to use if not specified
    /// \param output       The variable to parse into
    ///
    /// \return  True if able to parse, false if invalid format
    static bool parseServer(const char* server,
                            unsigned short defaultPort,
                            sockaddr_storage& output);

    /// \brief  Add a pin to the current m_partialForwarder
    ///
    /// \param pin  The Base64 encoded public key pin
    void addPin(const char* pin);

    /// \brief  The hostname to add to the current m_partialForwarder
    ///
    /// \param hostname  The hostname expected for the certificate
    ///                  of the forwarder
    void addHostname(const char* hostname);

    /// \brief  Set the IP and port for a new m_partialForwarder
    ///
    /// \param server  The server to add
    ///
    /// \see parseServer
    void addForwarder(const char* server);

    /// \brief  If there is a forwarder in the partial forwarder,
    ///         then push it to the m_forwarders
    void pushForwarder();

    /// \brief  Add a server to the configuration
    ///
    /// \param server  The server to add
    ///
    /// \see parseServer
    void addServer(const char* server);

    /// \brief  Set the maximum number of outgoing forwarder
    ///         connections at the same time
    ///
    /// \param maxConnections  A decimal string with the maximum
    void setMaxConnections(const char* maxConnections);

    /// Whether the parameters are valid
    bool m_valid;
    /// The currently being built forwarder
    Forwarder m_partialForwarder;
    /// The forwarders that were in the configuration
    std::vector<Forwarder> m_forwarders;
    /// The servers that were in the configuration
    std::vector<Server> m_servers;
    /// The IP to connect to and report hostname and pin for
    sockaddr_storage m_ipLookup;
    /// The OpenSSL ciphers to use
    std::string m_ciphers;
    /// The maximum number of open connections at a time
    std::size_t m_maxConnections;
    /// The location of the PID file to write
    std::string m_pidFile;
    /// Whether to fork and daemonise
    bool m_daemonise;
};

}  // namespace dote
