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

