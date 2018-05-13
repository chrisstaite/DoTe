
#include "config_parser.h"
#include "openssl/base64.h"

#include <arpa/inet.h>
#include <getopt.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

namespace dote {

namespace {

/// The default cipher suite to use
constexpr char DEFAULT_CIPHERS[] = "EECDH+ECDSA+AESGCM:EECDH+aRSA+AESGCM:EECDH+ECDSA+SHA256:EECDH+aRSA+SHA256:EECDH+ECDSA+SHA384:EECDH+ECDSA+SHA256:EECDH+aRSA+SHA384:EDH+aRSA+AESGCM:EDH+aRSA+SHA256:EDH+aRSA:EECDH:!aNULL:!eNULL:!MEDIUM:!LOW:!3DES:!MD5:!EXP:!PSK:!SRP:!DSS:!RC4:!SEED";

/// The default maximum open connections at a time
constexpr std::size_t DEFAULT_MAX_CONNECTIONS = 5u;

}  // anon namespace

ConfigParser::ConfigParser(int argc, char* const argv[]) :
    m_valid(true),
    m_partialForwarder(),
    m_forwarders(),
    m_servers(),
    m_ipLookup(),
    m_ciphers(),
    m_maxConnections(DEFAULT_MAX_CONNECTIONS),
    m_pidFile(),
    m_daemonise(false)
{
    m_ipLookup.ss_family = AF_UNSPEC;
    m_partialForwarder.remote.ss_family = AF_UNSPEC;

    parseConfig(argc, argv);

    // Add the defaults
    if (m_forwarders.empty())
    {
        defaultForwarders();
    }
    if (m_servers.empty())
    {
        defaultServers();
    }
    if (m_ciphers.empty())
    {
        m_ciphers = DEFAULT_CIPHERS;
    }
}

bool ConfigParser::parseServer(const char *server,
                               unsigned short defaultPort,
                               sockaddr_storage &output)
{
    std::string ip;

    if (*server == '[')
    {
        const char* end = strchr(&server[1], ']');
        if (!end)
        {
            return false;
        }
        output.ss_family = AF_INET6;
        ip = std::string{ &server[1], end };
        server = &end[1];
    }
    else
    {
        const char* end = strchr(server, ':');
        output.ss_family = AF_INET;
        if (end)
        {
            ip = std::string{ server, end };
            server = end;
        }
        else
        {
            ip = server;
        }
    }

    unsigned short port;
    if (*server == ':')
    {
        char *end;
        long longPort = strtol(&server[1], &end, 10);
        if (*end || longPort < 1 || longPort > 65535)
        {
            // Invalid port number
            return false;
        }
        port = longPort;
    }
    else
    {
        port = defaultPort;
    }

    if (output.ss_family == AF_INET)
    {
        auto& ip4 = reinterpret_cast<sockaddr_in&>(output);
        memset(ip4.sin_zero, 0, sizeof(ip4.sin_zero));
        ip4.sin_port = htons(port);
        if (inet_pton(output.ss_family, ip.c_str(), &ip4.sin_addr) != 1)
        {
            return false;
        }
    }
    else if (output.ss_family == AF_INET6)
    {
        auto& ip6 = reinterpret_cast<sockaddr_in6&>(output);
        ip6.sin6_port = htons(port);
        if (inet_pton(output.ss_family, ip.c_str(), &ip6.sin6_addr) != 1)
        {
            return false;
        }
    }

    return true;
}

void ConfigParser::addServer(const char* server)
{
    Server output { };
    if (!parseServer(server, 53, output.address))
    {
        m_valid = false;
    }
    else
    {
        m_servers.emplace_back(std::move(output));
    }
}

void ConfigParser::pushForwarder()
{
    if (m_partialForwarder.remote.ss_family != AF_UNSPEC)
    {
        m_forwarders.emplace_back(std::move(m_partialForwarder));
    }
    else
    {
        // If there's no IP, but there's a hostname or pin, then
        // mark the configuration as invalid
        if (!m_partialForwarder.host.empty() ||
                !m_partialForwarder.pin.empty())
        {
            m_valid = false;
        }
    }

    // Reset if it's been moved
    m_partialForwarder = Forwarder {};
    m_partialForwarder.remote.ss_family = AF_UNSPEC;
}

void ConfigParser::addForwarder(const char* server)
{
    if (!parseServer(server, 853, m_partialForwarder.remote))
    {
        m_valid = false;
    }
}

void ConfigParser::addHostname(const char* hostname)
{
    if (m_partialForwarder.host.empty())
    {
        m_partialForwarder.host = optarg;
    }
    else
    {
        m_valid = false;
    }
}

void ConfigParser::addPin(const char* pin)
{
    m_partialForwarder.pin = openssl::Base64::decode(pin);
    if (*pin && m_partialForwarder.pin.empty())
    {
        m_valid = false;
    }
}

void ConfigParser::setMaxConnections(const char* maxConnections)
{
    char *end;
    long longConnections = strtol(maxConnections, &end, 10);
    if (*end || longConnections < 1 || longConnections > 6000)
    {
        // Invalid number of connections
        m_valid = false;
    }
    else
    {
        m_maxConnections = longConnections;
    }
}

const sockaddr_storage& ConfigParser::ipLookup() const
{
    return m_ipLookup;
}

void ConfigParser::parseConfig(int argc, char* const argv[])
{
    int c;
    static option long_options[] = {
        {"server", required_argument, nullptr, 's'},
        {"forwarder", required_argument, nullptr, 'f'},
        {"hostname", required_argument, nullptr, 'h'},
        {"pin", required_argument, nullptr, 'p'},
        {"ciphers", required_argument, nullptr, 'c'},
        {"connections", required_argument, nullptr, 'm'},
        {"daemonise", no_argument, nullptr, 'd'},
        {"pid_file", required_argument, nullptr, 'P'},
        {"ip_lookup", required_argument, nullptr, 'l'},
        {nullptr, 0, nullptr, 0}
    };
    while ((c = getopt_long(argc, argv, "s:f:h:p:c:m:dP:l:", long_options, nullptr)) != -1)
    {
        switch (c)
        {
            case 's':
                // Server
                addServer(optarg);
                break;
            case 'f':
                // Forwarder
                pushForwarder();
                addForwarder(optarg);
                break;
            case 'h':
                // The hostname pin for the current forwarder
                addHostname(optarg);
                break;
            case 'p':
                // The certificate pin for the current forwarder
                addPin(optarg);
                break;
            case 'c':
                // The OpenSSL ciphers
                m_ciphers = optarg;
                break;
            case 'm':
                // The maximum number of connections
                setMaxConnections(optarg);
                break;
            case 'd':
                // Daemonise the process
                m_daemonise = true;
                break;
            case 'P':
                // Save the PID of the process to a file
                m_pidFile = optarg;
                break;
            case 'l':
                // Save the IP to lookup
                if (!parseServer(optarg, 853, m_ipLookup))
                {
                    m_valid = false;
                }
                break;
            default:
                // Unknown option
                m_valid = false;
                break;
        }
    }
    pushForwarder();

    // Check all options were parsed
    if (optind != argc)
    {
        m_valid = false;
    }
}

bool ConfigParser::valid() const
{
    return m_valid;
}

const std::vector<ConfigParser::Forwarder>& ConfigParser::forwarders() const
{
    return m_forwarders;
}

const std::vector<ConfigParser::Server>& ConfigParser::servers() const
{
    return m_servers;
}

const std::string& ConfigParser::ciphers() const
{
    return m_ciphers;
}

std::size_t ConfigParser::maxConnections() const
{
    return m_maxConnections;
}

bool ConfigParser::daemonise() const
{
    return m_daemonise;
}

const std::string& ConfigParser::pidFile() const
{
    return m_pidFile;
}

void ConfigParser::defaultForwarders()
{
    std::string hostname("cloudflare-dns.com");
    auto pin = openssl::Base64::decode(
        "DPPP3G7LCnpidYBiFiN38CespymEvOsP1HCpoVVPtUM="
    );
    Forwarder a{{}, hostname, pin};
    if (parseServer("[2606:4700:4700::1111]", 853, a.remote))
    {
        m_forwarders.emplace_back(std::move(a));
    }
    Forwarder b{{}, hostname, pin};
    if (parseServer("[2606:4700:4700::1001]", 853, b.remote))
    {
        m_forwarders.emplace_back(std::move(b));
    }
    Forwarder c{{}, hostname, pin};
    if (parseServer("1.1.1.1", 853, c.remote))
    {
        m_forwarders.emplace_back(std::move(c));
    }
    Forwarder d{{}, hostname, pin};
    if (parseServer("1.0.0.1", 853, d.remote))
    {
        m_forwarders.emplace_back(std::move(d));
    }
}

void ConfigParser::defaultServers()
{
    Server a{};
    if (parseServer("127.0.0.1", 53, a.address))
    {
        m_servers.push_back(a);
    }
    Server b{};
    if (parseServer("[::1]", 53, b.address))
    {
        m_servers.push_back(b);
    }
}

}  // namespace dote
