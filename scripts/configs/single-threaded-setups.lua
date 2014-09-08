-- /usr/bin/lua
-----------------------------------------------------------------------
-- S I N G L E - T H R E A D E D - S E T U P S
-----------------------------------------------------------------------
local C={};


-----------------------------------------------------------------------
--lb_config	 __sets up a simple load balancing configuration__
--		 __the engine reads from netmap-enabled eth3 and__
--		 __evenly splits traffic 5-way.		        __
function C:lb_config(pe)
	 local lb = Brick.new("LoadBalancer", 2)
	 lb:connect_input("eth3") 
         lb:connect_output("eth3{0", "eth3{1", "eth3{2", "eth3{3", "eth2")
	 --lb:connect_output("eth3{0", "eth3{1", "eth3{2", "eth3{3", "eth3{4", "eth3{5", "eth3{6", "eth3{7")
	 -- now link it!
	 pe:link(lb)
end


----------------------------------------------------------------------


--duplb_config	 __sets up a combo configuration of duplication__
--		 __and load balancing. The engine reads from __
--		 __netmap-enabled eth3 and first duplicates__
--		 __traffic and then splits traffic from 1 branch__
--		 __using a load balancer.		        __
function C:duplb_config(pe)
	 local dup = Brick.new("Duplicator")
         dup:connect_input("eth3") 
         dup:connect_output("eth3{0", "eth3{1")
	 local lb2 = Brick.new("LoadBalancer", 4)
	 lb2:connect_input("eth3}0")
	 lb2:connect_output("eth3{2", "eth3{3")
	 dup:link(lb2)
	 -- now link it!
	 pe:link(dup)
end


-----------------------------------------------------------------------


--dup_config	 __sets up a simple configuration using a duplicatior__
--		 __The engine reads from a netmap-enabled eth3 and__
--		 __duplicates traffic 4-way.		          __
function C:dup_config(pe)
         local dup = Brick.new("Duplicator")
         dup:connect_input("eth3")
         --dup:connect_output("eth3{0", "eth3{1", "eth3{2", "eth3{3")
	 dup:connect_output("eth3{0", "eth3{1", "eth3{2", "eth3{3", "eth3{4", "eth3{5", "eth3{6", "eth3{7")
	 -- now link it!
	 pe:link(dup)
end


-----------------------------------------------------------------------


--lbmrg_config	 __sets up a combo of load balancing and merge__
--		 __configuration. The engine reads from netmap-__
--		 __enabled eth3, then splits traffic and finally__
--		 __ merge packets into a netmap pipe.		__
function C:lbmrg_config(pe)
	 local lb = Brick.new("LoadBalancer", 4)
         lb:connect_input("eth3") 
         lb:connect_output("eth3{0", "eth3{1")
	 local mrg = Brick.new("Merge")
	 mrg:connect_input("eth3}0", "eth3}1")
	 mrg:connect_output("eth3{3")
	 lb:link(mrg)
	 -- now link it!
	 pe:link(lb)
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
--		    __The engine reads from netmap-enabled eth3__
--		    __and then writes all packets to pcap file__
function C:lbmrgpcap_config(pe)
	 local lb = Brick.new("LoadBalancer", 4)
         lb:connect_input("eth3") 
         lb:connect_output("eth3{0", "eth3{1")
	 local mrg = Brick.new("Merge")
	 mrg:connect_input("eth3}0", "eth3}1")
	 -- if the output is prepended with '>', packet-bricks
	 -- treats the channel as a pcap-compatible file
	 mrg:connect_output(">out.pcap")
	 lb:link(mrg)	 
	 -- now link it!
	 pe:link(lb)
end
-----------------------------------------------------------------------
--simple_config  __comments later__
function C:simple_config(pe)
	 local lb = Brick.new("LoadBalancer", 2)
	 lb:connect_input("em0") 
         lb:connect_output("em0{0")
	 -- now link it!
	 pe:link(lb)
end
-----------------------------------------------------------------------
return C;