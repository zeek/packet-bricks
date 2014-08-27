-- /usr/bin/lua
---------------------- STARTUP SCRIPT ---------------------------------





-----------------------------------------------------------------------
-- MACROS-&-UTILITY-FUNCS
-----------------------------------------------------------------------
STATS_PRINT_CYCLE_DEFAULT = 2
SLEEP_TIMEOUT = 1
PKT_BATCH=1024
NETMAP_LIN_PARAMS_PATH="/sys/module/netmap_lin/parameters/"
NETMAP_PIPES=64
NO_CPU_AFF=-1
NO_QIDS=-1
BUFFER_SZ=1024


--see if the directory exists

local function directory_exists(sPath)
      if type(sPath) ~= "string" then return false end

      local response = os.execute( "cd " .. sPath )
      if response == 0 then
      	 return true
      end
      return false
end



--sleep function __self-explanatory__

local clock = os.clock
function sleep(n)  -- seconds
	 local t0 = clock()
	 while clock() - t0 <= n do end
end



-- A neat function that reads shell output given a shell command `c'
function shell(c)
	 local o, h
	 h = assert(io.popen(c,"r"))
	 o = h:read("*all")
	 h:close()
	 return o
end


-- check if netmap module is loaded in the system

function netmap_loaded()
	 if string.find((tostring(shell('uname'))), 'Linux') ~= nil then
	    if directory_exists(NETMAP_LIN_PARAMS_PATH) then
	       return true
	    end
	 end
	 if string.find((tostring(shell('uname'), 'FreeBSD'))) ~= nil then
	    if (tostring(shell('sysctl -a --pattern netmap'))) ~= nil then
	       return true
	    end
	 end
	 return false
end


-- enable netmap pipes framework

function enable_nmpipes()
	if string.find((tostring(shell('uname'))), 'Linux') ~= nil then
	 	 shell("echo " .. tostring(NETMAP_PIPES) .. " > " .. NETMAP_LIN_PARAMS_PATH .. "default_pipes")
	end
	if string.find((tostring(shell('uname'))), 'FreeBSD') ~= nil then
	   	 shell("sysctl -w dev.netmap.default_pipes=\"" .. tostring(NETMAP_PIPES) .. "\"")
	end	
end




-- check if you are root
-- XXX This needs to be improved
local function check_superuser()
      if string.find((tostring(shell('whoami'))), 'root') == nil then 
      	 return false
      end
      return true
end

-----------------------------------------------------------------------









-----------------------------------------------------------------------
-- S I N G L E - T H R E A D E D - S E T U P
-----------------------------------------------------------------------

--setup_config1	 __sets up a simple load balancing configuration__
--		 __the engine reads from netmap-enabled eth3 and__
--		 __evenly splits traffic 5-way.		        __
function setup_config1(pe)
	 local lb = Element.new("LoadBalancer", 2)
	 lb:connect_input("eth3") 
         lb:connect_output("eth3{0", "eth3{1", "eth3{2", "eth3{3", "eth2")
	 --lb:connect_output("eth3{0", "eth3{1", "eth3{2", "eth3{3", "eth3{4", "eth3{5", "eth3{6", "eth3{7")
	 -- now link it!
	 pe:link(lb)
end


--setup_config2	 __sets up a combo configuration of duplication__
--		 __and load balancing. The engine reads from __
--		 __netmap-enabled eth3 and first duplicates__
--		 __traffic and then splits traffic from 1 branch__
--		 __using a load balancer.		        __
function setup_config2(pe)
	 local dup = Element.new("Duplicator")
         dup:connect_input("eth3") 
         dup:connect_output("eth3{0", "eth3{1")
	 local lb2 = Element.new("LoadBalancer", 4)
	 lb2:connect_input("eth3}0")
	 lb2:connect_output("eth3{2", "eth3{3")
	 dup:link(lb2)
	 -- now link it!
	 pe:link(dup)
end


--setup_config3	 __sets up a simple configuration using a duplicatior__
--		 __The engine reads from a netmap-enabled eth3 and__
--		 __duplicates traffic 4-way.		          __
function setup_config3(pe)
         local dup = Element.new("Duplicator")
         dup:connect_input("eth3")
         --dup:connect_output("eth3{0", "eth3{1", "eth3{2", "eth3{3")
	 dup:connect_output("eth3{0", "eth3{1", "eth3{2", "eth3{3", "eth3{4", "eth3{5", "eth3{6", "eth3{7")
	 -- now link it!
	 pe:link(dup)
end


--setup_config5	 __sets up a combo of load balancing and merge__
--		 __configuration. The engine reads from netmap-__
--		 __enabled eth3, then splits traffic and finally__
--		 __ merge packets into a netmap pipe.		__
function setup_config5(pe)
	 local lb = Element.new("LoadBalancer", 4)
         lb:connect_input("eth3") 
         lb:connect_output("eth3{0", "eth3{1")
	 local mrg = Element.new("Merge")
	 mrg:connect_input("eth3}0", "eth3}1")
	 mrg:connect_output("eth3{3")
	 lb:link(mrg)
	 -- now link it!
	 pe:link(lb)
end


--setup_config6	 __sets up a configuration of filter element__
--		 __The engine reads from netmap-enabled eth3__
--		 __and then splits traffic based on the filtering__
--		 __decisions between the output links.		__
--function setup_config6(pe)
--	 local lb = Element.new("LoadBalancer", 4)
--       lb:connect_input("eth3") 
--       lb:connect_output("eth3{0", "eth3{1")
--	 f = Element.new("Filter")
--	 f:connect_input("eth3}0")
--	 f:connect_output("eth3{2", "tcp");
--	 f:connect_output("eth3{3", "others");
--	 lb:link(f)
--	 -- now link it!
--	 pe:link(lb)
--end



--setup_config7	 __sets up a configuration of Merge element__
--		 __The engine reads from netmap-enabled eth3__
--		 __and then writes all packets to pcap file__
function setup_config7(pe)
	 local lb = Element.new("LoadBalancer", 4)
         lb:connect_input("eth3") 
         lb:connect_output("eth3{0", "eth3{1")
	 local mrg = Element.new("Merge")
	 mrg:connect_input("eth3}0", "eth3}1")
	 mrg:connect_output(">out.pcap")
	 lb:link(mrg)	 
	 -- now link it!
	 pe:link(lb)
end



--init function  __initializes pkteng thread and links it with a__
--		 __netmap-enabled interface. collects PKT_BATCH__
--		 __pkts at a time. "cpu" and "batch" params can remain__
--		 __unspecified by passing '-1'. Next creates 4 netmap__
--		 __pipe channels to forward packets to userland apps__

function init()
	 -- check if netmap module is loaded
	 if netmap_loaded() == false then
	    print 'Netmap module does not exist'
	    os.exit(-1)
	 end

	 -- check if you are root
	 if check_superuser() == false then
	    print 'You have to be superuser to run this function'
	    os.exit(-1)
	 end

	 -- create a global variable pe
	 local pe = PktEngine.new("e0", "netmap", BUFFER_SZ)


	 -- enable underlying netmap pipe framework
	 enable_nmpipes()

	 -- setup config1
	 --setup_config1(pe)

	 -- setup config2
	 --setup_config2(pe)

	 -- setup config3
	 --setup_config3(pe)
	 
	 -- setup config5
	 --setup_config5(pe)

	 -- setup config7
	 setup_config7(pe)
end
-----------------------------------------------------------------------
--start function  __starts pkteng and prints overall per sec__
--		  __stats for STATS_PRINT_CYCLE_DEFAULT secs__

function start()
	 -- start reading pkts from the interface
	 local pe = PktEngine.retrieve("e0")
	 pe:start()

	 local i = 0
	 repeat
	     sleep(SLEEP_TIMEOUT)
	     --pe:show_stats()
	     BRICKS.show_stats()
	     i = i + 1
	 until i > STATS_PRINT_CYCLE_DEFAULT
end
-----------------------------------------------------------------------
--stop function  __stops the pkteng before printing the final stats.__
--		 __it then unlinks the interface from the engine and__
--		 __finally frees the engine context from the system__
function stop()
	 local pe = PktEngine.retrieve("e0")
	 --pe:show_stats()
	 BRICKS.show_stats()

	 pe:stop()
	 sleep(SLEEP_TIMEOUT)

	 pe:delete()
	 --BRICKS.shutdown()
end
-----------------------------------------------------------------------











-----------------------------------------------------------------------
-- S T A R T _ OF _  S C R I P T
-----------------------------------------------------------------------
-- __"main" function (Commented for user's convenience)__
--
-------- __This command prints out the main help menu__
-- BRICKS.help()
-------- __This command shows the current status of BRICKS__
-- BRICKS.print_status()
-------- __This prints out the __pkt_engine__ help menu__
-- PktEngine.help()
-------- __Initialize the system__
-- init()
-------- __Start the engine__
-- start()
-------- __Stop the engine__
-- stop()
-------- __The following commands quits the session__
-- BRICKS.shutdown()
-----------------------------------------------------------------------
