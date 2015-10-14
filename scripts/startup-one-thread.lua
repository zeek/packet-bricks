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
--		 __unspecified by passing '-1'. Next creates _n_ netmap__
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
	 local pe = PktEngine.new("e0", BUFFER_SZ)

	 ---------------------------------------------------
	 -- SAMPLE SETUPS. PLZ ONLY ENABLE ONE OUT OF THE --
	 --             FOLLOWING SEVEN OPTIONS           --
	 ---------------------------------------------------

	 -- setup simple loadbalancer (setup to debug FreeBSD version)
	 -- (see configs/single-threaded-setups.lua file)
	 sampleSetup:simple_lbconfig(pe, "ix0")

	 -- setup loadbalancer config
	 -- (see configs/single-threaded-setups.lua file)
	 --sampleSetup:lb_config(pe, "eth3", "eth2")

	 -- setup dup/lb config
	 -- (see configs/single-threaded-setups.lua file)
	 --sampleSetup:duplb_config(pe, "ix0")

	 -- setup dup config
	 -- (see configs/single-threaded-setups.lua file)
	 --sampleSetup:dup_config(pe, "eth3")

	 -- setup pcapwrtr config
	 -- (see configs/single-threaded-setups.lua file)
	 --sampleSetup:pcapw_config(pe, "ix0")

	 -- setup lb/mrg config
	 -- (see configs/single-threaded-setups.lua file)
	 --sampleSetup:lbmrg_config(pe, "eth3")

	 -- setup lbmrgpcap config
	 -- don't enable this... this is still under construction
	 -- (see configs/single-threaded-setups.lua file)
	 --sampleSetup:lbmrgpcap_config(pe, "eth3")

	 -- setup lbfilt config
	 -- don't enable this... this is still under construction
	 -- (see configs/single-threaded-setups.lua file)
	 --sampleSetup:lbfilt_config(pe)

	 -- setup pcaprdr config
	 -- don't enable this... this is still under construction
	 -- (see configs/single-threaded-setups.lua file)
	 --sampleSetup:pcapr_config(pe, "ix0")

	 -- setup dummy config (for performance testing)
	 -- (see config/single-threaded-setups.lua file)
	 --sampleSetup:dummy_config(pe, "ix0")
	 
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

	 print ("Stopping engine e0")
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
