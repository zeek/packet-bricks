-- /usr/bin/lua
-----------------------------------------------------------------------
-- MACROS-&-UTILITY-FUNCS
-----------------------------------------------------------------------
STATS_PRINT_CYCLE_DEFAULT = 2
SLEEP_TIMEOUT = 1
PKT_BATCH=1024
NETMAP_LIN_PARAMS_PATH="/sys/module/netmap/parameters/"
NETMAP_PIPES=64
NO_CPU_AFF=-1
NO_QIDS=-1
BUFFER_SZ=1024
-----------------------------------------------------------------------
local C={};

--see if the directory exists

function C:directory_exists(sPath)
      if type(sPath) ~= "string" then return false end

      local response = os.execute( "cd " .. sPath )
      if response == 0 then
      	 return true
      end
      return false
end



-----------------------------------------------------------------------

--sleep function __self-explanatory__

function C:sleep(n)  -- seconds
	 local clock = os.clock
	 local t0 = clock()
	 while clock() - t0 <= n do end
end


-----------------------------------------------------------------------


-- A neat function that reads shell output given a shell command `c'
function C:shell(c)
	 local o, h
	 h = assert(io.popen(c,"r"))
	 o = h:read("*all")
	 h:close()
	 return o
end


-----------------------------------------------------------------------


-- check if netmap module is loaded in the system

function C:netmap_loaded()
	 if string.find((tostring(C:shell('uname'))), 'Linux') ~= nil then
	    if C:directory_exists(NETMAP_LIN_PARAMS_PATH) then
	       return true
	    end
	    return false
	 end
	 if string.find((tostring(C:shell('uname'))), 'FreeBSD') ~= nil then
	    if (string.find(tostring(C:shell('sysctl -a dev.netmap.flags')), "sysctl: unknown oid")) ~= nil then
	       return false
	    end
	    return true
	 end
	 -- control should not come here if the system is Linux/FreeBSD
	 return false
end


-----------------------------------------------------------------------


-- enable netmap pipes framework

function C:enable_nmpipes()
	if string.find((tostring(C:shell('uname'))), 'Linux') ~= nil then
	 	 C:shell("echo " .. tostring(NETMAP_PIPES) .. " > " .. NETMAP_LIN_PARAMS_PATH .. "default_pipes")
	end
	if string.find((tostring(C:shell('uname'))), 'FreeBSD') ~= nil then
	   	 C:shell("sysctl -w dev.netmap.default_pipes=\"" .. tostring(NETMAP_PIPES) .. "\"")
	end	
end


-----------------------------------------------------------------------


-- check if you are root
-- XXX This needs to be improved
function C:check_superuser()
      if string.find((tostring(C:shell('whoami'))), 'root') == nil then 
      	 return false
      end
      return true
end
-----------------------------------------------------------------------

return C;