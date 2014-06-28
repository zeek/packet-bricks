-- /usr/bin/lua
---------------------- STARTUP SCRIPT --------------------------------
-- Only a few commands are currently functional. Please check back for
-- updates later.

-- MACROS
STATS_PRINT_CYCLE_DEFAULT = 10
SLEEP_TIMEOUT = 1
-----------------------------------------------------------------------
--sleep function __self-explanatory__

local clock = os.clock
function sleep(n)  -- seconds
	 local t0 = clock()
	 while clock() - t0 <= n do end
end
-----------------------------------------------------------------------
--init function __initializes pkteng thread and links it with a netmap__
--		__enabled interface. collects 1024 packets at a time.__
--		__"cpu" and "batch" params are optional__

function init()
	 pkteng.new({name="e0", type="netmap", cpu=0})
	 pkteng.link({engine="e0", ifname="eth3", batch=1024})
end
-----------------------------------------------------------------------
--start function __starts the pkteng and prints overall per sec stats__
--		  __for STATS_PRINT_CYCLE_DEFAULT secs__

function start(c)
	 pkteng.start({engine="e0"})
	 local i = 0
	 repeat
	     sleep(SLEEP_TIMEOUT)
	     pkteng.show_stats({engine="e0"})
	     i = i + 1
	 until i > STATS_PRINT_CYCLE_DEFAULT
end
-----------------------------------------------------------------------
--stop function __stops the pkteng before printing the final stats. it__
--		__then unlinks the interface from the engine and finally__
--		__frees the engine context from the system__
function stop()
	 pkteng.show_stats({engine="e0"})
	 pkteng.stop({engine="e0"})
	 sleep(SLEEP_TIMEOUT)
	 pkteng.unlink({engine="e0", ifname="eth3"})
	 sleep(SLEEP_TIMEOUT)
	 pkteng.delete({engine="e0"})
	 -- pacf.shutdown()
end
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
-- init()
-------- __Start the engine__
-- start()
-------- __Stop the engine__
-- stop()
-------- __The following commands quits the session__
-- pacf.shutdown()
