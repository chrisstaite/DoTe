
#include "config_parser.h"
#include "server.h"
#include "loop.h"
#include "forwarder_config.h"
#include "client_forwarders.h"
#include "syslog_logger.h"
#include "log.h"
#include "openssl/context.h"

#include <unistd.h>
#include <signal.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

namespace {

/// The server instance to kill on a signal
std::shared_ptr<dote::Server> g_server;

/// \brief  The signal handler that is called when the
///         server should be shutdown
void shutdownHandler(int)
{
    dote::Log::info << "Shutdown signal received";
    g_server.reset();
}

/// \brief  Drop any root priviledges if we have them
void dropPriviledges()
{
    // Check if we are running as root
    if (geteuid() == 0)
    {
        uid_t uid = getuid();
        if (uid == 0)
        {
            // Set to nobody because this was run as root
            setreuid(65534, 65534);
        }
        else
        {
            // Set to the user it was run as
            setreuid(uid, uid);
        }
    }
#ifdef __linux__
#ifdef PR_SET_NO_NEW_PRIVS
    // Don't allow any new priviledges
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
#endif
#endif
}

}  // anon namespace

int main(int argc, char* const argv[])
{
    // Set up the logger
    dote::Log::setLogger(std::make_shared<dote::SyslogLogger>());

    // Parse the configuration from the command line
    dote::ConfigParser parser(argc, argv);
    if (!parser.valid())
    {
        fprintf(stderr, "Usage: %s [-s 127.0.0.1:53] [-f 1.1.1.1:853 [-h cloudflare-dns.com] [-p DPPP3G7LCnpidYBiFiN38CespymEvOsP1HCpoVVPtUM=]] [-c ALL] [-m 5]\n", argv[0]);
        return 1;
    }

    // Configure the context
    auto loop = std::make_shared<dote::Loop>();
    auto config = std::make_shared<dote::ForwarderConfig>();
    auto context = std::make_shared<dote::openssl::Context>(parser.ciphers());

    // Configure the forwarders
    auto forwarders = std::make_shared<dote::ClientForwarders>(
        loop, config, context, parser.maxConnections()
    );
    for (const auto& forwarderConfig : parser.forwarders())
    {
        config->addForwarder(forwarderConfig);
    }

    // Configure the server
    g_server = std::make_shared<dote::Server>(loop, forwarders);
    for (const auto& serverConfig : parser.servers())
    {
        if (!g_server->addServer(serverConfig))
        {
            fprintf(stderr, "Unable to bind to server\n");
            return 1;
        }
    }

    // Drop priviledges
    dropPriviledges();

    // Listen for kill signals to shutdown the server
    (void) signal(SIGINT, &shutdownHandler);

    // Start the event loop
    loop->run();

    return 0;
}
