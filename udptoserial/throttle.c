/* This is a variation on the leaky bucket timer.

   The serial port has a data rate.

   Imagine a bit bucket.  Bits drain out of the bucket at a rate proportional
   to the serial port data rate.

   Message packets are put in the bucket.  Every time a new
   packet is received, if it doesn't fit in the bucket, it is dropped.

   Message packets drain from the bucket in a FIFO order.  Each packet
   drains from the bucket in a time interval based on its bit count and
   the data rate.

   This has the disadvantage of delaying sparse message traffic 
   unnecessarily, but, that's probably OK if the packets are small
   compared to 1 second's data rate.
*/

