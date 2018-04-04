
#include "dote.h"
#include "loop.h"
#include "forwarder_config.h"
#include "client_forwarders.h"
#include "openssl/context.h"

int main(int argc, char* argv[])
{
    auto loop = std::make_shared<dote::Loop>();
    auto config = std::make_shared<dote::ForwarderConfig>();
    auto context = std::make_shared<dote::openssl::Context>();
    auto forwarders =
        std::make_shared<dote::ClientForwarders>(loop, config, context);
    dote::Dote dote(loop, forwarders);

    if (!dote.addServer("127.0.0.1"))
    {
        fprintf(stderr, "Unable to add server 127.0.0.1:53\n");
        return 1;
    }
    if (!dote.addServer("::1"))
    {
        fprintf(stderr, "Unable to add server ::1:53\n");
        return 1;
    }
/*
    config->addForwarder(
        "2606:4700:4700::1111",
        "cloudflare-dns.com",
        "DPPP3G7LCnpidYBiFiN38CespymEvOsP1HCpoVVPtUM="
    );
    config->addForwarder(
        "2606:4700:4700::1001",
        "cloudflare-dns.com",
        "DPPP3G7LCnpidYBiFiN38CespymEvOsP1HCpoVVPtUM="
    );
    */
    config->addForwarder(
        "1.1.1.1",
        "cloudflare-dns.com",
        "DPPP3G7LCnpidYBiFiN38CespymEvOsP1HCpoVVPtUM="
    );
    config->addForwarder(
        "1.0.0.1",
        "cloudflare-dns.com",
        "DPPP3G7LCnpidYBiFiN38CespymEvOsP1HCpoVVPtUM="
    );

    loop->run();

    return 0;
}
