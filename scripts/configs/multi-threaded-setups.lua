-----------------------------------------------------------------------
-- 4 - T H R E A D E D - S E T U P
-----------------------------------------------------------------------
local C={};


-----------------------------------------------------------------------
--lb_config	 __sets up a simple load balancing configuration__
--		 __the engine reads from netmap-enabled eth3 and__
--		 __forwards packets to a netmap pipe.	        __
function lb_config(pe, cnt)
	  local lb = Element.new("LoadBalancer", 4)
	  lb:connect_input("eth3")
	  lb:connect_output("eth3{" .. cnt)
	  pe:link(lb, PKT_BATCH, cnt)
end
-----------------------------------------------------------------------
return C;