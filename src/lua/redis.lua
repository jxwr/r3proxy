package.path = package.path .. ";lua/?.lua;../?.lua"

print ("Script Init Begin")

local pool = require("pool")

function parse(body)
   local lines = body:strip():split("\n")
   table.remove(lines, 1)

   local configs = {}
   -- parse
   for i,line in ipairs(lines) do
      local xs = line:split(" ")

      local addr = xs[4]:split(":")
      ip, port = addr[1], addr[2]

      local role = "master"
      if string.find(xs[5], "master") == nil then
         role = "slave"
      end

      local loc = xs[2]:split(":")

      local c = {
         id = xs[3],
         addr = xs[4],
         ip = ip,
         port = tonumber(port),
         role = role,
         master_id = xs[6],
         status = xs[10],
         readable = false,
         writable = false,
         region = loc[1],
         zone = loc[2],
         room = loc[3],
      }

      if string.find(xs[1], "r") then
         c.readable = true
      end
      if string.find(xs[1], "w") then
         c.writable = true
      end

      if role == "master" then
         local ranges = {}
         for i = 11, #xs do
            -- skip importing/migrating info
            if string.sub(xs[i],1,1) == "[" then
               break
            end

            local range = {}
            local pair = xs[i]:split("-")

            range.left = tonumber(pair[1])
            range.right = tonumber(pair[2])
            table.insert(ranges, range)
         end
         c.ranges = ranges
      end

      table.insert(configs, c)
   end
   return configs
end

function update_cluster_nodes(msg)
   if string.sub(msg,1,3) == "+OK" or string.sub(msg,1,3) == "$-1" then
      return
   end

   -- parse message returned by 'cluster nodes'
   local configs = parse(msg)
   
   -- reconstruct servers, fix adds and drops
   pool:set_servers(configs)

   -- rebuild replica sets
   pool:build_replica_sets()

   -- bind replica sets to slots
   pool:bind_slots()

   -- 0 is success
   return 0
end

print ("Script Init Done")


