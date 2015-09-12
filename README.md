# Reliable Transport Protocols
Implementation of three well known transport layer protocols for reliable transport: Alternating Bit, Go Back N and Selective Repeat (as part of an assignment).

The code executes in a simulated hardware/software environment. The simulator (written by [J.F.Kurose](http://lass.cs.umass.edu/~shenoy/courses/spring00/653/homeworks/prog2.html) ) simulates one way network delay, packet corruption (either the header or the data portion) and packet loss.  The simulator interfaces with the implemented layer 4 (transport layer) code from "above" and "below": it simulates application layer protocols and the lower network, link and physical layer protocols.

Just `make` and run. The `s` parameter is a seed for the simulator and `w` parameter is the window size for the Selective Repeat and Go Back N protocols. The simulator asks for the number of messages, packet loss probability, packet corruption probability, average time between messages from sender's layer 5, and the TRACE (0, 1 or 2, for debugging). Enter the appropriate values you want to see simulated.  

