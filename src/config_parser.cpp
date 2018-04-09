
#include "config_parser.h"
#include "openssl/base64.h"

#include <unistd.h>
#include <cstring>
#include <cstdlib>

namespace dote {

namespace {

/// The default cipher suite to use
constexpr char DEFAULT_CIPHERS[] = "EECDH+ECDSA+AESGCM:EECDH+aRSA+AESGCM:EECDH+ECDSA+SHA256:EECDH+aRSA+SHA256:EECDH+ECDSA+SHA384:EECDH+ECDSA+SHA256:EECDH+aRSA+SHA384:EDH+aRSA+AESGCM:EDH+aRSA+SHA256:EDH+aRSA:EECDH:!aNULL:!eNULL:!MEDIUM:!LOW:!3DES:!MD5:!EXP:!PSK:!SRP:!DSS:!RC4:!SEED";

}  // anon namespace

ConfigParser::ConfigParser(int argc, char* const argv[]) :
    m_valid(true),
    m_partialForwarder(),
    m_forwarders(),
    m_servers(),
    m_ciphers()
{
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

bool ConfigParser::parseServer(const char *server, Server &output)
{
    if (*server == '[')
    {
        const char* end = strchr(&server[1], ']');
        if (!end)
        {
            return false;
        }
        output.ip = std::string{ &server[1], end };
        server = &end[1];
    }
    else
    {
        const char* end = strchr(server, ':');
        if (end)
        {
            output.ip = std::string{ server, end };
            server = end;
        }
        else
        {
            output.ip = server;
        }
    }

    if (*server == ':')
    {
        char *end;
        long port = strtol(&server[1], &end, 10);
        if (*end || port < 1 || port > 65535)
        {
            // Invalid port number
            return false;
        }
        output.port = port;
    }
    return true;
}

void ConfigParser::addServer(const char* server)
{
    Server output { "", 0 };
    if (!parseServer(server, output))
    {
        m_valid = false;
    }
    else
    {
        if (output.port == 0)
        {
            output.port = 53;
        }
        m_servers.emplace_back(std::move(output));
    }
}

void ConfigParser::pushForwarder()
{
    if (!m_partialForwarder.ip.empty())
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
}

void ConfigParser::addForwarder(const char* server)
{
    Server output { "", 0 };
    if (!parseServer(server, output))
    {
        m_valid = false;
    }
    else
    {
        m_partialForwarder.ip = std::move(output.ip);
        if (output.port == 0)
        {
            m_partialForwarder.port = 853;
        }
        else
        {
            m_partialForwarder.port = output.port;
        }
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

void ConfigParser::parseConfig(int argc, char* const argv[])
{
    int c;
    while ((c = getopt(argc, argv, "s:f:h:p:c:")) != -1)
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

void ConfigParser::defaultForwarders()
{
    std::string hostname("cloudflare-dns.com");
    auto pin = openssl::Base64::decode(
        "DPPP3G7LCnpidYBiFiN38CespymEvOsP1HCpoVVPtUM="
    );
    m_forwarders.emplace_back(Forwarder {
        "2606:4700:4700::1111", hostname, pin, 853
    });
    m_forwarders.emplace_back(Forwarder {
        "2606:4700:4700::1001", hostname, pin, 853
    });
    m_forwarders.emplace_back(Forwarder {
        "1.1.1.1", hostname, pin, 853
    });
    m_forwarders.emplace_back(Forwarder {
        "1.0.0.1", hostname, pin, 853
    });
}

void ConfigParser::defaultServers()
{
    m_servers.emplace_back(Server { "127.0.0.1", 53 });
    m_servers.emplace_back(Server { "::1", 53 });
}

}  // namespace dote
