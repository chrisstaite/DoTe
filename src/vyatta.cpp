
#include "vyatta.h"
#include "config_parser.h"
#include "log.h"

#include <dlfcn.h>

namespace dote {

namespace {

using cstore_init = void*(*)();
using cstore_free = void(*)(void*);
using cstore_cfg_path_get_effective_value = char*(*)(void*, const char *[], int);

}  // anon namespace

Vyatta::Vyatta() :
    m_handle(dlopen("libvyatta-cfg.so.1", RTLD_NOW)),
    m_free(m_handle ? dlsym(m_handle, "cstore_free") : nullptr),
    m_get(m_handle ? dlsym(m_handle, "cstore_cfg_path_get_effective_value") : nullptr),
    m_config(nullptr)
{
    if (m_handle && m_free && m_get)
    {
        auto init = reinterpret_cast<cstore_init>(
            dlsym(m_handle, "cstore_init")
        );
        if (init)
        {
            m_config = init();
        }
        if (!m_config)
        {
            Log::err << "Unable to load VyOS configuration";
        }
    }
    else
    {
        Log::info << "Not running on VyOS, config through parameters";
    }
}

Vyatta::~Vyatta()
{
    if (m_config && m_free)
    {
        reinterpret_cast<cstore_free>(m_free)(m_config);
    }
    if (m_handle)
    {
        dlclose(m_handle);
    }
}

void Vyatta::loadConfig(ConfigParser& parser)
{
    loadServers(parser);
    loadForwarders(parser);
}

void Vyatta::loadServers(ConfigParser& parser)
{
    char name[] = "";
    char serverFlag[] = "-s";
    size_t index = 0u;
    while (true)
    {
        auto server = getValue({
            "custom-attribute",
            "dote-server" + std::to_string(index),
            "value"
        });
        if (server.empty())
        {
            break;
        }
        char* args[3] = { name, serverFlag, const_cast<char*>(server.c_str()) };
        parser.parseConfig(3, args);
        ++index;
    }
}

void Vyatta::loadForwarders(ConfigParser& parser)
{
    char name[] = "";
    char forwarderFlag[] = "-f";
    char pinFlag[] = "-p";
    char hostnameFlag[] = "-h";
    size_t index = 0u;
    while (true)
    {
        auto forwarder = getValue({
            "custom-attribute",
            "dote-forwarder" + std::to_string(index),
            "value"
        });
        if (forwarder.empty())
        {
            break;
        }
        auto pin = getValue({
            "custom-attribute",
            "dote-forwarder" + std::to_string(index) + "-pin",
            "value"
        });
        auto hostname = getValue({
            "custom-attribute",
            "dote-forwarder" + std::to_string(index) + "-hostname",
            "value"
        });
        char* args[] = { name, forwarderFlag, nullptr, nullptr, nullptr, nullptr, nullptr };
        auto argc = 2;
        args[argc++] = const_cast<char*>(forwarder.c_str());
        if (pin.empty())
        {
            args[argc++] = pinFlag;
            args[argc++] = const_cast<char*>(pin.c_str());
        }
        if (hostname.empty())
        {
            args[argc++] = hostnameFlag;
            args[argc++] = const_cast<char*>(hostname.c_str());
        }
        parser.parseConfig(argc, args);
        ++index;
    }
}

std::string Vyatta::getValue(const std::vector<std::string>& paths)
{
    if (!m_config)
    {
        return {};
    }

    std::vector<const char*> pathPointers;
    pathPointers.reserve(paths.size());
    for (auto& path : paths)
    {
        pathPointers.emplace_back(path.c_str());
    }
    char* value = reinterpret_cast<cstore_cfg_path_get_effective_value>(m_get)(
        m_config, pathPointers.data(), pathPointers.size()
    );
    std::string stringValue;
    if (value)
    {
        stringValue = value;
        free(value);
    }
    return stringValue;
}

}  // namespace dote
