-- /usr/bin/lua
---------------------- TEST SCRIPT -----------------------------------
C_DEFAULT = 10
SLEEP_TIMEOUT = 1

--sleep
local clock = os.clock
function sleep(n)  -- seconds
	 local t0 = clock()
	 while clock() - t0 <= n do end
end

--init
function init()
	 pkteng.new({name="e0", type="netmap", cpu=0})
	 pkteng.link({engine="e0", ifname="eth3", batch=1024})
end

--start
function start(c)
	 pkteng.start({name="e0"})
	 local i = 0
	 repeat
	     sleep(SLEEP_TIMEOUT)
	     pkteng.show_stats({name="e0"})
	     i = i + 1
	 until i > C_DEFAULT
end

--stop
function stop()
	 pkteng.show_stats({name="e0"})
	 pkteng.stop({name="e0"})
	 -- pkteng.unlink({engine="e0", ifname="eth3"})
	 -- pkteng.delete({name="e0"})
	 --pacf.shutdown()
end