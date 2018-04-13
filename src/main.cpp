
#include "config_parser.h"
#include "server.h"
#include "loop.h"
#include "forwarder_config.h"
#include "client_forwarders.h"
#include "syslog_logger.h"
#include "log.h"
#include "pid_file.h"
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
    // Stop listening, which will cause us to exit when
    // no more requests are in progress
    g_server.reset();
    // Reset the signal handler
    signal(SIGINT, SIG_DFL);
}

/// \brief  Drop any root priviledges if we have them
void dropPriviledges()
{
    static constexpr uid_t NOBODY = 65534;

    // Check if we are running as root
    if (geteuid() == 0)
    {
        uid_t uid = getuid();
        if (uid == 0)
        {
            // Set to nobody because this was run as root
            setreuid(NOBODY, NOBODY);
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

/// \brief  Print the usage of the command
///
/// \param appName  The name of the executable
void usage(const char* appName)
{
    fprintf(stderr, "\n Usage: %s [OPTIONS]\n\n", appName);
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "   -s --server IP[:port]     The server to listen on with optional port.\n");
    fprintf(stderr, "                             May be specified multiple times.  IPv6\n");
    fprintf(stderr, "                             addresses must be encapsulated in square\n");
    fprintf(stderr, "                             brackets (i.e. [::1])\n");
    fprintf(stderr, "   -f --forwarder IP[:port]  A forwarder to send requests on to with an\n");
    fprintf(stderr, "                             optional port number.\n");
    fprintf(stderr, "   -h --hostname  hostname   The hostname of the previously specified\n");
    fprintf(stderr, "                             forwarders' certificate.\n");
    fprintf(stderr, "   -p --pin  hash            The Base64 encoding of a SHA-256 hash of the\n");
    fprintf(stderr, "                             previously specified forwarders' public key.\n");
    fprintf(stderr, "   -c --ciphers  ciphers     The OpenSSL ciphers to use for connecting\n");
    fprintf(stderr, "   -m --connections  max     The maximum number of outgoing requests at a\n");
    fprintf(stderr, "                             time before buffering the requests.\n");
    fprintf(stderr, "   -d --daemonise            Daemonise this application\n");
    fprintf(stderr, "   -P --pid_file  filename   Write the PID of the process to a given file\n");
    fprintf(stderr, "\n");
}

/// \brief  Daemonise the process, only returns for the
///         daemoinised process, exits for the parent
void daemonise()
{
    // Fork from the parent
    pid_t pid = fork();
    // If there was an error, we can't continue
    if (pid < 0)
    {
        fprintf(stderr, "Unable to fork to daemonise\n");
        exit(1);
    }
    // If this is the parent, then we don't want to continue any further
    if (pid > 0)
    {
        exit(0);
    }

    // Ignore signal sent from the next child to this parent
    signal(SIGCHLD, SIG_IGN);

    // Double fork so that the child becomes owned by PID 1
    pid = fork();
    // Check for a double-fork error
    if (pid < 0)
    {
        fprintf(stderr, "Unable to double-fork to daemonise\n");
        exit(1);
    }
    // If this is the parent, then we're finished, exit
    if (pid > 0)
    {
        exit(0);
    }

    // Close any file descriptors that are open
    for (int fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
    {
        close(fd);
    }

    // Re-open the standard input and output to null
    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");
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
        usage(argv[0]);
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

    // Daemonise if requested
    if (parser.daemonise())
    {
        daemonise();
    }

    // Write the pid file if requested
    dote::PidFile pidFile(parser.pidFile());
    if (!pidFile.valid())
    {
        exit(1);
    }

    // Configure the server
    g_server = std::make_shared<dote::Server>(loop, forwarders);
    for (const auto& serverConfig : parser.servers())
    {
        if (!g_server->addServer(serverConfig))
        {
            dote::Log::err << "Unable to bind to server port";
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
