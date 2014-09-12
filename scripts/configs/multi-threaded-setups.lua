-----------------------------------------------------------------------
-- 4 - T H R E A D E D - S E T U P
-----------------------------------------------------------------------
local C={};


-----------------------------------------------------------------------
--lb_config	 __sets up a simple load balancing configuration__
--		 __the engine reads from netmap-enabled $int1 and__
--		 __forwards packets to a netmap pipe.	        __
function lb_config(pe, int1, cnt)
	  local lb = Brick.new("LoadBalancer", 4)
	  lb:connect_input(int1)
	  lb:connect_output(int1 .. "{" .. cnt)
	  pe:link(lb, PKT_BATCH, cnt)
end
-----------------------------------------------------------------------
return C;