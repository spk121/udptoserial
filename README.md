# udptoserial

This is a program that transmits UDP packets between two computers connected
only by a serial link.

Imagine that machine FOO wants to communicate with a UDP server on machine BAR
but the only connection between them is a serial link.  This program
`udptoserial` could run on both machines, providing the translation from serial
to UDP.

## Status

Status is **pre-alpha**

- Half-duplex serial driver: 50% complete
- Full-duplex serial driver: 0% complete
- Serial port handler: 10% complete
- Port mapping and translation: 0% complete
- Documentation: 10% complete

## Application

On the surface, this may seem like an unlikely scenario.  But there
are some limited applications where it is easier to connect a pair of
machines with a serial link than an Ethernet link.  There are some
applications like industrial automation and remote monitoring where
this might apply.

As an example, 900 MHz and 2.4GHz radio modems can offer long-range
performance; line-of-sight ranges up to 40 miles are possible.  Here
are a couple of example product lines.  These are smarter radios that
have data integrity functionality built in.

- Digi's XTend-PKG RF modems
- muRata ZN-241G General Purpose Wireless Serial Modems
- muRata HN 10 Series 2.4GHz Stand-Alone modems
- Laird Technologies ConnexLink stand-alone radio modules

Also, on the low end, RF modules are available for Arduino-style IoT
systems, often at 433MHz.  And some of these are half-duplex: they
cannot simultaneously transmit and receive.  So a collision prevention
procedure is required to determine who has the right to send at which
times.  Half-duplex requires a lot of struggle, and should probably be
avoided, but, `udptoserial` does have a half-duplex serial driver.

I haven't tried them, but, there are companies like Radiometrix that
make inexpensive half-duplex RF modems.

## Predecessors

Back in the olden days, this was actually a solved problem.  There
were protocols like SLIP and PPP that provided IP communication over
serial lines.  In fact, some versions of Linux and Windows still have
SLIP or PPP functionality hiding deep, deep down.  Usually getting PPP
to work requires kernel-level functionality.

## How it works

Consider two machines, Alpha and Bravo.  Alpha has a UDP server
application that listens on port 4000 to which Bravo wishes to
connect.  The `udptoserial` application on Bravo will be a server
forwarding application that listens on port 4000 and forwards the
packets to and from the original server on Alpha port 4000 over the
serial link.

## References

### Base-64 Encodings

Josefsson, S. (2006, October). _The Base16, Base32, and Base64 Data
Encodings_ https://tools.ietf.org/rfc4648

### Half-duplex communication

_Information processing -- Basic mode control procedures for data
communication systems_. (1975) Geneva: I.S.O.

_Basic Mode Control Procedures - Complements._ (1973). Switzerland:
I.S.O.

### SLIP

Romkey, J. (1988, June). _Nonstandard for transmission of IP datagrams
over serial lines: SLIP._ Retrieved December 12, 2017, from
https://tools.ietf.org/html/rfc1055

### ASIO

### Boost

