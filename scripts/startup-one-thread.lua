-- /usr/bin/lua
---------------------- STARTUP SCRIPT ---------------------------------
-- contains utility functions and macros
utilObj = dofile("scripts/utils.lua")
-- contains example setup scripts
sampleSetup = dofile("scripts/configs/single-threaded-setups.lua")









-----------------------------------------------------------------------
-- S I N G L E - T H R E A D E D - S E T U P
-----------------------------------------------------------------------
--init function  __initializes pkteng thread and links it with a__
--		 __netmap-enabled interface. collects PKT_BATCH__
--		 __pkts at a time. "cpu" and "batch" params can remain__
--		 __unspecified by passing '-1'. Next creates 4 netmap__
--		 __pipe channels to forward packets to userland apps__

function init()
	 -- check if netmap module is loaded
	 if utilObj:netmap_loaded() == false then
	    print 'Netmap module does not exist'
	    os.exit(-1)
	 end

	 -- check if you are root
	 if utilObj:check_superuser() == false then
	    print 'You have to be superuser to run this function'
	    os.exit(-1)
	 end

	 -- enable underlying netmap pipe framework
	 utilObj:enable_nmpipes()

	 -- create a global variable pe
	 local pe = PktEngine.new("e0", "netmap", BUFFER_SZ)

	 -- setup loadbalancer config
	 sampleSetup:lb_config(pe)

	 -- setup dup/lb config
	 --sampleSetup:duplb_config(pe)

	 -- setup dup config
	 --sampleSetup:dup_config(pe)
	 
	 -- setup lb/mrg config
	 --sampleSetup:lbmrg_config(pe)

	 -- setup lbmrgpcap config
	 --sampleSetup:lbmrgpcap_config(pe)
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
	     utilObj:sleep(SLEEP_TIMEOUT)
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
	 utilObj:sleep(SLEEP_TIMEOUT)

	 pe:delete()
	 --BRICKS.shutdown()
end
-----------------------------------------------------------------------











-----------------------------------------------------------------------
-- S T A R T _ OF _ S C R I P T
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
