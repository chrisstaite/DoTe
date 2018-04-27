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
