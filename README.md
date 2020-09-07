[![Build Status](https://travis-ci.org/chrisstaite/DoTe.svg?branch=master)](https://travis-ci.org/chrisstaite/DoTe)

DoTe
====

This is a simple event loop application to provide an easy method to wrap
UDP DNS requests and send them over TLS.  Its name comes from the acronym
DNS over TLS easy.  There are a number of clients out there that implement
this functionality, but they also try to implement other functions too.

This project will never be any more than a UDP server that wraps incoming
packets, sends them over TLS to another server and forwards any responses
back over UDP.  The most processing provided is that of verifying the TLS
remote host to have a valid certificate.

The design of this software is that of event loop.  It uses entirely non-
blocking IO for communications and is designed to run as a single thread
in a single process.  All TLS functionality is provided by OpenSSL.

Quick Start for EdgeOS
----------------------

You can simply download the latest release to /config, add a start u
script and then run it.

~~~~~
curl -L -o /config/dote_mips https://github.com/chrisstaite/DoTe/releases/latest/download/dote_mips
mkdir -p /config/scripts/post-config.d/
cat >/config/scripts/post-config.d/10-dote <<EOF
#!/bin/sh
/config/dote_mips -s [::1]:5353 -d -P /var/run/dote.pid
EOF
chmod +x /config/dote_mips /config/scripts/post-config.d/10-dote
sudo /config/scripts/post-config.d/10-dote
configure
set service dns forwarding options server=::1#5353
delete service dns forwarding name-server
commit
~~~~~

This downloads the latest release to the user-configured /config directory,
creates a start up script, makes them executable and then runs it.  Finally
the configuration changes the existing nameserver configuration to use DoTe
on localhost.  It needs to use the options configuration because EdgeOS
doesn't support specifying the port otherwise.

Read the run section below and edit the /config/scripts/post-config.d/10-dote
file for advanced options such as not using the default resolver.


EdgeOS Configuration
--------------------

Support for VyOS configuration file loading.

Listening server can now be configured at custom-attribute/dote-serverN/value where N is a is a number starting from 0 increasing for multiple values.

Forwarding server can be configured at custom-attribute/dote-forwarderN/value where N is a number starting from 0 increasing for multiple values.  The certificate pin may optionally be configured at custom-attribute/dote-forwarderN-pin and similarly the hostname pin can be configured at custom-attribute/dote-forwarderN-hostname.

If the dote-server value is changed, a manual re-start of the process is required as privileges will not allow it to listen to any new ports.

If the dote-forwarder is changed the new values will take effect immediately.

Command line arguments are added to the arguments configured.


Build
-----

Building requires OpenSSL, CMake and a C++11 compiler.

Then all you need do is run:

~~~~
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
~~~~

Test
----

Once built, you can run the unit tests by running:

~~~~
make test
~~~~

Run
---

By default DoTe will listen on localhost port 53 and
forward queries to 1.1.1.1 port 853 with hostname
verification and certificate pinned.

~~~~
./dote
~~~~

You can also specify different addresses to bind to
using the -s flag.  For an IPv4 it can be specified
by `-s 127.0.0.1:53` where the port is optional and
by default port 53.  An IPv6 address must be wrapped
with square brackets for example `-s [::1]:53`.

You can change the OpenSSL ciphers that can be used
from the baked-in defaults using the `-c` flag.  See
the OpenSSL documentation for information on the
cipher strings that are available.

Forwarders can be specified using the `-f` flag in
the same way as the `-s` flag.  By default the
hostname and certificate is not pinned.  To pin
them, the flags `-h` and `-p` need to be specified
after the `-f` flag they are to pin.  The `-h` will
be the hostname to match against and the `-p` flag
is the Base64 encoding of the certificate public
key.

The maximum number of outgoing forwarder requests
to be made at the same time may be limited by the
`-m 5` flag, which in this case would limit them
to five.  Any other requests are queued.  There is
currently no limit on the queue length, and if it
runs out of memory the process will simply quit with
an exception.

In order to execute the process as a service there
is the option to fork it into the background using
the `-d` flag.  This will continue the process
executing in the background.  A file containing the
PID of this background process may be obtained by
specifying the `-P dote.pid` argument.

To find the hostname and public key for a given DNS
host, a utility has been included which allows you
to do just that.  Simply call DoTe with the flag
`-l <IP>` and it will connect and return the current
common name and certificate public key hash.  The
IP may be IPv4 or IPv6 in square brackets, as when
specifying the forwarder.

By default, forwarders have 5 seconds to reply
before the request is dropped.  This can be
configured by using the `-t` option.
