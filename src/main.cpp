
#include "dote.h"
#include "config_parser.h"
#include "syslog_logger.h"
#include "console_logger.h"
#include "log.h"
#include "pid_file.h"
#include "ip_lookup.h"
#include "vyatta.h"

#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

#include <iostream>

namespace {

/// The dote instance to use
std::unique_ptr<dote::Dote> g_dote;

/// \brief  The signal handler that is called when the
///         server should be shutdown
///
/// \param signum  The signal that is being caught
void shutdownHandler(int signum)
{
    dote::Log::info << "Shutdown signal received";
    // Stop listening, which will cause us to exit when
    // no more requests are in progress
    g_dote->shutdown();
    // Reset the signal handler
    signal(signum, SIG_DFL);
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
            if (setreuid(NOBODY, NOBODY) == -1)
            {
                // Can't drop, so exit
                _exit(1);
            }
        }
        else
        {
            // Set to the user it was run as
            if (setreuid(uid, uid) == -1)
            {
                // Can't drop, so exit
                _exit(1);
            }
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
    std::cerr << "\n Usage: " << appName << " [OPTIONS]\n\n";
    std::cerr << "  Options:\n";
    std::cerr << "   -s --server IP[:port]     The server to listen on with optional port.\n";
    std::cerr << "                             May be specified multiple times.  IPv6\n";
    std::cerr << "                             addresses must be encapsulated in square\n";
    std::cerr << "                             brackets (i.e. [::1])\n";
    std::cerr << "   -f --forwarder IP[:port]  A forwarder to send requests on to with an\n";
    std::cerr << "                             optional port number.\n";
    std::cerr << "   -h --hostname  hostname   The hostname of the previously specified\n";
    std::cerr << "                             forwarders' certificate.\n";
    std::cerr << "   -p --pin  hash            The Base64 encoding of a SHA-256 hash of the\n";
    std::cerr << "                             previously specified forwarders' public key.\n";
    std::cerr << "   -c --ciphers  ciphers     The OpenSSL ciphers to use for connecting\n";
    std::cerr << "   -m --connections  max     The maximum number of outgoing requests at a\n";
    std::cerr << "                             time before buffering the requests.\n";
    std::cerr << "   -d --daemonise            Daemonise this application\n";
    std::cerr << "   -P --pid_file  filename   Write the PID of the process to a given file\n";
    std::cerr << "   -l --ip_lookup  IP        Lookup the hostname and certificate pin for\n";
    std::cerr << "                             an IP address and then exit.\n";
    std::cerr << "   -t --timeout  timeout     The number of seconds to allow a forwarder\n";
    std::cerr << "\n";
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
        std::cerr << "Unable to fork to daemonise\n";
        exit(1);
    }
    // If this is the parent, then we don't want to continue any further
    if (pid > 0)
    {
        exit(0);
    }
    
    // Don't get HUP'd by the executing terminal
    if (setsid() == -1)
    {
        std::cerr << "Unable to become process leader to daemonise\n";
        exit(1);
    }

    // Ignore signal sent from the next child to this parent
    signal(SIGCHLD, SIG_IGN);

    // Double fork so that the child becomes owned by PID 1
    pid = fork();
    // Check for a double-fork error
    if (pid < 0)
    {
        std::cerr << "Unable to double-fork to daemonise\n";
        exit(1);
    }
    // If this is the parent, then we're finished, exit
    if (pid > 0)
    {
        exit(0);
    }

    // Move to the root directory
    if (chdir("/") == -1)
    {
        std::cerr << "Unable to move to the root directory to daemonise\n";
        exit(1);
    }

    // Set the umask even though we don't create any files
    (void) umask(0);

    // Close any file descriptors that are open
    for (int fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
    {
        (void) close(fd);
    }

    // Re-open the standard input and output to null
    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");
}

dote::ConfigParser loadVyatta(dote::ConfigParser parser)
{
    dote::Vyatta vyatta;
    vyatta.loadConfig(parser);
    parser.setDefaults();
    return parser;
}

void reloadConfiguration(const dote::ConfigParser& parser)
{
    dote::ConfigParser config(parser);
    dote::Vyatta vyatta;
    vyatta.loadConfig(config);
    config.setDefaults();
    if (config.valid())
    {
        // Can't re-set listen because we dropped priviledges
        g_dote->setForwarders(config);
    }
}

}  // anon namespace

int main(int argc, char* const argv[])
{
    // Set up the logger
    dote::Log::setLogger(std::make_shared<dote::ConsoleLogger>());

    // Parse the configuration from the command line
    dote::ConfigParser clParser;
    clParser.parseConfig(argc, argv);
    dote::ConfigParser parser(loadVyatta(clParser));
    if (!parser.valid())
    {
        usage(argv[0]);
        return 1;
    }

    // Check if they want to do an IP lookup
    if (parser.ipLookup().ss_family != AF_UNSPEC)
    {
        dote::IpLookup lookup(parser);
        auto hostname = lookup.hostname();
        if (hostname.empty())
        {
            std::cerr << "Unable to connect to the given IP\n";
        }
        else
        {
            std::cout << "Hostname: " << lookup.hostname() << "\n";
            std::cout << "Pin: " << lookup.pin() << "\n";
        }
        return 0;
    }

    // Create the DoTe instance
    g_dote.reset(new dote::Dote(parser));

    // Daemonise if requested
    if (parser.daemonise())
    {
        auto logger = std::make_shared<dote::SyslogLogger>();
        daemonise();
        dote::Log::setLogger(std::move(logger));
    }

    // Write the pid file if requested
    dote::PidFile pidFile(parser.pidFile());
    if (!pidFile.valid())
    {
        return 1;
    }

    // Bind to the server ports
    if (!g_dote->listen(parser))
    {
        return 1;
    }

    // Drop priviledges
    dropPriviledges();

    // Listen for kill signals to shutdown the server
    (void) signal(SIGINT, &shutdownHandler);
    (void) signal(SIGTERM, &shutdownHandler);

    // TODO: Run reloadConfiguration if /config/config.boot changes
    
    // Start the event loop
    g_dote->run();

    return 0;
}
