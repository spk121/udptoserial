udptoserial - send UDP packets over a serial line

This is a daemon that opens one or more UDP ports and a serial connection.
It listens on the UDP port for packets.  When a packet is received, it 
is sent down the serial port using the SLIP protocol.

The daemon also listens for incoming data on the serial port.  If incoming
data is an SLIP-encoded IPv4 UDP packet, if that UDP port is being managed by
this daemon, the packet will be forwarded on to its destination.

Imagine two machines with a client/server application that would normally be connected by a router.
The client & server communicate on a single UDP bi-directional UDP packet.

                    ---+                           +---
  MACHINE 1            |                           |  MACHINE 2
  foo.spikycactus.com  |     Ethernet              |  bar.spikycactus.com
                       |- - - - - - - - - - - - - -| 
  GUI CLIENT           |                           |  SERVER
  PORT 4000            |                           |  PORT 4000
                    ---+                           +---

  foo.spikycactus.com:4000 <-------> bar.spikycactus.com:4000

If you put the udptoserial (U2S) on both machines

                          ---+                           +---
  MACHINE 1                  |                           |  MACHINE 2
  foo.spikycactus.com        |  RS-232                   |  bar.spikycactus.com
                             |- - - - - - - - - - - - - -| 
  GUI CLIENT     U2S         |                           |  U2S            SERVER
  PORT 4000 <--> PORT 4400   |                           |  PORT 4400 <--> PORT 4000
                          ---+                           +---

 foo.spikycactus.com:4000  sends UDP packet to U2S on foo.spikycactus.com:4400
 U2S sends packet down the serial line to the U2S on bar.spikycactus.com:4400
 U2S sends a UDP packet to the server on bar.spikycactus.com:4000



Step 1: an application sends a UDP packet to the daemon

For the original packet, the source address and source port are that of the UDP port
of the source application.  The destination address and destination port are that
of the machine on which the daemon is running, and of one of the ports it is managing.

Step 2: repackage the UDP packet for SLIP transmission.

There will be a bit of network address translation to make this work.


