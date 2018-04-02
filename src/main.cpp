
#include "dote.h"

int main(int argc, char* argv[])
{
    dote::Dote dote;

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

    dote.addForwarder("1.1.1.1", "cloudflare-dns.com", "");
    dote.addForwarder("1.0.0.1", "cloudflare-dns.com", "");

    dote.run();

    return 0;
}
