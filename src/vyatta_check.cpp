
#include "vyatta_check.h"
#include "vyatta.h"
#include "dote.h"
#include "loop.h"
#include "log.h"

#include <cstring>
#include <sys/inotify.h>
#include <unistd.h>

namespace dote {

namespace {

constexpr char CONFIG_LOCATION[] = "/config";
constexpr char CONFIG_FILE[] = "config.boot";

}  // anon namespace

VyattaCheck::VyattaCheck(const ConfigParser& parser) :
    m_fd(inotify_init1(IN_NONBLOCK)),
    m_dote(nullptr),
    m_config(parser)
{
    if (m_fd >= 0)
    {
        auto flags = IN_MOVED_TO | IN_CREATE | IN_CLOSE_WRITE;
        if (inotify_add_watch(m_fd, CONFIG_LOCATION, flags) == -1)
        {
            Log::err << "Unable to find configuration directory to watch";
            close(m_fd);
            m_fd = -1;
        }
    }
    else
    {
        Log::err << "Unable to watch for configuration changes";
    }
}

VyattaCheck::~VyattaCheck()
{
    if (m_fd >= 0)
    {
        if (m_dote)
        {
            auto loop = m_dote->looper();
            if (loop)
            {
                loop->removeRead(m_fd);
            }
        }
        close(m_fd);
    }
}

ConfigParser VyattaCheck::workingConfig() const
{
    Vyatta vyatta;
    ConfigParser config = m_config;
    vyatta.loadConfig(config);
    config.setDefaults();
    return config;
}

void VyattaCheck::configure(Dote& dote)
{
    auto loop = dote.looper();
    if (loop && m_fd >= 0)
    {
        m_dote = &dote;
        loop->registerRead(
            m_fd,
            std::bind(&VyattaCheck::handleRead, this, std::placeholders::_1),
            0u
        );
        Log::info << "Registered listener for configuration changes";
    }
    else
    {
        Log::err << "Dote instance didn't have a looper";
    }
}

void VyattaCheck::handleRead(int)
{
    // Not interested in the contents, but will read to clear the buffer
    char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
    while (true)
    {
        auto len = read(m_fd, buf, sizeof(buf));
        if (len == -1)
        {
            break;
        }

        const struct inotify_event* event = nullptr;
        for (char* ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len)
        {
            event = reinterpret_cast<const struct inotify_event*>(ptr);
            if (event->len && memcmp(event->name, CONFIG_FILE, sizeof(CONFIG_FILE)) == 0)
            {
                reloadConfiguration();
            }
        }
    }
}

void VyattaCheck::reloadConfiguration()
{
    auto dote = m_dote;
    if (dote)
    {
        ConfigParser config = workingConfig();
        if (config.valid())
        {
            // Can't re-set listen because we dropped priviledges
            Log::info << "Re-loaded forwarder configuration";
            dote->setForwarders(config);
        }
        else
        {
            Log::err << "New configuration invalid, keeping old one";
        }
    }
}

}  // namespace dote
