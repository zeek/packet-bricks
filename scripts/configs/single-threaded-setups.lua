-- /usr/bin/lua
-----------------------------------------------------------------------
-- S I N G L E - T H R E A D E D - S E T U P S
-----------------------------------------------------------------------
local C={};
PCAPDIR="/usr/home/ajamshed/"


-----------------------------------------------------------------------
--lb_config	 __sets up a simple load balancing configuration__
--		 __the engine reads from netmap-enabled $int1 and__
--		 __evenly splits traffic 5-way.		        __
function C:lb_config(pe, int1, int2)
	 local lb = Brick.new("LoadBalancer", 2)
	 lb:connect_input(int1) 
         lb:connect_output(int1 .. "{0", int1 .. "{1", int1 .. "{2", int1 .. "{3", int2)
	 -- now link it!
	 pe:link(lb)
end


----------------------------------------------------------------------


--duplb_config	 __sets up a combo configuration of duplication__
--		 __and load balancing. The engine reads from __
--		 __netmap-enabled $int1 and first duplicates__
--		 __traffic and then splits traffic from 1 branch__
--		 __using a load balancer.		        __
function C:duplb_config(pe, int1)
	 local dup = Brick.new("Duplicator")
         dup:connect_input(int1) 
         dup:connect_output(int1 .. "{0", int1 .. "{1")
	 local lb2 = Brick.new("LoadBalancer", 4)
	 lb2:connect_input(int1 .. "}0")
	 lb2:connect_output(int1 .. "{2", int1 .. "{3")
	 dup:link(lb2)
	 -- now link it!
	 pe:link(dup)
end


-----------------------------------------------------------------------


--dup_config	 __sets up a simple configuration using a duplicatior__
--		 __The engine reads from a netmap-enabled $int1 and__
--		 __duplicates traffic 4-way.		          __
function C:dup_config(pe, int1)
         local dup = Brick.new("Duplicator")
         dup:connect_input(int1)
         dup:connect_output(int1 .. "{0", int1 .. "{1", int1 .. "{2", int1 .. "{3")
	 -- now link it!
	 pe:link(dup)
end


-----------------------------------------------------------------------


--lbmrg_config	 __sets up a combo of load balancing and merge__
--		 __configuration. The engine reads from netmap-__
--		 __enabled $int1, then splits traffic and finally__
--		 __ merge packets into a netmap pipe.		__
function C:lbmrg_config(pe, int1)
	 local lb = Brick.new("LoadBalancer", 4)
         lb:connect_input(int1) 
         lb:connect_output(int1 .. "{0", int1 .. "{1")
	 local mrg = Brick.new("Merge")
	 mrg:connect_input(int1 .. "}0", int1 .. "}1")
	 mrg:connect_output(int1 .. "{3")
	 lb:link(mrg)
	 -- now link it!
	 pe:link(lb)
end



-----------------------------------------------------------------------

--mrg_config	 __sets up a simple merge brick configuration__
--		 __The engine reads from netmap-enabled $int1__
--		 __& $int2, and then pushes it in a common__
--		 __ netmap pipe.__
function C:mrg_config(pe, int1, int2)
	 local mrg = Brick.new("Merge")
         mrg:connect_input(int1, int2) 
         mrg:connect_output(int1 .. "{1")

	 -- The following works fine as well
	 local lb = Brick.new("LoadBalancer", 4)
	 lb:connect_input(int1 .. "}1")
	 lb:connect_output(int2 .. "{2", int2 .. "{3", int2 .. "{4", int2 .. "{5")
	 mrg:link(lb)
	 
	 -- now link it!
	 pe:link(mrg)
end



-----------------------------------------------------------------------


--lbfilt_config	 __sets up a configuration of filter brick__
--		 __The engine reads from netmap-enabled eth3__
--		 __and then splits traffic based on the filtering__
--		 __decisions between the output links.		__
function C:lbfilt_config(pe)
	 local lb = Brick.new("LoadBalancer", 4)
         lb:connect_input("eth3") 
         lb:connect_output("eth3{0", "eth3{1")
	 f = Brick.new("Filter")
	 f:connect_input("eth3}0")
	 f:connect_output("eth3{2", "eth3{3");
	 lb:link(f)
	 -- now link it!
	 pe:link(lb)
