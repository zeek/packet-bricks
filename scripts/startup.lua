-- /usr/bin/lua
---------------------- STARTUP SCRIPT ---------------------------------





-----------------------------------------------------------------------
-- MACROS-&-UTILITY-FUNCS
-----------------------------------------------------------------------
STATS_PRINT_CYCLE_DEFAULT = 2
SLEEP_TIMEOUT = 1
PKT_BATCH=1024
NETMAP_PARAMS_PATH="/sys/module/netmap_lin/parameters/"
NETMAP_PIPES=32

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
--init function  __initializes pkteng thread and links it with a__
--		 __netmap-enabled interface. collects 1024 packets at__
--		 __a time. "cpu" and "batch" params are optional__
--		 __Next creates 4 netmap pipe channels to forward__
--		 __packets to userland applications__

function init()
	 -- check if netmap module is loaded
	 if directory_exists(NETMAP_PARAMS_PATH) == false then
	    print 'Netmap module does not exist'
	    os.exit(-1)
	 end

	 -- check if you are root
	 if check_superuser() == false then
	    print 'You have to be superuser to run this function'
	    os.exit(-1)
	 end

	 pkteng.new({name="e0", type="netmap"})
	 pkteng.link({engine="e0", ifname="eth3", batch=PKT_BATCH})

	 -- enable underlying netmap pipe framework
	 shell("echo " .. tostring(NETMAP_PIPES) .. " > " .. NETMAP_PARAMS_PATH .. "default_pipes")

	 for cnt = 0, 3 do
	     -- Use this line to test SHARing
	     pkteng.open_channel({engine="e0", channel="netmap:eth3{" .. cnt, action="SHARE"})

	     -- Use this line to copy packets to all registered channels
	     --pkteng.open_channel({engine="e0", channel="netmap:eth3{" .. cnt, action="COPY"})

	     -- Use this line to drop all packets
	     --pkteng.drop_pkts({engine="e0"})

	     -- XXX
	     ----pkteng.open_channel({engine="e0", channel="netmap:bro{" .. cnt})
	 end

	 -- Use this line to forward packets to a different Ethernet port (under construction)
	 --pkteng.redirect_pkts({engine="e0", ifname="eth2"})
end
-----------------------------------------------------------------------
--start function  __starts pkteng and prints overall per sec__
--		  __stats for STATS_PRINT_CYCLE_DEFAULT secs__

function start()
	 pkteng.start({engine="e0"})

	 local i = 0
	 repeat
	     sleep(SLEEP_TIMEOUT)
	     --pkteng.show_stats({engine="e0"})
	     pacf.show_stats()
	     i = i + 1
	 until i > STATS_PRINT_CYCLE_DEFAULT
end
-----------------------------------------------------------------------
--stop function  __stops the pkteng before printing the final stats.__
--		 __it then unlinks the interface from the engine and__
--		 __finally frees the engine context from the system__
function stop()
	 --pkteng.show_stats({engine="e0"})
	 pacf.show_stats()

	 pkteng.stop({engine="e0"})
	 sleep(SLEEP_TIMEOUT)

	 pkteng.unlink({engine="e0", ifname="eth3"})
	 sleep(SLEEP_TIMEOUT)

	 pkteng.delete({engine="e0"})
	 --pacf.shutdown()
end
-----------------------------------------------------------------------





-----------------------------------------------------------------------
-- 4 - T H R E A D S - S E T U P
-----------------------------------------------------------------------
--init4 function __initializes 4 pkteng threads and links it with a__
--		 __netmap-enabled interface. collects 1024 packets __
--		 __at a time. "cpu", "batch" & "qid" params are    __
--		 __ optional.					   __
--		 __						   __
--		 ++_____________HOW TO USE H.W QUEUES______________++
--		 ++Please make sure that the driver is initialized ++
--		 ++with the right no. of h/w queues. In this setup,++
--		 ++ cpu_thread=0 is registered with H/W queue 0    ++
--		 ++ cpu_thread=1 is registered with H/W queue 1	   ++
--		 ++ cpu_thread=2 is registered with H/W queue 2	   ++
--		 ++ cpu_thread=3 is registered with H/W queue 3	   ++
--		 ++________________________________________________++

function init4()
	 -- check if netmap module is loaded
	 if directory_exists(NETMAP_PARAMS_PATH) == false then
	    print 'Netmap module does not exist'
	    os.exit(-1)
	 end

	 -- check if you are root
	 if check_superuser() == false then
	    print 'You have to be superuser to run this function'
	    os.exit(-1)
	 end

	 -- enable underlying netmap pipe framework
	 shell("echo " .. tostring(NETMAP_PIPES) .. " > " .. NETMAP_PARAMS_PATH .. "default_pipes")

	 for cnt = 0, 3 do
	 	 pkteng.new({name="e" .. cnt, type="netmap", cpu=cnt})
	 	 pkteng.link({engine="e" .. cnt, ifname="eth3", batch=PKT_BATCH, qid=cnt})

	 	 -- Use this line to test SHARing
		 pkteng.open_channel({engine="e" .. cnt, channel="netmap:eth3{" .. cnt, action="SHARE"})

	     	 -- Use this line to copy packets to all registered channels
		 --pkteng.open_channel({engine="e" .. cnt, channel="netmap:eth3{" .. cnt, action="COPY"})

		 -- Use this line to drop all packets
	     	 --pkteng.drop_pkts({engine="e0"})

		 -- Use this line to forward packets to a different Ethernet port (under construction)
	     	 --pkteng.redirect_pkts({engine="e" .. cnt, ifname="eth2"})
	 end
end
-----------------------------------------------------------------------
--start4 function __starts all 4 pktengs and prints overall per sec__
--		  __stats for STATS_PRINT_CYCLE_DEFAULT secs__

function start4()
	 for cnt = 0, 3 do
	 	 pkteng.start({engine="e" .. cnt})
	 end

	 local i = 0
	 repeat
	     sleep(SLEEP_TIMEOUT)
	     pacf.show_stats()
	     i = i + 1
	 until i > STATS_PRINT_CYCLE_DEFAULT
end
-----------------------------------------------------------------------
--stop4 function __stops the pktengs before printing the final stats.__
--		 __it then unlinks the interface from the engine and__
--		 __finally frees the engine context from the system__
function stop4()
	 pacf.show_stats()
	 for cnt = 0, 3 do
		 pkteng.stop({engine="e" .. cnt})
	 end

	 sleep(SLEEP_TIMEOUT)

	 for cnt = 0, 3 do
	 	 pkteng.unlink({engine="e" .. cnt, ifname="eth3"})
	 end

	 sleep(SLEEP_TIMEOUT)

	 for cnt = 0, 3 do
	 	 pkteng.delete({engine="e" .. cnt})
	 end

	 --pacf.shutdown()
end
-----------------------------------------------------------------------





-----------------------------------------------------------------------
-- S T A R T _ OF _  S C R I P T
-----------------------------------------------------------------------
-- __"main" function (Commented for user's convenience)__
--
-------- __This command prints out the main help menu__
-- pacf.help()
-------- __This command shows the current status of PACF__
-- pacf.print_status()
-------- __This prints out the __pkt_engine__ help menu__
-- pkteng.help()
-------- __Initialize the system__
-- init()
-------- __Start the engine__
-- start()
-------- __Stop the engine__
-- stop()
-------- __The following commands quits the session__
-- pacf.shutdown()
-----------------------------------------------------------------------