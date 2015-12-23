# README
Packet Bricks is a Linux/FreeBSD daemon that is capable of receiving
and distributing ingress traffic to userland applications. Its main 
responsibilities may include (i) load-balancing, (ii) duplicating 
and/or (iii) filtering ingress traffic across all registered 
applications. The distribution is flow-aware (i.e. packets of one 
connection will always end up in the same application). At the moment,
packet-bricks uses netmap packet I/O framework for receiving packets.
It employs netmap pipes to forward packets to end host applications. 

Before running packet-bricks, please make sure that you have
installed the netmap driver successfully. You should first try
running the sample pkt-gen binary (that is available in the netmap's
examples/) directory to see if the driver is operational. The netmap
module is continuously being updated with enhancements and bug fixes
(as is common for all open source tools including packet bricks). Before
using packet bricks, please make sure that you have downloaded and
updated the netmap kernel module with its latest version. The netmap
authors suggest building the netmap module into the kernel for
performance reasons. You can download the latest netmap driver from
the following link: https://github.com/luigirizzo/netmap.

The following guide provides a walkthrough of the
/usr/local/etc/bricks-scripts/startup-*.lua files and also explains
what this system is capable of.
```text
                    					    	     _________
                    						        |	      |
                    						        |  APP 1  |
                    						        |_________|
                    				   	  eth3}0     /
          	   	    	      	      ______________/
          	   	    	      	     |
          	   	    	      	     |		         _________
          	   	    	      	     |    eth3}1    |	      |
          	   	    	      	     |  ____________|  APP 2  |
          	   	    	      	     | |            |_________|
   1/10 Gbps link     	     	     | |	
   |||||||||||||| ------->eth3{PACKET-BRICKS}	     _________
                				     | |  eth3}2    |	      |
                				     | |____________|  APP 3  |
                    				 |  		    |_________|
                				     |
                				     |    eth3}3
                	 			     |_____________  
                    				     		   \
                        						    \_________
                        	      			        |	      |
                    	                            |  APP 4  |
                                				    |_________|
```		         
Figure 1: A sample packet-bricks instance redirecting ingress traffic from eth3 to 4 userland applications using a LoadBalancer brick


User can start the program by running the following command:
```tcsh
# bricks [-f startup_script_file]
```
To start the program as a daemon, either type:
```tcsh
# bricks -d [-f startup_script_file]
```
OR
```tcsh
# bricks-server [-f startup_script_file]
```
If the system is executed as a non-daemon, it opens a 
LUA-based shell. Type the following command to print the help menu:
```lua
	bricks> BRICKS.help()
	BRICKS Commands:
    	    help()
    	    print_status()
    	    show_stats()
            shutdown()
          Available subsystems within BRICKS have their own help() methods:
          pkteng
```
Alternatively, the user can run `bricks-shell` to access the
interface shell to communicate with daemonized bricks-server.

You can hit Ctrl+D to exit the bricks-shell client. To gracefully
turn the daemon off, the user may enter the following command to 
exit the program:
```lua
	bricks> BRICKS.shutdown()
	Goodbye!
```
Packet bricks relies on netmap pipes to distribute traffic across
multiple applications. Some OSes have netmap pipes disabled by
default. Please use the following command to enable netmap pipes
(if needed) in a packet bricks session:
```lua
	bricks> utilObj:enable_nmpipes()
```
In packet-bricks, the most important component is the pkteng module.
It manages the reception and distribution of ingress traffic coming
through an interface. Once instantiated, the pkteng module spawns a 
thread that starts the packet reception process. After running 
packet-bricks again, use the following command to create a pkteng 
instance, pe:
```lua
	bricks> pe = PktEngine.new("e0")
```
The "e0" field is internally used by packet-bricks as pkteng's ID. 
You can delete the pkteng instance by running the following command:
```lua
	bricks> pe:delete()
```
The pkteng instance can be instantiated with the possibility of
affinitizing the engine thread to an arbitrary cpu core id:
```lua
	bricks> pe = PktEngine.new("e0", 1024, 1)
```
The last parameter affinitizes the module to CPU 1 once the engine
thread starts reading packets.
In packet-bricks, ingress traffic can be manipulated with packet 
engine constructs called "bricks". Currently packet-bricks has 
the following built-in bricks that are available for use:

1. LoadBalancer: Brick that may be used to split flow-wise
   		 traffic to different applications using netmap 
		 pipes.

2. Duplicator: Brick that may be used to duplicate traffic
   	       across each registered netmap pipe.

3. Merge: Brick that may be used to combine traffic between
   	  2 or more netmap pipes.

4. PcapReader: Brick that may be used to read ingress traffic
   	       from a pcap dump file.

5. PcapWriter: Brick that may be used to redirect ingress
   	       traffic to a pcap dump file.
	       
