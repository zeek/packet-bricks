-- /usr/bin/lua
---------------------- STARTUP SCRIPT ---------------------------------





-----------------------------------------------------------------------
-- MACROS-&-UTILITY-FUNCS
-----------------------------------------------------------------------
STATS_PRINT_CYCLE_DEFAULT = 2
SLEEP_TIMEOUT = 1
PKT_BATCH=1024


--sleep function __self-explanatory__

local clock = os.clock
function sleep(n)  -- seconds
	 local t0 = clock()
	 while clock() - t0 <= n do end
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
	 pkteng.new({name="e0", type="netmap"})
	 pkteng.link({engine="e0", ifname="eth3", batch=PKT_BATCH})

	 for cnt = 0, 3 do
	     pkteng.open_channel({engine="e0", channel="netmap:eth3{" .. cnt})
	 end

	 -- VALE EXTENSIONS COMMENTED OUT --
	 --pkteng.open_channel({engine="e0", channel="valeA:s"})
	 --pkteng.open_channel({engine="e0", channel="valeB:s"})
	 --pkteng.open_channel({engine="e0", channel="valeC:s"})
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
	 pacf.shutdown()
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
	 for cnt = 0, 3 do
	 	 pkteng.new({name="e" .. cnt, type="netmap", cpu=cnt})
	 	 pkteng.link({engine="e" .. cnt, ifname="eth3", batch=PKT_BATCH, qid=cnt})
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

	 -- pacf.shutdown()
end
-----------------------------------------------------------------------





-----------------------------------------------------------------------
-- S T A R T _ OF _  S C R I P T
-----------------------------------------------------------------------
-- __"main" function (partially commented for user's convenience)__
--
-------- __This command prints out the main help menu__
pacf.help()
-------- __This command shows the current status of PACF__
pacf.print_status()
-------- __This prints out the __pkt_engine__ help menu__
pkteng.help()
-------- __Initialize the system__
init()
-------- __Start the engine__
start()
-------- __Stop the engine__
-- stop()
-------- __The following commands quits the session__
-- pacf.shutdown()
-----------------------------------------------------------------------