end


-----------------------------------------------------------------------


--lbmrgpcap_config  __sets up a configuration of Merge brick__
--		    __The engine reads from netmap-enabled $int1__
--		    __and then writes all packets to pcap file__
function C:lbmrgpcap_config(pe, int1)
	 local lb = Brick.new("LoadBalancer", 4)
         lb:connect_input(int1) 
         lb:connect_output(int1 .. "{0", int1 .. "{1")
	 local mrg = Brick.new("Merge")
	 mrg:connect_input(int1 .. "}0", int1 .. "}1")
	 mrg:connect_output(int1 .. "{2")
	 lb:link(mrg)

	 local pw = Brick.new("PcapWriter")
	 pw:connect_input(int1 .. "}2")
	 pw:connect_output("out.pcap")
	 mrg:link(pw)

	 -- now link it!
	 pe:link(lb)
end
-----------------------------------------------------------------------
--simple_lbconfig  __a trivial example of the load balancer usage__
--		   __specifically designed for FreeBSD config as__
--		   __it uses hard-coded "em0" interface name__
function C:simple_lbconfig(pe, intf)
	 local lb = Brick.new("LoadBalancer", 4)
	 lb:connect_input(intf) 
         lb:connect_output(intf .. "{0", intf .. "{1", intf .. "{2", intf .. "{3")
	 -- now link it!
	 pe:link(lb)
end
-----------------------------------------------------------------------
--pcapr_config     __a trivial example for reading pcap files__
--		   __adding more output nodes will divide traffic__
--		   __based on load balancing hashing algo        __
function C:pcapr_config(pe, intf)
	 local pr = Brick.new("PcapReader")
	 pr:connect_input(PCAPDIR, intf)
	 pr:connect_output(intf .. "{0")
         --pr:connect_output(intf .. "{0", intf .. "{1", intf .. "{2", intf .. "{3")
	 -- now link it!
	 pe:link(pr)
end
-----------------------------------------------------------------------
--pcapw_config     __a trivial example for writing pcap files__
function C:pcapw_config(pe, intf)
	 local lb = Brick.new("LoadBalancer", 4)
	 lb:connect_input(intf)
         lb:connect_output(intf .. "{0", intf .. "{1", intf .. "{2", intf .. "{3")
	 local pw1 = Brick.new("PcapWriter")
	 pw1:connect_input(intf .. "}0")
	 pw1:connect_output("0.pcap")
	 lb:link(pw1)
	 local pw2 = Brick.new("PcapWriter")
	 pw2:connect_input(intf .. "}2")
	 pw2:connect_output("2.pcap")
	 lb:link(pw2)

	 --local d = Brick.new("Dummy")
	 --d:connect_input("2.pcap")
	 --d:connect_output(intf .. "}4")
	 --pw2:link(d)
	 -- now link it!
	 pe:link(lb)
end
-----------------------------------------------------------------------
--dummy_config     __a trivial example for performance testing__
function C:dummy_config(pe, intf)
	 local pr = Brick.new("Dummy")
	 pr:connect_input(intf) 
         pr:connect_output(intf .. "{0")
	 -- now link it!
	 pe:link(pr)
end
-----------------------------------------------------------------------
--lbconfig  	   __a trivial example of the load balancer usage__
function C:lbconfig(pe, intf, split)
	 local lb = Brick.new("LoadBalancer", 4)
	 lb:connect_input(intf) 
         lb:connect_outputs(intf, split)
	 -- now link it!
	 pe:link(lb)
end
-----------------------------------------------------------------------
--dupconfig  	   __a trivial example of the duplicator usage__
function C:dupconfig(pe, intf, split)
	 local dup = Brick.new("Duplicator")
	 dup:connect_input(intf) 
         dup:connect_outputs(intf, split)
	 -- now link it!
	 pe:link(dup)
end
-----------------------------------------------------------------------
return C;