
#include "config_parser.h"
#include "server.h"
#include "loop.h"
#include "forwarder_config.h"
#include "client_forwarders.h"
#include "syslog_logger.h"
#include "log.h"
#include "openssl/context.h"

int main(int argc, char* const argv[])
{
    // Set up the logger
    dote::Log::setLogger(std::make_shared<dote::SyslogLogger>());

    // Parse the configuration from the command line
    dote::ConfigParser parser(argc, argv);
    if (!parser.valid())
    {
        fprintf(stderr, "Usage: %s [-s 127.0.0.1:53] [-f 1.1.1.1:853 [-h cloudflare-dns.com] [-p DPPP3G7LCnpidYBiFiN38CespymEvOsP1HCpoVVPtUM=]] [-c ALL]\n", argv[0]);
        return 1;
    }

    // Configure the context
    auto loop = std::make_shared<dote::Loop>();
    auto config = std::make_shared<dote::ForwarderConfig>();
    auto context = std::make_shared<dote::openssl::Context>(parser.ciphers());

    // Configure the forwarders
    auto forwarders =
        std::make_shared<dote::ClientForwarders>(loop, config, context);
    for (const auto& forwarderConfig : parser.forwarders())
    {
        config->addForwarder(forwarderConfig);
    }

    // Configure the server
    dote::Server server(loop, forwarders);
    for (const auto& serverConfig : parser.servers())
    {
        if (!server.addServer(serverConfig))
        {
            fprintf(stderr, "Unable to bind to server\n");
            return 1;
        }
    }

    // Start the event loop
    loop->run();

    return 0;
}
