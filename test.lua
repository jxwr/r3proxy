
print ("Script Init Begin")

-- init 

local region = pool.region()
local avaliable_zone = pool.avaliable_zone()
local failover_zones = pool.failover_zones()
local machine_room = pool.machine_room()

fazs = string.split(failover_zones, ",")

tag_idx_map = {}
tag_idx_map[avaliable_zone] = 0
for i,az in ipairs(fazs) do
   tag_idx_map[az] = i + 1
end

-- test prepare

server0 = server.new("server0","127.0.0.1",3002)
server1 = server.new("server1","127.0.0.1",4001)
server2 = server.new("server2","127.0.0.1",4002)
server3 = server.new("server3","127.0.0.1",4003)

rs = replicaset.new()

server.connect(server0)
server.connect(server1)
server.connect(server2)
server.connect(server3)

replicaset.set_master(rs, server0)
replicaset.add_slave(rs, 0, server2)
replicaset.add_slave(rs, 1, server1)
replicaset.add_slave(rs, 1, server3)

slots.set_replicaset(rs, 0, 16383)

-- main

G_server_list = {}
G_masters = {}

function server_existed(s, ss)
   for i = 1, #ss do
      if s.id == ss[i].id then
         return true
      end
   end
   return false
end

-- 1,2,3,4
-- 3,4,5,6
-- [5,6], [1,2]
function server_diff(server_list)
   local adds = {}
   local drops = {}
   local ss = server_list
   local css = G_server_list

   for i = 1, #ss do
      if not server_existed(ss[i], css) then
         table.insert(adds, ss[i])
      end
   end
   for i = 1, #css do
      if not server_existed(css[i], ss) then
         table.insert(drops, css[i])
      end
   end
   return adds,drops
end

function server_role_split(server_list)
   local masters = {}
   local slaves = {}
   local ss = server_list

   for i = 1, #ss do
      if ss[i].role == "master" then
         table.insert(masters, ss[i])
      else 
         table.insert(slaves, ss[i])
      end
   end
   return masters, slaves
end

function server_remove(ss, s)
   local idx = -1
   for i = 1, #ss do
      if ss[i].id == s.id then
         idx = i
         break
      end
   end
   if idx > 0 then
      table.remove(ss, idx)
   end
end

function server_add(ss, s)
   local idx = -1
   for i = 1, #ss do
      if ss[i].id == s.id then
         idx = i
         break
      end
   end
   if idx < 0 then
      table.insert(ss, s)
   end
end

function server_find(ss, id)
   for i = 1, #ss do
      if ss[i].id == id then
         return ss[i]
      end
   end
   return nil
end

function parse(body)
   local lines = body:strip():split("\n")
   table.remove(lines, 1)

   local server_list = {}
   -- parse
   for i,line in ipairs(lines) do
      local xs = line:split(" ")

      local addr = xs[2]:split(":")
      ip, port = addr[1], addr[2]

      local role = "master"
      if string.find(xs[3], "master") == nil then
         role = "slave"
      end

      local s = {}

      s.id = xs[1]
      s.addr = xs[2]
      s.ip = ip
      s.port = tonumber(port)
      s.role = role
      s.parentid = xs[4]
      s.status = xs[8]

      if role == "master" then
         local slots = {}
         for i = 9, #xs do
            local range = xs[i]
            local slot = {}
            local pair = range:split("-")

            slot.left = tonumber(pair[1])
            slot.right = tonumber(pair[2])
            table.insert(slots, slot)
         end
         s.slots = slots
      end

      table.insert(server_list, s)
   end
   return server_list
end

function fix_servers(server_list)
   local adds, drops = server_diff(server_list)
   print(string.format("add %d, drop %d servers", #adds, #drops))

   -- disconnect dropped servers
   for i = 1, #drops do
      local s = drops[i].internal
      server.disconnect(s)
      server_remove(G_server_list, drops[i])
   end

   -- connnect new added servers
   for i = 1, #adds do
      local s = adds[i]
      print(string.format("add %s", s.id))
      local internal = server.new(s.id, s.ip, s.port)
      server.connect(internal)
      s.internal = internal
      server_add(G_server_list, s)
   end
end

function build_replica_sets(ss)
   -- build replica sets
   local masters, slaves = server_role_split(ss)
   print(string.format("found %d masters, %d slaves", #masters, #slaves))

   local replicaset_masters = {}
   -- set master
   for i = 1, #masters do
      local master = masters[i]
      local rs = replicaset.new()

      master.rs = rs
      table.insert(replicaset_masters, master)
      print("master", master.id)
      replicaset.set_master(rs, master.internal)
   end
   -- add slaves
   for i = 1, #slaves do
      local slave = slaves[i]
      local master = server_find(ss, slave.parentid)
      print("slave", slave.id, master.id)
      slave.master = master
      replicaset.add_slave(master.rs, 0, slave.internal)
   end
   
   return replicaset_masters
end

function bind_slots(masters)
   for i = 1, #masters do
      local ranges = masters[i].slots
      for j = 1, #ranges do
         slots.set_replicaset(masters[i].rs, ranges[j].left, ranges[j].right)
      end
   end
end

function update_cluster_nodes(body)
   local server_list = parse(body)

   fix_servers(server_list)

   local replicaset_masters = build_replica_sets(G_server_list)
   G_masters = replicaset_masters

   bind_slots(replicaset_masters)
end

print ("Script Init Done")
