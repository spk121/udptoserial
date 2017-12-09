# udptoserial

This is a program that transmits UDP packets between two computers connected
only by serial ports.

Imagine that machine FOO wants to communicate with a UDP server on machine BAR
but the only connection between them is a serial cable.  This program
udptoserial could run on both machines, providing the translation from serial
to UDP.

On the surface, this may seem like a ridiculous scenario, but, I did actually
have a problem where two computers were connected only by serial comms over 
wireless 915 MHz RF radio modems.  On the unlicensed 915MHz band, one can
get reliable serial comms at 38,400 baud for nearby machines, and 1,200 baud
at 0.75 mile distances.  One of the problems with such modems is that they
are half-duplex: they cannot simultaneously transmit and receive.  So a
collision prevention procedure is required to determine who has the right
to send at which times.

While I didn't write thie program for Bluetooth, a more common scenario in
which point-to-point serial comms is used is with Bluetooth.  One might be
able to adapt this for that scenario.

Back in the olden days, this was actually a solved problem.  There were
protocols like SLIP and PPP that provided the translation.  In fact, some
versions of Linux and Windows still have SLIP or PPP functionality hiding
deep, deep down.

This program implements a version of SLIP along with a half-duplex
communication strategy.