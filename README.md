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

Currently there is no configuration system, so if you
want to change anything you'll have to modify src/main.cpp.

By default DoTe will listen on localhost port 53 and
forward queries to 1.1.1.1 port 853 with hostname
verification and certificate pinned.

~~~~
./dote
~~~~
