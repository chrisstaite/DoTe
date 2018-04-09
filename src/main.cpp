
#include "config_parser.h"
#include "server.h"
#include "loop.h"
#include "forwarder_config.h"
#include "client_forwarders.h"
#include "openssl/context.h"

int main(int argc, char* const argv[])
{
    dote::ConfigParser parser(argc, argv);
    if (!parser.valid())
    {
        fprintf(stderr, "Usage: %s [-s 127.0.0.1:53] [-f 1.1.1.1:53 [-h cloudflare-dns.com] [-p DPPP3G7LCnpidYBiFiN38CespymEvOsP1HCpoVVPtUM=]] [-c ALL]\n", argv[0]);
        return 1;
    }

    auto loop = std::make_shared<dote::Loop>();
    auto config = std::make_shared<dote::ForwarderConfig>();
    auto context = std::make_shared<dote::openssl::Context>(parser.ciphers());

    auto forwarders =
        std::make_shared<dote::ClientForwarders>(loop, config, context);
    for (const auto& forwarderConfig : parser.forwarders())
    {
        config->addForwarder(forwarderConfig);
    }

    dote::Server server(loop, forwarders);
    for (const auto& serverConfig : parser.servers())
    {
        if (!server.addServer(serverConfig))
        {
            fprintf(
                stderr, "Unable to add server %s:%d\n",
                serverConfig.ip.c_str(), serverConfig.port
            );
            return 1;
        }
    }

    loop->run();

    return 0;
}
