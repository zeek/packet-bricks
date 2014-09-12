-- /usr/bin/lua
---------------------- STARTUP SCRIPT ---------------------------------
-- contains utility functions and macros
utilObj = dofile("scripts/utils.lua")
-- contains example setup script(s)
sampleSetup = dofile("scripts/configs/multi-threaded-setups.lua")










-----------------------------------------------------------------------
-- 4 - T H R E A D S - S E T U P
-----------------------------------------------------------------------



--init4 function __initializes 4 pkteng threads and links it with a__
--		 __netmap-enabled interface. collects PKT_BATCH    __
--		 __pkts at a time. "cpu", "batch" & "qid" params   __
--		 __can remain unspecified by passing '-1'	   __
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

	 for cnt = 0, 3 do
	 	 local pe = PktEngine.new("e" .. cnt, BUFFER_SZ, cnt)
		 
		 --setup: load balancing config
		 -- (see configs/multi-threaded-setups.lua file)
		 lb_config(pe, "eth3", cnt)
	 end
end
-----------------------------------------------------------------------
--start4 function __starts all 4 pktengs and prints overall per sec__
--		  __stats for STATS_PRINT_CYCLE_DEFAULT secs__

function start4()
	 for cnt = 0, 3 do
	     	 local pe = PktEngine.retrieve("e" .. cnt)
	 	 pe:start()
	 end

	 local i = 0
	 repeat
	     utilObj:sleep(SLEEP_TIMEOUT)
	     BRICKS.show_stats()
	     i = i + 1
	 until i > STATS_PRINT_CYCLE_DEFAULT
end
-----------------------------------------------------------------------
--stop4 function __stops the pktengs before printing the final stats.__
--		 __it then unlinks the interface from the engine and__
--		 __finally frees the engine context from the system__
function stop4()
	 BRICKS.show_stats()
	 for cnt = 0, 3 do
	     	 local pe = PktEngine.retrieve("e" .. cnt)
		 pe:stop()
	 end

	 utilObj:sleep(SLEEP_TIMEOUT)

	 for cnt = 0, 3 do
	     	 local pe = PktEngine.retrieve("e" .. cnt)
	 	 pe:delete()
	 end

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
-- init4()
-------- __Start the engine__
-- start4()
-------- __Stop the engine__
-- stop4()
-------- __The following commands quits the session__
-- BRICKS.shutdown()
-----------------------------------------------------------------------