6. Filter: Brick that can be used for traffic shaping. It
   	   accepts remote requests over the network using
	   Bro's broker library. The message bundle is created
	   using the NetControl protocol. This can only be used
	   when Packet bricks is compiled with broker plugin.

A packet engine can be linked to any of these bricks with
any combination/configuration of user's liking. Please see the
scripts/ example directory to see how bricks can be used to 
run variants of such packet engines.

The succeeding text uses a simple load balancing brick 
("LoadBalancer") to show how packet-bricks runs. A freshly
instantiated packet engine can be used to link the LoadBalancer
as:

```lua
	bricks> lb = Brick.new("LoadBalancer", 2)
	bricks> lb:connect_input("eth3")
	bricks> lb:connect_output("eth3{0", "eth3{1", 
				  "eth3{2", "eth3{3", "eth2")
	bricks> pe:link(lb)
```
This binds pkteng pe with LoadBalancer brick and asks the system 
to read ingress packets from eth3 and split them flow-wise based on 
the 2-tuple (src & dst IP addresses) metadata of the packet header.
The "lb:connect_output(...)" command creates four netmap-specific 
pipes named "netmap:eth3{x" where 0 <= x < 4 and an egress 
interface named "eth2". The traffic is evenly split between all five
channels based on the 2 tuple header as previously mentioned. 
Userland applications can now use packet-bricks to get their fair 
share of ingress traffic. The brick is finally linked with the 
packet engine.

One can always unlink the brick by running the following command:
```lua
	bricks> pe:unlink()
```

If the user is content with the packet engine setup, he/she can
bypass unlinking and start the engine:

```lua
	bricks> pe:start()
```

The engine will start sniffing for packets from the interface 
promiscuously. You can use the following command to get the run-time
packets-related statistics from the engine:
```lua
	bricks> pe.show_stats()
```

Sample applications (e.g. netmap's pkt-gen) can read ingress
traffic from packet-bricks using following command line arguments:
```tcsh
	$ sudo ./pkt-gen -i netmap:eth3}0 -f rx &
	$ sudo ./pkt-gen -i netmap:eth3}1 -f rx &
	$ sudo ./pkt-gen -i netmap:eth3}2 -f rx &
	$ sudo ./pkt-gen -i netmap:eth3}3 -f rx &
```

Please note that you cannot unlink a brick from the pkteng
while the engine is running. To stop the engine, please run the 
following command:
```lua
	bricks> pe:stop()
```
For user's convenience, the packet-bricks package comes with a 
reference LUA script file: please see scripts/startup-*.lua files for
details. You can also use the following command to load the script
file in packet-bricks at startup:
```tcsh
# bricks -f /usr/local/etc/bricks-scripts/startup-one-thread.lua
```
OR
```tcsh
# bricks-server -f /usr/local/etc/bricks-scripts/startup-one-thread.lua
```
The user is recommended to skim through the script file. It is
heavily documented. The scripts directory also contains code for 
running a 4-threaded version of the pkteng (see 
/usr/local/etc/bricks-scripts/startup-multi-threads.lua).

### New
We have created 2 new tools that can quickly set up (i) load-balancer,
and (ii) duplicator.

i) The user can use bricks-load-balance to split traffic for a given
interface. Example usage:
```tcsh
$ bricks-load-balance eth3 4
```
The example above will split traffic on netmap-enabled interface, eth3,
to 4 netmap pipe channels named eth3}0, eth3}1, eth3}2 and eth3}3.

ii) The user can use bricks-duplicate to duplicate ingress traffic
for the given interface. Example usage:
```tcsh
$ bricks-duplicate eth3 4
```
The example above will duplicate traffic on netmap-enabled interface,
eth3, to 4 netmap pipe channels named eth3}0, eth3}1, eth3}2 and eth3}3.

### Connecting with broker
Packet bricks can be configured to accept remote requests via the
broker communication module. Users can perform simple traffic shaping
tasks by issuing requests to specific Filter bricks. Packet bricks uses
NetControl protocol for this purpose. A user can refer to each Filter
via the output netmap pipe name of the brick.

## Troubleshooting

Packet bricks is still in development stages. While we welcome 
feedback, we suggest that you refer to the following pointers before
contacting the authors for bug reports.

1. Please install the required libraries mentioned in INSTALL file.
As the system is still under active development, the program may
exhibit unexpected behavior if libraries are not installed.

2. The README file in the netmap/ directory contains pointers
on how to enhance packet I/O performance. We suggest you to follow
their guide for extracting maximum performance out of packet
bricks as well.

3. Some users have reported erroneous behavior when executing
tcpdump with netmap-libpcap patch. tcpdump has compatibility 
constraints with libpcap. Please refer to 
http://www.tcpdump.org/#old-releases to verify which tcpdump
version should be linked with the latest netmap-libpcap
release. At the time of writing, the latest netmap-libpcap
version is 1.6.0. The compatible tcpdump version for this 
release is 4.6.0.

4. Please report further bugs to: ajamshed@icsi.berkeley.edu
